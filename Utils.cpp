#if _MSC_VER>=1400
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <iostream>

#include "Define.h"
#include "Utils.h"

#if defined _WINDOWS_ || defined WIN32
#include <ws2tcpip.h>
#else
#include <netinet/ip_icmp.h>
#include <sys/sysinfo.h>
#endif


namespace AGBase
{

std::string GetModulePath()
{
#if defined _WINDOWS_ || defined WIN32

	char szFile[MAX_PATH];
	GetModuleFileNameA(GetModuleHandle(NULL), szFile, MAX_PATH);

	char szDrive[MAX_PATH], szPath[MAX_PATH];

	_splitpath(szFile, szDrive, szPath, NULL, NULL);
	strncat(szDrive, szPath, sizeof(szDrive)-strlen(szDrive)-1);

	std::string ret = szDrive;
	if ( ret.find_last_of('\\') != ret.length()-1 && ret.find_first_of('/') != ret.length()-1 )
	{
		ret.append("\\");
	}

	return ret;

#else

	char dir[1024] = {0};
	int count = readlink( "/proc/self/exe", dir, 1024 );
	if ( count < 0 || count >= 1024 )
		return "./";
	else
		dir[count] = 0;
	std::string ret = dir;
	std::string::size_type pos = ret.find_last_of('/');

	if( pos == std::string::npos ) return ret;

	return ret.substr(0, pos+1);

#endif
};

bool CreateDir(const char * path)
{
#if defined _WINDOWS_ || defined WIN32

	if( PathFileExistsA( path ) ) return true;
	return (CreateDirectoryA( path, NULL )==TRUE);

#else

	mkdir(path, 0755);
	return true;

#endif
}

short GetPeerPort(int socketid)
{
	if ( socketid == INVALID_SOCKET )
		return 0;

	struct sockaddr_in addr;
#if defined _WINDOWS_ || defined WIN32
	int namelen = sizeof(addr);
#else
	socklen_t namelen = sizeof(addr);
#endif
	memset(&addr, 0, sizeof(addr));
	getpeername(socketid, (sockaddr*)&addr, &namelen);

	return ntohs(addr.sin_port);
}

std::string GetPeerStringIP(int socketid)
{
	if ( socketid == INVALID_SOCKET )
		return "0.0.0.0";

	struct sockaddr_in addr;
#if defined _WINDOWS_ || defined WIN32
	int namelen = sizeof(addr);
#else
	socklen_t namelen = sizeof(addr);
#endif
	memset(&addr, 0, sizeof(addr));
	getpeername(socketid, (sockaddr*)&addr, &namelen);

	char* szaddr = inet_ntoa(addr.sin_addr);
	if ( szaddr == NULL )
		return "0.0.0.0";

	return (std::string)szaddr;
}

long GetPeerLongIP(int socketid)
{
	if ( socketid == INVALID_SOCKET )
		return 0;

	struct sockaddr_in addr;
#if defined _WINDOWS_ || defined WIN32
	int namelen = sizeof(addr);
#else
	socklen_t namelen = sizeof(addr);
#endif
	memset(&addr, 0, sizeof(addr));
	getpeername(socketid, (sockaddr*)&addr, &namelen);

	return addr.sin_addr.s_addr;
}

short GetPort(const struct sockaddr_in &addr)
{
	//return ntohs((*(sockaddr_in*)&addr).sin_port);
	return ntohs(addr.sin_port);
}

std::string GetIP(const struct sockaddr_in &addr)
{
	//char* szaddr = inet_ntoa((*(sockaddr_in*)&addr).sin_addr);
	char* szaddr = inet_ntoa(addr.sin_addr);
	if ( szaddr == NULL )
		return "0.0.0.0";

	return (std::string)szaddr;
};


#if defined _WINDOWS_ || defined WIN32
// ICMP packet types
#define ICMP_ECHO_REPLY 0
#define ICMP_DEST_UNREACH 3
#define ICMP_TTL_EXPIRE 11
#define ICMP_ECHO_REQUEST 8

// Minimum ICMP packet size, in bytes
#define ICMP_MIN 8

#ifdef _MSC_VER
// The following two structures need to be packed tightly, but unlike
// Borland C++, Microsoft C++ does not do this by default.
#pragma pack(1)
#endif

// The IP header
struct IPHeader {
	BYTE h_len:4;           // Length of the header in dwords
	BYTE version:4;         // Version of IP
	BYTE tos;               // Type of service
	USHORT total_len;       // Length of the packet in dwords
	USHORT ident;           // unique identifier
	USHORT flags;           // Flags
	BYTE ttl;               // Time to live
	BYTE proto;             // Protocol number (TCP, UDP etc)
	USHORT checksum;        // IP checksum
	ULONG source_ip;
	ULONG dest_ip;
};

// ICMP header
struct ICMPHeader {
	BYTE type;          // ICMP packet type
	BYTE code;          // Type sub code
	USHORT checksum;
	USHORT id;
	USHORT seq;
	ULONG timestamp;    // not part of ICMP, but we need it
};

#ifdef _MSC_VER
#pragma pack()
#endif

extern int setup_for_ping(const char* host, int ttl, SOCKET& sd, 
						  sockaddr_in& dest);
extern int send_ping(SOCKET sd, const sockaddr_in& dest, 
					 ICMPHeader* send_buf, int packet_size);
extern int recv_ping(SOCKET sd, sockaddr_in& source, IPHeader* recv_buf,
					 int packet_size);
extern int decode_reply(IPHeader* reply, int bytes, sockaddr_in* from);
extern void init_ping_packet(ICMPHeader* icmp_hdr, int packet_size, 
							 int seq_no);


USHORT ip_checksum(USHORT* buffer, int size) 
{
	unsigned long cksum = 0;

	// Sum all the words together, adding the final byte if size is odd
	while (size > 1) {
		cksum += *buffer++;
		size -= sizeof(USHORT);
	}
	if (size) {
		cksum += *(UCHAR*)buffer;
	}

	// Do a little shuffling
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);

	// Return the bitwise complement of the resulting mishmash
	return (USHORT)(~cksum);
}


//////////////////////////// setup_for_ping ////////////////////////////
// Creates the Winsock structures necessary for sending and recieving
// ping packets.  host can be either a dotted-quad IP address, or a
// host name.  ttl is the time to live (a.k.a. number of hops) for the
// packet.  The other two parameters are outputs from the function.
// Returns < 0 for failure.

int setup_for_ping( const char* host, int ttl, SOCKET& sd, sockaddr_in& dest)
{
	// Create the socket
	sd = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, 0, 0, 0);
	if (sd == INVALID_SOCKET) {
		LOG("BASE failed to create raw socket:err="<<WSAGetLastError());
		return -1;
	}

	if (setsockopt(sd, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl)) == SOCKET_ERROR) 
	{
		LOG("BASE TTL setsockopt failed:err="<<WSAGetLastError());
		return -1;
	}

	// Initialize the destination host info block
	memset(&dest, 0, sizeof(dest));

	// Turn first passed parameter into an IP address to ping
	unsigned int addr = inet_addr(host);
	if (addr != INADDR_NONE) {
		// It was a dotted quad number, so save result
		dest.sin_addr.s_addr = addr;
		dest.sin_family = AF_INET;
	}
	else {
		// Not in dotted quad form, so try and look it up
		hostent* hp = gethostbyname(host);
		if (hp != 0) {
			// Found an address for that host, so save it
			memcpy(&(dest.sin_addr), hp->h_addr, hp->h_length);
			dest.sin_family = hp->h_addrtype;
		}
		else {
			// Not a recognized hostname either!
			LOG("BASE failed to resolve:host="<<host);
			return -1;
		}
	}

	return 0;
}



/////////////////////////// init_ping_packet ///////////////////////////
// Fill in the fields and data area of an ICMP packet, making it 
// packet_size bytes by padding it with a byte pattern, and giving it
// the given sequence number.  That completes the packet, so we also
// calculate the checksum for the packet and place it in the appropriate
// field.

void init_ping_packet(ICMPHeader* icmp_hdr, int packet_size, int seq_no)
{
	// Set up the packet's fields
	icmp_hdr->type = ICMP_ECHO_REQUEST;
	icmp_hdr->code = 0;
	icmp_hdr->checksum = 0;
	icmp_hdr->id = (USHORT)GetCurrentProcessId();
	icmp_hdr->seq = seq_no;
	icmp_hdr->timestamp = GetTickCount();

	// "You're dead meat now, packet!"
	const unsigned long int deadmeat = 0xDEADBEEF;
	char* datapart = (char*)icmp_hdr + sizeof(ICMPHeader);
	int bytes_left = packet_size - sizeof(ICMPHeader);
	while (bytes_left > 0) {
		memcpy(datapart, &deadmeat, min(int(sizeof(deadmeat)), 
			bytes_left));
		bytes_left -= sizeof(deadmeat);
		datapart += sizeof(deadmeat);
	}

	// Calculate a checksum on the result
	icmp_hdr->checksum = ip_checksum((USHORT*)icmp_hdr, packet_size);
}


/////////////////////////////// send_ping //////////////////////////////
// Send an ICMP echo ("ping") packet to host dest by way of sd with
// packet_size bytes.  packet_size is the total size of the ping packet
// to send, including the ICMP header and the payload area; it is not
// checked for sanity, so make sure that it's at least 
// sizeof(ICMPHeader) bytes, and that send_buf points to at least
// packet_size bytes.  Returns < 0 for failure.

int send_ping(SOCKET sd, const sockaddr_in& dest, ICMPHeader* send_buf,
			  int packet_size)
{
	// Send the ping packet in send_buf as-is
	LOG("BASE Sending...:bytes="<<packet_size<<",destaddr="<<inet_ntoa(dest.sin_addr));
	int bwrote = sendto(sd, (char*)send_buf, packet_size, 0, 
		(sockaddr*)&dest, sizeof(dest));
	if (bwrote == SOCKET_ERROR) {
		LOG("BASE send failed:err="<<WSAGetLastError());
		return -1;
	}
	else if (bwrote < packet_size) {
		LOG("BASE sent:bytes="<<bwrote);
	}

	return 0;
}


/////////////////////////////// recv_ping //////////////////////////////
// Receive a ping reply on sd into recv_buf, and stores address info
// for sender in source.  On failure, returns < 0, 0 otherwise.  
// 
// Note that recv_buf must be larger than send_buf (passed to send_ping)
// because the incoming packet has the IP header attached.  It can also 
// have IP options set, so it is not sufficient to make it 
// sizeof(send_buf) + sizeof(IPHeader).  We suggest just making it
// fairly large and not worrying about wasting space.

int recv_ping(SOCKET sd, sockaddr_in& source, IPHeader* recv_buf, 
			  int packet_size)
{
	// Wait for the ping reply
	int fromlen = sizeof(source);
	int bread = recvfrom(sd, (char*)recv_buf, 
		packet_size /*+ sizeof(IPHeader)*/, 0,
		(sockaddr*)&source, &fromlen);
	if (bread == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEMSGSIZE) {
			LOG("BASE read failed:buffer too small]");
		}
		else {
			LOG("BASE read failed:err="<<WSAGetLastError());
		}
		return -1;
	}

	return 0;
}


///////////////////////////// decode_reply /////////////////////////////
// Decode and output details about an ICMP reply packet.  Returns -1
// on failure, -2 on "try again" and 0 on success.

int decode_reply(IPHeader* reply, int bytes, sockaddr_in* from) 
{
	// Skip ahead to the ICMP header within the IP packet
	unsigned short header_len = reply->h_len * 4;
	ICMPHeader* icmphdr = (ICMPHeader*)((char*)reply + header_len);

	// Make sure the reply is sane
	if (bytes < header_len + ICMP_MIN) {
		LOG("BASE too few bytes:from="<<inet_ntoa(from->sin_addr));
		return -1;
	}
	else if (icmphdr->type != ICMP_ECHO_REPLY) {
		if (icmphdr->type != ICMP_TTL_EXPIRE) {
			if (icmphdr->type == ICMP_DEST_UNREACH) {
				LOG("BASE destination unreachable:");
			}
			else {
				LOG("BASE unknown ICMP packet type received:type="<<int(icmphdr->type));
			}
			return -1;
		}
		// If "TTL expired", fall through.  Next test will fail if we
		// try it, so we need a way past it.
	}
	else if (icmphdr->id != (USHORT)GetCurrentProcessId()) {
		// Must be a reply for another pinger running locally, so just
		// ignore it.
		return -2;
	}

	// Figure out how far the packet travelled
	int nHops = int(256 - reply->ttl);
	if (nHops == 192) { 
		// TTL came back 64, so ping was probably to a host on the
		// LAN -- call it a single hop.
		nHops = 1;
	}
	else if (nHops == 128) {
		// Probably localhost
		nHops = 0;
	}

	// Okay, we ran the gamut, so the packet must be legal -- dump it
	//std::cout << bytes << " bytes from " << 
	//	inet_ntoa(from->sin_addr) << ", icmp_seq " << 		
	//	icmphdr->seq << ", ";
	if (icmphdr->type == ICMP_TTL_EXPIRE) {
		LOG("BASE TTL expired:bytes="<<bytes<<",from="<<inet_ntoa(from->sin_addr)<<",icmp_seq="<<icmphdr->seq);
	}
	else {
		LOG("BASE:hop="<<nHops<<(nHops == 1 ? "" : "s")<<",time="<<(GetTickCount() - icmphdr->timestamp)<<"ms.");
	}

	return 0;
}

/////////////////////////// allocate_buffers ///////////////////////////
// Allocates send and receive buffers.  Returns < 0 for failure.

int allocate_buffers(ICMPHeader*& send_buf, IPHeader*& recv_buf,
					 int packet_size)
{
	// First the send buffer
	send_buf = (ICMPHeader*)new char[packet_size];  
	if (send_buf == 0) {
		LOG("BASE failed to allocate output buffer:");
		return -1;
	}

	// And then the receive buffer
	recv_buf = (IPHeader*)new char[4096];
	if (recv_buf == 0) {
		LOG("BASE failed to allocate output buffer:");
		return -1;
	}

	return 0;
}

bool PingTest(const char *ips, int timeout)
{
	// Init some variables at top, so they aren't skipped by the
	// cleanup routines.
	bool pingret = false;
	int seq_no = 0;
	ICMPHeader* send_buf = 0;
	IPHeader* recv_buf = 0;

	// Figure out how big to make the ping packet
	int packet_size = 64;
	int ttl = 30;

	// Set up for pinging
	SOCKET sd = INVALID_SOCKET;
	sockaddr_in dest, source;

	HANDLE hselect = WSACreateEvent();

	if (setup_for_ping(ips, ttl, sd, dest) < 0) 
	{
		goto cleanup;
	}
	if (allocate_buffers(send_buf, recv_buf, packet_size) < 0) 
	{
		goto cleanup;
	}
	init_ping_packet(send_buf, packet_size, seq_no);

	// Send the ping and receive the reply
	if (send_ping(sd, dest, send_buf, packet_size) >= 0)
	{
		WSAEventSelect( sd, hselect, FD_READ );

		while ( WaitForSingleObject(hselect, timeout)==0 ) 
		{
			// Receive replies until we either get a successful read,
			// or a fatal error occurs.
			if (recv_ping(sd, source, recv_buf, 4096) < 0) 
			{
				// Pull the sequence number out of the ICMP header.  If 
				// it's bad, we just complain, but otherwise we take 
				// off, because the read failed for some reason.
				unsigned short header_len = recv_buf->h_len * 4;
				ICMPHeader* icmphdr = (ICMPHeader*)((char*)recv_buf + header_len);
				if (icmphdr->seq != seq_no) 
				{
					LOG("BASE bad sequence number:");
					continue;
				}
				else 
				{
					pingret = true;
					break;
				}
			}
			if (decode_reply(recv_buf, packet_size, &source) != -2)
			{
				// Success or fatal error (as opposed to a minor error) 
				// so take off.
				pingret = true;
				break;
			}
		}
	}

cleanup:
	delete[]send_buf;
	delete[]recv_buf;

	if ( sd!=INVALID_SOCKET )
	{
		::closesocket(sd);
	}

	if ( hselect!=INVALID_HANDLE_VALUE )
	{
		::CloseHandle( hselect );
	}

	return pingret;
}

#else

#define PING_PACKET_SIZE     4096

// 效验算法
unsigned short cal_chksum(unsigned short *addr, int len)
{
	int nleft=len;
	int sum=0;
	unsigned short *w=addr;
	unsigned short answer=0;

	while(nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}

	if( nleft == 1)
	{       
		*(unsigned char *)(&answer) = *(unsigned char *)w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;

	return answer;
}

// Ping函数
bool PingTest( const char *ips, int timeout)
{
	bool pingret = false;
	struct timeval timeo;
	int sockfd;
	struct sockaddr_in addr;
	unsigned long inaddr=0l; 
	struct sockaddr_in from;

	struct timeval *tval;
	struct ip *iph;
	struct icmp *icmp;

	char sendpacket[PING_PACKET_SIZE];
	char recvpacket[PING_PACKET_SIZE];

	pid_t pid;
	int maxfds = 0;
	fd_set readfds;

	// 设定Ip信息
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;

	if( (inaddr=inet_addr(ips))==INADDR_NONE) 
	{ 
		struct hostent *host; 
		if( (host=gethostbyname(ips))==NULL ) 
		{
			LOG("BASE gethostbyname error:");
			return pingret;
		} 
		memcpy( (char *)&addr.sin_addr,host->h_addr,host->h_length ); 
	} 
	else addr.sin_addr.s_addr = inet_addr(ips);   

	// 取得socket
	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) 
	{
		LOG("BASE socket error:ip="<<ips<<",err="<<errno);
		return pingret;
	}

	// 设定TimeOut时间
	timeo.tv_sec = timeout / 1000;
	timeo.tv_usec = timeout % 1000;

	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo)) == -1)
	{
		LOG("BASE setsockopt error:ip="<<ips<<",err="<<errno);
		close( sockfd );
		return pingret;
	}

	// 设定Ping包
	memset(sendpacket, 0, sizeof(sendpacket));

	// 取得PID，作为Ping的Sequence ID
	pid=getpid();
	int packsize;
	icmp=(struct icmp*)sendpacket;
	icmp->icmp_type=ICMP_ECHO;
	icmp->icmp_code=0;
	icmp->icmp_cksum=0;
	icmp->icmp_seq=0;
	icmp->icmp_id=pid;
	packsize=8+56;
	tval= (struct timeval *)icmp->icmp_data;
	gettimeofday(tval,NULL);
	icmp->icmp_cksum=cal_chksum((unsigned short *)icmp,packsize);

	// 发包
	if (sendto(sockfd, (char *)&sendpacket, packsize, 0, (struct sockaddr *)&addr, sizeof(addr)) < 1)
	{
		LOG("BASE sendto error:ip="<<ips<<",err="<<errno);
		close( sockfd );

		return pingret;
	}

	// 接受
	// 由于可能接受到其他Ping的应答消息，所以这里要用循环
	while(1)
	{
		// 设定TimeOut时间，这次才是真正起作用的
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
		maxfds = sockfd + 1;

		if (select(maxfds, &readfds, NULL, NULL, &timeo) <= 0)
		{
			LOG("BASE timeout error:ip="<<ips<<",err="<<errno);
			break;
		}
		// 接受
		memset(recvpacket, 0, sizeof(recvpacket));
		socklen_t fromlen = sizeof(from);
		if (recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0, (struct sockaddr *)&from, &fromlen) < 1) 
		{
			LOG("BASE recvfrom error:ip="<<ips<<",err="<<errno);
			break;
		}

		// 判断是否是自己Ping的回复
		char *from_ip = (char *)inet_ntoa(from.sin_addr);
		LOG("BASE:from_ip"<<from_ip);
		if (strcmp(from_ip,ips) != 0)
		{
			LOG("BASE ip wrong:ip="<<ips<<",err="<<errno);
			break;
		}

		iph = (struct ip *)recvpacket;
		icmp=(struct icmp *)(recvpacket + (iph->ip_hl<<2));

		// 判断Ping回复包的状态
		if ( icmp->icmp_type==ICMP_ECHOREPLY && icmp->icmp_id==pid )
		{
			// 正常就退出循环
			LOG("BASE success:ip="<<ips<<",icmp->icmp_type="<<icmp->icmp_type<<",icmp->icmp_id="<<icmp->icmp_id);
			pingret = true;
			break;
		} 
		else
		{
			// 否则继续等
			continue;
		}
	}

	// 关闭socket
	close(sockfd);

	return pingret;
}

#endif 



void SleepMillisecond(int millsec)
{
#if defined _WINDOWS_ || defined WIN32
	_sleep(millsec);
#else
	usleep( millsec*1000 );
	//struct timeval tv;
	//tv.tv_sec = 0;
	//tv.tv_usec = millsec * 1000;
	//select(0, NULL, NULL, NULL, &tv);
#endif

	return;
}

unsigned long long GetTimeMs( void )
{
	unsigned long long ret_time = 0;
#if defined _WINDOWS_ || defined WIN32

	struct tm	tm;
	time_t		now;
	SYSTEMTIME	wtm;

	GetLocalTime(&wtm);
	tm.tm_year    = wtm.wYear - 1900;
	tm.tm_mon     = wtm.wMonth - 1;
	tm.tm_mday    = wtm.wDay;
	tm.tm_hour    = wtm.wHour;
	tm.tm_min     = wtm.wMinute;
	tm.tm_sec     = wtm.wSecond;
	tm. tm_isdst  = -1;

	tm_to_time( &tm, &now );

	ret_time = (unsigned long long)now*1000UL + wtm.wMilliseconds;
#else
	struct timeval now;
	gettimeofday( &now , 0 );
	ret_time = now.tv_sec;
	ret_time = ret_time*1000000;
	ret_time += now.tv_usec;

	ret_time = ret_time/1000ULL;
#endif

	return ret_time;
}

unsigned long GetCPUTickCount( void )
{
#if defined _WINDOWS_ || defined WIN32
	return GetTickCount()/1000UL;
#else
	struct sysinfo sinfo;

	sinfo.uptime = 0;
	::sysinfo(&sinfo);

	return sinfo.uptime;
	/*return ::time(NULL);*/
#endif
}

int time_to_tm(time_t *time_input,struct tm* tm_result)
{
	static const char month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	static const bool leap_year[4] = {false, false, true, false};

	unsigned int leave_for_fouryear = 0;
	unsigned short four_year_count = 0;
	unsigned int temp_value = 0;

	tm_result->tm_sec = *time_input % 60;
	temp_value = *time_input / 60;// 分钟
	tm_result->tm_min = temp_value % 60;
	temp_value /= 60; // 小时

	temp_value += 8;// 加上时区

	tm_result->tm_hour = temp_value % 24;
	temp_value /= 24; // 天

	tm_result->tm_wday = (temp_value + 4) % 7;// 1970-1-1是4

	four_year_count = temp_value / (365 * 4 + 1);
	leave_for_fouryear = temp_value % (365 * 4 + 1);
	int leave_for_year_days = leave_for_fouryear;

	int day_count = 0;
	int i = 0;

	for (i = 0; i < 4; i++)
	{        
		day_count = leap_year[i] ? 366 : 365;

		if (leave_for_year_days <= day_count)
		{
			break;
		}
		else
		{
			leave_for_year_days -= day_count;
		}
	}

	tm_result->tm_year = four_year_count * 4 + i + 70;
	tm_result->tm_yday = leave_for_year_days;// 这里不是天数，而是标记，从0开始

	int leave_for_month_days = leave_for_year_days;

	int j = 0;
	for (j = 0; j < 12; j++)
	{
		if (leap_year[i] && j == 1)
		{
			if (leave_for_month_days < 29)
			{
				break;
			}
			else if (leave_for_month_days == 29)
			{
				j++;
				leave_for_month_days = 0;
				break;
			}
			else
			{
				leave_for_month_days -= 29;
			}

			continue;    
		}

		if (leave_for_month_days < month_days[j])
		{
			break;
		}
		else if(leave_for_month_days == month_days[j]){
			j++;
			leave_for_month_days = 0;
			break;
		}
		else
		{
			leave_for_month_days -= month_days[j];
		}                
	}

	tm_result->tm_mday = leave_for_month_days + 1;
	tm_result->tm_mon = j;
	if ( tm_result->tm_mon >= 12 )
	{
		tm_result->tm_year++;
		tm_result->tm_mon -= 12;
	}
	tm_result->tm_isdst=-1;

	return 0;
}

int tm_to_time(struct tm* tm_input, time_t *time_result)
{
	static short monthlen[12]   = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	static short monthbegin[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

	time_t t;

	t  = monthbegin[tm_input->tm_mon]
	+ tm_input->tm_mday-1
		+ (!(tm_input->tm_year & 3) && tm_input->tm_mon > 1);

	tm_input->tm_yday = t;
	t += 365 * (tm_input->tm_year - 70)
		+ (tm_input->tm_year - 69)/4;

	tm_input->tm_wday = (t + 4) % 7;

	t = t*86400 + (tm_input->tm_hour-8)*3600 + tm_input->tm_min*60 + tm_input->tm_sec;

	if (tm_input->tm_mday > monthlen[tm_input->tm_mon]+(!(tm_input->tm_year & 3) && tm_input->tm_mon == 1))
	{
		*time_result = mktime( tm_input );
	}
	else
	{
		*time_result = t;
	}

	return 0;
}

std::string GetTimeString( void )
{
	time_t now = time(NULL);
	struct tm t;
	time_to_tm( &now, &t );

	char curtime[256];
	sprintf( curtime, "%04d-%02d-%02d %02d:%02d:%02d", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

	return std::string( curtime );
}

std::string MemoryToString(const void *memory, int size)
{
	int newsize = size*4;
	char *pszMem = new char[newsize+1];
	memset(pszMem, 0, newsize+1);
	unsigned char *memoryBYTE = (unsigned char*)memory;
	for ( int i = 0, j = 0; i < size; ++i, ++j )
	{
		unsigned char ch = memoryBYTE[i];
		if ( ch >= 32 && ch <= 125 )
			pszMem[j] = ch;
		else
		{
			j += (snprintf(pszMem+j, newsize, "0x%02x", ch)-1);
		}
	}
	std::string mem(pszMem);
	delete []pszMem;

	return mem;
}

std::string Lower(const std::string &str)
{
	std::string tmp(str);
	for ( std::string::iterator it = tmp.begin(); it != tmp.end(); ++it )
	{
		//if ( *it >= 'A' && *it <= 'Z' )
		//	*it = *it + 32; //32='a'-'A'
		if ( (*it&0x80) == 0x80 )
			++it;
		else if ( *it >= 0x41 && *it <= 0x5A )
			*it = *it + 0x20;
	}

	return tmp;
}

std::string Upper(const std::string &str)
{
	std::string tmp(str);
	for ( std::string::iterator it = tmp.begin(); it != tmp.end(); ++it )
	{
		//if ( *it >= 'a' && *it <= 'z' )
		//	*it = *it - 32;
		if ( (*it&0x80) == 0x80 )
			++it;
		else if ( *it >= 0x61 && *it <= 0x7A )
			*it = *it - 0x20;
	}

	return tmp;
}

std::string Trim(const std::string &str)
{	
	std::string::size_type pos1 = str.find_first_not_of(' ');
	std::string::size_type pos2 = str.find_last_not_of(' ');

	return ( str.substr(pos1, (pos2-pos1+1)) );
}

std::string TrimLeft(const std::string &str)
{
	std::string::size_type pos1 = str.find_first_not_of(' ');
	return ( str.substr(pos1) );
}

std::string TrimRight(const std::string &str)
{
	std::string::size_type pos2 = str.find_last_not_of(' ');
	return ( str.substr(0, pos2+1) );
}

}

