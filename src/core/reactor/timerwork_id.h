/*************************************************************************
	> File Name: timerwork_id.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月27日 星期五 15时21分46秒
 ************************************************************************/
#ifndef _CORE_TIMEWORK_ID_H_
#define _CORE_TIMEWORK_ID_H_


#ifdef _MSC_VER
#include"./timerwork.h"
#else
#include"core/reactor/timerwork.h"
#endif

namespace imnet {

namespace core {

//brief:本对象由用户持有，可以用于定时任务查找、删除等操作
//becare:本对象为值语义，其内所有资源不可被外部访问
//      其资源只能被内部类访问(TimerworkQueue)
class TimerworkId {
public:
	TimerworkId(Timerwork *timer,Timerwork::SequenceType seq)
		:__ptr(timer),
		__sequence(seq){
	}

	//仅能被内部使用
	friend class TimerworkQueue;
private:
	Timerwork* __ptr;
	Timerwork::SequenceType __sequence;
};

}//!namespace core
}//!namsepace imnet

#endif

