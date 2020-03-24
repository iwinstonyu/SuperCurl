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
	BaiduBaikeCurl() {}
	virtual ~BaiduBaikeCurl() {}

	void PushRequest(int num);
	void ProcessResponse();

private:
	virtual void AssignCurl(CURL* curl, ResponseRef respRef);
	virtual void PrintLog(ECurlLogLev logLev, ostringstream& oss);
};