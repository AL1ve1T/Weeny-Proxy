#include "includes.hpp"
#include <unordered_map>
#include <vector>

namespace wp 
{
    #define BUFFER_SIZE 4028
    #define USE_IPV4

    ////////////////////////////////////////////////////////
    /// Logic part to detach proxy connection from a server
    class proxy 
    {
        public: 
            proxy ()
            {
            }
            static void* connection (int csock)
            {
                std::unordered_map<std::string, std::string> http_headers_map;
                std::vector<std::string> http_header;
                std::string http_body_string;
                std::stringstream read_stream;

                char buffer[BUFFER_SIZE];
                int body_starts;
                memset(buffer, 0, BUFFER_SIZE);
                int is_readed = 1;

                fcntl(csock, F_SETFL, O_NONBLOCK);
                
                while (is_readed != 0)
                {
                    // Reads data from socket to buffer
                    try
                    {
                        is_readed = recv(csock, buffer, BUFFER_SIZE, 0);
                        if (is_readed == -1 and read_stream.str().length() == 0)
                        {
                            std::cerr << "Cannot read from socket" << std::endl;
                            strerror(errno);
                            close(csock);
                            std::terminate();
                        }
                    }
                    catch (std::exception& e)
                    {
                        std::cerr << e.what() << std::endl;
                        throw;
                    }
                    if (is_readed < 0)
                    {
                        std::cerr << "Could not read from socket\nTermination.." << std::endl;
                    }
                    // Concatenate new data to existing data
                    read_stream << buffer;

                    // Offset must be 4
                    body_starts = read_stream.str().find("\r\n\r\n") + 4;

                    if (buffer[0] == 0)
                    {
                        break;
                    }
                    memset(buffer, 0, BUFFER_SIZE);
                }
                // Now parsing the header
                http_body_string = read_stream.str();
                
                http_header = http_parse_msg(http_body_string.substr(0, body_starts - 4));
                http_body_string.erase(0, body_starts);

                http_headers_map = http_make_key_value(http_header);

                ///////////////////////////////////////////////////////////////
                /// TODO: Same implementation for IPv6
                #ifdef USE_IPV4
                    ///////////////////////////////////////////////////////////
                    /// Getting the first string of the request
                    std::vector<std::string> first_str = split_string(http_header[1], " ");
                    // GET
                    std::string method = first_str[1];
                    // /home/test.html
                    std::string url = first_str[2];
                    // HTTP/1.1
                    std::string protocol = first_str[3];

                    char host[1024];
                    unsigned short port;
                    char path[1024];

                    /////////////////////////////////////////////////////////////////////
                    /// Scanning request message and collecting data for a new request
                    if (strncasecmp(url.c_str(), "http://", 7) == 0)
                    {
                        if (std::sscanf(url.c_str(), "http://%[^:/]:%d%s", &host, &port, &path) == 3)
                        {
                            // Cap
                        }
                        else if (std::sscanf(url.c_str(), "http://%[^/]%s", &host, &path) == 2)
                        {
                            port = 80;
                        }
                        else if (std::sscanf(url.c_str(), "http://%[^:/]:%d", &host, &port) == 2)
                        {
                            *path = '\0';
                        }
                        else if (std::sscanf(url.c_str(), "http://%[^/]", &host) == 1)
                        {
                            port = 80;
                            *path = '\0';
                        }
                        else
                        {
                            // Send "Bad Request" to the client
                            // Could use SSL
                        }
                    }
                    else if (method == "CONNECT")
                    {
                        if (std::sscanf(url.c_str(), "%[^:]:%d", &host, &port) == 2)
                        {
                            // Cap
                        }
                        else if (std::sscanf(url.c_str(), "%s", &host) == 1)
                        {
                            port = 443;
                        }
                        else
                        {
                            // Send "Bad Request" to the client
                            // Could use SSL
                        }
                    }
                    else
                    {
                        // Send "Bad Request" to the client
                        // Caps
                    }
                #endif

                int proxy_client = open_socket(host, port);

                send_proxy_request(method.c_str(), path, protocol.c_str(),
                    csock, proxy_client, http_body_string, http_headers_map);

                shutdown(proxy_client, SHUT_RDWR);
            }
            ~proxy ()
            {
            }
        private:
            ////////////////////////////////////////////////////////////////////
            /// Finaly sends request from to the distant server
            /// (Most of code is writen in raw C style)
            static void send_proxy_request
            (const char* method, const char* path, const char* protocol, 
                int csock, int proxy_client, std::string http_body, std::unordered_map<std::string, std::string> http_headers_map)
            {
                char _protocol[1024];
                char comment[1024];
                char http_msg[8192];
                std::stringstream send_buffer;
                std::stringstream recv_buffer;

                int _header;
                int connection_status;
                int _char;
                int bytes = 1;
                long int content_length = -1;
                
                //std::fprintf(buffer, "%s %s %s\r\n", method, path, protocol);
                send_buffer << trim(method) << ' ' << trim(path) << ' ' << trim(protocol) << "\r\n";
                
                for (auto header : http_headers_map)
                {
                    send_buffer << header.first + ": " + header.second + "\r\n";
                }
                send_buffer << "\r\n" << http_body;

                if (send(proxy_client, send_buffer.str().c_str(), strlen(send_buffer.str().c_str()), 0) == -1)
                {
                    std::cerr << "Socket is closed" << std::endl;
                    return;
                    // Cannot get access to the endpoint
                }

                ////////////////////////////////////////////////////////////////
                /// Receiving response and forward it back to the client
                content_length = -1;
                _header = 1;
                connection_status = -1;
                // Filling buffer with zeros
                memset(http_msg, 0, sizeof(http_msg));

                std::string http_msg_string(http_msg);
                
                fcntl(proxy_client, F_SETFL, O_NONBLOCK);

                while (bytes != 0)
                {
                    bytes = recv(proxy_client, http_msg, sizeof(http_msg), 0);
                    recv_buffer << trim(http_msg);
                    // content_length -= bytes;
                    if (_header)
                    {
                        std::sscanf(http_msg, "%[^ ] %d %s", _protocol, &connection_status, comment);
                        _header = 0;
                    }
                    if (strcasecmp(http_msg, "Content-Length:") == 0)
                    {
                        content_length = atol(&(http_msg[15]));
                    }
                    if (http_msg[0] == 0)
                    {
                        break;
                    }

                    memset(http_msg, 0, sizeof(http_msg));
                }

                std::cout << recv_buffer.str();

                if (send(csock, recv_buffer.str().c_str(), strlen(recv_buffer.str().c_str()), 0) == -1)
                {
                    std::cerr << "Error occured" << std::endl;
                }
                shutdown(csock, SHUT_RDWR);
                shutdown(proxy_client, SHUT_RDWR);
            }

            ////////////////////////////////////////////////////////////////////
            /// Opens socket for proxy request
            static int open_socket(const char* host, unsigned short port)
            {
                #ifdef USE_IPV4
                struct hostent* _hostent;
                struct sockaddr_in _sockaddr_in;

                // Variables
                size_t sockaddr_len;
                int sock_family;
                int sock_type;
                int sock_protocol;
                int sockfd;

                _hostent = gethostbyname(host);
                if (_hostent == (struct hostent*) 0)
                {
                    // Not Found
                }

                _sockaddr_in.sin_family = AF_INET;
                sock_family = AF_INET;
                sock_type = SOCK_STREAM;
                sock_protocol = IPPROTO_TCP;
                sockaddr_len = sizeof(_sockaddr_in);
                memcpy(&_sockaddr_in.sin_addr, _hostent->h_addr, _hostent->h_length);
                _sockaddr_in.sin_port = htons(port);

                // Now create socket
                sockfd = socket(sock_family, sock_type, sock_protocol);
                if (sockfd < 0)
                {
                    // "Internal Error"
                }
                if (connect(sockfd, (struct sockaddr*) &_sockaddr_in, sockaddr_len) < 0)
                {
                    std::cerr << strerror(errno) << std::endl;
                    std::cerr << "Endpoint is unavailable" << std::endl;
                    return 0;
                    // "Unavailable"
                }

                return sockfd;

                #endif
            }
            ////////////////////////////////////////////////////////////////////
            /// Parses http string to a divided http headers
            /// and uses "\r\n" as a delimeter
            static std::vector<std::string> http_parse_msg(std::string http_msg)
            {
                std::size_t current = 0;
                std::size_t previous = 0;
                std::vector<std::string> result;

                while (current != std::string::npos)
                {
                    result.push_back(http_msg.substr(previous, current - previous));
                    if (current != 0)
                    {
                        previous = current + 2;
                    }
                    current = http_msg.find("\r\n", previous);
                }
                result.push_back(http_msg.substr(previous, current - previous));
                return result;
            }
            ////////////////////////////////////////////////////////////////////
            /// Uses vector of divided http headers to produce a map
            /// of "Key": "Value" pairs
            static std::unordered_map<std::string, std::string> 
                http_make_key_value(std::vector<std::string> http_msg)
            {
                std::unordered_map<std::string, std::string> result;
                size_t current = 0;
                
                for (std::string header : http_msg)
                {
                    current = header.find(": ");
                    if (current == header.back())
                    {
                        continue;
                    }
                    if (current != std::string::npos)
                    {
                        result.insert(std::make_pair(header.substr(0, current), header.substr(current + 2, header.length())));
                    }
                    current += 2;
                }
                return result;
            }
            ////////////////////////////////////////////////////////////////////
            /// Just a spliter
            static std::vector<std::string> split_string(const std::string& str, const char* delim)
            {
                std::size_t current = 0;
                std::size_t previous = 0;
                std::vector<std::string> result;

                while (current != std::string::npos)
                {
                    result.push_back(str.substr(previous, current - previous));
                    if (current != 0)
                    {
                        previous = current + strlen(delim);
                    }
                    current = str.find(delim, previous);
                }
                result.push_back(str.substr(previous, current - previous));
                return result;
            }

            static std::string trim (const char* base_str)
            {
                std::string result(base_str);
                int len = result.length();
                
                while (result[len - 1] == '\n' || result[len - 1] == '\r')
                {
                    result.pop_back();
                }
                return result;
            }
    };
}
