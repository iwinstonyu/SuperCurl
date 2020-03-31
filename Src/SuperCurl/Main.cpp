//
//	<one line to give the program's name and a brief idea of what it does.>
//	Copyright (C) 2018. WenJin Yu. windpenguin@gmail.com.
//
//	Created at 2018/7/4 11:40:04
//	Version 1.0
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "BaiduBaikeCurl.h"
#include <Windows.h>
#include "IniFile.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib,"Psapi.lib")

size_t WriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	auto* response = reinterpret_cast<std::string *>(userdata);
	if (response == nullptr)
		return 0;

	size_t len = size * nmemb;
	response->append(ptr, len);
	return len;
}

void Test1()
{
	string response;
	string url = "http://baidu.com";

	auto curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	auto cm = curl_multi_init();
	curl_multi_add_handle(cm, curl);

	int runningHandles = 0;
	do {
		int numfds;

		auto code = curl_multi_perform(cm, &runningHandles);

		if (code == CURLM_OK) {
			code = curl_multi_wait(cm, nullptr, 0, 10000, &numfds);
		}

		if (code != CURLM_OK) {
			cout << "curl_multi_wait failed: " << code;
			break;
		}
	} while (runningHandles);
}

int main()
{
	SuperCurl::GlobalInit();

	wind::IniFile iniFile("../../../../conf.ini");
	if (!iniFile.IsOpen())
		return 1;

	string url = iniFile.ReadString("app1", "url", "");
	string key = iniFile.ReadString("app1", "key", "");
	string secret = iniFile.ReadString("app1", "secret", "");

	BaiduBaikeCurl baikeCurl(url, key, secret);

	thread thr([&baikeCurl]()->void {
		while (true) {
			int loop = rand() % 3;
			for (int i = 0; i < loop; ++i) {
				baikeCurl.PushRequest(rand() % 100);
			}

			Sleep(1000);
		}
	});

// 	int num = rand() % 100;
// 	baikeCurl.PushRequest(to_string(num));

	baikeCurl.ProcessResponse();

	return 0;
}

