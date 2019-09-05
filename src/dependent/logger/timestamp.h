/*************************************************************************
	> File Name: timestamp.h
	> Author: Dream9
	> Brief:时间戳的封装
	> Created Time: 2019年08月17日 星期六 09时30分35秒
 ************************************************************************/

#ifndef _IMNET_DEPENDENT_TIMESTAMP_H_
#define _IMNET_DEPENDENT_TIMESTAMP_H_

#ifdef __GNUC__
#include"dependent/types_extension.h"
#elif defined(_MSC_VER)
#include"../types_extension.h"
#endif

#include<boost/operators.hpp>

namespace imnet {

//brief:采用一个int64_t记录gettimeofday()的计算结果，并提供了相应的封装
//      因此时间表示的时POSIX时间，单位μs
class CTimestamp :public boost::equality_comparable<CTimestamp>,
	public boost::less_than_comparable<CTimestamp> {
	typedef CTimestamp SelfType;
	typedef int64_t TimeType;
public:
	static const int kRatioFromMicrosecondToSecond;

	CTimestamp() :__microseconds(0) {
		;
	}
	explicit CTimestamp(int64_t microseconds) :__microseconds(microseconds) {
		;
	}

	//brief:常见routine
	bool isValid() const {//与之对应的是当小于等于零时为非法时间戳
		return __microseconds > 0; }
	void swap(SelfType &other) {
		std::swap(__microseconds, other.__microseconds);
	}
	TimeType data()const {//获取完整的数据，以微秒计
		return __microseconds;
	}
	int getMicroseconds()const {//仅获取微秒部分的数据
		return static_cast<int>(__microseconds % kRatioFromMicrosecondToSecond);
	}
	time_t getSeconds()const {//仅获取整秒部分的数据
		return static_cast<time_t>(__microseconds / kRatioFromMicrosecondToSecond);
	}

	//brief:配合less_than_compable<>以及equality_compable<>
	bool operator<(const SelfType &other)const {
		return __microseconds < other.__microseconds;
	}
	bool operator==(const SelfType &other)const {
		return __microseconds == other.__microseconds;
	}

	//brief:outline
	//static SelfType setTime(time_t t);
	static SelfType getFromUnixTime(time_t t);
	static SelfType getFromUnixTime(time_t t, int microseconds);
	static SelfType getInvalid();
	static SelfType getNow();

	string toString()const;
	string toUTCString(bool showMicroseconds=true)const;

private:
	//millisecond:毫秒，microsecond：微妙
	TimeType __microseconds;
};

//brief:以s为单位的时间戳的运算
//becare:inline函数应该定义在头文件中
inline double timestampDifference(const CTimestamp &first, const CTimestamp &second) {
	return static_cast<double>(first.getMicroseconds()-second.getMicroseconds())/CTimestamp::kRatioFromMicrosecondToSecond;
}
inline CTimestamp timestampDifference(const CTimestamp &first, double seconds) {
	return CTimestamp(first.getSeconds() + static_cast<int64_t>(CTimestamp::kRatioFromMicrosecondToSecond*seconds));
}

}//namespace imnet


#endif
