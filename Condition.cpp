#include "Define.h"
#include "Lock.h"
#include "Condition.h"
#include "Utils.h"

namespace AGBase
{

#if defined _WINDOWS_ || defined WIN32

CCondition::CCondition( void )
{
	m_condid = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	assert(m_condid != INVALID_HANDLE_VALUE);
}

CCondition::~CCondition( void )
{
	if ( m_condid!=INVALID_HANDLE_VALUE )
		::CloseHandle( m_condid );
}

bool CCondition::Wait (CLock& locker)
{
	bool ret = false;
	locker.Unlock();
	if( ::WaitForSingleObject( m_condid, INFINITE )==WAIT_OBJECT_0 )
		ret = true;
	locker.Lock();

	::ResetEvent( m_condid );

	return ret;
}

bool CCondition::Wait (CLock& locker, unsigned int ms)
{
	bool ret = false;
	locker.Unlock();
	if( ::WaitForSingleObject( m_condid, ms )==WAIT_OBJECT_0 )
		ret = true;
	locker.Lock();

	::ResetEvent( m_condid );

	return ret;
}

void CCondition::Signal( void )
{
	::SetEvent( m_condid );
}

void CCondition::Broadcast( void )
{
	::SetEvent( m_condid );
}

#else

CCondition::CCondition( void )
{
	int  rc = ::pthread_cond_init(&m_condid,0);
	assert( rc == 0 );
}

CCondition::~CCondition ( void )
{
	::pthread_cond_destroy(&m_condid);
}

bool CCondition::Wait (CLock& locker)
{
	return ::pthread_cond_wait(&m_condid, &locker.m_crit)==0;
}

bool CCondition::Wait(CLock& locker, unsigned int ms)
{
	unsigned long long expires64 = GetTimeMs() + ms;
	timespec expiresTS;
	expiresTS.tv_sec = expires64 / 1000;
	expiresTS.tv_nsec = (expires64 % 1000) * 1000000L;

	return ::pthread_cond_timedwait(&m_condid, &locker.m_crit, &expiresTS)==0;
}

void CCondition::Signal ( void )
{
	::pthread_cond_signal(&m_condid);
}

void CCondition::Broadcast( void )
{
	::pthread_cond_broadcast(&m_condid);
}

#endif

}
