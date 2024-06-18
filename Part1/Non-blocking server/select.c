#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 24
#define MAX_CLIENTS 10000 // Increased the number of maximum clients
#define BUFFER_SIZE 1000

int main() {
    int server_socket, new_socket, max_sd, activity;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    fd_set readfds;
    int client_sockets[MAX_CLIENTS], valread;
    char buffer[BUFFER_SIZE];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("Listening failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        max_sd = server_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_socket = client_sockets[i];

            if (client_socket > 0) {
                FD_SET(client_socket, &readfds);
            }

            if (client_socket > max_sd) {
                max_sd = client_socket;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("Select error");
        }

        if (FD_ISSET(server_socket, &readfds)) {
            if ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size)) != -1) {
                printf("New connection, socket fd is %d\n", new_socket);

                int i;
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (client_sockets[i] == 0) {
                        client_sockets[i] = new_socket;
                        break;
                    }
                }
                if (i == MAX_CLIENTS) {
                    printf("Cannot accept more clients\n");
                    close(new_socket);
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_socket = client_sockets[i];

            if (FD_ISSET(client_socket, &readfds)) {
                if ((valread = read(client_socket, buffer, BUFFER_SIZE)) == 0) {
                    printf("Host disconnected, socket fd is %d\n", client_socket);
                    close(client_socket);
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    printf("Client Sent: %s\n", buffer);

                    if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                        perror("Error sending response");
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}

