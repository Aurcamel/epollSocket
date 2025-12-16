#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "proto.h"

#define MAX_EPOLL_EVENTS 1024

static void process_message(const message_st *msg, const int connfd);
static void echo_message(const message_st *msg, const int connfd, const int epfd);

int main(){

    // Create a TCP socket
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1){
        perror("socket error");
        exit(-1);
    }
    // bind
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("bind error");
        close(listenfd);
        exit(1);
    }
    // listen
    if (listen(listenfd, 45) == -1){
        perror("listen error");
        close(listenfd);
        exit(1);
    }

    // epoll
    int epfd = epoll_create(1);
    if (epfd == -1){
        perror("epoll_create error");
        close(listenfd);
        exit(1);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

    puts("Server is running and waiting for connections...");
    // Event loop
    struct epoll_event events[MAX_EPOLL_EVENTS];
    while (1){
        int nready = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, -1);
        if (nready == -1){
            if (errno == EINTR)
                continue;
            perror("epoll_wait error");
            break;
        }
        for (int i = 0; i < nready; i++){
            if (events[i].data.fd == listenfd){
                // Accept new connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_len);
                if (connfd == -1){
                    perror("accept error");
                    continue;
                }
                // Set non-blocking mode
                int flags = fcntl(connfd, F_GETFL);
                fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

                // Add new socket to epoll
                ev.events = EPOLLIN;
                ev.data.fd = connfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
                printf("New connection accepted: %d\n", connfd);
            }
            else{
                // Handle data from connected client
                int connfd = events[i].data.fd;
                message_st msg;
                ssize_t n = recv(connfd, &msg, sizeof(msg), 0);
                if (n <= 0){
                    if (n == 0){
                        printf("Client disconnected: %d\n", connfd);
                    }
                    else{
                        perror("recv error");
                    }
                    close(connfd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
                }
                else{
                    // Process the received message
                    process_message(&msg, connfd);
                    // Echo back the message
                    echo_message(&msg, connfd, epfd);
                }
            }
        }
    }

    close(epfd);
    close(listenfd);
    exit(0);
}

// Process the received message
static void process_message(const message_st *msg, const int connfd){
    char client_ip[INET_ADDRSTRLEN];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(connfd, (struct sockaddr *)&client_addr, &addr_len);
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    switch (msg->type){
    case MSG_TYPE_DATA:
        printf("Received DATA message from %s %d:\n %s\n", client_ip, ntohs(client_addr.sin_port) ,msg->data);
        break;
    case MSG_TYPE_LOGIN:
        printf("%s LOGIN\n", client_ip);
        break;
    case MSG_TYPE_LOGOUT:
        printf("%s LOGOUT\n", client_ip);
        break;
    default:
        break;
    }
}

// Echo back the received message
static void echo_message(const message_st *msg, const int connfd, const int epfd){
    message_st echo_msg;
    echo_msg.type = msg->type;
    switch(msg->type){
        case MSG_TYPE_DATA:
            snprintf(echo_msg.data, MAX_MSG_LEN, "Send successfully!");
            break;
        case MSG_TYPE_LOGIN:
            snprintf(echo_msg.data, MAX_MSG_LEN, "Welcome!");
            break;
        case MSG_TYPE_LOGOUT:
            snprintf(echo_msg.data, MAX_MSG_LEN, "Goodbye!");
            break;
        default:
            snprintf(echo_msg.data, MAX_MSG_LEN, "Unknown message type");
            break;
    }
    ssize_t n = send(connfd, &echo_msg, sizeof(echo_msg), 0);
    if (n == -1){
        perror("send error");
    }
    if(msg->type == MSG_TYPE_LOGOUT){
        close(connfd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
    }
}