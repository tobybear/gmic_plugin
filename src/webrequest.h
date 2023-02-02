#ifndef _WEBREQUEST_H_
#define _WEBREQUEST_H_

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
using namespace std;
#include "RFX_StringUtils.h"

#ifndef WIN32
#define Sleep(ms) usleep(ms * 1000)
#endif

#ifdef WIN32
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "Wininet.lib")
#include <Rpc.h>
#include <Windows.h>
#include <WinInet.h>
#else
size_t callback(void *contents, size_t size, size_t nmemb, string *s) {
	size_t newLength = size*nmemb;
	s->append((char*)contents, newLength);
	return newLength;
}

#endif

typedef map<unsigned char, string> optmap;

class requestData {
public:
	requestData() {
		usePost = false;
	}

	bool usePost;
	string url;
	string header;
	string data;
	string agent;
};

int request(requestData& r, string& result) {
	result = "";
	try {
		string _url = r.url;
		string _header = r.header;
		string _data = r.data;
		string _agent = r.agent;
		int status = -1;

		if (!r.usePost && _data != "-" && _data != "") {
			_url += "?" + _data;
			_data = "";
		}

		#ifdef WIN32
			HINTERNET hInternetOpen = NULL;
			HINTERNET hInternetConnect = NULL;
			HINTERNET hHttpOpenRequest = NULL;
			try {
				bool _useHttps = _url.find("https://") == 0;
				_url = strReplace(_url, "https://", "");
				_url = strReplace(_url, "http://", "");

				int idx = (int)_url.find("/");
				string _server = _url.substr(0, idx);
				_url = _url.substr(idx);
				if (_agent == "") _agent = "Mozilla/5.0";
				hInternetOpen = InternetOpenA(_agent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
				hInternetConnect = InternetConnectA(hInternetOpen, _server.c_str(), _useHttps?INTERNET_DEFAULT_HTTPS_PORT:INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
				hHttpOpenRequest = HttpOpenRequestA(hInternetConnect, r.usePost?"POST":"GET", _url.c_str(), NULL, NULL, NULL, _useHttps?INTERNET_FLAG_SECURE:0, 0);

				string headers;
				headers = "Content-Type: application/x-www-form-urlencoded\r\n";
				if (_header != "") {
					headers = strReplace(headers, "\\r", "\r");
					headers = strReplace(headers, "\\n", "\n");
					headers += _header + "\r\n";
				}
				DWORD length = sizeof(DWORD);
				BOOL sent = HttpSendRequestA(hHttpOpenRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)_data.c_str(), (DWORD)_data.length());
				if (sent) {
					DWORD filesize = 0;
					HttpQueryInfo(hHttpOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &filesize, &length, NULL);
					DWORD dwBufSize = 8192;
					char* buffer = new char[dwBufSize + 1];
					while (true) {
						DWORD dwBytesRead;
						BOOL bRead;
						bRead = InternetReadFile(hHttpOpenRequest, buffer, dwBufSize, &dwBytesRead);
						if (dwBytesRead == 0) {
							break;
						}
						buffer[dwBytesRead] = 0;
						result += string(buffer, dwBytesRead);
					}
					delete[] buffer;
				}
				DWORD statusCode = 0;
				HttpQueryInfo(hHttpOpenRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &length, NULL);
				if (statusCode > 0) status = statusCode;

			} catch(...) {
			}
			if (hHttpOpenRequest) InternetCloseHandle(hHttpOpenRequest);
			if (hInternetConnect) InternetCloseHandle(hInternetConnect);
			if (hInternetOpen) InternetCloseHandle(hInternetOpen);
		#else
			CURL *curl;
			CURLcode res;
			curl_global_init(CURL_GLOBAL_ALL);
			curl = curl_easy_init();
			if (!curl) {
				curl_global_cleanup();
				return 1;
			}
			curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
			curl_easy_setopt(curl, CURLOPT_POST, r.usePost?1:0);

			if (_agent != "") {
				curl_easy_setopt(curl, CURLOPT_USERAGENT, _agent.c_str());
			}
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
			curl_easy_setopt (curl, CURLOPT_VERBOSE, 0L);

			struct curl_slist* list = NULL;		

			if (r.usePost) {
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, _data.c_str());
			}

			vector<string> hlist;
			strSplit(_header + "\r", '\r', hlist, false);
			for (int i = 0; i < (int)hlist.size(); i++) {
				int pos = (int)hlist[i].find(":");
				if (pos > 0) {
					hlist[i] = strReplace(hlist[i], "\\s", " ");
					list = curl_slist_append(list, hlist[i].c_str());
				}
			}
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

			res = curl_easy_perform(curl);
			curl_slist_free_all(list);
			if (res == CURLE_OK ) {
				long response_code;
				res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				if (res == CURLE_OK ) {
					status = (int)response_code;
				} else {
					status = -1;
				}
			} else {
				status = -1;
			}
			curl_easy_cleanup(curl);
			curl_global_cleanup();
		#endif

		return status;
	} catch(...) {
		return -1; // default code for generic error;
	}
}

#endif
