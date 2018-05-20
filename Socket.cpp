#include "Socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <iostream>

int Socket(char* hostname,int host_port) {
	//һ������������������socket��connect������Ϊ
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return -1;
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(host_port);

	
	//�ۺϿ���URL�������������(������DNS�е���������IP��ַ)
	//in_addr_t temp_addr = inet_addr(hostname);
	//if (temp_addr != INADDR_NONE) {					//URI��ΪIP��ַ
	//	server_addr.sin_addr.s_addr = temp_addr;
	//}
	//else {											//URI��Ϊ����
		struct hostent *temp_host;			
		temp_host = gethostbyname(hostname);
		if (temp_host == NULL)
			return -1;
		//fprintf(stdout, "%s\n", temp_host->h_name);
		//����������IP��ַ����Ӧ�ṹ����
		memcpy(&server_addr.sin_addr, *(temp_host->h_addr_list), temp_host->h_length);
	//}
	//fprintf(stdout, "%s\n", inet_ntoa(server_addr.sin_addr));
	if (connect(sockfd, (sockaddr*) &server_addr, sizeof(server_addr)) < 0)
		return -1; 
	return sockfd;
}