#ifndef PROXY_H
#define PROXY_H

#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <boost/asio.hpp>
#include "user.h"

using namespace boost::asio;
using namespace boost::asio::ip;
class User;
class Proxy : public std::enable_shared_from_this<Proxy> {
public:
    Proxy(io_service& ioService, const std::string& host, short port);
    void start();
    size_t userCount() const;
    void removeUser(std::shared_ptr<User> user);
    void checkForMessages();
    void forwardMessage(std::shared_ptr<std::string> message, std::shared_ptr<User> sender);
    void doAccept();
    void sendACK(std::shared_ptr<User> user);
    std::shared_ptr<User> findUserBySecret(const std::string& secret);
    void setSecretUser(const std::string& secret, const std::shared_ptr<User>& user) {
        user_secrets_[user] = secret;
    }
    std::mutex& getUsersMutex() {
        return usersMutex_;
    }
    const std::vector<std::shared_ptr<User>>& getUsers() const {
        return users_;
    }
    void setUsers(const std::vector<std::shared_ptr<User>>& users) {
        users_ = users;
    }


private:
    steady_timer timer_;
    tcp::acceptor acceptor_;
    std::vector<std::shared_ptr<User>> users_;
    std::unordered_map<std::shared_ptr<User>, std::string> user_secrets_;
    std::mutex usersMutex_;
};

#endif // PROXY
