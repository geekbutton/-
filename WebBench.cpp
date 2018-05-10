// WebBench.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <iostream>
#include <unistd.h>
#include <getopt.h>

using namespace std;

struct option long_option[] = {
	{"--time",1, 0,'-t'},
	{"--client_nums",1,0,'-n'},
	{"--help",0,0,'-h'}
};

void help_information() {
	fprintf(stderr,
		"Please check your parametes:\n"
		"WebBench [option][URL]\n"
		"-t | --time <sec>	Run each client <sec> seconds, default 60\n"
		"-n | --client_nums <n>	Run n clients, default 1\n"
		"-h | --help	Get help information"
		"\n"
	);
}

int main(int argc,char *argv[])
{
	if (argc < 2) {				//�жϲ����Ƿ����Ҫ��
		help_information();
		return 1;				//����1 ��Ӧ��������
	}

	system("pause");
    return 0;
}

