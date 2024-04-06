#ifndef SOCKETFUNCTIONS_H
#define SOCKETFUNCTIONS_H

/**************************
���ƣ�socketFunctions.h
����������socketͨ����صĺ�������
***************************/




/**************************
���ƣ�recv_data(int socket, char *d_p, int length)
���������տͻ��˷������ݵĺ���
������int socket��   �ͻ����׽���
      char *d_p:     ���ջ�����
      int length:    ���ճ���
����ֵ����
��ע����������
***************************/
void recv_data(int socket, char *d_p, int length);

/**************************
���ƣ�void send_frame(int socket, char *d_p, int length);
��������ͻ��˷������ݵĺ���
������ int socket:   �ͻ����׽���
       char *d_p :   ���������ڴ�ָ��
       int length:   ���ͳ���
***************************/
void send_frame(int socket, char *d_p, int length);

/**************************
���ƣ�void init_socket();
��������ʼ���׽���
������ ��
����ֵ�� ��
***************************/
void init_socket();

/**************************
���ƣ�int create_socket();
�����������׽���
��������
����ֵ�� �����õ��׽���id
***************************/
int create_socket();

/**************************
���ƣ�int bind_listen(int sockfd, int port);
���������׽��ְ󶨵�ָ���˿�
������ int sockfd�� ���׽���id
       int port  :  �󶨶˿�
����ֵ�� 0�󶨳ɹ�����-1��ʧ��
***************************/
int bind_listen(int sockfd, int port);

/**************************
���ƣ�int accept_client(int sockfd);
�������ȴ��ͻ������Ӻ���
������int sockfd:   �������׽���
����ֵ�� �󶨵��׽���
��ע�� Ϊ��������
***************************/
int accept_client(int sockfd);



#endif
