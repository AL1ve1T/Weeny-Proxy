#include "includes.hpp"
#include "server.cpp"


int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "usage: weeny_proxy <IP address> <port>" << std::endl;
        return 1;
    }
    wp::server server(argv[1], atoi(argv[2]));
    return 0;
}