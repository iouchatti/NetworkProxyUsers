#ifndef USER_H
#define USER_H

#include <memory>
#include <array>
#include <chrono>
#include <boost/asio.hpp>
#include "proxy.h"

using namespace boost::asio;
using namespace boost::asio::ip;

class Proxy;
class User : public std::enable_shared_from_this<User> {
public:
    User(tcp::socket socket, std::shared_ptr<Proxy> proxy, const std::string& secret);

    std::shared_ptr<std::string> retrieveMessage();
    void initialize();
    void start();
    void doRead();
    void doWrite();
    tcp::socket& socket();
    void start_timer();

    void increment_message_count() { ++message_count_; }
    std::chrono::steady_clock::time_point get_connection_time() const { return connection_time_; }
    std::size_t get_message_count() const { return message_count_; }
    std::string get_secret() const { return secret_; }

private:
    tcp::socket socket_;
    streambuf sb;
    std::array<char, 24> rData_;
    std::array<char, 24> wData_;
    std::shared_ptr<Proxy> proxy_;
    steady_timer timer_;
    bool has_partner_;
    std::shared_ptr<std::string> message_;
    std::chrono::steady_clock::time_point connection_time_;
    std::size_t message_count_ = 0;
    std::string secret_;
};

#endif // USER_H
