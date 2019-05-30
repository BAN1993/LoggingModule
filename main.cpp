#include <cstdio>
#include <errno.h>
#include <iostream>
#include <sys/time.h>
#include <deque>
#include <vector>
#include <string>
#include <malloc.h>
#include <string.h>
#include <fstream>

#include "Thread.h"
#include "Lock.h"

using namespace std;
using namespace AGBase;

#define LOG(os) cout<<os<<endl;

bool doPopen(const char* cmdline, std::vector<string>& retVec)
{
        FILE *file = popen(cmdline, "r");
        char line[128]={0};
        string strline;
        if(NULL==file)
        {
                LOG("file is NULL");
                return false;
        }
        while(fgets(line, 128, file) != NULL)
        {
                strline = line;
                retVec.push_back(line);
        }
        pclose(file);
        return true;
}

bool getCpuInfo(int& pid, float& cpu, int& rss)
{
        pid=getpid();
        char cmdline[100]={0};
        sprintf(cmdline,"ps -o %%cpu,rss -p %d 2>&1",pid);
        vector<string> vec;
        if(!doPopen(cmdline,vec))
        {
                LOG("can not get cpuinfo");
                return false;
        }
        sscanf(vec[1].c_str(), "%f %d", &cpu, &rss);
        return true;
}

int getRSS()
{
        int pid=0;
        float cpu=0.0;
        int rss=0;
        if(!getCpuInfo(pid,cpu,rss))
        {
                return 0;
        }
        return rss;
}

unsigned long long getTimeStampMsec()
{
#if defined _WINDOWS || defined WIN32 || defined WIN64
	struct timeval tv;
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tv.tv_sec = clock;
	tv.tv_usec = wtm.wMilliseconds * 1000;
	return ((unsigned long long)tv.tv_sec * 1000 + (unsigned long long)tv.tv_usec / 1000);
#else  
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((unsigned long long)tv.tv_sec * 1000 + (unsigned long long)tv.tv_usec / 1000);
#endif 
}

const static string gStr = "[XX:XX:XX][I][XXXXX]:[XXXXX][../XXXXXXX/XXXXXXXXXXXXX/XXXXXXXXXXXXXXXXXXX.cpp.XXX]"
							//"POSTERS:pool=0x1c7ce10,appid=40073,type=42,subtype=40073,postkey=1001,connect=0x1c7f4c0,timeout=2000"
							"POSTERS:pool=0x1c7ce10,appid=40073,type=42,subtype=40073,postkey=1001,connect=0x1c7f4c0,timeout=2000";
							
static unsigned long long writeTime = 0;
static int writeCount = 0;
static int dropCount = 0; 
static unsigned long long waitPushLockTime = 0;
static unsigned long long waitWriteLockTime = 0;
static unsigned long long waitEndLockTime = 0;

class AutoWaitLockTime
{
public:
	unsigned long long _time;
	unsigned long long *_p;
	AutoWaitLockTime(unsigned long long *t)
	{
		_p = t;
		_time = getTimeStampMsec();
	}
	~AutoWaitLockTime()
	{
		*_p += (getTimeStampMsec()-_time);
	}
};
				
class FileC
{
public:
	void open(const char *filename)
	{
		_fp = fopen(filename,"w");
	}
	
	void write(const char *buf)
	{
		fwrite(buf,strlen(buf),1,_fp);
	}
	
	void close()
	{
		fclose(_fp);
	}

	FILE *_fp;
};
		
class FileCpp
{
public:
	void open(const char *filename)
	{
		_fp.open(filename, ios::out| ios::app);
	}
	
	void write(const char *buf)
	{
		_fp<<buf;
	}
	
	void close()
	{
		_fp.close();
	}

	fstream _fp;
};
		
class LogBase : public CThread
{
public:
	virtual void push(const char *str) = 0;
	virtual int Run(void) = 0;
	virtual bool hadData(void) = 0;
};
							
class LogNew : public LogBase
{
public:
	const static unsigned int MAX_LOG_LEN = 1024;
	const static unsigned int MAX_LOG_COUNT = 2048;
	
	struct LogPacket
	{
		char data[1024];
		LogPacket(){memset(data,0,sizeof(data));}
	};
	
	LogNew()
	{
		m_fp.open("testNew.log");
	}
	
	~LogNew()
	{
		m_fp.close();
	}
	
	void push(const char *str)
	{
		LogPacket *packet = new LogPacket();
		memcpy(packet->data, str, strlen(str));
		packet->data[strlen(str)] = 0;
		
		{
			AutoWaitLockTime a(&waitPushLockTime);
			CSelfLock l(_lock);
			if(m_packets.size() >= MAX_LOG_COUNT)
			{
				dropCount++;
				delete packet;
				return;
			}
			m_packets.push_back(packet);
		}
	}
	
	int Run(void)
	{
		LogPacket *packet = 0;
		while (IsRunning())
		{
			bool write = true;
			unsigned long long begin = getTimeStampMsec	();
			{
				AutoWaitLockTime a(&waitWriteLockTime);
				CSelfLock l(_lock);
				if(!m_packets.empty())
				{
					packet = m_packets.front();
					m_packets.pop_front();
				}
			}
			if(packet==0)
			{
				usleep(200);
				write = false;
			}
			else
			{
				LogFile(packet->data);
				delete packet;
				packet = 0;
			}
			if(write)
			{
				writeTime += (getTimeStampMsec()-begin);
				writeCount++;
			}
		}
		return 0;
	}
	
	void LogFile(const char* str)
	{
		m_fp.write(str);
	}
	
	bool hadData(void)
	{
		AutoWaitLockTime a(&waitEndLockTime);
		CSelfLock l(_lock);
		return m_packets.size() > 0 ? true : false;
	}
	
	CLock _lock;
	deque<LogPacket*> m_packets;
	FileC m_fp;
};

class LogBuffer : public LogBase
{
public:
	const static int MAX_LOG_BUFFER_SIZE = 614400;

	LogBuffer() : m_pLogCur(m_data)
	{
		m_fp.open("testBuffer.log");
	}
	
	~LogBuffer()
	{
		m_fp.close();
	}
	
	int length()
	{
		return m_pLogCur - m_data;
	}
	
	void push(const char *str)
	{
		if((length()+strlen(str))>(MAX_LOG_BUFFER_SIZE-200))
		{
			dropCount++;
			return;
		}
		// 代码里有通过string转换一次,所以这里也要用string
		string logstr;
		logstr = str;
		logstr += "\n";
		
		{
			AutoWaitLockTime a(&waitPushLockTime);
			CSelfLock l(_lock);
			memcpy(m_pLogCur, logstr.c_str(), logstr.length());
			m_pLogCur += logstr.length();
		}
	}
	
	int Run(void)
	{
		while (IsRunning())
		{
			bool write = true;
			unsigned long long begin = getTimeStampMsec	();
			int use = 0;
			{
				//AutoWaitLockTime a(&waitWriteLockTime);
				//CSelfLock l(_lock); // 这里先不加锁
				use = length();
			}
			if(use <= 0)
			{
				usleep(200);
				write = false;
			}
			else
			{
				LogFile();
			}
			if(write)
			{
				writeTime += (getTimeStampMsec()-begin);
				writeCount++;
			}
		}
		return 0;
	}
	
	void LogFile()
	{
		int tmpLine = 0;
		{
			AutoWaitLockTime a(&waitWriteLockTime);
			CSelfLock l(_lock);
			tmpLine = length();
			memcpy(m_tmpData,m_data,tmpLine);
			m_pLogCur = m_data;
		}
		m_tmpData[tmpLine]=0;
		m_fp.write(m_tmpData);
	}
	
	bool hadData(void)
	{
		AutoWaitLockTime a(&waitEndLockTime);
		CSelfLock l(_lock);
		return length() > 0 ? true : false;
	}
	
	CLock _lock;
	char m_data[MAX_LOG_BUFFER_SIZE];
	char *m_pLogCur;
	char m_tmpData[MAX_LOG_BUFFER_SIZE];
	FileC m_fp;
};

int main(int argc, char* argv[])
{	
	//LOG("---------------------------------------------------")
	//LOG("- Test Log File")
	//LOG("- ARG1 0-LogNew(default) 1-LogBuffer");
	int choose = 0;
	if(argc > 1)
	{
		choose = atoi(argv[1]);
	}
	//LOG("- Now Choose "<< choose);
	//LOG("---------------------------------------------------")
	
	LogBase * log;
	if(choose == 0)
		log = new LogNew();
	else
		log = new LogBuffer();
	log->Start();
	
	unsigned long long pushTime = 0;
	unsigned long long runTime = 0;
	
	const static int allTimes = 1000000;
	int nowTimes=0;
	int startRss=getRSS();
	
	unsigned long long runBegin = getTimeStampMsec();
	while(nowTimes++<allTimes)
	{
		// 每运行 allTimes/10次 获取一下内存
		if(nowTimes % (allTimes/10) == 0)
			LOG("- nowTimes="<<nowTimes<<",rss="<<getRSS());
		
		unsigned long long tmpbegin = getTimeStampMsec();
		log->push(gStr.c_str());
		pushTime += (getTimeStampMsec()-tmpbegin);
		usleep(10);
	}
	while(log->hadData())
	{
		usleep(10);
	}
	
	log->Terminate();
	delete log;
	
	LOG("---------------------------------------------------")
	runTime = (getTimeStampMsec()-runBegin);
	if(choose==0)
	{
		LOG("- Test LogNew");
	}
	else
	{
		LOG("- Test LogBuffer");
	}
	LOG("- Test end,beginRss="<<startRss<<",endRss="<<getRSS());
	LOG("- pushCount="<<nowTimes<<",pushTime="<<pushTime);
	LOG("- writeTime="<<writeTime<<",writeCount="<<writeCount);
	LOG("- runTime="<<runTime<<",dropCount="<<dropCount);
	LOG("- waitPushLockTime="<<waitPushLockTime<<",waitWriteLockTime="<<waitWriteLockTime<<",waitEndLockTime="<<waitEndLockTime);
	LOG("---------------------------------------------------")
	
	return 0;
}


