/*************************************************************************
	> File Name: file_extension.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月03日 星期二 14时38分42秒
 ************************************************************************/
#ifdef __GNUC__
#include"dependent/file_extension.h"
#elif defined _MSC_VER
#include"file_extension.h"
typedef int ssize_t;
#endif

#include<fcntl.h>
#include<errno.h>
#include<cerrno>
#include<cstdio>
#include<algorithm>

#include<unistd.h>
#include<sys/stat.h>

namespace imnet{

namespace file_extension {
Readfile::Readfile(const CStringArg& filename)
	:__file(open(filename.c_str(),O_RDONLY|O_CLOEXEC)),__err(0){
	if (__file < 0) {
		__err = errno;
		perror("open() failed:");
		//fprintf(stderr, "open() failed, %s", strerror(__err));
	}
	*__buf = '\0';
}

Readfile::Readfile(CStringArg&& filename)
	:__file(open(filename.c_str(),O_RDONLY|O_CLOEXEC)),__err(0){
	if (__file < 0) {
		__err = errno;
		fprintf(stderr, "open() failed, %s", strerror(__err));
	}
	*__buf = '\0';
}

Readfile::~Readfile() {
	if (__file > 0) {
		::close(__file);
	}
}

//brief:读取指定大小或者直到EOF的文件内容到用户buf
//     已经考虑了EINTR，外部不需要再次判断
//return：成功for0,失败for错误序号，期待strerror等解析
int Readfile::readToString(int max_size, string& buf) {
	static_assert(sizeof(off_t) == 8, "Compile your programs with \"gcc - D_FILE_OFFSET_BITS = 64.\""
		"This forces all file access calls to use the 64 bit variants.");
	assert(max_size >= 0);
	__err = 0;
	if (__file >= 0) {
		buf.clear();

		//循环读取至EOF或者指定大小
		while (buf.size() <static_cast<size_t>( max_size)) {
			size_t this_round_to_read = std::min(static_cast<size_t>(max_size) - buf.size(), sizeof __buf);
			ssize_t n = ::read(__file, __buf, this_round_to_read);
			if(n>0)
				buf.append(__buf, n);
			else if (n < 0) {
				if (errno == EINTR)
					continue;
				__err = errno;
				break;
			}
			else {
				break;
			}
		}
	}
	else {
		__err = EBADF;
	}
	return __err;
}

//brief:获取文件相关的元数据信息
int Readfile::readMetadata(int64_t& size_in_bytes, int64_t& last_modification,
	int64_t& last_access, int64_t&last_change_status) {
	//assert(size_in_bytes);
	__err = 0;
	if (__file >= 0) {
		struct stat stat_parser;
		if (::fstat(__file, &stat_parser) == 0) {
			if (S_ISREG(stat_parser.st_mode)) {
				size_in_bytes = stat_parser.st_size;
			}
			else if (S_ISDIR(stat_parser.st_mode)) {
				__err = EISDIR;
			}
			last_modification = stat_parser.st_mtime;
			last_access = stat_parser.st_atime;
			last_change_status = stat_parser.st_ctime;
		}
		else
			__err = errno;
	}
	else {
		__err = EBADF;
	}

	return __err;
}

//brief:这种读取适合小文件（64kB-1B）,它默认只从头开始读取
//     通过循环设置offset，并取走__buf中的内容，直到EOF,可以实现全部文件读取
//parameter:在返回0的情况下，参数size如果有效，那么将其设置为读到的字节数，或者0代表EOF
//return:成功返回0，否则返回错误序号，期待外部通过strerror之类的函数进行解析
//becare:没有考虑EINTR，需要外部循环判断
int Readfile::readToBuf(int* size, off_t offset) {
	__err = 0;
	if (__file >= 0) {
		//becare:始终是从
		ssize_t n = ::pread(__file, __buf, sizeof(__buf) - 1, offset);
		if (n < 0)
			//BECARE:没有考虑EINTR
			__err = errno;
		else if (n>0) {
			__buf[n] = '\0';
		}
		if(size)
			*size = static_cast<int>(n);
	}
	else {
		__err = EBADF;//坏的文件描述符
	}
	return __err;
}



/////////
//brief:Appendfile的构造析构例程
Appendfile::Appendfile(const CStringArg& filename)
	:__file(fopen(filename.c_str(),"ae")),__written_bytes(0) {
	if (!__file)
		perror("fopen() failed:");
	else
		::setbuffer(__file, __buf, sizeof __buf);

}

Appendfile::Appendfile(CStringArg&& filename)
	:__file(fopen(filename.c_str(),"ae")),__written_bytes(0) {
	if (!__file)
		perror("fopen() failed:");
	else
		::setbuffer(__file, __buf, sizeof __buf);

}

Appendfile::~Appendfile() {
	if (__file)
		fclose(__file);
}
//brief；带锁的二进制写，循环检测直到写完或者出错
//becare:加锁版本
void Appendfile::writeToEnd(const char* content,size_t len) {
	size_t left = len;
	size_t pos = 0;
	::flockfile(__file);//先获得锁
	while (left > 0) {
		size_t n = ::fwrite_unlocked(content + pos, 1, left, __file);
		if (n > 0) {
			left -= n;
			pos += n;
		}
		else {
			int err = ferror(__file);
			if (err == EINTR)
				continue;
			if (err) {
				perror("fwrite_unlocked() failed:");
			}
			break;
		}
	}
	::funlockfile(__file);//释放锁
	__written_bytes += len;
}

//brief；不带锁的二进制写，循环检测直到写完或者出错
//becare:没有加锁,不是线程安全的,但是读写速度更快
//      因此在外部使用该对象的此函数时，需要和Mutex联合使用
void Appendfile::writeToEndUnlocked(const char* content,size_t len) {
	size_t left = len;
	size_t pos = 0;
	while (left > 0) {
		size_t n = ::fwrite_unlocked(content + pos, 1, left, __file);
		if (n > 0) {
			left -= n;
			pos += n;
		}
		else {
			int err = ferror(__file);
			if (err == EINTR)
				continue;
			if (err) {
				perror("fwrite_unlocked() failed:");
			}
			break;
		}
	}
	__written_bytes += len;
}

//brief:刷新缓冲区
void Appendfile::flush() {
	if (__file)
		fflush(__file);
}
}//!namespace file_extension

}//!namespace imnet
