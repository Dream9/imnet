/*************************************************************************
	> File Name: process_information.h
	> Author: Dream9
	> Brief: 获取进程当前状态的一系列相关函数
	> Created Time: 2019年09月05日 星期四 09时08分14秒
 ************************************************************************/

#ifndef _IMNET_DEPENDENT_PROCESS_INFOMATION_H_
#define _IMNET_DEPENDENT_PROCESS_INFOMATION_H_

#ifdef _MSC_VER
#include"string_piece.h"
#include"types_extension.h"
#include"./logger/timestamp.h"
#elif defined __GNUC__
#include"dependent/string_piece.h"
#include"dependent/types_extension.h"
#incldue"dependent/logger/timestamp.h"
#endif

#include<vector>

#include<sys/types.h>

namespace imnet {

//brief:相关函数没有必要封装为一个类，因此通过namespace提供一层间接性
namespace process_information {

string getHostname();
pid_t getPid();
uid_t getUid();
uid_t getEuid();
CTimestamp getProcessStartTime();

//The number of clock ticks per second.The corresponding
//variable is obsolete.It was of course called CLK_TCK.
//int getClockTicksPerSecond();

long getPageSize();
string getProcStat();
string getTaskStat();
int getOpenedFileNumber();
int getMaxOpenFileNumber();
std::vector<pid_t> getCreatedThread();



}//!namespace process_information


}//!namespace imnet

#endif

