//
// Created by 路遥 on 2023/4/24.
//

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include "client.h"

bool exit_flag = false;
struct sockaddr_in server_address;


/*
 * set up client socket
 * @return client socket fd
 */
int setup_client() {
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating client socket");
        exit(1);
    }

    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_address.sin_port = htons(SERVER_PORT);

    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0 && errno != EINPROGRESS) {
        perror("Error connecting to server");
        exit(1);
    }

    return client_socket;
}



int send_msg(int sock, char *msg) {
    char buf[BUF_SIZE];
    int num_bytes = snprintf(buf, BUF_SIZE, "%s\n", msg);
    if (send(sock, buf, num_bytes, 0) == -1) {
        perror("Error sending message");
        exit(1);
    }
    return 0;
}

