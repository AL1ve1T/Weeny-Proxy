#include "includes.hpp"
#include "server.cpp"


int main(int argc, char* argv[])
{
    // if (argc < 3)
    // {
    //     std::cerr << "usage: weeny_proxy <IP address> <port>" << std::endl;
    //     return 1;
    // }
    // wp::server server(argv[1], atoi(argv[2]));
    wp::server server("192.168.1.109", 8000);
    return 0;
}