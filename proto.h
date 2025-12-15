#ifndef PROTO_H__
#define PROTO_H__

#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"

#define MAX_MSG_LEN 1024

//消息类型
typedef enum {
    MSG_TYPE_DATA = 1,            //数据消息
    MSG_TYPE_LOGIN,      //登录消息
    MSG_TYPE_LOGOUT         //登出消息
} msg_type_t;

//消息结构体
typedef struct {
    msg_type_t type;               //消息类型
    char data[MAX_MSG_LEN];        //消息数据
} message_st;

#endif // PROTO_H__