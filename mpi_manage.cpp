#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifdef _WIN64
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <mpi.h>
#include "mpi_manage.h"
#include "socketFunctions.h"
#include "comm.h"
#include "resource.h"
using namespace std;
void commit_new_type(MPI_Datatype &MPI_CONFIG, MPI_Datatype &MPI_RESULT)
{
	//为MPI注册自定义的configStruct数据类型
	int blocklens_array[CONFIG_TYPE_NUMS];
	MPI_Aint displs_array[CONFIG_TYPE_NUMS];//使用MPI_Aint，分别存放各数据类型在结构体中占的大小
	MPI_Datatype old_type_array[CONFIG_TYPE_NUMS];

	ConfigStruct mydata;

	old_type_array[0] = MPI_INT;//这里对应结构体中有的数据类型
	old_type_array[1] = MPI_FLOAT;
	blocklens_array[0] = CONFIG_INT_NUMS;
	blocklens_array[1] = CONFIG_FLOAT_NUMS;

	MPI_Get_address(&mydata.command, &displs_array[0]);//第一个int型相对于结构体头地址的偏移
	MPI_Get_address(&mydata.arg[0], &displs_array[1]);//第一个float型相对于结构体头地址的偏移
	displs_array[1] = displs_array[1] - displs_array[0];//差值即该类型在结构体中占的大小
	displs_array[0] = 0;
	//生成新的数据类型并提交
	MPI_Type_create_struct(CONFIG_TYPE_NUMS, blocklens_array, displs_array, old_type_array, &MPI_CONFIG);
	MPI_Type_commit(&MPI_CONFIG);

	/*
	MPI_Aint extent = 0;
	MPI_Aint lb = 0;
	MPI_Type_get_extent(MPI_CONFIG, &lb, &extent);
	printf("sizeof MPI_CONFIG:%d	sizeof configStruct:%d\n", extent, sizeof(ConfigStruct));
	*/



	//为MPI注册自定义的resultStruct数据类型
	int blocklens_array2[RESULT_TYPE_NUMS];
	MPI_Aint displs_array2[RESULT_TYPE_NUMS];//使用MPI_Aint，分别存放各数据类型在结构体中占的大小
	MPI_Datatype old_type_array2[RESULT_TYPE_NUMS];

	ResultStruct result;

	old_type_array2[0] = MPI_INT;//这里对应结构体中有的数据类型
	old_type_array2[1] = MPI_FLOAT;
	blocklens_array2[0] = RESULT_INT_NUMS;
	blocklens_array2[1] = RESULT_FLOAT_NUMS;

	MPI_Get_address(&result.idx, &displs_array2[0]);//第一个int型相对于结构体头地址的偏移
	MPI_Get_address(&result.arg_float[0], &displs_array2[1]);//第一个float型相对于结构体头地址的偏移
	displs_array2[1] = displs_array2[1] - displs_array2[0];//差值即该类型在结构体中占的大小
	displs_array2[0] = 0;
	//生成新的数据类型并提交
	MPI_Type_create_struct(RESULT_TYPE_NUMS, blocklens_array2, displs_array2, old_type_array2, &MPI_RESULT);
	MPI_Type_commit(&MPI_RESULT);



	/*
	MPI_Type_get_extent(MPI_RESULT, &lb, &extent);
	printf("sizeof MPI_RESULT:%d	sizeof resultStruct:%d\n", extent, sizeof(ResultStruct));
	*/
}




void exit_AllProcess(int procNum, MPI_Datatype& MPI_CONFIG, MPI_Datatype& MPI_RESULT)
{
	unsigned int exitNum = 0;
	MPI_Status status;
	ConfigStruct sendBuf;
	ConfigStruct recvBuf;
	while (true)
	{
		MPI_Recv(&recvBuf, 1, MPI_CONFIG, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);	//接收从进程计算请求
		ConfigStruct sendBuf;
		sendBuf.command = ProcStatus::EXIT;
		MPI_Ssend(&sendBuf, 1, MPI_CONFIG, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
		exitNum++;

		//向所有从进程发送完结束命令，函数返回
		if (exitNum == procNum - 1)
			return;
	}
}

void recv_CurRoundAllResults(int procNum, ResultStruct* results, int resultsLen, MPI_Datatype& MPI_CONFIG, MPI_Datatype& MPI_RESULT, int socketfd, std::mutex* socketMutex)
{
	MPI_Status status;
	ConfigStruct sendBuf;
	ConfigStruct recvBuf;
	int resultBufOffset = 0;
	//接收所有从进程结果
	for (int i = 1; i < procNum; i++)
	{
		MPI_Recv(&recvBuf, 1, MPI_CONFIG, i, 0, MPI_COMM_WORLD, &status);	//接收从进程计算请求
		sendBuf.command = ProcStatus::OVER;
		MPI_Ssend(&sendBuf, 1, MPI_CONFIG, status.MPI_SOURCE, 0, MPI_COMM_WORLD);

		MPI_Probe(status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
		int recvNumPerProc = 0;
		MPI_Get_count(&status, MPI_RESULT, &recvNumPerProc);

		MPI_Recv(results + resultBufOffset, recvNumPerProc, MPI_RESULT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);
		resultBufOffset += recvNumPerProc;
	}


	//收到所有进程的结果后，将结果发回上位机。计算结束，返回
	//将收到的乱序结果数据排序
	//printf("[DEBUG]:master ready to write result!\n");
	ResultStruct* tmp_results = new ResultStruct[resultsLen];
	//printf("[DEBUG]:master new complete!,configs.size = %d\n", resultsLen);
	for (int i = 0; i < resultsLen; i++)
	{
		int orderIdx = results[i].idx;
		//printf("[DEBUG]:results[%d].idx = %d\n", i, results[i].idx);
		tmp_results[orderIdx] = results[i];
	}
	
	//向上位机发送结果时加锁，避免两个线程同时发送socket数据，产生影响
	socketMutex->lock();
	int sendedLength = 0;
	Frame frame;
	while (true)
	{
		frame.command = CommCommand::RESULT;
		if (sendedLength + 1024 < resultsLen * sizeof(ResultStruct))
		{
			memcpy(frame.data, (char*)tmp_results + sendedLength, 1024);
			frame.length = 1024;
			send_frame(socketfd, (char*)&frame, sizeof(Frame));
			sendedLength += 1024;
		}
		else
		{
			//std::cout<<"[debug] sizeof(ResStruct)="<<sizeof(ResultStruct)<<" res len="<<resultsLen<<std::endl;
			memcpy(frame.data, (char*)tmp_results + sendedLength, resultsLen * sizeof(ResultStruct) - sendedLength);
			frame.length = resultsLen * sizeof(ResultStruct) - sendedLength;
			send_frame(socketfd, (char*)&frame, sizeof(Frame));
			break;
		}
	}
	socketMutex->unlock();

	delete tmp_results;
}

void send_Task(ConfigStruct sendBuf, MPI_Datatype& MPI_CONFIG, MPI_Datatype& MPI_RESULT)
{
	MPI_Status status;
	ConfigStruct recvBuf;
	MPI_Recv(&recvBuf, 1, MPI_CONFIG, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);	//接收从进程计算请求
	MPI_Ssend(&sendBuf, 1, MPI_CONFIG, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
}

//// 从 /proc/stat 文件中读取CPU使用信息
//CPUUsage getCPUUsage() {
//    CPUUsage usage = {0, 0, 0, 0, 0, 0, 0};
//    ifstream statFile("/proc/stat");
//    if (statFile.is_open()) {
//        string line;
//        getline(statFile, line);
//        if (line.find("cpu") == 0) {
//            sscanf(line.c_str(), "cpu %lld %lld %lld %lld %lld %lld %lld",
//                &usage.user, &usage.nice, &usage.system, &usage.idle,
//                &usage.iowait, &usage.irq, &usage.softirq);
//        }
//        statFile.close();
//    }
//    return usage;
//}
//
//// 计算CPU的占用率
//double calculateCPUUsage(CPUUsage& prev, CPUUsage& current) {
//    long long prevTotal = prev.user + prev.nice + prev.system + prev.idle +
//                          prev.iowait + prev.irq + prev.softirq;
//    long long currentTotal = current.user + current.nice + current.system + current.idle +
//                             current.iowait + current.irq + current.softirq;
//
//    long long totalDelta = currentTotal - prevTotal;
//    long long idleDelta = current.idle - prev.idle;
//
//    if (totalDelta == 0) {
//        return 0.0;  // 避免除以零
//    }
//
//    return 100.0 * (1.0 - static_cast<double>(idleDelta) / totalDelta);
//}
//
////计算内存占用率
//double MemUsage()
//{
//
//    std::ifstream meminfo("/proc/meminfo");
//    std::string line;
//    long total_memory = 0;
//    long free_memory = 0;
//
//    while (std::getline(meminfo, line)) {
//        if (line.find("MemTotal:") != std::string::npos) {
//            std::istringstream iss(line.substr(10));
//            iss >> total_memory;
//        } else if (line.find("MemFree:") != std::string::npos) {
//            std::istringstream iss(line.substr(9));
//            iss >> free_memory;
//        }
//    }
//
//    if (total_memory > 0 && free_memory >= 0) {
//        double memory_usage = 100.0 * (1.0 - static_cast<double>(free_memory) / static_cast<double>(total_memory));
//        // std::cout << "Total Memory: " << total_memory / 1024 << " MB" << std::endl;
//        // std::cout << "Free Memory: " << free_memory / 1024 << " MB" << std::endl;
//        // std::cout << "Memory Usage: " << memory_usage << "%" << std::endl;
//    } else {
//        std::cerr << "Failed to read memory information." << std::endl;
//    }
//
//    return 100.0 * (1.0 - static_cast<double>(free_memory) / static_cast<double>(total_memory));
//}

//void send_Resource(int socketfd,std::mutex& socketMutex) {
//	CPUUsage prevUsage = getCPUUsage();
//	Resource resource;
//	 while (true) {
//        this_thread::sleep_for(chrono::seconds(1));  // 每隔1秒钟检查一次CPU使用情况
//        CPUUsage currentUsage = getCPUUsage();
//        double cpuUsage = calculateCPUUsage(prevUsage, currentUsage);
//        double Mem;
//        Mem = MemUsage();
//        cout << "CPU Usage: " << cpuUsage << "%" << endl;
//        prevUsage = currentUsage;
//        cout << "MemUsage:"<< Mem <<"%" << endl;
//        resource.cpu_usage_rate=cpuUsage;
//        resource.mem_usage_rate=Mem;
//		//回报服务器计算资源信息，传输时加锁
//		Frame frame;
//		frame.command = CommCommand::RESOURCE;
//		frame.length = sizeof(resource);
//		memcpy(frame.data, (char*)&resource, sizeof(resource));
//		socketMutex.lock();
//		send_frame(socketfd, (char*)&frame, sizeof(Frame));
//		socketMutex.unlock();
//    }
//
//
//
//}
