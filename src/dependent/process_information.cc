/*************************************************************************
	> File Name: process_information.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月05日 星期四 09时08分23秒
 ************************************************************************/
#ifdef _MSC_VER
#include"process_information.h"
#include"file_extension.h"
#include"thread_local_variable.h"
#elif defined __GNUC__
#include"dependent/process_information.h"
#include"dependent/file_extension.h"
#include"dependent/thread_local_variable.h"
#endif

#include<unistd.h>
#include<dirent.h>
#include<sys/resource.h>

namespace imnet {

//对外部隐藏
namespace detail {

thread_local int tl_opened_files_number = 0;
thread_local std::vector<pid_t>* tl_task_recorder;

//brief:用于筛选(实际上总是筛掉，return 0),但是把结果记录在相应数据处
int isFileDescriptor(const struct dirent* dir) {
	if (::isdigit(dir->d_name[0]))
		++tl_opened_files_number;
	return 0;
}
int isTask(const struct dirent* dir) {
	if (::isdigit(dir->d_name[0]))
		tl_task_recorder->emplace_back(::atoi(dir->d_name));
	return 0;
}

//brief:相当于for_each()
int scanDirent(const char *dirpath, int(*filter)(const struct dirent*)) {
	struct dirent** namelist = NULL;
	//
	//SYNOPSIS         top
	//int scandir(const char *dirp, struct dirent ***namelist,
	//	int(*filter)(const struct dirent *),
	//	int(*compar)(const struct dirent **, const struct dirent **));
	//int alphasort(const struct dirent **a, const struct dirent **b);
	//int versionsort(const struct dirent **a, const struct dirent **b);
	//int scandirat(int dirfd, const char *dirp, struct dirent ***namelist,
	//	int(*filter)(const struct dirent *),
	//	int(*compar)(const struct dirent **, const struct dirent **));
	//
	//The scandir() function scans the directory dirp, calling filter() on
	//each directory entry.Entries for which filter() returns nonzero are
	//stored in strings allocated via malloc(3), sorted using qsort(3) with
	//the comparison function compar(), and collected in array namelist
	//which is allocated via malloc(3).If filter is NULL, all entries are
	//selected.

	//The alphasort() and versionsort() functions can be used as the
	//comparison function compar().The former sorts directory entries
	//using strcoll(3), the latter using strverscmp(3) on the strings
	//(*a)->d_name and (*b)->d_name.
	//
	int result = ::scandir(dirpath, &namelist, filter, ::alphasort);
	//becare:传进来的策略总是全部筛掉
	assert(namelist == NULL);

	return result;
}

}//!namespace detail

namespace process_information {

//brief:记录程序开始时间
const CTimestamp g_process_start_time = CTimestamp::getNow();

//#include <unistd.h>
//long sysconf(int name);
//const long g_clock_ticks = ::sysconf(_SC_CLK_TCK);

const long g_page_size=::sysconf(_SC_PAGESIZE);

//brief:转调gethostname
string getHostname() {
	//gethostname() returns the null - terminated hostname in the character
	//array name, which has a length of len bytes.If the null - terminated
	//hostname is too large to fit, then the name is truncated, and no
	//error is returned(but see NOTES below).POSIX.1 says that if such
	//truncation occurs, then it is unspecified whether the returned buffer
	//includes a terminating null byte.
	//
	//ENAMETOOLONG
	//(glibc gethostname()) len is smaller than the actual size.
	//(Before version 2.1, glibc uses EINVAL for this case.)
	//
	//USv2 guarantees that "Host names are limited to 255 bytes".POSIX.1
	//guarantees that "Host names (not including the terminating null byte)
	//are limited to HOST_NAME_MAX bytes".  On Linux, HOST_NAME_MAX is
	//defined with the value 64, which has been the limit since Linux 1.0
	//(earlier kernels imposed a limit of 8 bytes).
#ifdef __LINUX__
	char buf[64];
#else 
	char buf[256];//取POSIX的极限标准
#endif

	if (0 == ::gethostname(buf, sizeof buf)) {
		return buf;
	}
	return "unknownhost";
}

//brief:获得进程，用户，有效用户ID
pid_t getPid() {
	return ::getpid();// These functions are always successful.
}
uid_t getUid() {
	return ::getuid();// These functions are always successful.
}
uid_t getEuid() {
	return ::geteuid();// These functions are always successful.
}

//brief:直接返回缓存的数据
CTimestamp getProcessStartTime() {
	return g_process_start_time;
}
long getPageSize() {
	return g_page_size;
}

//brief:获得进程、线程信息
string getProcStat() {
	string result;
	file_extension::Readfile read_agent("/proc/self/stat");
	read_agent.readToString(65536, result);
	return result;
	
}
string getTaskStat() {
	string result;
	char buf[64];
	snprintf(buf, sizeof buf, "/proc/self/task/%d/stat", thread_local_variable::gettid());
	file_extension::Readfile read_agent(buf);
	read_agent.readToString(65536, result);
	return result;
}

//brief:获得本进程的可执行文件的路径
string getExePath() {
	string result;
	char buf[1024];
	//
	//readlink() places the contents of the symbolic link pathname in the
	//buffer buf, which has size bufsiz.readlink() does not append a null
	//byte to buf.It will(silently) truncate the contents(to a length
	//	of bufsiz characters), in case the buffer is too small to hold all of
	//the contents.
	//
	ssize_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
	if (n > 0) {
		//因为它并不会自动追加'\0'
		result.assign(buf, n);
	}
	return result;
}

//brief;获得进程打开的文件数
int getOpenedFileNumber() {
	detail::tl_opened_files_number = 0;
	detail::scanDirent("/proc/self/fd", detail::isFileDescriptor);
	return detail::tl_opened_files_number;
}

//brief:获得进程创建的线程tid
std::vector<pid_t> getCreatedThreads() {
	std::vector<pid_t> result;
	detail::tl_task_recorder = &result;
	detail::scanDirent("/proc/self/task", detail::isTask);
	return result;
}

//brief:获得进程最大可打开文件数
//becare:查询失败时返回当前打开的文件数目
int getMaxOpenFileNumber() {
	struct rlimit rl;
	if (::getrlimit(RLIMIT_NOFILE, &rl)) {
		//查询失败
		return getOpenedFileNumber();
	}
	return static_cast<int>(rl.rlim_cur);
}

}//!namespace process_information

}//!namespace imnet

