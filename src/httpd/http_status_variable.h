/*************************************************************************
	> File Name: http_status_variable.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019��11��13�� ������ 11ʱ13��59��
 ************************************************************************/
#ifndef _HTTPD_HTTP_STATUS_VARIABLE_H_
#define _HTTPD_HTTP_STATUS_VARIABLE_H_

#include"dependent/types_extension.h"

namespace imnet {

namespace http {

//brief:���󷽷�
enum Method {
	kUninit,
	kOptions,
	kHead,
	kGet,
	kPost,
	kPut,
	kDelete,
	kTrace
};

//brief:http�汾��
enum Version {
	kUnknown,
	kHttp10,
	kHttp11
};

//brief:http��Ӧ״̬��
enum ResponseStatusCode {
	kInvalid,
	kOk = 200,
	kMovedPermanently = 301,
	kBadRequest = 400,
	kForbidden = 403,
	kNotFound = 404,
	kServerError = 500,
};

}//!namespace http
}//!namespace imnet


#endif
