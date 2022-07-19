#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <unistd.h>
#include <string.h> 

#   include <netinet/tcp.h>
#   include <sys/ioctl.h>

#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>
#include <socket_wrapper/socket_class.h>

#define BUF_SIZE 500

int main(int argc, char const * const argv[])
{
    socket_wrapper::SocketWrapper sock_wrap;

           if (argc < 3) {
               fprintf(stderr, "Usage: %s host port msg...\n", argv[0]);
               exit(EXIT_FAILURE);
           }

           struct addrinfo hints;
           struct addrinfo *result, *rp;
           int sfd, s;
           size_t len;
           ssize_t nread;
           char buf[BUF_SIZE];

           memset(&hints, 0, sizeof(hints));
           hints.ai_family = AF_UNSPEC;    
           hints.ai_socktype = SOCK_STREAM;
           hints.ai_flags = 0;
           hints.ai_protocol = IPPROTO_TCP;   
           
           s = getaddrinfo(argv[1], argv[2], &hints, &result);
           if (s != 0) {
               fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
               exit(EXIT_FAILURE);
           }


           for (rp = result; rp != NULL; rp = rp->ai_next) {
               sfd = socket(rp->ai_family, SOCK_STREAM,
                            IPPROTO_TCP);
               if (sfd == -1)
                   continue;

               if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
                   break;                  

               close(sfd);
           }

           freeaddrinfo(result);           

           if (rp == NULL) {               
               fprintf(stderr, "Could not connect\n");
               exit(EXIT_FAILURE);
           }    
 

    std::string request;
    char response[256] = {0};
    ssize_t recv_bytes;
       
    while (true)
    {
    if (!std::getline(std::cin, request)) break;
    if (send(sfd, request.c_str(), sizeof(request) - 1, 0) < 0)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    while (true)
        { 
        recv_bytes = recv(sfd, response, sizeof(response) - 1, 0);
        if (recv_bytes > 0)
        {   
        std::cout << response << std::endl; 
        } else if (-1 == recv_bytes)
            {
                if (EINTR == errno) continue;
                if (0 == errno) break;
            }
            break;
         }  
     }
     return EXIT_FAILURE;
}       
