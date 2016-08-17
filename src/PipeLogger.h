
#ifndef SHARPVPN_PIPELOGGER_H
#define SHARPVPN_PIPELOGGER_H
#include <sstream>
#ifdef TARGET_WIN32
#include <Winsock2.h>
typedef HANDLE PIPE_FD;
#endif

class PipeLogger {
private:
	CRITICAL_SECTION cs;
	std::stringstream ss;
	PIPE_FD pd[2];
	void flush();
public:
	static PipeLogger logger;
	PipeLogger();
	PipeLogger& operator << (int _Val) {
		EnterCriticalSection(&cs);
		ss << _Val;
		LeaveCriticalSection(&cs);
		return *this;
	}
	PipeLogger& operator << (double _Val) {
		EnterCriticalSection(&cs);
		ss << _Val;
		LeaveCriticalSection(&cs);
		return *this;
	}
	PipeLogger& operator << (const char *_Val) {
		EnterCriticalSection(&cs);
		ss << _Val;
		if (_Val[strlen(_Val) - 1] == '\n') {
			flush();
		}
		LeaveCriticalSection(&cs);
		return *this;
	}
	PipeLogger& operator << (std::string _Val) {
		EnterCriticalSection(&cs);
		ss << _Val;
		LeaveCriticalSection(&cs);
		return *this;
	}
	PipeLogger& operator << (DWORD _Val) {
		EnterCriticalSection(&cs);
		ss << _Val;
		LeaveCriticalSection(&cs);
		return *this;
	}
	unsigned long read_pipe(char *buf, unsigned long max_len);
};

#define LOG(security) PipeLogger::logger << "\r\n"
#define LOGGER PipeLogger::logger
#endif