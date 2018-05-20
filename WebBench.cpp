// WebBench.cpp : 定义控制台应用程序的入口点。
//
/*注意不同版本下请求头的要求
程序返回值：
0：程序成功执行并返回
1：输入参数或URL存在问题
2：其他错误信息
*/

/*
HTTP0.9: 仅支持GET，不支持请求头等
HTTP1.0: 增加了GET,HEAD,POST
HTTP1.1: 增加了OPTIONS,PUT,DELETE,TRACE,CONNECT
*/

/*常见状态码：
1xx：指示信息--表示请求已接收，继续处理
2xx：成功--表示请求已被成功接收、理解、接受
3xx：重定向--要完成请求必须进行更进一步的操作
4xx：客户端错误--请求有语法错误或请求无法实现
5xx：服务器端错误--服务器未能实现合法的请求
200 OK                        //客户端请求成功
301 redirect				  //代表永久性转移
302 redirect				  //代表暂时性转移
400 Bad Request               //客户端请求有语法错误，不能被服务器所理解
401 Unauthorized              //请求未经授权，这个状态代码必须和WWW-Authenticate报头域一起使用
403 Forbidden                 //服务器收到请求，但是拒绝提供服务
404 Not Found                 //请求资源不存在，eg：输入了错误的URL
500 Internal Server Error     //服务器发生不可预期的错误
503 Server Unavailable        //服务器当前不能处理客户端的请求，一段时间后可能恢复正常
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
void Get_socket(char*);

int head_v = 0;			//request请求方法，0,1,2,3对应GET,HEAD,OPTIONS,TRACE,默认为GET
struct option long_option[] = {
	{"time",2, NULL,'t'},
	{"client_nums",2,NULL,'c'},
	{"http09",0,NULL,'0'},
	{"http10",0,NULL,'1'},
	{"http11",0,NULL,'2'},
	{"GET",0,&head_v,0},
	{"HEAD",0,&head_v,1},
	{"OPTIONS",0,&head_v,2},
	{"TRACE",0,&head_v,3},
	{"proxy",1,NULL,'p'},
	{"help",0,NULL,'h'}
};

void help_information() {
	fprintf(stderr,
		"Please check your parametes:\n"
		"WebBench [option][URL]\n"
		"	-t | --time <sec>	Run each client <sec> seconds, default 60\n"
		"	-c | --client_nums <n>	Run n clients, default 1\n"
		"	-0 | --http09		HTTP-0.9 Version\n"
		"	-1 | --http10		HTTP-1.0 Version\n"
		"	-2 | --http11		HTTP-1.1 Version\n"
		"	--GET			GET request\n"
		"	--HEAD			HEAD request\n"	
		"	--OPTIONS		OPTIONS request (only for HTTP1.1)\n"
		"	--TRACE			TRACE request (only for HTTP1.1)\n"
		"	-p | --proxy<name:port>Using proxy server\n"
		"	-h | --help		Get help information"
		"\n"
	);
}

char request[1024];			//HTTP请求(请求行，请求头，回车换行，请求数据)
char Hostname[1024];		
int Hostport = 80;
int proxy_flag = 0;
char Proxyname[1024];
int Time = 60;				//设置默认执行时间和访问客户端数
int clients = 1;
int http_v = 2;				//HTTP版本，0,1,2分别对应http0.9,http1.0,http1.1,默认为http1.1
int time_flag = 0;			//超时标志位

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
	char* proxy_index = NULL;

	while ((opt = getopt_long(argc, argv, "t::c::012p:h", long_option, &opt_index)) != -1) {
		switch (opt) {
		case('t'):	if (optarg != NULL)	Time = atoi(optarg); break;
		case('c'):	if (optarg != NULL) clients = atoi(optarg); break;
		case('1'):	http_v = 1; break;
		case('2'):	http_v = 2; break;
		case('p'):	
			proxy_index = strchr(optarg, ':');
			if (proxy_index == NULL) {
				fprintf(stdout, "Error: hostname and port is both needed for a proxy server.\n");
				return 1;
			}
			strncpy(Proxyname,optarg,(proxy_index-optarg));
			Hostport = atoi(proxy_index + 1);
			proxy_flag = 1;
			fprintf(stdout, "proxy: %s %d\n", Proxyname,Hostport);
			break;
		case('h'):
		case('?'):	help_information();return 0;
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
	//获取请求方法，同时处理请求方法与HTTP协议版本不匹配的问题
	switch (head_v) {
		case(0):	strcpy(request, "GET "); break;
		case(1):	strcpy(request, "HEAD "); if (http_v == 0) http_v = 1; break;
		case(2):	strcpy(request, "OPTIONS "); if (http_v != 2) http_v = 2; break;		//实测OPTIONS选项大部分情况返回302
		case(3):	strcpy(request, "TRACE "); if (http_v != 2) http_v = 2;; break;			//实测TRACE选项大部分情况返回302
	}
	if (proxy_flag && http_v == 0)
		http_v = 1;

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
	
	if (!proxy_flag) {									//对于非代理模式仅需获取URL中的path路径
		strcat(request, argv + index_path);			//请求行中的URL部分
		strcat(request, " ");
	}
	else {						//代理模式下使用绝对路径
		strcat(request, argv);
		strcat(request, " ");
	}

	//目前只支持HTTP0.9,HTTP1.1
	//请求行中的协议版本(注意结尾应为回车换行符)
	if (!http_v) {					//对应HTTP0.9，仅支持GET请求，无协议头
		//注意http0.9不需要在请求行指定版本。
		//Any request without a protocoll - version should be treaten as HTTP / 0.9.
		strcat(request, "\r\n");
		fprintf(stdout, "request: %s", request);
		return;
	}
	if (http_v == 1)
		strcat(request, "HTTP/1.0\r\n");
	else if (http_v = 2)
		strcat(request, "HTTP/1.1\r\n");

	//指定请求头部
	strcat(request, "User-Agent: Mokaka's WebBench\r\n");
	strcat(request, "Host: ");			//HTTP1.1必须指定Host字段
	strcat(request, Hostname);
	strcat(request, "\r\n");
	//HTTP1.1默认长连接，须关闭
	if(http_v==2)
		strcat(request, "Connection: close\r\n");	//区分长连接短连接
	if (proxy_flag) {
		strcat(request, "Pragma:	no-cache\r\n");	//指定请求结果不缓存
	}

	strcat(request, "\r\n");			//注意HTTP请求的格式，请求头和请求数据间有一个空行，
										//非常重要一定不能遗漏，缺失会造成问题。

	fprintf(stdout, "request: %s", request);
}

int Get_clients() {
	int fd[2];
	pid_t pid;
	FILE* f;

	//首先判断到指定服务器的连接是否可用
	int sockfd = 0;
	if(proxy_flag)
		sockfd = Socket(Proxyname, Hostport);
	else
		sockfd = Socket(Hostname, Hostport);
	if (sockfd < 0) {
		fprintf(stderr, "Error: can not connect to the server.\n");
		return 2;
	}

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
		if (proxy_flag)						//发起连接
			Get_socket(Proxyname);			
		else
			Get_socket(Hostname);
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

void Get_socket(char* hostname) {
	char response[1024];
	struct sigaction sa;
	sa.sa_handler = Alarm;
	sa.sa_flags = 0;
	if (sigaction(SIGALRM, &sa, NULL)) {		//sigaction返回-1表示处理失败
		fprintf(stderr, "Error: Signal processing failed\n");
		exit(2);
	}
	//设定定时器
	alarm(Time);
	//开始对指定URL发起请求，并记录成功和失败次数
	while (1) {
		if (time_flag) {			//首先判断是否超时
			//fprintf(stdout, "%d %d %d\n", Succeed, Failed, bytes);
			return;
		}

		//未超时对指定URL发起连接
		int sockfd;
		sockfd = Socket(hostname, Hostport);
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
				//fprintf(stdout, "response: %s\n", response);
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