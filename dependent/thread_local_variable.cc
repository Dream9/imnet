/*************************************************************************
	> File Name: thread_local_variable.cc
	> Author: Dream9
	> Brief: 初始化线程周期变量
	> Created Time: 2019年08月20日 星期二 09时11分23秒
 ************************************************************************/
#ifdef __GNUC__
#include"dependent/thread_local_variable.h"
#elif defined(_MSC_VER)
#include"thread_local_variable.h"
#endif

#include<algorithm>

#include<sys/syscall.h>
#include<sys/types.h>
#include<unistd.h>
//for demangle and backtrace
#include<cxxabi.h>
#include<execinfo.h>




namespace imnet {

namespace detail {
//brief:gettid:linux的扩展函数
inline pid_t gettid() {
	return static_cast<pid_t>(::syscall(SYS_gettid));
}
}//namespace detail

namespace thread_local_variable {

thread_local pid_t tl_tid = 0;
thread_local string tl_tid_string("0");
thread_local string tl_thread_name("Unknown");

//废弃
//应为不同平台对于pid_t的定义可能不一样，比如定义为无符型
//brief:由于封装的gettid()将pid_t转化为int
//static_assert(std::is_same<int, pid_t>::value, "binary compatibility error:pid_t is not int");

//brief:对tid的首次缓存
void thread_local_variable_initialization() {
	if (tl_tid == 0) {
		tl_tid = detail::gettid();
	}
}

//brief:
bool isMainThread() {
	return tl_tid == ::getpid();
}

//breif:业务逻辑禁止使用
void sleep_for(int64_t usec) {
	struct timespec ts = { 0,0 };
	ts.tv_sec = static_cast<time_t>(usec/(1000*1000));
	ts.tv_nsec = static_cast<long>(usec%(1000*1000) * 1000);
	::nanosleep(&ts, nullptr);
}

//brief:bt功能的实现
string backTrace(bool need_demangle) {

	//man backtrace
	//SYNOPSIS
    //  #include <execinfo.h>

	//  int backtrace(void **buffer, int size);

	//  char **backtrace_symbols(void *const *buffer, int size);

	//  void backtrace_symbols_fd(void *const *buffer, int size, int fd);

	//DESCRIPTION

	//  backtrace()  returns  a backtrace for the calling program, in the array pointed to by buffer.A backtrace is the series of currently active function calls for the program.Each
	//	item in the array pointed to by buffer is of type void *, and is the return address from the corresponding stack frame.The  size  argument  specifies  the  maximum  number  of
	//	addresses that can be stored in buffer.If the backtrace is larger than size, then the addresses corresponding to the size most recent function calls are returned; to obtain the
	//	complete backtrace, make sure that buffer and size are large enough.

	//	Given the set of addresses returned by backtrace() in buffer, backtrace_symbols() translates the addresses into an array of strings that describe the addresses symbolically.The
	//	size  argument  specifies the number of addresses in buffer.The symbolic representation of each address consists of the function name(if this can be determined), a hexadecimal
	//	offset into the function, and the actual return address(in hexadecimal).The address of the array of string pointers is returned as the function result of  backtrace_symbols().
	//	This array is malloc(3)ed by backtrace_symbols(), and must be freed by the caller.  (The strings pointed to by the array of pointers need not and should not be freed.)

	//	backtrace_symbols_fd() takes the same buffer and size arguments as backtrace_symbols(), but instead of returning an array of strings to the caller, it writes the strings, one per
	//	line, to the file descriptor fd.backtrace_symbols_fd() does not call malloc(3), and so can be employed in situations where the latter function might fail, but see NOTES.
	//
	//RETURN VALUE
	//	backtrace() returns the number of addresses returned in buffer, which is not greater than size.If the return value is less than size, then the full backtrace was stored; if  it
	//	is equal to size, then it may have been truncated, in which case the addresses of the oldest stack frames are not returned.

	//	On success, backtrace_symbols() returns a pointer to the array malloc(3)ed by the call; on error, NULL is returned.

	//编译时加上 -rdynamic命令

	const int depth = 100;
	void* frame[depth];


	//获得地址
	int number_of_address = ::backtrace(frame, depth);
	//解析为字符串，注意释放内存
	char** bt_infos = ::backtrace_symbols(frame, number_of_address);

	string bt;
	if (bt_infos) {
		if (need_demangle) {
			size_t buf_len = 256;
			char *buf = static_cast<char*>(::malloc(buf_len));
			if (!buf) {
				free(bt_infos);
				return bt;
			}
			//形如:
			// ./prog(myfunc3 + 0x1f)[0x8048783]
			// ./prog()[0x8048810]
			// ./prog(myfunc + 0x21)[0x8048833
			//target:把(XXXX + 0xBBBB)部分进行逆向mangle,其他部分保持不变
			for (int i = 1; i < number_of_address; ++i) {
				char *start_pos = strchr(bt_infos[i],'(');
				char *end_pos = strchr(bt_infos[i],'+');

				if (start_pos && end_pos) {
					*end_pos = '\0';
					int status = 0;
	//abi::_cxa_demangle's Document
	//char* abi::__cxa_demangle(const char * 	mangled_name,
	//char * 	output_buffer,
	//size_t * 	length,
	//int * 	status
	//)
	//New ABI - mandated entry point in the C++ runtime library for demangling.

	//Parameters:
	//mangled_name 	A NUL - terminated character string containing the name to be demangled.
	//output_buffer 	A region of memory, allocated with malloc, of *length bytes, into which the demangled name is stored.If output_buffer is not long enough, it is expanded using realloc.output_buffer may instead be NULL; in that case, the demangled name is placed in a region of memory allocated with malloc.
	//length 	If length is non - NULL, the length of the buffer containing the demangled name is placed in *length.
	//status 	*status is set to one of the following values :
	//0 : The demangling operation succeeded.
	//- 1 : A memory allocation failiure occurred.
	//- 2 : mangled_name is not a valid name under the C++ ABI mangling rules.
	//- 3 : One of the arguments is invalid.
					char* demangle_name = abi::__cxa_demangle(start_pos + 1, buf, &buf_len, &status);
					*end_pos = '+';
					if (status == 0) {
						buf = demangle_name;//防止realloc导致原来指针空悬
						bt.append(bt_infos[i], start_pos + 1);
						bt.append(buf);
						bt.append(end_pos);
						bt.push_back('\n');
						continue;
					}
				}
				//不需要demangle或者demagnle失败的都只需到结果之中即可
				bt.append(bt_infos[i]);
				bt.push_back('\n');
			}
			free(buf);
		}
		else {
			//不进行demangle
			std::for_each(bt_infos + 1, bt_infos + number_of_address,
				[&](char *c) {bt.append(c); bt.push_back('\n'); return; });
		}
		free(bt_infos);
	}
	return bt;
}

} //namespace thread_local_variable
} //namespace imnet

