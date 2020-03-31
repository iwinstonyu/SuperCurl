#include "BaiduBaikeCurl.h"
#include <cassert>
#include <sstream>
#include <fstream>
#include "md5.h"
#include <json/json.h>

#ifndef LP_IF
#define LP_IF if
#endif

#define EASYLOGINF_BAIKE_CURL EasyLogger(bind(&BaiduBaikeCurl::PrintLog, this, ECurlLogLev::Inf, std::placeholders::_1)).Stream()
#define EASYLOGERR_BAIKE_CURL EasyLogger(bind(&BaiduBaikeCurl::PrintLog, this, ECurlLogLev::Err, std::placeholders::_1)).Stream()

void BaiduBaikeCurl::PushRequest(int num)
{
	BaikeRequestRef reqRef = make_shared<BaikeRequest>();
	reqRef->num_ = num;

	__super::PushRequest(reqRef);
}

void BaiduBaikeCurl::ProcessResponse()
{
	while (true) {
		if (resps_.Size())
			EASYLOGINF_BAIKE_CURL << "Response queue length: " << resps_.Size();

		ResponseRef respRef = nullptr;
		while (respRef = resps_.Read()) {
			auto reqRef = dynamic_pointer_cast<BaikeRequest>(respRef->reqRef_);
			assert(reqRef);

			EASYLOGINF_BAIKE_CURL << "Response: " << respRef->res_ << endl << reqRef->id_ << " " << reqRef->num_ << endl << respRef->respData_;
		}

		Sleep(1000);
	}
}

void BaiduBaikeCurl::AssignCurl(CURL* curl, ResponseRef respRef)
{
	auto reqRef = dynamic_pointer_cast<BaikeRequest>(respRef->reqRef_);
	assert(reqRef);

	reqRef->url_ = url_;

	Json::Value val1;
	val1["1"] = 1;
	Json::FastWriter writer;
	string evt = writer.write(val1);

	Json::Value val2;
	val2["EventValue"] = evt.substr(0, evt.length() - 1);
	val2["AppKey"] = key_;
	val2["ServerID"] = std::to_string(1);
	val2["EventID"] = "RegionUserCnt";
	val2["CreateTime"] = static_cast<unsigned int>(time(nullptr));
	string report1 = writer.write(val2);
	reqRef->report_ = report1.substr(0, report1.length() - 1);

	Json::Value val3;
	val3["AppKey"] = key_;
	val3["TimeStamp"] = std::to_string(time(nullptr));
	val3["Sign"] = md5(key_ + to_string(time(nullptr)) + secret_).substr(7, 6);
	string token1 = "_SD_TOKEN_:"+writer.write(val3);
	reqRef->token_ = token1.substr(0, token1.length() - 1);

	//string token = "_SD_TOKEN_:{\"AppKey\":\"3406808ABA9B41DA9A676DC40AB31D06\", \"TimeStamp\":\"" + to_string(time(nullptr)) + "\", \"Sign\":\""
	//	+ md5("3406808ABA9B41DA9A676DC40AB31D06"+to_string(time(nullptr))+"89B48629E8BD4F8B832837637C2602D4").substr(7, 6) + "\"}";

	//string report = "{\"EventValue\":\"{}\",\"AppKey\":\"3406808ABA9B41DA9A676DC40AB31D06\",\"ServerID\":\"1\",\"EventID\":\"RegionUserCnt\",\"CreateTime\":" + to_string(time(nullptr)) + "}";
	//string report = "{\"AppKey\":\"3406808ABA9B41DA9A676DC40AB31D06\",\"CreateTime\":1584698163,\"EventID\":\"RegionUserCnt\",\"EventValue\":\"{\\\"1\\\":1}\",\"ServerID\":\"1\"}";

	reqRef->headers_ = curl_slist_append(reqRef->headers_, "Content-Type:application/json;charset=utf-8");
	reqRef->headers_ = curl_slist_append(reqRef->headers_, reqRef->token_.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, reqRef->url_.c_str());
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, reqRef->headers_);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reqRef->report_.c_str());
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, reqRef->report_.length());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &SuperCurl::WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respRef->respData_);
}

void BaiduBaikeCurl::PrintLog(ECurlLogLev logLev, ostringstream& oss)
{
	static WindCS cs;
	static ofstream ofs("baidubaike.txt", ios::app);

	WindLocker locker(&cs);

	switch(logLev){
	case ECurlLogLev::Inf:
		cout << "[INF] " << oss.str() << endl;
		ofs << "[INF] " << oss.str() << endl;
		break;
	case ECurlLogLev::Err:
		cout << "[ERR] " << oss.str() << endl;
		ofs << "[ERR] " << oss.str() << endl;
		break;
	default:
		break;
	}
}
