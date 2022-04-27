#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

#include <socket_wrapper/socket_class.h>
#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>

#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>


// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    std::not1(std::ptr_fun<int, int>(std::isspace))));
}


// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace)))
                    .base(),
            s.end());
}


// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}


// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}


int main(int argc, char const *argv[]) {

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    socket_wrapper::SocketWrapper sock_wrap;
    const int port{std::stoi(argv[1])};

    socket_wrapper::Socket sock = {AF_INET, SOCK_DGRAM, IPPROTO_UDP};

    std::cout << "Starting echo server on the port " << port << "...\n";

    if (!sock) {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in addr =
            {
                    .sin_family = PF_INET,
                    .sin_port = htons(port),
            };

    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) != 0) {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        // Socket will be closed in the Socket destructor.
        return EXIT_FAILURE;
    }

    char buffer[256];

    // socket address used to store client address
    struct sockaddr_in client_address = {0};
    socklen_t client_address_len = sizeof(sockaddr_in);
    ssize_t recv_len = 0;

    std::cout << "Running echo server...\n"
              << std::endl;
    char client_address_buf[INET_ADDRSTRLEN];

    while (true) {
        // Read content into buffer from an incoming client.
        recv_len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            reinterpret_cast<sockaddr *>(&client_address),
                            &client_address_len);

        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            std::cout
                    << "Client with address "
                    << inet_ntop(AF_INET, &client_address.sin_addr, client_address_buf, sizeof(client_address_buf) / sizeof(client_address_buf[0]))
                    << ":" << ntohs(client_address.sin_port)
                    << std::endl
                    << "Client with name ";
            int error;
            char hostname[NI_MAXHOST];
            error = getnameinfo((struct sockaddr *) &client_address, client_address_len, hostname, NI_MAXHOST, NULL, 0, 0);
            if (error != 0) {
                fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
                continue;
            }
            if (*hostname != '\0')
                std::cout
                        << hostname
                        << " sent datagram "
                        << "[length = "
                        << recv_len
                        << "]:\n'''\n"
                        << buffer
                        << "\n'''"
                        << std::endl;
            char exit_command[] = "connection closed";
            std::string command_string = {buffer, 0, strlen(buffer)};
            trim(command_string);
            std::cout << command_string << std::endl;
            if ("exit" == command_string) {
                sendto(sock, exit_command, strlen(exit_command), 0, reinterpret_cast<const sockaddr *>(&client_address),
                       client_address_len);
                close(sock);
            } else {
                sendto(sock, buffer, recv_len, 0, reinterpret_cast<const sockaddr *>(&client_address),
                       client_address_len);
                sendto(sock, hostname, strlen(hostname), 0, reinterpret_cast<const sockaddr *>(&client_address),
                       client_address_len);
            }
        }
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}
