#ifndef SIM_LIB_H
#define SIM_LIB_H

#define CONFIG_TYPE_NUMS 2
#define CONFIG_INT_NUMS 5
#define CONFIG_FLOAT_NUMS 3
//#define CONFIG_CHAR_NUMS 20
#define RESULT_TYPE_NUMS 2
#define RESULT_INT_NUMS 4
#define RESULT_FLOAT_NUMS 3
//#define RESULT_CHAR_NUMS 20

enum class ProcStatus
{
	READY_FOR_CALCU = 0,
	START_TO_CALCU,
	OVER,
	SLAVEPROCESS_EXIT,
	EXIT
};

//仿真输入参数
struct ConfigStruct
{
	ProcStatus command;
	int idx;
	int arg_int[3];//用户仿真模型的输入
	float arg[3];
};

//仿真输出参数
struct ResultStruct
{
	int idx;
	int arg_int[3];
	float arg_float[3];
};

int main_run(float* config, int idx, ResultStruct* result);

#endif // !SIM_LIB_H
