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
                // std::vector<std::string> http_body;
                std::string http_body_string;
                std::stringstream read_stream;

                char *buffer;
                int body_starts;
                bzero(buffer, BUFFER_SIZE);
                bool readed;
                
                while (readed)
                {
                    // Reads data from socket to buffer
                    bool is_readed = recv(csock, buffer, BUFFER_SIZE, 0);
                    if (is_readed < 0)
                    {
                        std::cerr << "Could not read from socket\nTermination.." << std::endl;
                    }
                    // Concatenate new data to existing data
                    read_stream << buffer;
                    bzero(buffer, BUFFER_SIZE);

                    // Offset must be 4
                    body_starts = read_stream.str().find("\r\n\r\n") + 4;
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
                    std::vector<std::string> first_str = split_string(http_header[0], " ");
                    // GET
                    std::string method = first_str[0];
                    // /home/test.html
                    std::string url = first_str[1];
                    // HTTP/1.1
                    std::string protocol = first_str[2];

                    std::string host;
                    unsigned short port;
                    std::string path;

                    /////////////////////////////////////////////////////////////////////
                    /// Scanning request message and collecting data for a new request
                    if (strncasecmp(url.c_str(), "http://", 7) == 0)
                    {
                        if (std::sscanf(url.c_str(), "http://%[^:/]:%d%s", host, &port, path) == 3)
                        {
                            // Cap
                        }
                        else if (std::sscanf(url.c_str(), "http://%[^/]%s", host, path) == 2)
                        {
                            port = 80;
                        }
                        else if (std::sscanf(url.c_str(), "http://%[^:/]:%d", host, &port) == 2)
                        {
                            path = "";
                        }
                        else if (std::sscanf(url.c_str(), "http://%[^/]", host) == 1)
                        {
                            port = 80;
                            path = "";
                        }
                        else
                        {
                            // Send "Bad Request" to the client
                            // Could use SSL
                        }
                    }
                    else if (method == "CONNECT")
                    {
                        if (std::sscanf(url.c_str(), "%[^:]:%d", host, &port) == 2)
                        {
                            // Cap
                        }
                        else if (std::sscanf(url.c_str(), "%s", host) == 1)
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

                int client_sock = open_socket(host, port);
                FILE* read_sock = fdopen(client_sock, "r");
                FILE* write_sock = fdopen(client_sock, "w");

                send_proxy_request(method.c_str(), path.c_str(), protocol.c_str(),
                    read_sock, write_sock, http_body_string, http_headers_map);

                close(client_sock);
            }
            ~proxy ()
            {
            }
        private:
            ////////////////////////////////////////////////////////////////////
            /// Finaly sends request from to the distant server
            /// (Most of code is writen in raw C style)
            static void send_proxy_request
            (const char* method, const char* path, const char* protocol, FILE* read_sock, FILE* write_sock,
                std::string http_body, std::unordered_map<std::string, std::string> http_headers_map)
            {
                char _protocol[8192];
                char comment[8192];

                int _header;
                int connection_status;
                int _char;
                long int content_length = -1;
                
                std::fprintf(write_sock, "%s %s %s\r\n", method, path, protocol);
                
                for (auto header : http_headers_map)
                {
                    fputs((header.first + ": " + header.second + "\r\n").c_str(), write_sock);
                }
                content_length = std::stoi(http_headers_map["Content-Length"]);
                fputs("\r\n", write_sock);
                fputs(http_body.c_str(), write_sock);
                fflush(write_sock);

                for (int i = 0; i < content_length && (_char = getchar()) != EOF; i++)
                {
                    putc(_char, write_sock);
                }
                fflush(write_sock);

                ////////////////////////////////////////////////////////////////
                /// Receiving response and forward it back to the client
                content_length = -1;
                _header = 1;
                connection_status = -1;

                //TODO: Receiving data, and forwarding it to the client
            }

            ////////////////////////////////////////////////////////////////////
            /// Opens socket for proxy request
            static int open_socket(std::string host, unsigned short port)
            {
                #ifdef USE_IPV4
                struct hostent* _hostent;
                struct sockaddr_in _sockaddr_in;

                // Variables
                int sockaddr_len;
                int sock_family;
                int sock_type;
                int sock_protocol;
                int sockfd;

                _hostent = gethostbyname(host.c_str());
                if (_hostent = (struct hostent*) 0)
                {
                    // Not Found
                }

                sock_family = _sockaddr_in.sin_family = _hostent->h_addrtype;
                sock_type = SOCK_STREAM;
                sock_protocol = 0;
                sockaddr_len = sizeof(_sockaddr_in);
                (void*) memmove(&_sockaddr_in, _hostent->h_addr, _hostent->h_length);
                _sockaddr_in.sin_port = htons(port);

                // Now create socket
                sockfd = socket(sock_family, sock_type, sock_protocol);
                if (sockfd < 0)
                {
                    // "Internal Error"
                }
                if (connect(sockfd, (struct sockaddr*) &_sockaddr_in, sockaddr_len) < 0)
                {
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
                    previous = current + 2;
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
                
                for (auto header : http_msg)
                {
                    current = header.find(": ");
                    if (current == header.back())
                    {
                        continue;
                    }
                    result.insert(std::make_pair(header.substr(0, current), header.substr(current + 2, header.length())));
                    current += 2;
                }
                return result;
            }
            ////////////////////////////////////////////////////////////////////
            /// Just a spliter
            static std::vector<std::string> split_string(std::string str, const char* delim)
            {
                std::size_t current = 0;
                std::size_t previous = 0;
                std::vector<std::string> result;

                while (current != std::string::npos)
                {
                    result.push_back(str.substr(previous, current - previous));
                    previous = current + strlen(delim);
                    current = str.find(delim, previous);
                }
                result.push_back(str.substr(previous, current - previous));
                return result;
            }
    };
}