#include <cstdio>
#include <cstdlib>
#include <mpi.h>
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
using namespace std;

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
}TotalResource;

// 从 /proc/stat 文件中读取CPU使用信息
struct CPUUsage {
    long long user;     // 用户态
    long long nice;     // 低优先级用户态
    long long system;   // 内核态
    long long idle;     // 空闲
    long long iowait;   // I/O等待
    long long irq;      // 硬中断
    long long softirq;  // 软中断
};
CPUUsage getCPUUsage() {
    CPUUsage usage = {0, 0, 0, 0, 0, 0, 0};
    ifstream statFile("/proc/stat");
    if (statFile.is_open()) {
        string line;
        getline(statFile, line);
        if (line.find("cpu") == 0) {
            sscanf(line.c_str(), "cpu %lld %lld %lld %lld %lld %lld %lld",
                &usage.user, &usage.nice, &usage.system, &usage.idle,
                &usage.iowait, &usage.irq, &usage.softirq);
        }
        statFile.close();
    }
    return usage;
}

// 计算CPU的占用率
double calculateCPUUsage(CPUUsage& prev, CPUUsage& current) {
    long long prevTotal = prev.user + prev.nice + prev.system + prev.idle +
                          prev.iowait + prev.irq + prev.softirq;
    long long currentTotal = current.user + current.nice + current.system + current.idle +
                             current.iowait + current.irq + current.softirq;

    long long totalDelta = currentTotal - prevTotal;
    long long idleDelta = current.idle - prev.idle;

    if (totalDelta == 0) {
        return 0.0;  // 避免除以零
    }

    return 100.0 * (1.0 - static_cast<double>(idleDelta) / totalDelta);
}

//计算内存占用率
double MemUsage()
{

    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    long total_memory = 0;
    long free_memory = 0;

    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") != std::string::npos) {
            std::istringstream iss(line.substr(10));
            iss >> total_memory;
        } else if (line.find("MemFree:") != std::string::npos) {
            std::istringstream iss(line.substr(9));
            iss >> free_memory;
        }
    }

    if (total_memory > 0 && free_memory >= 0) {
        double memory_usage = 100.0 * (1.0 - static_cast<double>(free_memory) / static_cast<double>(total_memory));
        // std::cout << "Total Memory: " << total_memory / 1024 << " MB" << std::endl;
        // std::cout << "Free Memory: " << free_memory / 1024 << " MB" << std::endl;
        // std::cout << "Memory Usage: " << memory_usage << "%" << std::endl;
    } else {
        std::cerr << "Failed to read memory information." << std::endl;
    }

    return 100.0 * (1.0 - static_cast<double>(free_memory) / static_cast<double>(total_memory));
}


//从节点接收资源利用率
void ReceiveResource(const int count,const int source,const int source_2,const int tag,const int myid)
{
    Resource resource_1 = {};//从节点1获取的resource
    Resource resource_2 = {};//从节点2获取的resource
    while (true) {
        MPI_Recv(&resource_1, count, MPI_FLOAT, source, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&resource_2, count, MPI_FLOAT, source_2, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("processor %d received %f from 0\n", myid, resource_1.cpu_usage_rate);
        printf("processor %d received %f from 0\n", myid, resource_1.mem_usage_rate);
        printf("processor %d received %f from 1\n", myid, resource_2.cpu_usage_rate);
        printf("processor %d received %f from 1\n", myid, resource_2.mem_usage_rate);
        TotalResource.resource_1=resource_1;
        TotalResource.resource_2=resource_2;
    }
}

//节点将资源利用率发送给主节点，send=false时为计算当前节点的资源利用率（主节点）并保存在resource_3
void  MPISendResource(Resource *resource,const int count,const int destination,const int tag,bool send)//每隔一秒获取CPU使用率和内存使用率
{
    CPUUsage prevUsage = getCPUUsage();
    while(true) {
        this_thread::sleep_for(chrono::seconds(1));  // 每隔1秒钟检查一次CPU使用情况
        CPUUsage currentUsage = getCPUUsage();
        double cpuUsage = calculateCPUUsage(prevUsage, currentUsage);
        double Mem;
        Mem = MemUsage();
        //cout << "CPU Usage: " << cpuUsage << "%" << endl;
        prevUsage = currentUsage;
        //cout << "MemUsage:"<< Mem <<"%" << endl;
        resource->cpu_usage_rate = cpuUsage;
        resource->mem_usage_rate = Mem;
        if (send){
            MPI_Send(resource, count, MPI_FLOAT, destination, tag, MPI_COMM_WORLD);
        }
        else{
            TotalResource.resource_3.cpu_usage_rate=cpuUsage;//当选择主节点则为不发送，则将resource保存在TotalResource.resource_3（主节点的resource）
            TotalResource.resource_3.mem_usage_rate=Mem;
        }
    }
}

//计算资源利用率
void CalculateResource(Resource* resource)
{
    CPUUsage prevUsage = getCPUUsage();
    while(true) {
        this_thread::sleep_for(chrono::seconds(1));  // 每隔1秒钟检查一次CPU使用情况
        CPUUsage currentUsage = getCPUUsage();
        double cpuUsage = calculateCPUUsage(prevUsage, currentUsage);
        double Mem;
        Mem = MemUsage();
        //cout << "CPU Usage: " << cpuUsage << "%" << endl;
        prevUsage = currentUsage;
        //cout << "MemUsage:"<< Mem <<"%" << endl;
        resource->cpu_usage_rate = cpuUsage;
        resource->mem_usage_rate = Mem;
        TotalResource.resource_3.cpu_usage_rate=cpuUsage;
        TotalResource.resource_3.mem_usage_rate=Mem;
    }
}

//主节点将各个节点的资源利用率发送给上位机
void SendAllResource()
{
    while(true) {
        this_thread::sleep_for(chrono::milliseconds (1100));
        //测试
        cout << "节点1CPU利用率：" << TotalResource.resource_1.cpu_usage_rate << endl;
        cout << "节点1内存利用率：" << TotalResource.resource_1.mem_usage_rate << endl;
        cout << "节点2CPU利用率：" << TotalResource.resource_2.cpu_usage_rate << endl;
        cout << "节点2内存利用率：" << TotalResource.resource_2.mem_usage_rate << endl;
        cout << "主节点CPU利用率：" << TotalResource.resource_3.cpu_usage_rate << endl;
        cout << "主节点内存利用率：" << TotalResource.resource_3.mem_usage_rate << endl;
    }
}


int main(int argc, char * argv[]){
    int myid, numprocs;
    int tag, source,source_2, destination, count;
    int buffer;
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    tag = 888;
    source = 0;
    destination = 1;
    source_2 = 2;
    count = 2;//resource中有CPU和Mem所以count为2
    if(myid == source) {
        Resource resource_1{};
        bool send = true;
        MPISendResource(&resource_1,count,destination,tag,send);
    }
    if(myid == source_2) {
        Resource resource_2{};
        bool send = true;
        MPISendResource(&resource_2,count,destination,tag,send);
    }
    if(myid == destination) {
        thread receive_resource_thread(ReceiveResource,count,source,source_2,tag,destination);
        Resource resource_3{};
        thread calculate_resource_thread(MPISendResource,&resource_3,0,0,0,false);//不发送资源，count,destination,tag置零，send置false
        thread send_resource_thread(SendAllResource);
        send_resource_thread.join();
        receive_resource_thread.join();
        calculate_resource_thread.join();
    }
    MPI_Finalize();
}

