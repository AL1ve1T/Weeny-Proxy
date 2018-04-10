#include "includes.hpp"
#include "proxy.cpp"
#include <map>

namespace wp
{
    //////////////////////////////////////////////////////
    /// Server listens to incoming connection and then 
    /// creates thread that catches clients' requests
    class server
    {
        public:
            server(const char* servip, int _port)
            {
                if ((sock_des = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                {
                    std::cerr << "Cannot create a socket" << std::endl;
                    std::cerr << strerror(errno) << std::endl;
                    shutdown(sock_des, 0);
                    std::exit(1);
                }
                addr.sin_family = AF_INET;
                addr.sin_port = htons(_port);
                addr.sin_addr.s_addr = inet_addr(servip);
                /////////////////////////////////////////////////////////
                /// Make sure that s_addr != -1
                if (addr.sin_addr.s_addr < 0)
                {
                    std::cerr << "Address is invalid" << std::endl;
                    shutdown(sock_des, 0);
                    std::exit(0);
                }
                /////////////////////////////////////////////////////////
                /// Bind socket to the port
                if (bind(sock_des, (struct sockaddr*)&addr, sizeof(addr)) == -1)
                {
                    std::cerr << "Binding failed." << std::endl;
                    std::cerr << strerror(errno) << std::endl;
                    shutdown(sock_des, 0);
                    std::exit(0);
                }
                /////////////////////////////////////////////////////////
                /// Listen on the socket (max connections = 10)
                if (listen(sock_des, 10) == -1)
                {
                    std::cerr << "Cannot listen to socket." << std::endl;
                    std::cerr << strerror(errno) << std::endl;
                    shutdown(sock_des, 0);
                    std::exit(0);
                }
                /////////////////////////////////////////////////////////
                /// Everything is OK:)
                std::cout << "Server established" << std::endl;
                while (true)
                {
                    try
                    {
                        struct sockaddr_storage remoteaddr;
                        socklen_t addrlen = sizeof(remoteaddr);
                        int client_sock = accept(sock_des, (sockaddr*)&remoteaddr, &addrlen);
                        if (client_sock == -1)
                        {
                            std::cerr << "Error occured: cannot accept socket" << std::endl;
                            std::cerr << strerror(errno) << std::endl;
                            std::exit(0);
                        }
                        handle_connection(client_sock);
                    } 
                    catch (std::exception& e)
                    {
                        std::cerr << "Server shutting down...:" << e.what() << std::endl;
                        shutdown(sock_des, 0);
                        std::exit(0);
                    }
                }
            }
            void handle_connection(int csock)
            {
                try
                {
                    std::thread* handler = new std::thread(&_proxy.connection, csock);
                    handler->detach();
                }
                catch(std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                    throw;
                }
            }
            ~server()
            {
                shutdown(sock_des, 0);
            }
        private:
            wp::proxy _proxy;
            struct sockaddr_in addr;
            int sock_des;
    };
}
