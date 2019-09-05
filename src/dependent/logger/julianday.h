/*************************************************************************
	> File Name: julianday.h
	> Author: Dream9
	> Brief: t提供儒略日计算时间的功能
	> Created Time: 2019年08月22日 星期四 09时16分58秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_DATE_H_
#define _IMNET_DEPENDENT_DATE_H_

#ifdef __GNUC__
#include"dependent/types_extension.h"
#elif defined(_MSC_VER)
#include"types_extension.h"
#endif

struct tm;

namespace imnet {

//brief:
typedef struct {
	int year;
	int month;
	int day;
} DateType;

//brief:以儒略日格式记录时间
//      便于时间计算
//becare:当前计算的起点为儒略历的公元前4713年1月1日中午12点（在格里历是公元前4714年11月24日）
class CJulianday {
public:
	using DayType = int;
	
	//brief:各种构造routine
	CJulianday() :__julianday(0) {

	}
	explicit CJulianday(DayType number) :__julianday(number) {

	}
	CJulianday(int year, int month, int day);
	explicit CJulianday(const struct tm &tm_value);

	//brief:
	DateType getDate()const;
	DayType getJulianday() {
		return __julianday;
	}
	int getYear()const {
		return getDate().year;
	}
	int getMonth()const {
		return getDate().month;
	}
	int getDay()const {
		return getDate().day;
	}
	//return:0-6分别代表Sunday-Saturday
	int getWeekday()const {
		return (__julianday + 1) % 7;
	}

	//brief:其他常规routine
	void swap(CJulianday &other) {
		std::swap(__julianday, other.__julianday);
	}

	bool isValid()const {
		return __julianday > 0;
	}

	string toString()const;

private:
	DayType __julianday;
};

#define _BINARY_COMPARE_OPERATOR_FOR_CJULIANDAY_(cmp) \
inline bool operator cmp(CJulianday first, CJulianday second) {\
	return first.getJulianday() cmp second.getJulianday();\
}

_BINARY_COMPARE_OPERATOR_FOR_CJULIANDAY_(< );
_BINARY_COMPARE_OPERATOR_FOR_CJULIANDAY_(> );
_BINARY_COMPARE_OPERATOR_FOR_CJULIANDAY_(<= );
_BINARY_COMPARE_OPERATOR_FOR_CJULIANDAY_(>= );
_BINARY_COMPARE_OPERATOR_FOR_CJULIANDAY_(== );
_BINARY_COMPARE_OPERATOR_FOR_CJULIANDAY_(!= );

#undef _BINARY_COMPARE_OPERATOR_FOR_CJULIANDAY_
}

#endif

