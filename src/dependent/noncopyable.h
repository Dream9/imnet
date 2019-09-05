/*************************************************************************
	> File Name: noncopyable.h
	> Author: Dream9
	> Brief:
	> Created Time: 2019年08月13日 星期二 09时20分35秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_NONCOPYABLE_H_
#define _IMNET_DEPENDENT_NONCOPYABLE_H_

namespace imnet {
//brief:禁止拷贝和复制操作的tag
//becare:建议private继承，体现has-a的设计
//      注意移动是允许的
//      主要是体现类的对象语义而非值语义
class noncopyable {
public:
	void operator=(const noncopyable &) = delete;
	noncopyable(const noncopyable&) = delete;
protected:
	noncopyable() = default;
	~noncopyable() = default;
};
}//namespace imnet

#endif
