/*************************************************************************
	> File Name: t.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月23日 星期五 14时43分55秒
 ************************************************************************/
#define _TEST_TIMEZONE_NEED_DETAIL_INFO_

#include"dependent/logger/timezone.h"

#include<cstdio>
#include<algorithm>
#include<functional>
#include<time.h>

#include<unistd.h>

using namespace imnet;

const int kBufLen = 256;

void lookup_timezone_info(const char *filename){
	if(access(filename,R_OK)<0){
		printf("file(%s) is not readable or lost",filename);
		exit(0);
	}
	printf("test tzfile\n");
	imnet::CTimezone gtz(filename);
	printf("isValid()?%d\n",gtz.isValid());
	printf("end\n");

}

//brief:timezone的测试用例
class Testcase4timezone {
public:
	const char *gmt;
	const char *localtime;
	bool isdst;

	//brief:将标准字符串装欢utc下的tm结构
	//The  strptime()  function is the converse of strftime(3);
	//it converts the character string pointed to by s to values
	//which are stored in the "broken-down time" structure pointed to by tm,
	//using the format specified by format.
	struct tm transformToTm(const char *str) {
		struct tm ret;
		strptime(str, "%F %H:%M:%S", &ret);
		return ret;
	}

	//brief:将符合%F %H:%M:%S格式字符串转换为utc下的time_t数值
	//Synopsis
    //#include <time.h>
	//	time_t timelocal(struct tm *tm);
	//time_t timegm(struct tm *tm);
	//Feature Test Macro Requirements for glibc(see feature_test_macros(7)) :
	//	timelocal(), timegm() : _BSD_SOURCE || _SVID_SOURCE
	//	Description
	//	The functions timelocal() and timegm() are the inverses of
	//	localtime(3) and
	//	gmtime(3).
	time_t transformToTime_t(const char *str) {
		struct tm tmp = transformToTm(str);
		return ::timegm(&tmp);
	}

	time_t getTime_t() {
		return transformToTime_t(gmt);
	}
};

//brief:执行转换比较
void test_timezone(const CTimezone &tz, Testcase4timezone &tc) {
	time_t gmt_time_t = tc.getTime_t();
	{
		//测试从utc下面的时间转换到localtime
		struct tm local_test;
		tz.localtime_r(gmt_time_t, local_test);
		char buf[kBufLen];
		::strftime(buf, kBufLen, "%F %H:%M:%S%z(%Z)", &local_test);
		if (strcmp(tc.localtime, buf) != 0 || tc.isdst != local_test.tm_isdst) {
			printf("TEST FAILED:expect:%s (real:%s)\n", tc.localtime, buf);
		}
		else{
			printf("TEST PASSED:%s\n",buf);
		}
	} 
	{
		//测试从localtime转换到utc
		//TODO
	}
}

//brief:以下为具体的测试过程
void testHongkong() {
	CTimezone tz("/usr/share/zoneinfo/Asia/Hong_Kong");
	Testcase4timezone cases[] = {
		{ "2011-04-03 00:00:00", "2011-04-03 08:00:00+0800(HKT)", false},
	};
	auto f=std::bind(test_timezone,std::ref(tz),std::placeholders::_1);
	std::for_each(std::begin(cases), std::end(cases), f);
}
//brief:测试手动设置规则
void testFixedTimezone() {
	CTimezone tz(8 * 60 * 60, "CST");
	Testcase4timezone cases[] = {
		{ "2014-04-03 00:00:00", "2014-04-03 08:00:00+0800(CST)", false},
	};
	auto f=std::bind(test_timezone,std::ref(tz),std::placeholders::_1);
	std::for_each(std::begin(cases), std::end(cases), f);

}

int main(int argc ,char*argv[]){
	int c;
	bool flag=false;
	
	while ((c = getopt(argc, argv, "ih")) != -1) {
		switch (c) {
		case 'h':
			printf("usage: [-i]:for check detail info\t[-h]:for help\n");
			exit(0);
		case 'i':
			flag = true;
			break;
		default:
			printf("unrecongnized option,get more info by -i\n");
			//break;
			exit(0);
		}
	}

	if(flag){
		printf("test tzfile info\n");
		lookup_timezone_info("../../../doc/tz-file/PRC");
	}

	printf("test timezone\n");
	testFixedTimezone();
	testHongkong();
}

