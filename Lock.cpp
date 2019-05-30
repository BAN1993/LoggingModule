#include "Define.h"
#include "Lock.h"

namespace AGBase
{

CLock::CLock( void )
{
#if defined _WINDOWS_ || defined _WINDOWS_
	::InitializeCriticalSection(&m_crit);
#else
	int ret = ::pthread_mutex_init(&m_crit, 0);
	assert(ret==0);
#endif	
};

CLock::~CLock( void )
{
#if defined _WINDOWS_ || defined _WINDOWS_
	::DeleteCriticalSection(&m_crit);
#else
	int ret = ::pthread_mutex_destroy(&m_crit);

	assert( ret != EBUSY );
	assert( ret == 0 );
#endif
}

void CLock::Lock( void )
{
#if defined _WINDOWS_ || defined _WINDOWS_
	::EnterCriticalSection(&m_crit);
#else
	int rc = ::pthread_mutex_lock(&m_crit);

    assert( rc != EINVAL );
    assert( rc != EDEADLK );
    assert( rc == 0 );
#endif
}

void CLock::Unlock( void )
{
#if defined _WINDOWS_ || defined _WINDOWS_
	::LeaveCriticalSection(&m_crit);
#else
	int rc = ::pthread_mutex_unlock(&m_crit);

    assert( rc != EINVAL );
    assert( rc != EPERM );
    assert( rc == 0 );
#endif
};

CSelfLock::CSelfLock(CLock &lock) : m_lock(lock)
{
	m_lock.Lock();
}

CSelfLock::~CSelfLock( void )
{
	m_lock.Unlock(); 
}

}

