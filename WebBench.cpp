// WebBench.cpp : �������̨Ӧ�ó������ڵ㡣
//

//#include "stdafx.h"
#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

using namespace std;

void Get_request(const char*);

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

int main(int argc,char *argv[])
{
	if (argc < 2) {				//�жϲ����Ƿ����Ҫ��
		help_information();
		return 1;				//����1 ��Ӧ��������
	}

	int opt;
	int opt_index;
	int Time = 60;				//����Ĭ��ִ��ʱ��ͷ��ʿͻ�����
	int clients = 1;

	while ((opt = getopt_long(argc, argv, "t::c::h", long_option, &opt_index)) != -1) {
		switch (opt) {
		case('t'):
			if (optarg != NULL)
				Time = atoi(optarg);
			break;
		case('c'):	if(optarg!=NULL) clients = atoi(optarg); break;
		case('h'):
		case('?'):	help_information();break;
		}
	}
	//fprintf(stdout, "Time: %d\nclients: %d\n", Time, clients);
	if (optind == argc) {		//�����URL����optindӦ�ñ�argc��1
		fprintf(stderr, "ERROR: URL is needed for the WebBench\n");
		return 1;
	}
	Get_request(argv[optind]);

    return 0;
}

void Get_request(const char* argv) {
	//Ŀǰ��ʱֻ֧��GET��ʽ
	strcpy(request, "GET");

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

	//Ŀǰ��ʱ��֧�ִ���ģʽ����˽����ȡURL�е�path·��
	int index_path = strchr(argv + 7, '/') - argv;			//�����ҵ�://֮��ĵ�һ��'/'��λ��
	//�ж�URL���Ƿ�����˿ں�
	if (strchr(argv + 7, ':') != NULL) {
		int index = strchr(argv + 7, ':') - argv;
		char hostport[1024];
		strncpy(hostport, argv + index + 1, index_path-index-1);
		Hostport = atoi(hostport);
		fprintf(stdout, "%d\n", Hostport);
	}
}
