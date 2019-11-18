/*************************************************************************
	> File Name: http_request.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年11月12日 星期二 10时03分25秒
 ************************************************************************/
#ifndef HTTPD_HTTP_REQUEST_H_
#define HTTPD_HTTP_REQUEST_H_

#ifdef _MSC_VER
#include"http_status_variable.h"
#include"../dependent/logger/timestamp.h"
#else
#include"http_status_variable.h"
#include"dependent/logger/timestamp.h"
#endif

#include<map>

namespace imnet {

namespace http {
//brief:本对象是对请求报文内容的解析和封装
//      本对象由HttpContext负责填写，并交由用户使用
class HttpRequest {
public:
		HttpRequest() :__method(Method::kUninit), __version(Version::kUnknown) {	}

	bool setMethod(const char*start, const char* end);
	Method getMethod()const {
		return __method;
	}

	void setVersion(Version v) {
		__version = v;
	}
	Version getVersion()const {
		return __version;
	}

	void setPath(const char* start, const char* end) {
		__path.assign(start, end);
	}
	string getPath()const {
		return __path;
	}

	void setQuery(const char* start, const char* end) {
		__query.assign(start, end);
	}
	string getQuery()const {
		return __query;
	}

	//void setTime(const CTimestamp& timestamp) {//没有必要，拷贝一个int而已
	void setTime(CTimestamp timestamp) {
		__receive_time = timestamp;
	}
	CTimestamp getTime()const {
		return __receive_time;
	}

	//brief:向属性表中添加新的属性值
	void addHeader(const char* start, const char* colon, const char* end);
	string getHeader(const string& key)const {
		auto iter = __headers.find(key);
		if (iter != __headers.end())
			return iter->second;
		return "";
	}
	const std::map<string, string>& getHeaders()const {
		return __headers;
	}

	void setBody(const char* start, const char* end) {
		__body.assign(start, end);
	}
	void appendBody(const char* start, const char* end) {
		__body.append(start, end);
	}
	string getBody()const {
		return __body;
	}

	void swap(HttpRequest& other) {
		std::swap(__method, other.__method);
		std::swap(__version, other.__version);
		__path.swap(other.__path);
		__query.swap(other.__query);
		__receive_time.swap(other.__receive_time);
		__headers.swap(other.__headers);
		__body.swap(other.__body);
	}
	static string MethodToString(Method val);

private:
	Method __method;//请求方法
	Version __version;//http版本
	string __path;//请求的资源路径
	string __query;//请求的参数，可选
	CTimestamp __receive_time;//接收时间
	std::map<string, string> __headers;//请求头中的属性信息
	string __body;//请求体，可选
};

}//!namespace http

}//!namespace imnet


#endif

