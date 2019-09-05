/*************************************************************************
	> File Name: atomic_type.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月28日 星期三 14时27分13秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_ADVANCED_ATOMIC_TYPE_H_
#define _IMNET_DEPENDENT_ADVANCED_ATOMIC_TYPE_H_

#ifdef _MSC_VER
#include"../noncopyable.h"
#else
#include"dependent/noncopyable.h"
#endif

//#define __GNUC__ 4
//#define __GNUC_MINOR__ 8

//becare:gcc4.7以上版本使用__atomic系列builtin函数作为__sync系列builtin函数的升级
#if defined __GNUC__ && (__GNUC__>=5 || (__GNUC__==4 && __GNUC_MINOR__>=7))

//An atomic operation can both constrain code motion and be mapped to hardware instructions for synchronization between threads(e.g., a fence).To which extent this happens is
//controlled by the memory orders, which are listed here in approximately ascending order of strength.The description of each memory order is only meant to roughly illustrate 
//the effects and is not a specification; see the C++11 memory model for precise semantics.
//
//__ATOMIC_RELAXED
//Implies no inter - thread ordering constraints.
//
//__ATOMIC_CONSUME
//This is currently implemented using the stronger __ATOMIC_ACQUIRE memory order because of a deficiency in C++11’s semantics for memory_order_consume.
//
//__ATOMIC_ACQUIRE
//Creates an inter - thread happens - before constraint from the release(or stronger) semantic store to this acquire load.Can prevent hoisting of code to before the operation.
//
//__ATOMIC_RELEASE
//Creates an inter - thread happens - before constraint to acquire(or stronger) semantic loads that read from this release store.Can prevent sinking of code to after the operation.
//
//__ATOMIC_ACQ_REL
//Combines the effects of both __ATOMIC_ACQUIRE and __ATOMIC_RELEASE.
//
//__ATOMIC_SEQ_CST
//Enforces total ordering with all other __ATOMIC_SEQ_CST operations.

#define _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE

#ifdef __ATOMIC_MEMORY_USE_RELAXED_
#define _IMNET_ATOMIC_MEMORY_ORDER_ __ATOMIC_RELAXED//线程间不做同步
#else 
#define _IMNET_ATOMIC_MEMORY_ORDER_ __ATOMIC_SEQ_CST//强制线程间同步
#endif

#endif

#include<cstdint>

namespace imnet {

//brief:detail内部函数对外隐藏
namespace detail {
//brief:封装了__atomic/__sync系列函数
//      对于gcc4.7版本及以上，可以指定内存序，仔原子操作基础之上，可进行线程间同步
template<typename Ty>
class Atomictype:noncopyable {
public:
	//brief:原子性的获得当前值
	Ty load() {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		return __atomic_load_n(&__value, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		return __sync_val_compare_and_swap(&__value, 0, 0);
#endif
	}

	//brief:原子性的修改值
	void store(Ty v) {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		__atomic_store_n(&__value, v, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		__sync_lock_test_and_set(&__value,v);
#endif
	}

	//brief:原子性的改变其值，并返回修改前数值
	Ty exchange(Ty v) {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		return __atomic_exchange_n(&__value, v, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		return __sync_lock_test_and_set(&__value, v);
#endif
	}

	//brief:重载类型转换
	operator Ty() {
		return load();
	}

	//TODO:compare_exchange_weak以及compare_exchange_strong
	//封装 __atomic_compare_exchange_n
private:
	volatile Ty __value;

};

//brief:int32_t的特化
//     额外提供了部分数值计算功能
template<> class Atomictype<int32_t>:noncopyable{
public:
	using ValueType = int32_t;

	//brief:原子性后加，返回修改前值
	ValueType fetch_add(ValueType v) {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		return __atomic_fetch_add(&__value, v,_IMNET_ATOMIC_MEMORY_ORDER_);
#else
		return __sync_fetch_and_add(&__value, v);
#endif
	}

	//brief:原子性先加，返回修改后值
	ValueType add_fetch(ValueType v) {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		return __atomic_add_fetch(&__value, v,_IMNET_ATOMIC_MEMORY_ORDER_);
#else
		return __sync_add_and_fetch(&__value, v);
#endif
	}

	//brief:原子性的获得当前值
	ValueType load() {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		return __atomic_load_n(&__value, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		return __sync_val_compare_and_swap(&__value, 0, 0);
#endif
	}

	//brief:
	void store(ValueType v) {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		__atomic_store_n(&__value, v, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		__sync_lock_test_and_set(&__value,v);
#endif
	}
	//brief:原子性的改变其值，并返回修改前数值
	ValueType exchange(ValueType v) {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		return __atomic_exchange_n(&__value, v, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		return __sync_lock_test_and_set(&__value, v);
#endif
	}

	//brief:
	ValueType operator++() {
		return add_fetch(1);
	}
	ValueType operator--() {
		return add_fetch(-1);
	}
	ValueType operator++(int) {
		return fetch_add(1);
	}
	ValueType operator--(int) {
		return fetch_add(-1);
	}
	ValueType operator=(ValueType v) {
		store(v);
		return v;
	}
	operator ValueType() {
		return load();
	}

	//becare:初始化不是线程安全的，被多个线程调用的结果是未定义的
	explicit Atomictype(ValueType v=0) :__value(v) {

	}
private:
	volatile ValueType __value;
};


//brief:int64_t的特化
//     额外提供了部分数值计算功能
template<> class Atomictype<int64_t> :noncopyable {
public:
	using ValueType = int64_t;

	//brief:原子性后加，返回修改前值
	ValueType fetch_add(ValueType v) {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		return __atomic_fetch_add(&__value, v, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		return __sync_fetch_and_add(&__value, v);
#endif
	}

	//brief:原子性先加，返回修改后值
	ValueType add_fetch(ValueType v) {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		return __atomic_add_fetch(&__value, v, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		return __sync_add_and_fetch(&__value, v);
#endif
	}

	//brief:原子性的获得当前值
	ValueType load() {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		return __atomic_load_n(&__value, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		return __sync_val_compare_and_swap(&__value, 0, 0);
#endif
	}

	//brief:
	void store(ValueType v) {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		__atomic_store_n(&__value, v, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		__sync_lock_test_and_set(&__value, v);
#endif
	}
	//brief:原子性的改变其值，并返回修改前数值
	ValueType exchange(ValueType v) {
#ifdef _ATOMIC_BULITIN_FUNCTION_IS_AVAILABLE
		return __atomic_exchange_n(&__value, v, _IMNET_ATOMIC_MEMORY_ORDER_);
#else
		return __sync_lock_test_and_set(&__value, v);
#endif
	}

	//brief:
	ValueType operator++() {
		return add_fetch(1);
	}
	ValueType operator--() {
		return add_fetch(-1);
	}
	ValueType operator++(int) {
		return fetch_add(1);
	}
	ValueType operator--(int) {
		return fetch_add(-1);
	}
	ValueType operator=(ValueType v) {
		store(v);
		return v;
	}
	operator ValueType() {
		return load();
	}

	//becare:初始化不是线程安全的，被多个线程调用的结果是未定义的
	explicit Atomictype(ValueType v = 0) :__value(v) {

	}
private:
	volatile ValueType __value;
};
}//!namespace detail

using AtomicInt32 = detail::Atomictype<int32_t>;
using AtomicInt64 = detail::Atomictype<int64_t>;

}//!namespace imnet

 
 
#endif//!_IMNET_DEPENDENT_ADVANCED_ATOMIC_EXTENSION_H_

