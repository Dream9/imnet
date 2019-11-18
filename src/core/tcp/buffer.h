/*************************************************************************
	> File Name: buffer.h
	> Author: Dream9
	> Brief:
	> Created Time: 2019年10月15日 星期二 14时22分03秒
 ************************************************************************/
#ifndef _CORE_BUFFER_H_
#define _CORE_BUFFER_H_

#ifdef _MSC_VER
#include"../../dependent/noncopyable.h"
#include"../../dependent/types_extension.h"
#include"socket_extension.h"
//#include"../../dependent/exception_extension.h"//不做抛出异常
#else
#include"dependent/noncopyable.h"
#include"dependent/types_extension.h"
#include"core/tcp/socket_extension.h"
#endif

#include<vector>
#include<algorithm>
#include<assert.h>
#include<string.h>

namespace imnet {

//FIXME:Buffer对象是在onMessage过程中可以被用户直接使用的
//namespace detail {
namespace core{

//brief:TcpConnection内置输入/输出缓冲区
//     内部通过vector<uint8_t>记录数据，通过读写索引量分为三块区域:prependable area,data area,idle area
//     用户级的读写操作已经考虑了网络序主机序转换
//becare:索引量为size_t,防止迭代器失效
class Buffer : noncopyable {
public:
	typedef char InnerType;
	typedef InnerType* InnerPtr;
	typedef const InnerType* ConstInnerPtr;

	explicit Buffer(size_t size=kDefaultVectorSize)
		:__buf(size+kPrependLength),
		__read_index(kPrependLength),
		__write_index(kPrependLength){

	}

	//获取状态信息
	//通过两个索引位置可以将
	size_t getPrependableSize()const {
		return __read_index;
	}
	size_t getDataSize()const {
		return __write_index - __read_index;
	}
	size_t getIdleSize()const {
		return __buf.size() - __write_index;
	}
	size_t getAllSize()const {
		return __buf.size();
	}

	//brief从套接字fd出读取数据到缓冲区
	//return:读取的字节数，若出错，返回-1
	ssize_t readFrom(int fd, int* savedErrno = nullptr);
	//becare:发送数据到套接字fd通过TcpConnection的其他函数来辅助完成

	//////////////////////////////////////////////////////////////////
	//以下为application读写缓冲区数据

	//brief:find系列函数：从缓冲流中定位特定的分隔数据(crlf 以及eol)
	//becare:如果指定位置，则必须保证该位置位于data区内，否则获得的是dirty data
	ConstInnerPtr findCrlf()const {
		return findCrlf(peak());
	}
	ConstInnerPtr findCrlf(ConstInnerPtr pos)const {
		assert(pos <= beginWrite());//确保访问数据的合法性
		assert(peak() >= pos);

		char tmp[] = "\r\n";
		ConstInnerPtr ans = std::search(pos, beginWrite(), tmp, tmp + 2);
		return ans == beginWrite() ? nullptr : ans;
	}
	ConstInnerPtr findEol()const {
		return findEol(peak());
	}
	ConstInnerPtr findEol(ConstInnerPtr pos)const {
		assert(beginWrite() >= pos);//确保访问数据的合法性
		assert(peak() >= pos);

		const void* ans = memchr(pos, '\n', beginWrite() - pos);
		//return ans == beginWrite() ? nullptr : ans;
		//The memchr() return a pointer to the matching byte or NULL if the
		//character does not occur in the giver memory area
		return static_cast<ConstInnerPtr>(ans);
	}

	//brief:retrieve系列函数：从缓冲区中取出(同时从缓冲区中移除)数据
	//becare:本系列函数是为了与peek搭配,单位为字节
	//      仅仅是调整了索引，数据并没有被覆写，这也是dirty data的原因
	//      因此使用者不应该访问data区之外的任何数据
	void retrieve(size_t length) {
		if (length < getDataSize()) {
			__read_index += length;
		}
		else {
			retrieveAll();
		}
	}
	//brief:指定取出截至的位置
	void retrieveUntil(ConstInnerPtr pos) {
		assert(pos >= peak());
		assert(pos < beginWrite());

		retrieve(pos - peak());
	}
	//brief:取出所有数据
	void retrieveAll() {
		//__read_index = __write_index;
		//重新恢复到起始状态
		__read_index = kPrependLength;
		__write_index = kPrependLength;
	}

	//brief:特殊的情况，获取并取出数据
	string retrieveAllToString() {
		return retrieveToString(getDataSize());
	}
	string retrieveToString(size_t length){
		string out(peak(), std::min(length, getDataSize()));
		retrieveAll();
		return out;
	}


	//brief:peak系列函数：获得缓冲区记录的最前面可读取的数据，但不取出
	//becare:本系列函数是为了和C库的相关处理兼容，因此读取完数据后，务必通过retrieve取出已读取数据
	//      peak系列为const成员属性
	//      已经自动完成网络序的转换
	//      如果data区域数据小于期待读取的数据，应该抛出异常？   不做抛出异常处理
	//      调用者应该确保数据是足够的
	ConstInnerPtr peak()const {
		return beginRead();
	}

	uint64_t peakInt64()const {
		//Fixme:这种转换如果再主机为大端的上面就是错误的
		//const uint64_t* data = reinterpret_cast<const uint64_t*>(peak());
		//return socket_ops::be64toh(*data);

		//Fixme:空间不足时，调用失败将如何把这种情况告知调用者 特殊的返回值？抛出异常？
		assert(getDataSize() >= sizeof(uint64_t));

		//是否有必要？
		//if (getDataSize() < sizeof(uint64_t))
		//	throw ImnetException("Data is not enough.");

		uint64_t out;
		::memcpy(&out, peak(), sizeof out);
		return socket_ops::ntoh64(out);
	}
	uint32_t peakInt32()const;
	uint16_t peakInt16()const;
	uint8_t peakInt8()const;

	//brief:read系列函数：在peak系列函数基础之上，会取出数据
	//becare:主要注意网络序的转换
	uint64_t readInt64();
	uint32_t readInt32();
	uint16_t readInt16();
	uint8_t readInt8();

	//brief:append系列函数：向缓冲区中追加数据
	//becare:主要注意网络序的转换
	void append(const void* data, size_t length) {
		ensureCapacity(length);

		ConstInnerPtr iter = static_cast<ConstInnerPtr>(data);
		std::copy(iter, iter+ length, beginWrite());
		__write_index += length;
	}
	//brief:作为一个中转层,已废弃
	//void append(const void* data, size_t length) {
	//	append(static_cast<ConstInnerPtr>(data), length);
	//}
	//brief:针对网络序的特化
	void appendInt64(uint64_t data);
	void appendInt32(uint32_t data);
	void appendInt16(uint16_t data);
	void appendInt8(uint8_t data) {
		append(&data, sizeof data);
	}
	void append(const string& str) {
		append(str.c_str(), str.size());
	}

	//brief:prepend系列函数：向缓冲区添加前缀数据(通过预留前缀空间完成)
	//becare:用户应该检查前缀空间足够
	//      注意网络序转换
	void prepend(const void* data, size_t length) {
		assert(__read_index >= length);

		__read_index -= length;
		ConstInnerPtr iter = static_cast<ConstInnerPtr>(data);
		std::copy(iter, iter + length, &__buf[__read_index]);
	}
	
	void prependInt64(uint64_t data);
	void prependInt32(uint32_t data);
	void prependInt16(uint16_t data);
	void prependInt8(uint8_t data) {
		prepend(&data, sizeof data);
	}
	
	//通用接口实现
	void swap(Buffer& peer) {
		__buf.swap(peer.__buf);
		std::swap(__read_index, peer.__read_index);
		std::swap(__write_index, peer.__write_index);
	}

	InnerPtr beginWrite(){
		return begin() + __write_index;
	}
	//brief:const版本是为了配合beginRead只有const版本，许多stl算法的迭代器输入类型需要保持一致
	ConstInnerPtr beginWrite()const {
		return begin() + __write_index;
	}
	//becare:只提供const版本，本函数获得handle只能读取data数据
	ConstInnerPtr beginRead()const{
		return begin() + __read_index;
	}
	
	//brief:确保空间大小
	void ensureCapacity(size_t len) {
		size_t idle = getIdleSize();
		if (len <= idle) {
			return;
		}

		if (getPrependableSize() + idle >= kPrependLength + len) {
			//重新调整前缀空间
			//memcpy(peek(), begin() + kPrependLength, getDataSize());
			memcpy(begin() + kPrependLength, peak(), getDataSize());
			__read_index = kPrependLength;
			__write_index = __read_index + getDataSize();
		}
		else {
			__buf.resize(__write_index + len);
			//std::vector<InnerType> tmp;
			//tmp.reserve(__buf.size() + len);
			//std::copy(begin() + __read_index, begin() + __write_index, std::back_instertor(tmp));
		}
	}
	


private:
	
	//becare:以下接口是为了方便与其他库函数兼容，
	//      data区内的数据通过peak系列访问时只能进行读取，不可修改，
	//      const版本的begin就是为了迎合此
	InnerPtr begin() {
		return &*__buf.begin();
	}
	//brief:为了迎合peak的使用
	ConstInnerPtr begin()const {
		return &*__buf.begin();
	}

	//内部数据组织
	//std::vector<uint8_t> __buf;//废弃
	std::vector<InnerType> __buf;
	//becare:使用索引防止迭代器或者指针失效
	size_t __read_index;
	size_t __write_index;

	static const int kPrependLength;
	static const int kDefaultVectorSize;

};

//}//!namespace detail
}//!namespace core 
}//!namespace imnet


#endif
