/*************************************************************************
	> File Name: timestamp.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月17日 星期六 09时30分42秒
 ************************************************************************/
#ifdef __GNUC__
#include"dependent/logger/timestamp.h"
#elif defined _MSC_VER
#include"timestamp.h"
#endif

#include<cstdio>
#include<cinttypes>

#include<sys/time.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
using namespace imnet;

//static_assert(sizeof(CTimestamp)==sizeof(int64_t),"CTimestamp should have  the same size");

//brief:static 成员的定义
const int CTimestamp::kRatioFromMicrosecondToSecond = 1000 * 1000;

//inline typename CTimestamp::SelfType CTimestamp::setTime(time_t t) {
//	;
//}
inline typename CTimestamp::SelfType CTimestamp::getFromUnixTime(time_t t) {
	return getFromUnixTime(t, 0);
}
inline typename CTimestamp::SelfType CTimestamp::getFromUnixTime(time_t t, int microseconds) {
	return SelfType(static_cast<TimeType>(t)*kRatioFromMicrosecondToSecond + microseconds);
}
inline typename CTimestamp::SelfType CTimestamp::getInvalid() {
	return CTimestamp();

}

//brief:通过gettimeofday()返回当前时间的Timestamp
//becare:本函数是一个static成员，方便外部调用
typename CTimestamp::SelfType CTimestamp::getNow() {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return SelfType(kRatioFromMicrosecondToSecond*tv.tv_sec + tv.tv_usec);
}

//brief:返回以s为单位的字符串格式的时间
//becare:通过小数点区分微秒位
string CTimestamp::toString()const {
	char buf[32];//由于numeric_limits<int64_t>::digits10位18,即63*（log10(2)）,故32位空间是足够的
	buf[0] = '\0';
	TimeType seconds = __microseconds / kRatioFromMicrosecondToSecond;
	TimeType microseconds = __microseconds % kRatioFromMicrosecondToSecond;
	snprintf(buf, (sizeof buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);//自动以'\0'结尾
	return buf;
}

//brief:得到UTC下的年月日时分秒字符串
//      格式为:"%4d-%02d-%02d %02d:%02d:%02d.%06"或者"%4d-%02d-%02d %02d:%02d:%02d"
//      借助于gmtime_r转换位
//parameter:showMicroseconds:是否显示微秒值，默认显示
string CTimestamp::toUTCString(bool showMicroseconds)const {
	char buf[32 << 1];
	buf[0] = '\0';
	time_t seconds = static_cast<time_t>(__microseconds / kRatioFromMicrosecondToSecond);
	struct tm tm_answer;
	gmtime_r(&seconds, &tm_answer);

	if (showMicroseconds) {
		TimeType microseconds = __microseconds % kRatioFromMicrosecondToSecond;
		snprintf(buf, (sizeof buf) - 1, "%4d-%02d-%02d %02d:%02d:%02d.%06" PRId64,
			tm_answer.tm_year + 1990, tm_answer.tm_mon, tm_answer.tm_mday, tm_answer.tm_hour,
			tm_answer.tm_min, tm_answer.tm_sec, microseconds);
	}
	else {
		snprintf(buf, (sizeof buf) - 1, "%4d-%02d-%02d %02d:%02d:%02d",
			tm_answer.tm_year + 1990, tm_answer.tm_mon, tm_answer.tm_mday, tm_answer.tm_hour,
			tm_answer.tm_min, tm_answer.tm_sec);
	}
	return buf;
}


