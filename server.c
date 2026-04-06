#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#define MAX_CLIENTS 64
#define BUF_SIZE 256

struct ClientState {
    int fd;
    int state; 
    char name[100];
    char mssv[20];
};

void generate_email(char *name, char *mssv, char *email) {
    char temp_name[100];
    strcpy(temp_name, name);
    for (int i = 0; temp_name[i]; i++) temp_name[i] = tolower(temp_name[i]);

    char *words[10];
    int count = 0;
    char *token = strtok(temp_name, " ");
    while (token != NULL && count < 10) {
        words[count++] = token;
        token = strtok(NULL, " ");
    }

    if (count == 0) {
        strcpy(email, "unknown@sis.hust.edu.vn");
        return;
    }

    strcpy(email, words[count - 1]);
    strcat(email, ".");
    
    for (int i = 0; i < count - 1; i++) {
        int len = strlen(email);
        email[len] = words[i][0];
        email[len + 1] = '\0';
    }
    
    strcat(email, mssv);
    strcat(email, "@sis.hust.edu.vn");
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    listen(listener, 5);
    
    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    printf("Server dang lang nghe tren cong 9000...\n");

    struct ClientState clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].state = 0;

    fd_set fdread;
    while (1) {
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);
        int max_fd = listener;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].state > 0) {
                FD_SET(clients[i].fd, &fdread);
                if (clients[i].fd > max_fd) max_fd = clients[i].fd;
            }
        }

        if (select(max_fd + 1, &fdread, NULL, NULL, NULL) < 0) break;

        if (FD_ISSET(listener, &fdread)) {
            int client_fd = accept(listener, NULL, NULL);
            if (client_fd > 0) {
                ioctl(client_fd, FIONBIO, &ul);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].state == 0) {
                        clients[i].fd = client_fd;
                        clients[i].state = 1;
                        printf("[LOG] Client %d ket noi moi.\n", client_fd);
                        char *msg = "Vui long nhap Ho ten: ";
                        send(client_fd, msg, strlen(msg), 0);
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].state > 0 && FD_ISSET(clients[i].fd, &fdread)) {
                char buf[BUF_SIZE];
                int n = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
                
                if (n <= 0) {
                    printf("[LOG] Client %d ngat ket noi.\n", clients[i].fd);
                    close(clients[i].fd);
                    clients[i].state = 0;
                    continue;
                }

                buf[n] = '\0';
                while(n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) {
                    buf[--n] = '\0';
                }

                if (clients[i].state == 1) {
                    printf("[RECV] Client %d - Ten: %s\n", clients[i].fd, buf);
                    strcpy(clients[i].name, buf);
                    clients[i].state = 2;
                    char *msg = "Vui long nhap MSSV: ";
                    send(clients[i].fd, msg, strlen(msg), 0);
                } else if (clients[i].state == 2) {
                    printf("[RECV] Client %d - MSSV: %s\n", clients[i].fd, buf);
                    strcpy(clients[i].mssv, buf);
                    
                    char email[150], response[256];
                    generate_email(clients[i].name, clients[i].mssv, email);
                    
                    printf("[SEND] Client %d - Email tao ra: %s\n", clients[i].fd, email);
                    
                    sprintf(response, "Email: %s\n--------------------\nNhap Ho ten moi: ", email);
                    send(clients[i].fd, response, strlen(response), 0);
                    clients[i].state = 1;
                }
            }
        }
    }
    return 0;
}