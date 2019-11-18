/*************************************************************************
	> File Name: callback_types.h
	> Author: Dream9
	> Brief: 常有类型的前置声明，本文件可以被用户直接使用
	> Created Time: 2019年10月15日 星期二 09时15分37秒
 ************************************************************************/
#ifndef _CORE_CALLBACK_TYPES_H_ 
#define _CORE_CALLBACK_TYPES_H_ 

#include<memory>
#include<functional>

namespace imnet {

//namespace detail {
//class Buffer;
//}

class CTimestamp;

namespace core {

class TcpConnection;
class Buffer;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallbackType;
//FIXME:Buffer不应位于detail空间中
//typedef std::function<void(const TcpConnectionPtr&, detail::Buffer*, const CTimestamp&)> MessageCallbackType;
typedef std::function<void(const TcpConnectionPtr&, core::Buffer*, const CTimestamp&)> MessageCallbackType;
typedef std::function<void(const TcpConnectionPtr&)> CloseCallbackType;
typedef std::function<void(const TcpConnectionPtr&)> ErrorCallbackType;
typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallbackType;
typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterCallbackType;
//TODO
//typedef std::function<void(const TcpConnectionPtr&)> CallbackType;


//默认的处理例程，实现位于tcp_connection.cc中
void defaultConnectionCallback(const TcpConnectionPtr& connect);
//void defaultMessageCallback(const TcpConnectionPtr& connect, detail::Buffer* buf, const CTimestamp& timestamp);
void defaultMessageCallback(const TcpConnectionPtr& connect, core::Buffer* buf, const CTimestamp& timestamp);



}//!namespace core
}//!namespace imnet

#endif
