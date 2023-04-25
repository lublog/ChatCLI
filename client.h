//
// Created by 路遥 on 2023/4/24.
//

#ifndef MAIN_CLIENT_H
#define MAIN_CLIENT_H


#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 8080
#define BUF_SIZE 1024

int client_socket;



int setup_client();
void* receive_messages(void * win_output);
int send_msg(int sock, char *msg);
#endif //MAIN_CLIENT_H
