#include "socketFunctions.h"
#include <stdio.h>
#include <string.h>

#ifdef linux
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/tcp.h>

#endif
#ifdef _UNIX
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/tcp.h>
#endif
#ifdef __WINDOWS_
#include <winsock2.h>

#endif
#ifdef _WIN32
#include <winsock2.h>

#endif

#pragma comment(lib, "Ws2_32.lib")

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#pragma comment(lib, "Ws2_32.lib")

#define BACKLOG 5 

void recv_data(int socket, char *d_p, int length)
{
	int l = 0;
	int suml = length;
	while (true)
	{
		int m_length = recv(socket, d_p, length, 0);
		if (m_length == -1)
			continue;
		l += m_length;
		if (l == suml)
			return;
		else
		{
			d_p = d_p + m_length;
			length = length - m_length;
		}
	}
}

void send_frame(int socket, char *d_p, int length)
{
	int sended = 0;
	while (true)
	{
		int sendedLength = send(socket, d_p + sended, length-sended , 0);
		sended += sendedLength;
		if (sended == length)
			return;
	}
}

void init_socket()
{
#ifdef __WINDOWS_
	WORD versionRequired;
	WSADATA wsadata;
	versionRequired = MAKEWORD(2, 2);
	WSAStartup(versionRequired, &wsadata);
#endif
#ifdef _WIN32
	WORD versionRequired;
	WSADATA wsadata;
	versionRequired = MAKEWORD(2, 2);
	WSAStartup(versionRequired, &wsadata);

#endif
}

int create_socket()
{
	int sockfd;
	int on,ret;
	
	
	
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	ret = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
	
	if (sockfd == INVALID_SOCKET)
	{
		perror("socket create failed");
		return -1;
	}
	

	return sockfd;
}

int bind_listen(int sockfd, int port)
{
	int new_fd;

	struct sockaddr_in my_addr;


	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), 0, 8);

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == SOCKET_ERROR)
	{
		printf("bind failed!");
		return -1;
	}

	if (listen(sockfd, BACKLOG) == SOCKET_ERROR)
	{
		printf("listen failed");
		return -1;
	}
	return 0;
	
}

int accept_client(int sockfd)
{
	int new_fd;
	int sin_size = sizeof(struct sockaddr_in);

	struct sockaddr_in their_addr;
	printf("waiting for connect\n");
#ifdef linux
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, (socklen_t *)&sin_size);
#endif
#ifdef _UNIX
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, (socklen_t *)&sin_size);
#endif
#ifdef __WINDOWS_
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
#endif
#ifdef _WIN32
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
#endif
	printf("connected\n");

	return new_fd;
}
