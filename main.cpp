#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include<windows.h>
#include"getopt.h"
#define THREAD_HANDLE int
#endif // _WIN32

#ifdef _WIN64
#define _CRT_SECURE_NO_WARNINGS
#include<windows.h>
#include"getopt.h"
#endif // _WIN64

#ifdef linux
#include<unistd.h>
#include<pthread.h>
#include<signal.h>
#define THREAD_HANDLE pthread_t
#endif // linux


#include<stdio.h>
#include"mpi.h"
#include<stdlib.h>
#include<time.h>
#include<string.h>
#include<vector>
#include<thread>

#include"mpi_manage.h"
#include"socketFunctions.h"
#include"comm.h"
#include "resource.h"

using namespace std;

struct CalcuInfo {
	ConfigStruct* configs;
	int configsNum;
	int procNum;
	MPI_Datatype MPI_CONFIG;
	MPI_Datatype MPI_RESULT;
	int socketfd;
	mutex* socketMutex;
};

void* calcThreadFunction(void *argv) {
	CalcuInfo calcuInfo = *(CalcuInfo*)argv;
	ConfigStruct sendBuf;

	ResultStruct* results = new ResultStruct[calcuInfo.configsNum];

	unsigned int sendIdx = 0;
	unsigned int recvIdx = 0;

	unsigned int taskNum = calcuInfo.configsNum;	//本次需要计算的任务数量
	printf("[calcThreadFun]: start calculate %d tasks\n", taskNum);
	if (calcuInfo.procNum == 1) {
		printf("[WARNING]only master process is alive,please check!\n");
		goto EXIT;
	}
	if (calcuInfo.procNum > 1)
	{
		while (true)
		{
			if (sendIdx < taskNum)
			{
				//向请求队列下发任务
				sendBuf = calcuInfo.configs[sendIdx];
				sendBuf.command = ProcStatus::START_TO_CALCU;
				//printf("[calcThreadFun]: calculate the result %d/%d,\t\t\n", calcuInfo.configs[sendIdx].idx, calcuInfo.configsNum);
				send_Task(sendBuf, calcuInfo.MPI_CONFIG, calcuInfo.MPI_RESULT);
				sendIdx++;
			}
			else
			{
				//当前轮次所有条目计算完成后，向所有进程发送结束信号，并等待接收其计算结果
				recv_CurRoundAllResults(calcuInfo.procNum, results, taskNum, calcuInfo.MPI_CONFIG, calcuInfo.MPI_RESULT, calcuInfo.socketfd,calcuInfo.socketMutex);
				break;
			}
		}
	}

EXIT:
	delete results;
	return nullptr;
}


void master(int myid, int procNum, MPI_Datatype MPI_CONFIG, MPI_Datatype MPI_RESULT, int port)
{
	init_socket();
	int sockfd = create_socket();
	bind_listen(sockfd, port);
	int newSockfd = accept_client(sockfd);
	ConfigStruct* configs=nullptr;
	std::mutex socketMutex;
    thread send_resource_thread(SendAllResource,ref(newSockfd),ref(socketMutex));//将三个节点的资源使用率发送给上位机
	CalcuInfo calcuInfo;
	THREAD_HANDLE calcThread;	//计算线程句柄
	memset(&calcThread, 0, sizeof(THREAD_HANDLE));
	while (true) {
		Frame frame;
		recv_data(newSockfd, (char*)&frame, sizeof(Frame));

		switch (frame.command)
		{
		case(CommCommand::CONFIG_DATA): {
			cout<<"[debug] recv config_data"<<endl;
			int len = frame.length;
			calcuInfo.configsNum=len;
			if(configs){
				delete configs;
				configs=nullptr;
			}
				
			configs = new ConfigStruct[len];
			recv_data(newSockfd, (char*)configs, len * sizeof(ConfigStruct));
			break;
		}
		case(CommCommand::CALCU): {
			//配置并开始
			cout<<"[debug] recv CALCU"<<endl;
			calcuInfo.configs = configs;
			calcuInfo.MPI_CONFIG = MPI_CONFIG;
			calcuInfo.MPI_RESULT = MPI_RESULT;
			calcuInfo.procNum = procNum;
			calcuInfo.socketfd = newSockfd;
			calcuInfo.socketMutex = &socketMutex;
#ifdef linux
			//开始执行计算线程，当计算线程正在执行时，则先关闭线程后再重新执行
			if (calcThread != 0 && pthread_kill(calcThread, 0) == 0)
			{
				pthread_cancel(calcThread);
				pthread_join(calcThread, NULL);
				printf("restart");
			}
			pthread_create(&calcThread, NULL, calcThreadFunction, (void*)&calcuInfo);
#endif // linux
			break;
		}
		case(CommCommand::RESOURCE): {
			//send_Resource(newSockfd,socketMutex);
		}
		case(CommCommand::EXIT): {
			exit_AllProcess(procNum, MPI_CONFIG, MPI_RESULT);
			return;
		}
		default:
			break;
		}
	}
    send_resource_thread.join();


}

void slave(int myid, char* hostname, MPI_Datatype& MPI_CONFIG, MPI_Datatype& MPI_RESULT)
{
	//printf("this is slave!\n");
	ConfigStruct sendBuf;
	ConfigStruct recvBuf;
	vector<ResultStruct>results;
	unsigned int resultNum = 0;
	while (true)
	{
		sendBuf.command = ProcStatus::READY_FOR_CALCU;
		MPI_Ssend(&sendBuf, 1, MPI_CONFIG, 0, 0, MPI_COMM_WORLD);	//向主进程发送计算请求
		MPI_Recv(&recvBuf, 1, MPI_CONFIG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);	//接收主进程命令
		//printf("recvBuf.command = %d\n", recvBuf.command);
		switch (recvBuf.command)
		{
		case ProcStatus::OVER:
		{
			//向主进程回传结果
			sendBuf.command = ProcStatus::SLAVEPROCESS_EXIT;
			if (!results.empty())
			{
				//printf("send %d results,\t\tPID= %d\n", results.size(), myid);
				MPI_Ssend(&results[0], results.size(), MPI_RESULT, 0, 0, MPI_COMM_WORLD);
				results.clear();
			}
			else
				MPI_Ssend(&sendBuf, 1, MPI_RESULT, 0, 0, MPI_COMM_WORLD);
			break;
		}

		case ProcStatus::START_TO_CALCU:
		{
			//开始计算...
			ResultStruct output;
			//printf("before calculate the %dth result,\t\tPID= %d\n", recvBuf.idx, myid);
			main_run(recvBuf.arg, recvBuf.idx, &output);	//用户自定义计算模型入口
			output.idx = recvBuf.idx;	//输出序号一定要和输入序号对应
			results.push_back(output);

			//printf("计算第%d条结果\t\t进程ID: %d\n", recvBuf.idx, myid);
			break;
		}
		case ProcStatus::EXIT:
		{
			//退出
			return;
		}
		}

	}
}



int main(int argc, char* argv[])
{
	int o;
	int port = 4000;
	// 选项-p,后跟参数，表示主机socket要绑定的端口号
	const char *optstring = "p:";
	
	while ((o = getopt(argc, argv, optstring)) != -1) {
		switch (o) {
		case 'p':
			printf("opt is p, oprarg is: %s\n", optarg);
			port = atoi(optarg);
			break;
		case '?':
			printf("error optopt: %c\n", optopt);
			printf("error opterr: %d\n", opterr);
			printf("default port is 4000\n");
			break;
		}
	}

	int myid, numprocs, namelen, provided_thread_rank;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	char hostname[100];

	//初始化MPI，线程级别设置为：多线程, 但只有主线程会进行MPI调用
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided_thread_rank);
	if (provided_thread_rank < MPI_THREAD_MULTIPLE)
		MPI_Abort(MPI_COMM_WORLD, 1);
	//MPI_Init(&argc, &argv);        // starts MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);  // get current process id
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);      // get number of processes
	MPI_Get_processor_name(processor_name, &namelen);

	//printf("\nID of processes: %d\n...\n", myid);

	MPI_Datatype MPI_CONFIG, MPI_RESULT;
	commit_new_type(MPI_CONFIG, MPI_RESULT);

	double time = 0;
	time = MPI_Wtime();
    int tag, source,source_2, destination, count;//MPI的tag，发送进程1，2，发送目的地（主进程），发送数
    tag = 888;//胡写的，吉利
    source = 1;//修改为节点1中的进程号
    destination = 0;//主节点中的主进程
    source_2 = 2;//修改为节点2中的进程号
    count = 2;//resource中有CPU和Mem所以count为2
	if (myid == 0)//主进程
	{
        thread receive_resource_thread(ReceiveResource,count,source,source_2,tag,destination);
        Resource resource_3{};
        thread calculate_resource_thread(MPISendResource,&resource_3,0,0,0,false);//不发送资源，count,destination,tag置零，send置false

		master(myid, numprocs, MPI_CONFIG, MPI_RESULT, port);
        receive_resource_thread.join();
        calculate_resource_thread.join();

	}
    else if(myid == source)//节点1中的进程，id号待修改
    {
        Resource resource_1{};
        bool send = true;
        MPISendResource(&resource_1,count,destination,tag,send);
    }
    else if(myid == source_2)
    {
        Resource resource_2{};
        bool send = true;
        MPISendResource(&resource_2,count,destination,tag,send);
    }
	else//其余进程都用来计算
	{
		slave(myid, hostname, MPI_CONFIG, MPI_RESULT);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	time = MPI_Wtime() - time;
	if (myid == 0)
	{
		printf("\n\ncalculate over!the total time is %lf s.\n\n", time);
	}

	MPI_Finalize();
	return 0;
}


