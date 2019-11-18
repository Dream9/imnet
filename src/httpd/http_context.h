/*************************************************************************
	> File Name: http_context.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年11月12日 星期二 10时03分10秒
 ************************************************************************/
#ifndef _HTTPD_HTTP_CONTEXT_H_
#define _HTTPD_HTTP_CONTEXT_H_

#ifdef _MSC_VER
#include"http_request.h"
#else
#include"httpd/http_request.h"
#endif

namespace imnet {

namespace core {

class Buffer;

}//!namespace core

namespace http {

using core::Buffer;

//brief:对正在被解析的报文内容(上下文)的封装
//      每次信息到达后，上层（HttpServer）会令本对象负责解析其内容，
//      当一条请求内容完整解析完成后，上层将会业务处理
//TODO:请求体的处理方式：Content-Length/Chunked
class HttpContext {
public:
	//标记当前解析状态
	enum State{
		kOnRequestLine,
		kOnHeader,
		kOnBody,
		kDone
	};

	HttpContext():__state(kOnRequestLine),__body_flag(-1){}

	//brief:上层调用解析接口
	int parseRequest(Buffer* buf, CTimestamp timestamp);

	//brief:是否已经获得一份完整的请求
	bool isDone()const {
		return __state == kDone;
	}

	HttpRequest& getHttpRequest() {
		return __request;
	}
	const HttpRequest& getHttpRequest()const {
		return __request;
	}

	//brief:重置状态
	void reset() {
		HttpRequest tmp;
		__request.swap(tmp);
		__state = kOnRequestLine;
	}
private:

	int __parseRequestLine(Buffer* buf, CTimestamp timestamp);
	int __parseHeader(Buffer* buf);
	int __parseBody(Buffer* buf);
	int __parseBodyChunked(Buffer* buf);

	int __dealBodyTransferType();
	
	State __state;
	int __body_flag;//针对存在请求体，并且没有采用Chunked的传送方式
	HttpRequest __request;

	static const int kChunked;

};

}//!namespace http
}//!namespace imnet

#endif
