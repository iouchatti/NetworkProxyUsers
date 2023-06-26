#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include "proxy.h"

int main() {
    try {
        io_service ioService;
        
        auto proxy = std::make_shared<Proxy>(ioService, "127.0.0.1", 12345);
        proxy->start();
        ioService.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
