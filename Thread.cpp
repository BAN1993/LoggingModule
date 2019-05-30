#include "Define.h"
#include "Thread.h"

#ifdef __GNUC__
#ifdef USEGPROF

	#include <dlfcn.h>

	static void * wrapper_routine(void *);  

	/* Original pthread function */ 
	typedef int (*pthread_create_fun)(pthread_t *__restrict, __const pthread_attr_t *__restrict, void *(*)(void *), void *__restrict);

	static pthread_create_fun pthread_create_orig = NULL;  

	/* Library initialization function */ 
	void wooinit(void) __attribute__((constructor));  

	void wooinit(void)
	{ 
		pthread_create_orig = (pthread_create_fun)dlsym(RTLD_NEXT, "pthread_create");
		LOG("BASE pthreads: using profiling hooks for gprof:");
		if(pthread_create_orig == NULL)
		{ 
			char *error = dlerror(); 
			if(error == NULL) 
			{ 
				error = "pthread_create is NULL";
			}
			LOG("BASE "<<error":");
			exit(EXIT_FAILURE);
		} 
	}  

	/* Our data structure passed to the wrapper */ 
	typedef struct wrapper_s 
	{ 
		void * (*start_routine)(void *); 
		void * arg;  
		pthread_mutex_t lock; 
		pthread_cond_t  wait;  
		struct itimerval itimer;  
	} wrapper_t; 

	/* The wrapper function in charge for setting the itimer value */ 
	static void * wrapper_routine(void * data)
	{ 
		/* Put user data in thread-local variables */ 
		void * (*start_routine)(void *) = ((wrapper_t*)data)->start_routine;
		void * arg = ((wrapper_t*)data)->arg;  
		
		/* Set the profile timer value */ 
		setitimer(ITIMER_PROF, &((wrapper_t*)data)->itimer, NULL);  
		/* Tell the calling thread that we don't need its data anymore */ 
		pthread_mutex_lock(&((wrapper_t*)data)->lock); 
		pthread_cond_signal(&((wrapper_t*)data)->wait);
		pthread_mutex_unlock(&((wrapper_t*)data)->lock); 

		/* Call the real function */
		return start_routine(arg); 
	} 

	/* Our wrapper function for the real pthread_create() */ 
	int pthread_create(pthread_t *__restrict thread, __const pthread_attr_t *__restrict attr, void * (*start_routine)(void *), void *__restrict arg)
	{ 
		wrapper_t wrapper_data; 
		int i_return; 
		
		/* Initialize the wrapper structure */ 
		wrapper_data.start_routine = start_routine; 
		wrapper_data.arg = arg; 
		
		getitimer(ITIMER_PROF, &wrapper_data.itimer);
		pthread_cond_init(&wrapper_data.wait, NULL); 
		pthread_mutex_init(&wrapper_data.lock, NULL); 
		pthread_mutex_lock(&wrapper_data.lock); 
		
		/* The real pthread_create call */ 
		i_return = pthread_create_orig(thread, attr, &wrapper_routine, &wrapper_data);  
		
		/* If the thread was successfully spawned, wait for the data * to be released */
		if(i_return == 0) 
		{ 
			pthread_cond_wait(&wrapper_data.wait, &wrapper_data.lock);
		} 
		
		pthread_mutex_unlock(&wrapper_data.lock); 
		pthread_mutex_destroy(&wrapper_data.lock); 
		pthread_cond_destroy(&wrapper_data.wait);  
		
		return i_return;
	} 

#endif
#endif

namespace AGBase
{

#if defined _WINDOWS_ || defined WIN32
	static unsigned int __stdcall ThreadFunc(void *pval)
	{
		CThread* pThis = static_cast< CThread* >(pval);
		assert(pThis);

		std::string threadname = pThis->GetName();
		LOG("BASE thread is running:threadname="<<threadname);

		pThis->Run( );
		pThis->Broadcast( );

		::_endthreadex( 0 );

		return 0;
	}
#else
	extern "C"
	{
		static void* ThreadFunc(void *pval)
		{
			CThread* pThis = static_cast< CThread* >(pval);
			assert(pThis);

			std::string threadname = pThis->GetName();

			pThis->Run( );
			pThis->Broadcast( );

			::pthread_exit(NULL);

			return 0;
		}
	}
#endif


CThread::CThread(const std::string &name/*=""*/)
: m_hThread(INVALID_THREAD_VALUE)
, m_name(name)
, m_brunning(false)
{

}

CThread::~CThread( void )
{
	Terminate( 1000 );
}

std::string	CThread::GetName() const
{
	return m_name; 
}

bool CThread::IsRunning( void )
{
	//CSelfLock sl(m_lock);
	return m_brunning;
}

bool CThread::IsStop( void )
{
	//CSelfLock sl(m_lock);
	return !m_brunning;
}
      
bool CThread::Start( void )
{
	CSelfLock l(m_lock);
	if ( m_hThread != INVALID_THREAD_VALUE || m_brunning )
	{
		LOG("BASE failed to start:threadname="<<m_name<<",m_brunning="<<m_brunning);
		return false;
	}

	m_brunning = true;
	int err = 0;

#if defined _WINDOWS_ || defined WIN32
	unsigned int threadID = 0;
	m_hThread = (HANDLE)_beginthreadex(0, 0, ThreadFunc, (void*)this, 0, &threadID);
	if ( m_hThread == INVALID_HANDLE_VALUE )
	{
		err = GetLastError();

		LOG("BASE create failed:threadname="<<m_name<<",err="<<err);

		m_brunning = false;

		return false;
	}

	return true;
#else
	int retval = pthread_create( &m_hThread, 0, ThreadFunc, (void*)this);
	if ( retval != 0 )
	{
		err = retval;

		LOG("BASE create failed:threadname="<<m_name<<",err="<<err);

		m_brunning = false;

		return false;
	}

	pthread_detach( m_hThread );

	return true;
#endif
}

void CThread::Terminate( unsigned int ms )
{
	CSelfLock l(m_lock);

	if ( m_brunning )
	{
		m_brunning  = false;
		if( !m_condition.Wait(m_lock, ms) )
		{
			if ( m_hThread!=INVALID_THREAD_VALUE )
			{
//#if defined _WINDOWS_ || defined WIN32
//				if ( GetCurrentThread() == m_hThread )	::_endthreadex( 0 );
//				else									::TerminateThread(m_hThread, 0);
//#else
//				if ( pthread_self() == m_hThread )		::pthread_exit(NULL);
//				else									::pthread_cancel(m_hThread);
//#endif
			}
		}

#if defined _WINDOWS_ || defined WIN32
		if ( m_hThread!=INVALID_THREAD_VALUE )			::CloseHandle(m_hThread);
#endif
		m_hThread = INVALID_THREAD_VALUE;
	}
}

void CThread::Broadcast( void )
{
	CSelfLock l(m_lock);
	m_condition.Broadcast( );
}

}
