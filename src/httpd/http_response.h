/*************************************************************************
	> File Name: http_response.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年11月12日 星期二 10时03分41秒
 ************************************************************************/
#ifndef _HTTPD_HTTP_RESPONSE_H_
#define _HTTPD_HTTP_RESPONSE_H_

#ifdef _MSC_VER
#include"http_status_variable.h"
#else
#include"httpd/http_status_variable.h"
#endif

#include<map>

namespace imnet {

namespace core {
class Buffer;
}

namespace http {

using core::Buffer;

class HttpResponse {
public:
	explicit HttpResponse(bool keep_alive = true)
		:__status(ResponseStatusCode::kInvalid),
		__keep_alive(keep_alive) 
	{}

	void setStatus(ResponseStatusCode val) {
		__status = val;
	}

	void setKeepAlive(bool on) {
		__keep_alive = on;
	}
	bool keepAlive()const {
		return __keep_alive;
	}

	//设置响应体
	void setBody(const string& body) {
		__body = body;
	}
	void setBody(const char* start, const char* end) {
		__body.assign(start, end);
	}
	void appendBody(const string& body) {
		__body.append(body);
	}
	void appendBody(const char* start, const char* end) {
		__body.append(start, end);
	}
	string& getBodyHandler(){
		return __body;
	}

	//brief:设置头部信息
	void addHeader(const string& key, const string& value) {
		__headers.insert({ key,value });
	}
	//brief:最常有的一项设置
	void setContentType(const string& type) {
		__headers.insert({ "Content-Type",type });
	}
	
	void appendToBuffer(Buffer* output);
	static string ResponseStatusCodeToString(ResponseStatusCode val);

private:
	ResponseStatusCode __status;
	bool __keep_alive;
	std::map<string, string> __headers;
	string __body;
};

}//!namespace http

}//!namespace imnet



#endif
