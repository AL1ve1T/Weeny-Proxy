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
            server()
            {
                if ((sock_des = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                {
                    std::cerr << "Cannot create a socket" << std::endl;
                    shutdown(sock_des, 0);
                    std::exit(1);
                }
                addr.sin_family = AF_INET;
                addr.sin_port = 8000;
                addr.sin_addr.s_addr = INADDR_LOOPBACK;
                if (bind(sock_des, (struct sockaddr*)&addr, sizeof(sockaddr_in)) != -1 &&
                    listen(sock_des, 10) != -1)
                    {
                        while (true)
                        {
                            try
                            {
                                struct sockaddr_storage remoteaddr;
                                int client_sock = accept(sock_des, (sockaddr*)&remoteaddr, (socklen_t*)sizeof(remoteaddr));
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
            }
            void handle_connection(int csock)
            {
                wp::proxy* _proxy = new wp::proxy();
                std::thread* handler = new std::thread(*_proxy->connection, csock);
                handler->detach();

                delete(_proxy);
                delete(handler);
                close(csock);
            }
            ~server()
            {
                shutdown(sock_des, 0);
            }
        private:
            // int port = foo_port
            struct sockaddr_in addr;
            int sock_des;
    };
}
