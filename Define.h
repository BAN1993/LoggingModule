#pragma once

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <errno.h>
#include <map>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// linux
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include<sys/time.h>

#define INVALID_SOCKET -1

using namespace std;

#define LOG(os)                         \
if (true){                              \
	std::ostringstream ss;              \
	ss << "[" << __FUNCTION__ << "]["   \
		<< __FILE__ << "."              \
		<< __LINE__ << "]"              \
		<< os;                          \
	std::string str(ss.str());          \
	cout << str << endl; }

