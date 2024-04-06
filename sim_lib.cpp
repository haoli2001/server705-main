#include "sim_lib.h"
#include<iostream>

//用户自定义的仿真程序入口
int main_run(float* config, int idx, ResultStruct* result) {
	ResultStruct res;
	res.idx = idx;
	//计算...
	result->idx = idx;
	return 0;
}