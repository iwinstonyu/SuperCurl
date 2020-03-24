#pragma once;
#include <Windows.h>
#include <functional>
#include <sstream>
using namespace std;

class WindCS {
public:
	WindCS() { ::InitializeCriticalSection(&cs_); }
	~WindCS() { ::DeleteCriticalSection(&cs_); }

	void Lock() { ::EnterCriticalSection(&cs_); }
	void Unlock() { ::LeaveCriticalSection(&cs_); }

private:
	WindCS(const WindCS&);
	WindCS& operator=(const WindCS&);

	::CRITICAL_SECTION cs_;
};

class WindLocker {
public:
	explicit WindLocker(WindCS* pwcs) : pwcs_(pwcs) { pwcs_->Lock(); }
	~WindLocker() { pwcs_->Unlock(); }

private:
	WindLocker(const WindLocker&);
	WindLocker& operator=(const WindLocker&);

	WindCS* pwcs_;
};

enum ECurlLogLev
{
	Inf,
	Err,
};

class EasyLogger {
public:
	typedef function<void(ostringstream& oss)> TLogFunc;

public:
	explicit EasyLogger(TLogFunc logFunc) : logFunc_(logFunc) {}
	EasyLogger::~EasyLogger() {
		if (logFunc_)
			logFunc_(oss_);
	}

	std::ostringstream& Stream() { return oss_; }

private:
	EasyLogger(const EasyLogger&);
	void operator=(const EasyLogger&);

	std::ostringstream oss_;
	TLogFunc logFunc_;
};