/*************************************************************************
	> File Name: callback_types.h
	> Author: Dream9
	> Brief: �������͵�ǰ�����������ļ����Ա��û�ֱ��ʹ��
	> Created Time: 2019��10��15�� ���ڶ� 09ʱ15��37��
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
//FIXME:Buffer��Ӧλ��detail�ռ���
//typedef std::function<void(const TcpConnectionPtr&, detail::Buffer*, const CTimestamp&)> MessageCallbackType;
typedef std::function<void(const TcpConnectionPtr&, core::Buffer*, const CTimestamp&)> MessageCallbackType;
typedef std::function<void(const TcpConnectionPtr&)> CloseCallbackType;
typedef std::function<void(const TcpConnectionPtr&)> ErrorCallbackType;
typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallbackType;
typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterCallbackType;
//TODO
//typedef std::function<void(const TcpConnectionPtr&)> CallbackType;


//Ĭ�ϵĴ������̣�ʵ��λ��tcp_connection.cc��
void defaultConnectionCallback(const TcpConnectionPtr& connect);
//void defaultMessageCallback(const TcpConnectionPtr& connect, detail::Buffer* buf, const CTimestamp& timestamp);
void defaultMessageCallback(const TcpConnectionPtr& connect, core::Buffer* buf, const CTimestamp& timestamp);



}//!namespace core
}//!namespace imnet

#endif
