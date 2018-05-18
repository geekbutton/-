// WebBench.cpp : �������̨Ӧ�ó������ڵ㡣
//
/*ע�ⲻͬ�汾������ͷ��Ҫ��
���򷵻�ֵ��
0������ɹ�ִ�в�����
1�����������URL��������
2������������Ϣ
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

int head_v = 0;			//request���󷽷���0,1��ӦGET,HEAD
struct option long_option[] = {
	{"time",2, NULL,'t'},
	{"client_nums",2,NULL,'c'},
	{"http09",0,NULL,'0'},
	{"http10",0,NULL,'1'},
	{"http11",0,NULL,'2'},
	{"GET",0,&head_v,0},
	{"HEAD",0,&head_v,1},
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
		"	-h | --help		Get help information"
		"\n"
	);
}

char request[1024];			//HTTP����(�����У�����ͷ���س����У���������)
char Hostname[1024];		
int Hostport = 80;
int Time = 60;				//����Ĭ��ִ��ʱ��ͷ��ʿͻ�����
int clients = 1;
int http_v = 0;				//HTTP�汾��0,1,2�ֱ��Ӧhttp0.9,http1.0,http1.1
int time_flag = 0;			//��ʱ��־λ

//�����¼
int Succeed = 0;
int Failed = 0;
int bytes = 0;

void Alarm(int signal) {
	//fprintf(stdout, "Time out");
	time_flag = 1;
}

int main(int argc,char *argv[])
{
	if (argc < 2) {				//�жϲ����Ƿ����Ҫ��
		help_information();
		return 1;				//����1 ��Ӧ��������
	}

	int opt;
	int opt_index;

	while ((opt = getopt_long(argc, argv, "t::c::012h", long_option, &opt_index)) != -1) {
		switch (opt) {
		case('t'):	if (optarg != NULL)	Time = atoi(optarg); break;
		case('c'):	if (optarg != NULL) clients = atoi(optarg); break;
		case('1'):	http_v = 1; break;
		case('2'):	http_v = 2; break;
		case('h'):
		case('?'):	help_information();return 0;
		}
	}
	fprintf(stdout, "Time: %d sec clients: %d\n", Time, clients);
	if (optind == argc) {		//�����URL����optindӦ�ñ�argcС1��optind����Ҫ�������һ������������
		fprintf(stderr, "ERROR: URL is needed for the WebBench\n");
		return 1;
	}
	Get_request(argv[optind]);		//��ȡ���󲿷�

	return Get_clients();			//�����ͻ��ˣ���������
}

void Get_request(const char* argv) {
	//Ŀǰ��ʱֻ֧��GET��ʽ
	switch (head_v) {
		case(0):	strcpy(request, "GET "); break;
		case(1):	strcpy(request, "HEAD "); break;
	}

	//���URL�Ƿ�Ϸ�
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

	int index_path = strchr(argv + 7, '/') - argv;			//�����ҵ�://֮��ĵ�һ��'/'��λ��
	//�ж�URL���Ƿ�����˿ں�
	if (strchr(argv + 7, ':') != NULL) {
		int index = strchr(argv + 7, ':') - argv;
		char hostport[1024];
		strncpy(hostport, argv + index + 1, index_path-index-1);
		Hostport = atoi(hostport);
		fprintf(stdout, "Port: %d\n", Hostport);

		//��ȡ������
		strncpy(Hostname, argv + 7, index - 7);
		fprintf(stdout, "Hostname: %s\n", Hostname);
	}
	else {
		//��ȡ������
		strncpy(Hostname, argv + 7, index_path - 7);
		fprintf(stdout, "Hostname: %s\n", Hostname);
	}
	
	//Ŀǰ��ʱ��֧�ִ���ģʽ����˽����ȡURL�е�path·��
	strcat(request, argv + index_path);			//�������е�URL����
	strcat(request, " ");

	//Ŀǰֻ֧��HTTP0.9,HTTP1.1
	//�������е�Э��汾(ע���βӦΪ�س����з�)
	if (!http_v) {					//��ӦHTTP0.9����֧��GET������Э��ͷ
		//ע��http0.9����Ҫ��������ָ���汾��
		//Any request without a protocoll - version should be treaten as HTTP / 0.9.
		strcat(request, "\r\n");
		fprintf(stdout, "request: %s", request);
		return;
	}
	if (http_v == 1)
		strcat(request, "HTTP/1.0\r\n");
	else if (http_v = 2)
		strcat(request, "HTTP/1.1\r\n");

	//ָ������ͷ��
	strcat(request, "User-Agent: Mokaka's WebBench\r\n");
	strcat(request, "Host: ");			//HTTP1.1����ָ��Host�ֶ�
	strcat(request, Hostname);
	strcat(request, "\r\n");
	//HTTP1.1Ĭ�ϳ����ӣ���ر�
	if(http_v==2)
		strcat(request, "Connection: close\r\n");	//���ֳ����Ӷ�����

	strcat(request, "\r\n");			//ע��HTTP����ĸ�ʽ������ͷ���������ݼ���һ�����У�
										//�ǳ���Ҫһ��������©��ȱʧ��������⡣

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
	if (pid == 0) {		//�ӽ���
		Get_socket();			//��������
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
	if (sigaction(SIGALRM, &sa, NULL)) {		//sigaction����-1��ʾ����ʧ��
		fprintf(stderr, "Error: Signal processing failed\n");
		exit(2);
	}
	//�趨��ʱ��
	alarm(Time);
	//��ʼ��ָ��URL�������󣬲���¼�ɹ���ʧ�ܴ���
	while (1) {
		if (time_flag) {			//�����ж��Ƿ�ʱ
			//fprintf(stdout, "%d %d %d\n", Succeed, Failed, bytes);
			return;
		}

		//δ��ʱ��ָ��URL��������
		int sockfd;
		sockfd = Socket(Hostname, Hostport);
		if (sockfd < 0) {			//����ʧ��
			++Failed;
			continue;
		}
		//����д��HTTP����
		int flag = write(sockfd, request, strlen(request));
		if (flag < 0) {
			++Failed;
			close(sockfd);
			continue;
		}
		
		//������Ӧ
		int temp_flag = 0;						//��ȡ��־�����ж��Ƿ��ȡʧ�����ͷ����
		while (1) {
			if (time_flag) {
				break;
			}
			int temp_size = read(sockfd, response, 1024);
			if (temp_size < 0) {				//��ȡʧ��
				++Failed;
				close(sockfd);
				temp_flag = 1;					//���ñ�־��
				break;
			}
			else if (temp_size == 0) {			//��ȡ���
				break;
			}
			else {								//��¼��ȡ���ֽ���
				bytes += temp_size;
			}
		}
	
		if (temp_flag == 1)
			continue;
		//�ر�����
		if (close(sockfd)) {
			++Failed;
			continue;
		}
		++Succeed;
	}

}