#pragma once

// Windows版本
//	两个线程不可以等待一个handler，会产生Wait失败，也就是Windows下不需要Broadcast
// Linux下可以
//  


namespace AGBase
{

	class CLock;

	class CCondition
	{

#if defined _WINDOWS_ || defined WIN32
	typedef HANDLE			condid;
#else
	typedef pthread_cond_t	condid;
#endif

	public:
		explicit  CCondition( void );
		~CCondition( void );

		// 等待信号到达
		// ms为超时时间，单位为毫秒
		bool Wait (CLock& locker);
		bool Wait (CLock& locker, unsigned int ms);

		// 发送信号条件改变
		// Signal-〉唤醒一个线程，Broadcast-〉唤醒所有线程
		void Signal( void );
		void Broadcast( void );

	private:
		condid	m_condid;

	private:
		CCondition (const CCondition&);
		CCondition& operator= (const CCondition&);

	};

};
