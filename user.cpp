#include "user.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

 

User::User(tcp::socket socket, std::shared_ptr<Proxy> proxy, const std::string& secret)
    : socket_(std::move(socket)),
    proxy_(proxy),
    timer_(socket_.get_executor()),
    connection_time_(std::chrono::steady_clock::now()),
    secret_(secret) {}

std::shared_ptr<std::string> User::retrieveMessage() {
    std::shared_ptr<std::string> msg = message_;
    message_->clear();
    return msg;
}

void User::initialize() {
    timer_.expires_after(std::chrono::seconds(30));
    connection_time_ = std::chrono::steady_clock::now();
}

void User::start() {
    rData_.fill('\0');
    wData_.fill('\0');
    doRead();
    doWrite();
}

void User::doRead() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(rData_),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                message_ = std::make_shared<std::string>(rData_.data(), length);
                std::cout << "Received message: " << *message_ << std::endl;

                if (message_->substr(0, 8) == "password") {
                    // The message starts with "password", so extract the password
                    // TODO encryption management
                    secret_ = message_->substr(8);  
                    std::string secret = message_->substr(8);
                    {
                        std::lock_guard<std::mutex> lock(proxy_->getUsersMutex());
                        proxy_->setSecretUser(secret, shared_from_this());  // Add the user to the secrets map
                    }

                    std::cout << "Password set to: " << secret_ << std::endl;
                }
                else if (*message_ == "Hello, I am still alive") {
                    proxy_->sendACK(self);  // Send ACK if the message is "alive"
                }
                else {
                    proxy_->forwardMessage(message_, self);
                    increment_message_count();
                }

                doRead();
            }
            else {
                std::cerr << "Read error: " << ec.message() << std::endl;
                proxy_->removeUser(self);
            }
        });
} 

void User::doWrite() {
    auto self(shared_from_this());
    std::size_t length = wData_.size();  

    async_write(socket_, buffer(wData_, length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (ec) {
                std::cerr << "Write error: " << ec.message() << std::endl;
                proxy_->removeUser(self);
            }
        });
}

tcp::socket& User::socket() {
    return socket_;
}

void User::start_timer() {
    timer_.expires_after(std::chrono::seconds(30));  
    timer_.async_wait([this](const boost::system::error_code& ec) {
        if (!ec && proxy_->userCount() == 1) {
            std::cerr << "No other user connected within 30 seconds. Disconnecting." << std::endl;
            socket_.close();
            proxy_->removeUser(shared_from_this());
        }
        });
}
