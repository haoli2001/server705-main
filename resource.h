#ifndef RESOURCE_H
#define RESOURCE_H
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>
#include "comm.h"
#include <cstring>

// 结构体用于存储CPU的使用信息
struct CPUUsage {
    long long user;     // 用户态
    long long nice;     // 低优先级用户态
    long long system;   // 内核态
    long long idle;     // 空闲
    long long iowait;   // I/O等待
    long long irq;      // 硬中断
    long long softirq;  // 软中断
};

CPUUsage getCPUUsage();
double calculateCPUUsage(CPUUsage& prev, CPUUsage& current);
double MemUsage();
void ReceiveResource(const int count,const int source,const int source_2,const int tag,const int myid);
void  MPISendResource(Resource *resource,const int count,const int destination,const int tag,bool send);
void SendAllResource(int socketfd,std::mutex& socketMutex);

#endif//RESOURCE_H
