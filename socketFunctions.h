#ifndef SOCKETFUNCTIONS_H
#define SOCKETFUNCTIONS_H

/**************************
名称：socketFunctions.h
描述：关于socket通信相关的函数集合
***************************/




/**************************
名称：recv_data(int socket, char *d_p, int length)
描述：接收客户端发送数据的函数
参数：int socket：   客户端套接字
      char *d_p:     接收缓冲区
      int length:    接收长度
返回值：无
备注：阻塞函数
***************************/
void recv_data(int socket, char *d_p, int length);

/**************************
名称：void send_frame(int socket, char *d_p, int length);
描述：想客户端发送数据的函数
参数： int socket:   客户端套接字
       char *d_p :   发送数据内存指针
       int length:   发送长度
***************************/
void send_frame(int socket, char *d_p, int length);

/**************************
名称：void init_socket();
描述：初始化套接字
参数： 无
返回值： 无
***************************/
void init_socket();

/**************************
名称：int create_socket();
描述：创建套接字
参数：无
返回值： 创建好的套接字id
***************************/
int create_socket();

/**************************
名称：int bind_listen(int sockfd, int port);
描述：将套接字绑定到指定端口
参数： int sockfd： 绑定套接字id
       int port  :  绑定端口
返回值： 0绑定成功，，-1绑定失败
***************************/
int bind_listen(int sockfd, int port);

/**************************
名称：int accept_client(int sockfd);
描述：等待客户端连接函数
参数：int sockfd:   服务器套接字
返回值： 绑定的套接字
备注： 为阻塞函数
***************************/
int accept_client(int sockfd);



#endif
