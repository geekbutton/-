// WebBench.cpp : 定义控制台应用程序的入口点。
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
	if (argc < 2) {				//判断参数是否符合要求
		help_information();
		return 1;				//返回1 对应参数错误
	}

	system("pause");
    return 0;
}

