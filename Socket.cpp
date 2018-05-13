#include "Socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

int Socket(char* hostname,int host_port) {
	//一个包裹函数，包含了socket和connect两个行为
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return -1;
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(host_port);

	
	//当前仅考虑了hostname为字符串的情况
	struct hostent *temp_host;
	temp_host = gethostbyname(hostname);
	if (temp_host == NULL)
		return -1;
	memcpy(&server_addr.sin_addr, *(temp_host->h_addr_list), temp_host->h_length);
	if (connect(sockfd, (sockaddr*) &server_addr, sizeof(server_addr)) < 0)
		return -1; 
	return sockfd;
}
