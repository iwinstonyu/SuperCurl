#pragma once
#include <curl/curl.h>
#include "SingleQueue.h"
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <iostream>
#include <deque>
#include <map>
#include <fstream>
#include <sstream>
#include "Log.h"
using namespace std;

class SuperCurl
{
public:
	static void GlobalInit();

	struct Request
	{
		Request() {}
		virtual ~Request() {}

		uint64_t id_ = 0;
		string url_;
		struct curl_slist* headers_ = nullptr;
	};
	typedef shared_ptr<Request> RequestRef;

	struct Response
	{
		RequestRef reqRef_ = nullptr;
		CURLcode res_ = CURL_LAST;
		string respData_;
	};
	typedef shared_ptr<Response> ResponseRef;

public:
	SuperCurl();
	virtual ~SuperCurl() {}

	void PushRequest(RequestRef reqRef);

	/// @warning 由于是虚函数，不要在构造函数中调用
	virtual void PrintLog(ECurlLogLev logLev, ostringstream& oss);

protected:
	virtual void AssignCurl(CURL* curl, ResponseRef respRef);

	static size_t WriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata);

private:
	void SendRequest();

protected:
	uint64_t id_ = 1;
	int multiAmount_ = 8;
	SingleQueue<Request> reqs_;
	deque<RequestRef> retryReqs_;
	SingleQueue<Response> resps_;
	thread thrSendReq_;
};