/*************************************************************************
	> File Name: julianday.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月22日 星期四 09时17分04秒
 ************************************************************************/
#ifdef __GNUC__
#include"dependent/logger/julianday.h"
#elif defined(_MSC_VER)
#include"julianday.h"
#endif

#include<assert.h>

namespace imnet {
namespace detail {//以下函数不适合暴露给用户
//brief:因为numeric_limits<int16_t>::max 为32767,不足以进行JD计数
static_assert(sizeof(int) >= sizeof(int32_t), "int should be 32-bits at least");

//brief:求出给定年(I)，月(J)，日(K)的儒略日:
//     儒略日 = K - 32075 + 1461 * (I + 4800 + (J - 14) / 12) / 4 + 367 * (J - 2 - (J - 14) / 12 * 12) / 12 - 3 * ((I + 4900 + (J - 14) / 12) / 100) / 4
typename CJulianday::DayType transformToJulianday(int year, int month, int day) {
	int a = (14 - month) / 12;
	int y = year + 4800 - a;
	int m = month + 12 * a - 3;
	return day - 32075 + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400;
}

//brief:根据儒略日反求年月日
//     https://blog.csdn.net/weixin_42763614/article/details/82880007#儒略日转公历
DateType transformToDate(typename CJulianday::DayType jd_number) {
	int a = jd_number + 32044;
	int b = (4 * a + 3) / 146097;
	int c = a - ((b * 146097) / 4);
	int d = (4 * c + 3) / 1461;
	int e = c - ((1461 * d) / 4);
	int m = (5 * e + 2) / 153;
	return { e - ((153 * m + 2) / 5) + 1, m + 3 - 12 * (m / 10), b * 100 + d - 4800 + (m / 10) };
}

}//namespace detail;
}//namespace imnet;

using namespace imnet;
using namespace imnet::detail;

CJulianday::CJulianday(int year, int month, int day)
	:__julianday(transformToJulianday(year,month,day)){
}

CJulianday::CJulianday(const tm &tm_value)
	:__julianday(transformToJulianday(tm_value.tm_year,tm_value.tm_mon,tm_value.tm_mday)){
}

//brief:获得日期
DateType CJulianday::getDate()const {
	return transformToDate(__julianday);
}



string CJulianday::toString()const {
	DateType date = transformToDate(__julianday);
	char buf[16];
	int ret = snprintf(buf, sizeof buf, "%04d-%02d-%02d", date.year, date.month, date.day);
	assert(ret > 0);
	return buf;
}

//brief:


