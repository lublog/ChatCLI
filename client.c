//
// Created by 路遥 on 2023/4/24.
//

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
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

void* receive_messages(void * win_output) {
    char buf[BUF_SIZE];
    int num_bytes;

    while (!exit_flag) {
        if ((num_bytes = recv(client_socket, buf, BUF_SIZE, 0)) > 0) {
            buf[num_bytes] = '\0';
            waddstr(win_output, buf);
            wrefresh(win_output);
        }

        if (num_bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Error receiving message");
            exit(1);
        }
    }

    pthread_exit(NULL);
}

//int send_msg(int sock, char *msg) {
//    char buf[BUF_SIZE];
//    int num_bytes = snprintf(buf, BUF_SIZE, "%s\n", msg);
//    if (send(sock, buf, num_bytes, 0) == -1) {
//        perror("Error sending message");
//        exit(1);
//    }
//    return 0;
//}

//int main() {
//    // 定义一个线程ID变量
//    pthread_t tid;
//
//    // 客户端初始化
//    client_socket = setup_client();
//    pthread_create(&tid, NULL, receive_messages, NULL);
//
//    while (true) {
//        char message[BUF_SIZE];
//
//        int c = wgetch(win_input);
//
//        if (c == KEY_ENTER || c == '\n') {
//            waddch(win_output, '\n');
//            wrefresh(win_output);
//
//            char buf[BUF_SIZE];
//            int num_bytes = snprintf(buf, BUF_SIZE, "%s\n", message);
//            if (send(client_socket, buf, num_bytes, 0) == -1) {
//                perror("Error sending message");
//                exit(1);
//            }
//
//            if (strcmp(message, "quit\n") == 0) {
//                exit_flag = true;
//                break;
//            }
//
//            memset(message, 0, BUF_SIZE);
//        } else if (c == KEY_BACKSPACE || c == '\b' || c == 127) {
//            int len = strlen(message);
//            message[len - 1] = '\0';
//            wdelch(win_input);
//            wrefresh(win_input);
//        } else {
//            int len = strlen(message);
//            message[len] = c;
//            message[len + 1] = '\0';
//            waddch(win_input, c);
//            wrefresh(win_input);
//        }
//    }
//
//    close(client_socket);
//    pthread_join(tid, NULL);
//
//    return 0;
//}

