#include <iostream>
#include <boost/asio.hpp>
#include "server.h"

int main(int argc, char* argv[])
{

    if (argc < 2)
    {
        std::cerr << "Usage: <port>\n";
        return 1;
    }

    boost::asio::io_service io_service;
    tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[1]));
    Server (io_service, endpoint);

    io_service.run();

  return 0;
}
