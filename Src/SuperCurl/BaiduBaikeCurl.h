#pragma once
#include "SuperCurl.h"
using namespace std;

/// @brief 查询百度百科
class BaiduBaikeCurl : public SuperCurl
{
public:
	struct BaikeRequest : public SuperCurl::Request
	{
		BaikeRequest() {}
		virtual ~BaikeRequest() {}

		int num_ = 0;
		string token_;
		string report_;
	};
	typedef shared_ptr<BaikeRequest> BaikeRequestRef;

public:
	BaiduBaikeCurl(string url, string key, string secret) : url_(url), key_(key), secret_(secret) {}
	virtual ~BaiduBaikeCurl() {}

	void PushRequest(int num);
	void ProcessResponse();

private:
	virtual void AssignCurl(CURL* curl, ResponseRef respRef);
	virtual void PrintLog(ECurlLogLev logLev, ostringstream& oss);

	string url_;
	string key_;
	string secret_;
};