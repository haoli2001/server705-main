#ifndef MPI_MANAGE_H
#define MPI_MANAGE_H

#include "sim_lib.h"
#include <vector>
#include <mutex>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <cstring>
void commit_new_type(MPI_Datatype &MPI_CONFIG, MPI_Datatype &MPI_RESULT);
int getConfigData(std::vector<ConfigStruct>& configs);
void exit_AllProcess(int procNum, MPI_Datatype& MPI_CONFIG, MPI_Datatype& MPI_RESULT);
unsigned int getCurCalcuEndIdx(unsigned int allTaskNum, unsigned int nums_per_round, unsigned int sendIdx);
unsigned int getCurRoundIdx();
void setCurRoundIdx(unsigned int roundIdx);
void recv_CurRoundAllResults(int procNum, ResultStruct* results, int resultsLen, MPI_Datatype& MPI_CONFIG, MPI_Datatype& MPI_RESULT, int socketfd, std::mutex* socketMutex);
void send_Task(ConfigStruct sendBuf, MPI_Datatype& MPI_CONFIG, MPI_Datatype& MPI_RESULT);
//void send_Resource(int socketfd, std::mutex& socketMutex);
//CPUUsage getCPUUsage();
//double calculateCPUUsage(CPUUsage& prev, CPUUsage& current);
//double MemUsage();
#endif // !MPI_MANAGE_H
