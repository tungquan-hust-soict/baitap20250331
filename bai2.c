#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/ioctl.h>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Cú pháp: udp_chat <port_s> <ip_d> <port_d>\n");
        return 1;
    }
    int port_s = atoi((char*)argv[1]);
    char* ip_d = argv[2];
    int port_d = atoi(argv[3]);
    
    int my_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    unsigned long ul = 1;
    ioctl(my_socket, FIONBIO, &ul);

    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(port_s);
    bind(my_socket, (struct sockaddr*)&my_addr, sizeof(my_addr));

    struct sockaddr_in d_addr;
    d_addr.sin_family = AF_INET;
    d_addr.sin_addr.s_addr = inet_addr(ip_d);
    d_addr.sin_port = htons(port_d);

    fd_set fdread;
    char buf[256];
    while (1) {
        FD_ZERO(&fdread);
        FD_SET(STDIN_FILENO, &fdread);
        FD_SET(my_socket, &fdread);
        
        select(my_socket + 1, &fdread, NULL, NULL, NULL);
        
        if (FD_ISSET(STDIN_FILENO, &fdread)) {
            fgets(buf, sizeof(buf), stdin);
            sendto(my_socket, buf, strlen(buf), 0, (struct sockaddr*)&d_addr, sizeof(d_addr));
        }
        
        if (FD_ISSET(my_socket, &fdread)) {
            int ret = recvfrom(my_socket, buf, sizeof(buf) - 1, 0, NULL, NULL);
            if (ret == -1) {
                if (errno != EWOULDBLOCK) {
                    break;
                }
            } else if (ret > 0) {
                buf[ret] = 0;
                printf("Tin nhan nhan duoc: %s\n", buf);
            }
        }
    }
    close(my_socket);
    return 0;
}