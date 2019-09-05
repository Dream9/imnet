/*************************************************************************
	> File Name: countdown_latch.h
	> Author: Dream9
	> Brief: 倒计时器的封装
	> Created Time: 2019年08月27日 星期二 09时17分31秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_COUNTDOWN_LATCH_H_
#define _IMNET_DEPENDENT_COUNTDOWN_LATCH_H_


#ifdef __GNUC__
#include"dependent/advanced/condition.h"
#elif defined(_MSC_VER)
#include"condition.h"
#endif

namespace imnet {

//brief:利用Condition完成的CountdownLatch高级并发部件
//becare:类中成员的构造顺序是声明的顺序，而其中的Condition需要使用__mutex，因此需要注意声明顺序
class CountdownLatch :noncopyable {
public:
	explicit CountdownLatch(int number)
		:__mutex(), __condition(__mutex), __count(number) {
	}

	void wait();
	void countDown();
	int getCount()const;

private:
	mutable Mutex __mutex;
	Condition __condition;
	int __count;
};
}



#endif
