#include "PipeLogger.h"

void PipeLogger::flush() {
	std::string s = ss.str();
	const char *buf = s.c_str();
	WriteFile(pd[1], buf, s.length(), NULL, NULL);
	ss.str("");
}
#ifdef TARGET_WIN32
PipeLogger::PipeLogger() {
	CreatePipe(&pd[0], &pd[1], NULL, 200);
	InitializeCriticalSection(&cs);
}

unsigned long PipeLogger::read_pipe(char *buf, unsigned long max_len) {
	unsigned long len;
	ReadFile(pd[0], buf, max_len, &len, NULL);
	return len;
}
#endif
PipeLogger PipeLogger::logger;