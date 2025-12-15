#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

#include "proto.h"

#define CLIENT_IP "192.168.91.130"

static void receive_message(const int sockfd);

int main(){

    // Create a TCP socket
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("socket error");
        exit(1);
    }
    // Connect to server
    struct sockaddr_in server_addr, client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(0); // Let OS choose the port
    client_addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1){
        perror("bind error");
        close(sockfd);
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("connect error");
        close(sockfd);
        exit(1);
    }
    printf("Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);
    // Communicate with server
    message_st msg;
    msg.type = MSG_TYPE_LOGIN;
    memset(msg.data, 0, MAX_MSG_LEN);
    send(sockfd, &msg, sizeof(msg), 0);
    receive_message(sockfd);

    msg.type = MSG_TYPE_DATA;
    while (1){
        printf("Enter: ");
        fgets(msg.data, MAX_MSG_LEN, stdin);
        if(strncmp(msg.data, "exit", 4) == 0){
            msg.type = MSG_TYPE_LOGOUT;
            send(sockfd, &msg, sizeof(msg), 0);
            receive_message(sockfd);
            break;
        }
        send(sockfd, &msg, sizeof(msg), 0);
        receive_message(sockfd);
    }
    // Close the socket
    close(sockfd);
    exit(0);
}

static void receive_message(const int sockfd){
    message_st msg;
    ssize_t n = recv(sockfd, &msg, sizeof(msg), 0);
    if (n > 0){
        printf("Received from server: %s\n", msg.data);
    }
    else if (n == 0){
        printf("Server closed connection.\n");
    }
    else{
        perror("recv error");
    }
}