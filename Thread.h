#pragma once

#include <string>
#include "Lock.h"
#include "Condition.h"

namespace AGBase
{
	class CThread
	{
#if defined _WINDOWS_ || defined _WINDOWS_
#define INVALID_THREAD_VALUE INVALID_HANDLE_VALUE
		typedef HANDLE				THREADID;
#else
#define INVALID_THREAD_VALUE 0
		typedef pthread_t			THREADID;
#endif
	public :
		CThread(const std::string &name="");
		virtual ~CThread( void );

		std::string	GetName() const;

		// 是否线程循环开启
		bool IsRunning( void );
		// 是否停止
		bool IsStop( void );

		// 开启线程循环
		bool Start( void );
		// 终止线程循环，ms（单位是毫秒）是等待线程退出循环，如果等待超时，则强行关闭
		void Terminate( unsigned int ms=500 );

	public :
		// 必须继承的主循环
		virtual int Run() = 0;
		// 通知关闭
		void Broadcast( void );

	private:
		// No copies do not implement
		CThread(const CThread &l);
		CThread &operator=(const CThread &l);

	protected:
		THREADID			m_hThread;
		const std::string	m_name;
		volatile bool		m_brunning;
		CLock				m_lock;
		CCondition			m_condition;
	};

}

