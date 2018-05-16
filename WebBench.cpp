// WebBench.cpp : 定义控制台应用程序的入口点。
//
/*注意不同版本下请求头的要求
*/

//#include "stdafx.h"
#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include "Socket.h"

using namespace std;

void Get_request(const char*);
int Get_clients();
void Get_socket();

struct option long_option[] = {
	{"time",2, NULL,'t'},
	{"client_nums",2,NULL,'c'},
	{"help",0,NULL,'h'}
};

void help_information() {
	fprintf(stderr,
		"Please check your parametes:\n"
		"WebBench [option][URL]\n"
		"-t | --time <sec>	Run each client <sec> seconds, default 60\n"
		"-c | --client_nums <n>	Run n clients, default 1\n"
		"-h | --help		Get help information"
		"\n"
	);
}

char request[1024];
char Hostname[1024];
int Hostport = 80;
int Time = 60;				//设置默认执行时间和访问客户端数
int clients = 1;
int time_flag = 0;

//请求记录
int Succeed = 0;
int Failed = 0;
int bytes = 0;

void Alarm(int signal) {
	//fprintf(stdout, "Time out");
	time_flag = 1;
}

int main(int argc,char *argv[])
{
	if (argc < 2) {				//判断参数是否符合要求
		help_information();
		return 1;				//返回1 对应参数错误
	}

	int opt;
	int opt_index;

	while ((opt = getopt_long(argc, argv, "t::c::h", long_option, &opt_index)) != -1) {
		switch (opt) {
		case('t'):
			if (optarg != NULL)
				Time = atoi(optarg);
			break;
		case('c'):	if (optarg != NULL) clients = atoi(optarg); break;
		case('h'):
		case('?'):	help_information();break;
		}
	}
	fprintf(stdout, "Time: %d sec clients: %d\n", Time, clients);
	if (optind == argc) {		//如果有URL，则optind应该比argc小1，optind代表要处理的下一个参数的索引
		fprintf(stderr, "ERROR: URL is needed for the WebBench\n");
		return 1;
	}
	Get_request(argv[optind]);		//获取请求部分

	return Get_clients();			//建立客户端，发起连接
}

void Get_request(const char* argv) {
	//目前暂时只支持GET方式
	strcpy(request, "GET ");

	//检测URL是否合法
	if (strncasecmp(argv, "http://", 7) != 0) {
		fprintf(stderr, "Error: Can only support http protocol now.\n");
		exit(1);
	}
	else if (strlen(argv) > 1024) {
		fprintf(stderr, "Error: URL is too long, it shoule be less than 1024 characters.\n");
		exit(1);
	}
	else if (strchr(argv + 7, '/') == NULL) {
		fprintf(stderr, "Error: The host name must end with a \'/\'.\n");
		exit(1);
	}

	int index_path = strchr(argv + 7, '/') - argv;			//首先找到://之后的第一个'/'的位置
	//判断URL中是否包含端口号
	if (strchr(argv + 7, ':') != NULL) {
		int index = strchr(argv + 7, ':') - argv;
		char hostport[1024];
		strncpy(hostport, argv + index + 1, index_path-index-1);
		Hostport = atoi(hostport);
		fprintf(stdout, "Port: %d\n", Hostport);

		//获取主机名
		strncpy(Hostname, argv + 7, index - 7);
		fprintf(stdout, "Hostname: %s\n", Hostname);
	}
	else {
		//获取主机名
		strncpy(Hostname, argv + 7, index_path - 7);
		fprintf(stdout, "Hostname: %s\n", Hostname);
	}
	
	//目前暂时不支持代理模式，因此仅需获取URL中的path路径
	strcat(request, argv + index_path);
	strcat(request, " ");

	//目前只支持HTTP1.1
	strcat(request, "HTTP/1.1\r\n");
	strcat(request, "User-Agent: WebBench 1.0\r\n");
	strcat(request, "Host: ");			//HTTP1.1必须指定Host字段
	strcat(request, Hostname);
	strcat(request, "\r\n");
	//strcat(request, "Connection: close\r\n");
	strcat(request, "\r\n");			//注意HTTP请求的格式，请求头和请求数据间有一个空行，
										//非常重要一定不能遗漏，缺失会造成问题。

	fprintf(stdout, "request: %s", request);
}

int Get_clients() {
	int fd[2];
	pid_t pid;
	FILE* f;

	if (pipe(fd) != 0) {
		fprintf(stderr, "Error: pipe failed\n");
		return 2;
	}

	for (int i = 0; i < clients; ++i) {
		pid = fork();
		if (pid == 0)
			break;
		if (pid < 0) {
			fprintf(stderr, "Error: fork failed\n");
			return 2;
		}
	}
	if (pid == 0) {		//子进程
		Get_socket();			//发起连接
		f = fdopen(fd[1], "w");
		if (f == NULL) {
			fprintf(stderr, "Error: read form pipe failed");
			return 2;
		}
		fprintf(f, "%d %d %d\n", Succeed, Failed, bytes);
		fclose(f);
		return 0;
	}
	else if (pid != 0) {
		f = fdopen(fd[0], "r");
		if (f == NULL) {
			fprintf(stderr, "Error: write to pipe failed");
			return 2;
		}
		int temp_succeed = 0, temp_failed = 0, temp_bytes = 0;
		while (clients) {
			if (fscanf(f, "%d %d %d", &temp_succeed, &temp_failed, &temp_bytes) < 3) {
				fprintf(stderr, "Error: some data is lossed\n");
				break;
			}
			Succeed += temp_succeed;
			Failed += temp_failed;
			bytes += temp_bytes;
			--clients;
		}
		fclose(f);
		fprintf(stdout, "Requests: %d succeed, %d failed\nSpeed: %d requests/min, %d bytes/sec\n", \
			Succeed, Failed, (int)((Succeed + Failed) / ((float)Time / 60)), (bytes) / Time);
		return 0;
	}
}

void Get_socket() {
	char response[1024];
	struct sigaction sa;
	sa.sa_handler = Alarm;
	sa.sa_flags = 0;
	if (sigaction(SIGALRM, &sa,NULL)) {		//sigaction返回-1表示处理失败
		fprintf(stderr, "Error: Signal processing failed\n");
		exit(2);
	}
	//设定定时器
	alarm(Time);
	//开始对指定URL发起请求，并记录成功和失败次数
	while (1) {
		if (time_flag) {			//首先判断是否超时
			fprintf(stdout, "%d %d %d\n", Succeed, Failed, bytes);
			return;
		}

		//未超时对指定URL发起连接
		int sockfd;
		sockfd = Socket(Hostname, Hostport);
		if (sockfd < 0) {			//连接失败
			++Failed;
			continue;
		}
		//接着写入HTTP请求
		int flag = write(sockfd, request, strlen(request));
		if (flag < 0) {
			++Failed;
			close(sockfd);
			continue;
		}
		
		//读入响应
		int temp_flag = 0;						//读取标志量，判断是否读取失败需从头再来
		while (1) {
			if (time_flag) {
				break;
			}
			int temp_size = read(sockfd, response, 1024);
			if (temp_size < 0) {				//读取失败
				++Failed;
				close(sockfd);
				temp_flag = 1;					//设置标志量
				break;
			}
			else if (temp_size == 0) {			//读取完毕
				break;
			}
			else {								//记录读取的字节数
				bytes += temp_size;
			}
		}
	
		if (temp_flag == 1)
			continue;
		//关闭连接
		if (close(sockfd)) {
			++Failed;
			continue;
		}
		++Succeed;
	}

}