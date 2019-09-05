/*************************************************************************
	> File Name: countdown_latch.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月27日 星期二 09时18分40秒
 ************************************************************************/

#ifdef __GNUC__
#include"dependent/advanced/countdown_latch.h"
#elif defined(_MSC_VER)
#include"countdown_latch.h"
#endif

using namespace imnet;

//brief:等待倒计时(条件)完成、
//     是对Conditon的标准使用步骤
void CountdownLatch::wait() {
	MutexUnique_lock mulock(__mutex);
	while (__count > 0) {
		__condition.wait();
	}
}

//brief:减少计数，当所有条件完成之后负责唤醒
//     是对Condition的标准使用步骤
void CountdownLatch::countDown() {
	MutexUnique_lock mulock(__mutex);
	--__count;
	if (__count <= 0) {
		__condition.notifyall();
	}
}

//brief:获得当前计数，不保证准确性，仅作参考用
//becare:读取__count数据，但是并没有加锁
//      这个功能本来就比较鸡肋，
//      如果为读加锁(必须在外部完成)，则需要暴露内部的__mutex,
//      而且需要用户保证lock和unlock的正确搭配
//      这与RAII封装的目的相违背
int CountdownLatch::getCount()const {
	//在内部加锁是没有意义的
	return __count;
}
