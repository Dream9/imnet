/*************************************************************************
	> File Name: buffer.h
	> Author: Dream9
	> Brief:
	> Created Time: 2019��10��15�� ���ڶ� 14ʱ22��03��
 ************************************************************************/
#ifndef _CORE_BUFFER_H_
#define _CORE_BUFFER_H_

#ifdef _MSC_VER
#include"../../dependent/noncopyable.h"
#include"../../dependent/types_extension.h"
#include"socket_extension.h"
//#include"../../dependent/exception_extension.h"//�����׳��쳣
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

//FIXME:Buffer��������onMessage�����п��Ա��û�ֱ��ʹ�õ�
//namespace detail {
namespace core{

//brief:TcpConnection��������/���������
//     �ڲ�ͨ��vector<uint8_t>��¼���ݣ�ͨ����д��������Ϊ��������:prependable area,data area,idle area
//     �û����Ķ�д�����Ѿ�������������������ת��
//becare:������Ϊsize_t,��ֹ������ʧЧ
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

	//��ȡ״̬��Ϣ
	//ͨ����������λ�ÿ��Խ�
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

	//brief���׽���fd����ȡ���ݵ�������
	//return:��ȡ���ֽ���������������-1
	ssize_t readFrom(int fd, int* savedErrno = nullptr);
	//becare:�������ݵ��׽���fdͨ��TcpConnection�������������������

	//////////////////////////////////////////////////////////////////
	//����Ϊapplication��д����������

	//brief:findϵ�к������ӻ������ж�λ�ض��ķָ�����(crlf �Լ�eol)
	//becare:���ָ��λ�ã�����뱣֤��λ��λ��data���ڣ������õ���dirty data
	ConstInnerPtr findCrlf()const {
		return findCrlf(peak());
	}
	ConstInnerPtr findCrlf(ConstInnerPtr pos)const {
		assert(pos <= beginWrite());//ȷ���������ݵĺϷ���
		assert(peak() >= pos);

		char tmp[] = "\r\n";
		ConstInnerPtr ans = std::search(pos, beginWrite(), tmp, tmp + 2);
		return ans == beginWrite() ? nullptr : ans;
	}
	ConstInnerPtr findEol()const {
		return findEol(peak());
	}
	ConstInnerPtr findEol(ConstInnerPtr pos)const {
		assert(beginWrite() >= pos);//ȷ���������ݵĺϷ���
		assert(peak() >= pos);

		const void* ans = memchr(pos, '\n', beginWrite() - pos);
		//return ans == beginWrite() ? nullptr : ans;
		//The memchr() return a pointer to the matching byte or NULL if the
		//character does not occur in the giver memory area
		return static_cast<ConstInnerPtr>(ans);
	}

	//brief:retrieveϵ�к������ӻ�������ȡ��(ͬʱ�ӻ��������Ƴ�)����
	//becare:��ϵ�к�����Ϊ����peek����,��λΪ�ֽ�
	//      �����ǵ��������������ݲ�û�б���д����Ҳ��dirty data��ԭ��
	//      ���ʹ���߲�Ӧ�÷���data��֮����κ�����
	void retrieve(size_t length) {
		if (length < getDataSize()) {
			__read_index += length;
		}
		else {
			retrieveAll();
		}
	}
	//brief:ָ��ȡ��������λ��
	void retrieveUntil(ConstInnerPtr pos) {
		assert(pos >= peak());
		assert(pos < beginWrite());

		retrieve(pos - peak());
	}
	//brief:ȡ����������
	void retrieveAll() {
		//__read_index = __write_index;
		//���»ָ�����ʼ״̬
		__read_index = kPrependLength;
		__write_index = kPrependLength;
	}

	//brief:������������ȡ��ȡ������
	string retrieveAllToString() {
		return retrieveToString(getDataSize());
	}
	string retrieveToString(size_t length){
		string out(peak(), std::min(length, getDataSize()));
		retrieveAll();
		return out;
	}


	//brief:peakϵ�к�������û�������¼����ǰ��ɶ�ȡ�����ݣ�����ȡ��
	//becare:��ϵ�к�����Ϊ�˺�C�����ش�����ݣ���˶�ȡ�����ݺ����ͨ��retrieveȡ���Ѷ�ȡ����
	//      peakϵ��Ϊconst��Ա����
	//      �Ѿ��Զ�����������ת��
	//      ���data��������С���ڴ���ȡ�����ݣ�Ӧ���׳��쳣��   �����׳��쳣����
	//      ������Ӧ��ȷ���������㹻��
	ConstInnerPtr peak()const {
		return beginRead();
	}

	uint64_t peakInt64()const {
		//Fixme:����ת�����������Ϊ��˵�������Ǵ����
		//const uint64_t* data = reinterpret_cast<const uint64_t*>(peak());
		//return socket_ops::be64toh(*data);

		//Fixme:�ռ䲻��ʱ������ʧ�ܽ���ΰ����������֪������ ����ķ���ֵ���׳��쳣��
		assert(getDataSize() >= sizeof(uint64_t));

		//�Ƿ��б�Ҫ��
		//if (getDataSize() < sizeof(uint64_t))
		//	throw ImnetException("Data is not enough.");

		uint64_t out;
		::memcpy(&out, peak(), sizeof out);
		return socket_ops::ntoh64(out);
	}
	uint32_t peakInt32()const;
	uint16_t peakInt16()const;
	uint8_t peakInt8()const;

	//brief:readϵ�к�������peakϵ�к�������֮�ϣ���ȡ������
	//becare:��Ҫע���������ת��
	uint64_t readInt64();
	uint32_t readInt32();
	uint16_t readInt16();
	uint8_t readInt8();

	//brief:appendϵ�к������򻺳�����׷������
	//becare:��Ҫע���������ת��
	void append(const void* data, size_t length) {
		ensureCapacity(length);

		ConstInnerPtr iter = static_cast<ConstInnerPtr>(data);
		std::copy(iter, iter+ length, beginWrite());
		__write_index += length;
	}
	//brief:��Ϊһ����ת��,�ѷ���
	//void append(const void* data, size_t length) {
	//	append(static_cast<ConstInnerPtr>(data), length);
	//}
	//brief:�����������ػ�
	void appendInt64(uint64_t data);
	void appendInt32(uint32_t data);
	void appendInt16(uint16_t data);
	void appendInt8(uint8_t data) {
		append(&data, sizeof data);
	}
	void append(const string& str) {
		append(str.c_str(), str.size());
	}

	//brief:prependϵ�к������򻺳������ǰ׺����(ͨ��Ԥ��ǰ׺�ռ����)
	//becare:�û�Ӧ�ü��ǰ׺�ռ��㹻
	//      ע��������ת��
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
	
	//ͨ�ýӿ�ʵ��
	void swap(Buffer& peer) {
		__buf.swap(peer.__buf);
		std::swap(__read_index, peer.__read_index);
		std::swap(__write_index, peer.__write_index);
	}

	InnerPtr beginWrite(){
		return begin() + __write_index;
	}
	//brief:const�汾��Ϊ�����beginReadֻ��const�汾�����stl�㷨�ĵ���������������Ҫ����һ��
	ConstInnerPtr beginWrite()const {
		return begin() + __write_index;
	}
	//becare:ֻ�ṩconst�汾�����������handleֻ�ܶ�ȡdata����
	ConstInnerPtr beginRead()const{
		return begin() + __read_index;
	}
	
	//brief:ȷ���ռ��С
	void ensureCapacity(size_t len) {
		size_t idle = getIdleSize();
		if (len <= idle) {
			return;
		}

		if (getPrependableSize() + idle >= kPrependLength + len) {
			//���µ���ǰ׺�ռ�
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
	
	//becare:���½ӿ���Ϊ�˷����������⺯�����ݣ�
	//      data���ڵ�����ͨ��peakϵ�з���ʱֻ�ܽ��ж�ȡ�������޸ģ�
	//      const�汾��begin����Ϊ��ӭ�ϴ�
	InnerPtr begin() {
		return &*__buf.begin();
	}
	//brief:Ϊ��ӭ��peak��ʹ��
	ConstInnerPtr begin()const {
		return &*__buf.begin();
	}

	//�ڲ�������֯
	//std::vector<uint8_t> __buf;//����
	std::vector<InnerType> __buf;
	//becare:ʹ��������ֹ����������ָ��ʧЧ
	size_t __read_index;
	size_t __write_index;

	static const int kPrependLength;
	static const int kDefaultVectorSize;

};

//}//!namespace detail
}//!namespace core 
}//!namespace imnet


#endif
