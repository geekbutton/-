#include "Socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <iostream>

int Socket(char* hostname,int host_port) {
	//一个包裹函数，包含了socket和connect两个行为
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return -1;
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(host_port);

	
	//综合考虑URL中主机名的情况(可以是DNS中的主机名或IP地址)
	//in_addr_t temp_addr = inet_addr(hostname);
	//if (temp_addr != INADDR_NONE) {					//URI中为IP地址
	//	server_addr.sin_addr.s_addr = temp_addr;
	//}
	//else {											//URI中为域名
		struct hostent *temp_host;			
		temp_host = gethostbyname(hostname);
		if (temp_host == NULL)
			return -1;
		//fprintf(stdout, "%s\n", temp_host->h_name);
		//复制主机的IP地址到对应结构体中
		memcpy(&server_addr.sin_addr, *(temp_host->h_addr_list), temp_host->h_length);
	//}
	//fprintf(stdout, "%s\n", inet_ntoa(server_addr.sin_addr));
	if (connect(sockfd, (sockaddr*) &server_addr, sizeof(server_addr)) < 0)
		return -1; 
	return sockfd;
}