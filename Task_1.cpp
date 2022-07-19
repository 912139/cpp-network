#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>
#include <socket_wrapper/socket_class.h>


const size_t buffer_size = 256;


int main(int argc, char const * const argv[])
{
    using namespace std::chrono_literals;
    int newsock;
    struct sockaddr_storage client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    socket_wrapper::SocketWrapper sock_wrap;

    const int port { std::stoi(argv[1]) };

    std::cout << "Receiving messages on the port " << port << "...\n";

    struct sockaddr_in6 addr = {.sin6_family = PF_INET6, .sin6_port = htons(port)};
    addr.sin6_addr = in6addr_any;


    socket_wrapper::Socket sock(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if (!sock)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&broadcast), sizeof(broadcast));

    if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == -1)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    char buffer[buffer_size] = {};
    std::string request = { "The message is received" };
    ssize_t recv_bytes;

    listen(sock, 5);

     std::cout << "Waiting for a connection..." << std::endl;
     newsock = accept(sock, reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);
  if (newsock < 0)
   { 
    throw std::runtime_error("ERROR on accept");
   }
     std::cout << "Connection established" << std::endl;
        while (true)
        { 
        recv_bytes = recv(newsock, buffer, sizeof(buffer) - 1, 0);
        if (recv_bytes > 0)
        {   
        std::cout << buffer << std::endl; 
        } else if (recv_bytes <= 0)
            {
                if (EINTR == errno) continue;
                if (0 == errno) break;
            }
        if (send(newsock, request.c_str(), request.size() - 1, 0) < 0)
        {        
            std::cerr << sock_wrap.get_last_error_string() << std::endl;
            return EXIT_FAILURE;
        }
        }
        return EXIT_FAILURE;       
}
