#include "client.h"

int main(int argc, char* argv[])
{

    if (argc != 3)
    {
        std::cerr << "Usage: chat_client <host> <port>\n";
        return 1;
    }

    init_logger("client_%Y-%m-%d_%H-%M-%S.%N.log");

    BOOST_LOG_SEV(my_logger::get(), logging::trivial::info) << "Client started!\n";

    Client c(argv[1], argv[2]);
    c.start();

    return 0;
}
