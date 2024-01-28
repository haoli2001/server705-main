#ifndef COMM
#define COMM
enum class CommCommand
{
	CONFIG_DATA = 0,
	CALCU,
	STOP,
	EXIT,
	RESOURCE,
	RESULT
};

struct Resource
{
	float cpu_usage_rate;	//cpu占用率
	float mem_usage_rate;	//内存占用率
};

struct TotalResource
{
    Resource resource_1;
    Resource resource_2;
    Resource resource_3;
};

struct Frame
{
	CommCommand command;
	int length;
	char data[1024];
};
#endif // COMM
