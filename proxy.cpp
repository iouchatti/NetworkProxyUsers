#include "proxy.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

std::string format_time_point(std::chrono::steady_clock::time_point tp) {
    auto time_now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(time_now - tp);
    auto time_t_diff = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - duration);
    std::tm tm;
    localtime_s(&tm, &time_t_diff);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %X");
    return ss.str();
}

Proxy::Proxy(io_service& ioService, const std::string& host, short port)
    : timer_(ioService), acceptor_(ioService, tcp::endpoint(boost::asio::ip::address::from_string(host), port)) {
}

void Proxy::start() {
    doAccept();
}

size_t Proxy::userCount() const {
    return users_.size();
}

void Proxy::removeUser(std::shared_ptr<User> user) {
    std::lock_guard<std::mutex> lock(usersMutex_);
    users_.erase(std::remove(users_.begin(), users_.end(), user), users_.end());
    std::cout << "User disconnected" << std::endl;
}

void Proxy::sendACK(std::shared_ptr<User> user) {
    std::shared_ptr<std::string> ackMsg = std::make_shared<std::string>("ACK");
    boost::asio::async_write(user->socket(), boost::asio::buffer(*ackMsg),
        [ackMsg](boost::system::error_code ec, std::size_t) {
            if (ec) {
                std::cerr << "Write error: " << ec.message() << std::endl;
            }
        });
}

void Proxy::forwardMessage(std::shared_ptr<std::string> message, std::shared_ptr<User> sender) {
    std::string sender_secret = sender->get_secret();

    for (const auto& user : users_) {
        // If no password is set or the passwords match, forward the message
        if (user != sender && (sender_secret.empty() || user->get_secret() == sender_secret)) {
            boost::asio::async_write(user->socket(), boost::asio::buffer(*message),
                [message, self = shared_from_this()](boost::system::error_code ec, std::size_t) {
                    if (ec) {
                        std::cerr << "Write error: " << ec.message() << std::endl;
                    }
                }
            );
        }
    }

    auto endpoint = sender->socket().remote_endpoint();
    std::cerr << "Message to be forwarded: " << *message
        << "\nFrom user: " << endpoint.address().to_string() << ":" << endpoint.port()
        << "\nConnected since: " << format_time_point(sender->get_connection_time())
        << "\nMessage count: " << sender->get_message_count() << std::endl;
}

void Proxy::doAccept() {
    auto self = shared_from_this();
    acceptor_.async_accept(
        [this, self](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::string secret = "";
                auto user = std::make_shared<User>(std::move(socket), shared_from_this(), secret);
                {
                    std::lock_guard<std::mutex> lock(usersMutex_);
                    users_.push_back(user);
                }
                if (userCount() == 1) {
                    user->initialize();
                }
                else {
                    // Inform the new user about the existing connections
                    auto msg = std::make_shared<std::string>("You are connected to an existing user.");
                    async_write(user->socket(), buffer(*msg),
                        [self, msg](boost::system::error_code ec, std::size_t) {
                            if (ec) {
                                std::cerr << "Write error: " << ec.message() << std::endl;
                            }
                        });
                }

                // Inform existing users about the new connection
                auto msg = std::make_shared<std::string>("New user connected. You will be connected to this user.");
                for (const auto& u : users_) {
                    if (u != user) {
                        async_write(u->socket(), buffer(*msg),
                            [self, msg](boost::system::error_code ec, std::size_t) {
                                if (ec) {
                                    std::cerr << "Write error: " << ec.message() << std::endl;
                                }
                            });
                    }
                }

                user->start();  // Start reading and writing for the user

                if (userCount() == 1) {
                    user->start_timer();  // Start the timer if there is only one user
                }

                auto endpoint = user->socket().remote_endpoint();
                auto time_now = std::chrono::system_clock::now();
                std::time_t time_now_t = std::chrono::system_clock::to_time_t(time_now);

                std::time_t tmp_time_t = std::chrono::system_clock::to_time_t(time_now);
                std::tm tm_time_now;
                localtime_s(&tm_time_now, &time_now_t);
                std::tm tm_tmp_time;
                localtime_s(&tm_tmp_time, &tmp_time_t);

                char time_str[26];
                ctime_s(time_str, sizeof(time_str), &time_now_t);
                time_str[strlen(time_str) - 1] = '\0';  // Remove the trailing newline character

                char tmp_time_str[26];
                ctime_s(tmp_time_str, sizeof(tmp_time_str), &tmp_time_t);
                tmp_time_str[strlen(tmp_time_str) - 1] = '\0';  // Remove the trailing newline character

                std::cerr
                    << "\nNew user connected : "
                    << "\n From user: " << endpoint.address().to_string() << ":" << endpoint.port()
                    << "\n Current time : " << time_str
                    << "\n Connexion time : " << tmp_time_str
                    << "\n Connected since: " << format_time_point(user->get_connection_time())
                    << "\n Message count: " << user->get_message_count() << std::endl;
            }
            else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }

            doAccept();
        }
    );
}
