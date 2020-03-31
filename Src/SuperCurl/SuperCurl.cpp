#include "SuperCurl.h"
#include <cassert>
#include <sstream>
#include <fstream>
#include "md5.h"
#include <json/json.h>

#ifndef LP_IF
#define LP_IF if
#endif

#define EASYLOGINF_SUPER_CURL EasyLogger(bind(&SuperCurl::PrintLog, this, ECurlLogLev::Inf, std::placeholders::_1)).Stream()
#define EASYLOGERR_SUPER_CURL EasyLogger(bind(&SuperCurl::PrintLog, this, ECurlLogLev::Err, std::placeholders::_1)).Stream()

#define CURL_MULTI_WAIT_MSEC 10000
#define CURL_MAX_RETRY_QUEUE_LENGTH 10000

void SuperCurl::GlobalInit()
{
	curl_global_init(CURL_GLOBAL_ALL);
}

SuperCurl::SuperCurl()
{
	assert(multiAmount_);
	thrSendReq_ = thread(&SuperCurl::SendRequest, this);
}

void SuperCurl::PushRequest(RequestRef reqRef)
{
	reqRef->id_ = id_++;
	reqs_.Write(reqRef);
}

void SuperCurl::AssignCurl(CURL* /*curl*/, ResponseRef /*respRef*/)
{

}

void SuperCurl::PrintLog(ECurlLogLev /*logLev*/, ostringstream& /*oss*/)
{
}

void SuperCurl::SendRequest()
{
	auto cm = curl_multi_init();
	assert(cm);

	vector<CURL*> curls;
	curls.assign(multiAmount_, nullptr);

	while(true) {
		if(reqs_.Size())
			EASYLOGINF_SUPER_CURL << "Request queue length: " << reqs_.Size();

		try {
			map<CURL*, ResponseRef> curlRespMap;
			RequestRef reqRef = nullptr;
			int idx = 0;
			while (idx < multiAmount_) {
				reqRef = reqs_.Read();
				if (!reqRef && !retryReqs_.empty()) {
					reqRef = retryReqs_.front();
					retryReqs_.pop_front();
				}
				if (!reqRef)
					break;

				if (!curls[idx]) {
					curls[idx] = curl_easy_init();
					assert(curls[idx]);
				}

				ResponseRef respRef = make_shared<Response>();
				respRef->reqRef_ = reqRef;

				curlRespMap[curls[idx]] = respRef;

				AssignCurl(curls[idx], respRef);

				auto res = curl_multi_add_handle(cm, curls[idx]);
				if (res != CURLM_OK) {
					EASYLOGERR_SUPER_CURL << "Fail curl_multi_add_handle: " << res;
				}

				++idx;
			}

			if (!idx)
				continue;

			int runningHandles = 0;
			do {
				int numfds;

				auto code = curl_multi_perform(cm, &runningHandles);

				if (code == CURLM_OK) {
					code = curl_multi_wait(cm, nullptr, 0, CURL_MULTI_WAIT_MSEC, &numfds);
				}

				if (code != CURLM_OK) {
					EASYLOGERR_SUPER_CURL << "curl_multi_wait failed: " << code;
					break;
				}
			} while(runningHandles);


			CURLMsg* curlMsg = nullptr;
			int msgInQue = 0;
			while (curlMsg = curl_multi_info_read(cm, &msgInQue)) {
				auto curl = curlMsg->easy_handle;

				ResponseRef respRef = nullptr;
				if (!curlRespMap.count(curl)) {
					EASYLOGERR_SUPER_CURL << "Fail match curlRespMap: " << curl;
				}
				else {
					respRef = curlRespMap[curl];

					if (curlMsg->msg == CURLMSG_DONE) {
						respRef->res_ = curlMsg->data.result;
					}
				}

				curl_multi_remove_handle(cm, curl);
			}

			for (int i = 0; i < idx; ++i) {
				if (curls[i]) {
					curl_easy_reset(curls[i]);
				}
			}

			for (auto& cr : curlRespMap) {
				auto respRef = cr.second;

				if (respRef) {
					auto reqRef = respRef->reqRef_;

					if (reqRef) {
						if (reqRef->headers_) {
							curl_slist_free_all(respRef->reqRef_->headers_);
							reqRef->headers_ = nullptr;
						}

						if (respRef->res_ != CURLE_OK) {
							if (reqRef->retries_ < retries_ && retryReqs_.size() < CURL_MAX_RETRY_QUEUE_LENGTH) {
								EASYLOGERR_SUPER_CURL << "Fail curl and retry: " << respRef->res_ << " " << reqRef->id_ << " " << reqRef->retries_;
								++reqRef->retries_;
								retryReqs_.push_back(reqRef);
								continue;
							}
							else {
								EASYLOGERR_SUPER_CURL << "Fail curl and retry over: " << respRef->res_ << " " << reqRef->id_ << " " << reqRef->retries_;
							}
						}
					}

					resps_.Write(respRef);
				}
				else {
					assert(0);
				}
			}
		}
		catch (std::exception e) {
			EASYLOGERR_SUPER_CURL << "Exception: " << e.what();
		}
	}
}

size_t SuperCurl::WriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	auto* response = reinterpret_cast<std::string *>(userdata);
	if (response == nullptr)
		return 0;

	size_t len = size * nmemb;
	response->append(ptr, len);
	return len;
}