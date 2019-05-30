#pragma once

#include <pthread.h>

namespace AGBase
{
	// 锁的包装类，
	// 注意：
	//		windows使用CRITICAL_SECTION，是一个可重入锁
	//		linux使用pthread_mutex_t，默认参数是一个不可重入锁
	class CLock
	{
		friend class CCondition;
	public:
		explicit CLock( void );
		~CLock( void );

		// 加锁
		void Lock( void );
		// 解锁
		void Unlock( void );

	private:
	#if defined _WINDOWS_ || defined WIN32
		CRITICAL_SECTION	m_crit;
	#else
		pthread_mutex_t		m_crit;
	#endif

		// No copies do not implement
		CLock(const CLock &l);
		CLock &operator=(const CLock &l);
	};

	// 简化的CLock包装类，使用时直接定义即可
	class CSelfLock
	{
	public:
		explicit CSelfLock(CLock &lock);
		~CSelfLock( void );

	private :
		CLock &m_lock;

		// No copies do not implement
		CSelfLock(const CSelfLock &l);
		CSelfLock &operator=(const CSelfLock &l);
	};
}
