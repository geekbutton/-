// WebBench.cpp : �������̨Ӧ�ó������ڵ㡣
//
/*ע�ⲻͬ�汾������ͷ��Ҫ��
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
int Time = 60;				//����Ĭ��ִ��ʱ��ͷ��ʿͻ�����
int clients = 1;
int time_flag = 0;

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
	if (optind == argc) {		//�����URL����optindӦ�ñ�argcС1��optind����Ҫ�������һ������������
		fprintf(stderr, "ERROR: URL is needed for the WebBench\n");
		return 1;
	}
	Get_request(argv[optind]);		//��ȡ���󲿷�

	return Get_clients();			//�����ͻ��ˣ���������
}

void Get_request(const char* argv) {
	//Ŀǰ��ʱֻ֧��GET��ʽ
	strcpy(request, "GET ");

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
	strcat(request, argv + index_path);
	strcat(request, " ");

	//Ŀǰֻ֧��HTTP1.1
	strcat(request, "HTTP/1.1\r\n");
	strcat(request, "User-Agent: WebBench 1.0\r\n");
	strcat(request, "Host: ");			//HTTP1.1����ָ��Host�ֶ�
	strcat(request, Hostname);
	strcat(request, "\r\n");
	//strcat(request, "Connection: close\r\n");
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
	if (sigaction(SIGALRM, &sa,NULL)) {		//sigaction����-1��ʾ����ʧ��
		fprintf(stderr, "Error: Signal processing failed\n");
		exit(2);
	}
	//�趨��ʱ��
	alarm(Time);
	//��ʼ��ָ��URL�������󣬲���¼�ɹ���ʧ�ܴ���
	while (1) {
		if (time_flag) {			//�����ж��Ƿ�ʱ
			fprintf(stdout, "%d %d %d\n", Succeed, Failed, bytes);
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