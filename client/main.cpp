#include "client.h"

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: chat_client <host> <port>\n";
        return 1;
    }

    Client c(argv[1], argv[2]);
    c.start();

    return 0;
}
