/*************************************************************************
	> File Name: file_extension.h
	> Author: Dream9
	> Brief: 针对文件读写分别进行的封装
	> Created Time: 2019年09月03日 星期二 14时38分32秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_FILE_EXTENSION_H_
#define _IMNET_DEPENDENT_FILE_EXTENSION_H_

#ifdef __GNUC__
#include"dependent/types_extension.h"
#include"dependent/string_piece.h"
#elif defined _MSC_VER
#include"types_extension.h"
#include"string_piece.h"
#endif

#include<sys/types.h>

namespace imnet {

namespace file_extension {
//brief:提供了两种形式的封装：适合小文件的读，文件的末尾追加

//brief:适合小文件的读
class Readfile {
public:
	typedef int HandleType;

	explicit Readfile(const CStringArg& filename);
	explicit Readfile(CStringArg&& filename);
	~Readfile();
	
	//brief:读取到内置buf中，然后通过c_str()获取
	int readToBuf(int* size, off_t offset=0);
	const char* c_str()const {
		return __buf;
	}

	//brief:读取到外部buf中
	int readToString(int max_size, string& buf);
	int readMetadata(int64_t& size_in_bytes,int64_t& last_modification,
		int64_t& last_access, int64_t& last_change_status);

	static const int kBufferSize = 64 * 1024;

private:
	HandleType __file;
	char __buf[kBufferSize];
	int __err;
};

//brief:只在文件尾部追加数据
//     将会为FILE*流主动提供缓冲区
class Appendfile {
public:
	typedef FILE* HandleType;

	explicit Appendfile(const CStringArg& filename);
	explicit Appendfile(CStringArg&& filename);
	~Appendfile();

	void writeToEndUnlocked(const char* content, size_t len);
	void writeToEnd(const char* content, size_t len);

	void flush();

	off_t writtenBytes()const {
		//TODO:
		return 0;
	}
	
	static const int kBufferSize = 64 * 1024;

private:
	HandleType __file;
	char __buf[kBufferSize];
	off_t __written_bytes;
};

}//!namespace file_extension

}//!namespace imnet


#endif
