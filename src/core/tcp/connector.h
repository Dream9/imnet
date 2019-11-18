/*************************************************************************
	> File Name: connector.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019��10��29�� ���ڶ� 15ʱ08��17��
 ************************************************************************/
#ifndef _CORE_CONNECTOR_H_
#define _CORe_CONNECTOR_H_

#ifdef _MSC_VER
#include"sockaddr.h"
#include"../../dependent/noncopyable.h"
#else
#include"core/tcp/sockaddr.h"
#include"dependent/noncopyable.h"
#endif

#include<functional>
#include<memory>
#include<atomic>

namespace imnet {

namespace core {

class Eventloop;
class Channel;
class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr; 

//brief:���ฺ����ָ����ַ�������ӣ�����������fdͨ��ע���ConnectionCallback���䴫�ݸ��ͻ��ˣ�
//     ͨ����TcpClient�����fd�ĺ���ά����������Ҫ����Ӧ��connect�����е��쳣����ͳ�ʱ����
//becare:�����ǿ����ظ�ʹ�õģ���restart�����������ǲ�һ����(�ı䱾������˿�)
class Connector :noncopyable ,public std::enable_shared_from_this<Connector>{
public:
	typedef std::function<void(int sockfd)> ConnectionCallbackType;

	Connector(Eventloop* __loop, const Sockaddr& address);
	~Connector();

	//�û�Ҫ����ָ����ַ��������
	void start();
	void restartInLoop();//����������ͻ��˵��Զ���������
	void stop();
	void stop(const ConnectorPtr& self);//Ϊ���ӻ�����

	const Sockaddr& getServerAddress()const {
		return __serve_address;
	}

	bool isDisconnected()const {
		return __state == kDisconnected;
	}

	//beacre:���������������û�ֱ��ʹ��
	void setConnectionCallback(const ConnectionCallbackType& call) {
		__on_connect = call;
	}

private:
	//brief:�����״̬Ǩ��Ϊ����ʼkDisconnected,
	//     ����start(�״�)/restart,״̬ΪkConnecting,
	//     ������ɲ�__on_connect֮��ΪkConnected
	//     ֻ��restart�����ظ��޸�kConnecting
	enum State{
		kDisconnected,
		kConnecting,
		kConnected
	};

	void __startInLoop();
	void __stopInLoop();
	void __stopInLoop(const ConnectorPtr& self);//Ϊ���ӻ�����

	void __connect();
	void __connecting(int sockfd);
	void __retryConnect(int sockfd);

	void handleError();
	void handleWrite();

	int __getFdAndRestChannel();
	void __resetChannelNextLoop();

	void __setState(State state) {
		__state = state;
	}

	Eventloop* __loop;
	Sockaddr __serve_address;
	State __state;//����״̬
	int __backoff;//�˱ܲ���
	//bool __need_connecting;//�û��Ƿ���Ҫ����
	std::atomic_bool __need_connecting;//�û��Ƿ���Ҫ����,�ñ������Բ���IO�߳����޸�
	std::unique_ptr<Channel> __channel;

	ConnectionCallbackType __on_connect;

	static const int kMaxBackoff;
	static const int kInitBackoff;
};

}//!namespace core
}//!namespace imnet




#endif
