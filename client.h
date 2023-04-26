//
// Created by 路遥 on 2023/4/24.
//

#ifndef MAIN_CLIENT_H
#define MAIN_CLIENT_H


#define SERVER_ADDRESS "121.89.240.66"
#define SERVER_PORT 8888
#define BUF_SIZE 1024

int client_socket;



int setup_client();
int send_msg(int sock, char *msg);

#endif //MAIN_CLIENT_H
