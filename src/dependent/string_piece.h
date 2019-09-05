/*************************************************************************
	> File Name: string_piece.h
	> Author: Dream9
	> Brief:  将c风格和c++风格的字符串以统一的方式存储
	> Created Time: 2019年08月13日 星期二 13时59分50秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_STRING_PIECE_H_
#define _IMNET_DEPENDENT_STRING_PIECE_H_

#ifdef __GNUC__
#include"dependent/types_extension.h"
#elif defined _MSC_VER
#include"types_extension.h"
#endif

#include<cstring>
#include<iosfwd>

namespace imnet {

//brief:以统一形式表述
//becare:如果string不是以\0结尾，读取时是存在问题的
class CStringArg {
public:
	CStringArg(const char *str) :__ptr(str) {}
	CStringArg(const string &str) :__ptr(str.c_str()) {}

	const char* c_str()const {
		return __ptr;
	}
private:
	const char *__ptr;
};

//brief:提供c和c++字符串的统一对外接口
//becare:只提供访问字符串的接口，并没有提供修改字符串的接口
//       并没有获得原始字符串的句柄，并不掌控它们的生命周期
//       其使用类似于PIMPL手法
class CStringPiece {
public:
	//brief:构造函数
	//becare:采用string::data()初始化可能并没有以'\0'终结
	CStringPiece()
		:__ptr(nullptr), __len(0) {}
	CStringPiece(const char *str)
		:__ptr(str), __len(static_cast<int>(strlen(__ptr))) {}
	CStringPiece(const string &str)
		:__ptr(str.data()), __len(static_cast<int>(str.size())) {}
	CStringPiece(const char *str, int len)
		:__ptr(str), __len(len) {}
	CStringPiece(const unsigned char *str)
		:__ptr(reinterpret_cast<const char *>(str)),
		__len(static_cast<int>(strlen(__ptr))) {}

	//brief:提供一致的对外接口
	const char* data() const { return __ptr; }
	int size() const { return __len; }
	bool empty() const { return __len == 0; }
	const char* begin() const { return __ptr; }
	const char* end() const { return __ptr + __len; }
	//becare:仅仅能够访问(没有返回引用)，不能修改字符串数据
	char operator[](int i) const { return __ptr[i]; }

	//brief:提供修改内部成员的接口
	void clear() { __ptr = nullptr; __len = 0; }
	void set(const char *str) {
		__ptr = str;
		__len = static_cast<int>(strlen(__ptr));
	}
	void set(const void *str, int len) {
		__ptr = reinterpret_cast<const char *>(str);
		__len = len;
	}
	void set(const char *str, int len) {
		__ptr = str;
		__len = len;
	}

	//brief:由于本类作为一个中间层，可以方便的实现移除前缀和后缀
	void removePrefix(int n) {
		assert(n <= __len);
		__ptr += n;
		__len -= n;
	}
	void removeSuffix(int n) {
		assert(n <= __len);
		__len -= n;
	}

	//brief:本类作为一个中间层，相等的判断要转移到字符串上面
	bool operator==(const CStringPiece &other) const {
		return (__len == other.__len) &&
			(memcmp(__ptr, other.__ptr, __len) == 0);
	}
	bool operator!=(const CStringPiece &other) const {
		return !(this->operator==(other));
	}
	//brief:其他几个比较函数的重载
#define STRINGPIECE_BINARY_PREDICATE(cmp,auxcmp) \
  bool operator cmp (const CStringPiece& x) const { \
    int r = memcmp(__ptr, x.__ptr, __len < x.__len ? __len: x.__len); \
    return ((r auxcmp 0) || ((r == 0) && (__len cmp x.__len))); \
  }
	STRINGPIECE_BINARY_PREDICATE(< , < );
	STRINGPIECE_BINARY_PREDICATE(<= , < );
	STRINGPIECE_BINARY_PREDICATE(>= , > );
	STRINGPIECE_BINARY_PREDICATE(> , > );
#undef STRINGPIECE_BINARY_PREDICATE

	int compare(const CStringPiece &other) const {
		int r = memcmp(__ptr, other.__ptr, __len < other.__len ? __len : other.__len);
		return r != 0 ? r : (__len < other.__len ? -1 : (__len > other.__len ? 1 : 0));
	}

	//brief:得到string
	//注意当进行RVO时，外部的接收者只会进行一次拷贝构造的调用
	string getString() const {
		return string(data(), size());
	}
	//brief:直接assign string
	void copyToString(string *str) {
		str->assign(data(), size());
	}

	//brief:比较前缀
	bool startWith(const CStringPiece &prefix) {
		return (__len >= prefix.__len) && (memcmp(__ptr, prefix.__ptr, prefix.__len) == 0);
	}
private:
	const char *__ptr;
	int __len;

};
}

//brief:类型萃取帮助编译器进行优化
#ifdef HAVE_TYPE_TRAITS
#pragma message("type_traits actived")
template<>
struct __type_traits<imnet::CStringPiece> {
	typedef __true_type has_trivial_default_constructor;
	typedef __ture_type has trivial_copy_constructor;
	typedef __true_type has_trivial _assignment_operator;
	typedef __true_type has_trivial_destructor;
	typedef __true_type is_POD_type;
};
#else
#pragma message("type_traits is not detected")
#endif

//brief：前向声明,使其支持<<操作
std::ostream& operator<<(std::ostream &os, const imnet::CStringPiece &str);

#endif
