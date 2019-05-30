#pragma once

#include <string>

namespace AGBase
{
	// 通用功能类和函数
	// 获得程序目录
	extern std::string	GetModulePath( void );
	// 创建新目录
	extern bool			CreateDir(const char * path);

	// 获得IP地址的字符串、port
	extern short		GetPeerPort(int socketid);
	extern std::string	GetPeerStringIP(int socketid);
	extern long			GetPeerLongIP(int socketid);
	extern short		GetPort(const struct sockaddr_in &addr);
	extern std::string	GetIP(const struct sockaddr_in &addr);
	extern bool			PingTest(const char *ips, int timeout);

	// sleep, 毫秒
	extern void			SleepMillisecond(int millsec);
	// 获得系统时间，以毫秒表示
	extern unsigned long long GetTimeMs( void );
	// 获得cpu开机以来的时钟脉冲次数, 秒级别
	extern unsigned long GetCPUTickCount( void );

	//time_t -> tm 多线程安全
	//time_t *time_input:输入的time_t类型的时间
	//struct tm* tm_result:输出的tm结构的时间
	extern int			time_to_tm(time_t *time_input,struct tm* tm_result);
	//tm -> time_t 多线程安全
	extern int			tm_to_time(struct tm* tm_input, time_t *time_result);
	// 时间字符串
	extern std::string	GetTimeString( void );

	// 若字节为非可显示字符, 则转化成16进制输出
	extern std::string MemoryToString(const void *memory, int size);

	// Converts all the characters in this string to lowercase characters
	extern std::string Lower(const std::string &str);
	// Converts all the characters in this string to uppercase characters
	extern std::string Upper(const std::string &str);
	// Trims all leading and trailing whitespace characters from the string
	extern std::string Trim(const std::string &str);
	// Trims leading whitespace characters from the string
	extern std::string TrimLeft(const std::string &str);
	// Trims trailing whitespace characters from the string
	extern std::string TrimRight(const std::string &str);
}

