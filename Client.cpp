#include <boost/asio.hpp>
#include <iostream>
#include <future>

using namespace boost::asio;
using namespace boost::asio::ip;


class Client : public std::enable_shared_from_this<Client> {
public:
    Client(io_service& ioService, const tcp::resolver::results_type& endpoints)
        : socket_(ioService), endpoints_(endpoints) {
        doConnect();
    }

    void start() {

        doRead();
        doWrite();
    }

private:
    void doConnect() {
        async_connect(socket_, endpoints_,
            [this](boost::system::error_code ec, tcp::endpoint) {
                if (!ec) {
                    std::cout << "Connected to server" << std::endl;
                    this->start();
                }
                else {
                    std::cerr << "Connect error: " << ec.message() << std::endl;
                }
            });
    }


    void doRead() {
        std::cout << "REAAAAAAAAAAD: ";
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(rData_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    rData_[length] = '\0'; // Append null character for proper termination
                    std::string message(rData_.data());
                    std::cout << "Received message: " << message << std::endl;
                    doRead();
                }
                else {
                    std::cerr << "Read error: " << ec.message() << std::endl;
                }
            }
        );
    }



    void doWrite() {
        std::cin.getline(cindata_, max_length);
        std::size_t length = std::strlen(cindata_);
        cinMessage.assign(cindata_, length);
        auto self(shared_from_this());
        async_write(socket_, buffer(cinMessage),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }
                doWrite();
            });
    }


    std::string cinMessage;
    tcp::socket socket_;
    const tcp::resolver::results_type& endpoints_;
    enum { max_length = 1024 };
    char cindata_[max_length];
    std::string rData_ = "";
    std::string wData_ = "";
};

int main() {
    try {
        io_service ioService;
        tcp::resolver resolver(ioService);
        auto endpoints = resolver.resolve("127.0.0.1", "12345");

        // Create the Client object with a shared_ptr
        auto client = std::make_shared<Client>(ioService, endpoints);


        ioService.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
