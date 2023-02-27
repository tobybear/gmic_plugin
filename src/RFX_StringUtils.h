/*
 #
 #  File        : RFX_String_Utils.h
 #
 #  Description : A self-contained header file with helper functions for    
 #                string operations on std::string    
 #
 #  Copyright   : Tobias Fleischer / reduxFX Productions (http://www.reduxfx.com)
 #
 #  Licenses        : This file is 'dual-licensed', you have to choose one
 #                    of the two licenses below to apply.
 #
 #                    CeCILL-C
 #                    The CeCILL-C license is close to the GNU LGPL.
 #                    ( http://www.cecill.info/licences/Licence_CeCILL-C_V1-en.html )
 #
 #                or  CeCILL v2.0
 #                    The CeCILL license is compatible with the GNU GPL.
 #                    ( http://www.cecill.info/licences/Licence_CeCILL_V2-en.html )
 #
 #  This software is governed either by the CeCILL or the CeCILL-C license
 #  under French law and abiding by the rules of distribution of free software.
 #  You can  use, modify and or redistribute the software under the terms of
 #  the CeCILL or CeCILL-C licenses as circulated by CEA, CNRS and INRIA
 #  at the following URL: "http://www.cecill.info".
 #
 #  As a counterpart to the access to the source code and  rights to copy,
 #  modify and redistribute granted by the license, users are provided only
 #  with a limited warranty  and the software's author,  the holder of the
 #  economic rights,  and the successive licensors  have only  limited
 #  liability.
 #
 #  In this respect, the user's attention is drawn to the risks associated
 #  with loading,  using,  modifying and/or developing or reproducing the
 #  software by the user in light of its specific status of free software,
 #  that may mean  that it is complicated to manipulate,  and  that  also
 #  therefore means  that it is reserved for developers  and  experienced
 #  professionals having in-depth computer knowledge. Users are therefore
 #  encouraged to load and test the software's suitability as regards their
 #  requirements in conditions enabling the security of their systems and/or
 #  data to be ensured and,  more generally, to use and operate it in the
 #  same conditions as regards security.
 #
 #  The fact that you are presently reading this means that you have had
 #  knowledge of the CeCILL and CeCILL-C licenses and that you accept its terms.
 #
*/

#ifdef _WIN32
#pragma warning (disable:4996)
#endif
#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>
#include <sys/stat.h>
using namespace std;

namespace reduxfx {

static string strLowercase(const string& str, int length = -1) {
	string s = str;
	if (length < 0 || length > (int)str.length())
		transform(s.begin(), s.end(), s.begin(), ::tolower);
	else
		transform(s.begin(), s.begin() + length, s.begin(), ::tolower);
	return s;
}

static string strTrim(const string& str, const string& whitespace = " \t") {
	const size_t strBegin = str.find_first_not_of(whitespace);
	if (strBegin == string::npos) return ""; // no content
	const size_t strEnd = str.find_last_not_of(whitespace);
	const size_t strRange = strEnd - strBegin + 1;
	return str.substr(strBegin, strRange);
}

static string strReplace(const string s, const string search, string replace, bool firstOnly = false, bool caseInsensitive = false) {
	string str = s;
	size_t pos = 0;
	if (caseInsensitive) {
		string strL = strLowercase(str);
		string searchL = strLowercase(search);
		while ((pos = strL.find(searchL, pos)) != string::npos) {
			strL.erase(pos, search.size());
			strL.insert(pos, replace);
			str.erase(pos, search.size());
			str.insert(pos, replace);
			pos += replace.size();
			if (firstOnly) break;
		}
	} else {
		while ((pos = str.find(search, pos)) != string::npos) {
			str.replace(pos, search.size(), replace);
			pos += replace.size();
			if (firstOnly) break;
		}
	}
	return str;
}

static void strSplit(const string& s, const char seperator, vector<string>& results, bool addEmpty = true) {
	results.clear();
	string::size_type prev_pos = 0, pos = 0;
	while((pos = s.find(seperator, pos)) != std::string::npos) {
		string substring(s.substr(prev_pos, pos-prev_pos) );
		substring = strTrim(substring);
		if (substring != "" || addEmpty) results.push_back(substring);
		prev_pos = ++pos;
	}
	string substring = strTrim(s.substr(prev_pos, pos - prev_pos));
	if (substring != "" || addEmpty) results.push_back(substring); // Last word
}

static string intToString(const int i) {
	stringstream ss;
	ss << i;
	return ss.str();
}

static string floatToString(const float i) {
	stringstream ss;
	 ss << setprecision(8) << i;
	return ss.str();
}

static string strRemoveXmlTags(const string& s, bool replaceEntities = false) {
	string r;
	bool inHtml = false;
	for (unsigned int i = 0; i < s.size(); i++) {
		if (s[i] == '<') {
			inHtml = true;
		} else if (s[i] == '>') {
			inHtml = false;
		} else if (!inHtml) {
			r += s[i];
		}
	}
	if (replaceEntities) {
		strReplace(r, "&amp;", "&");
		strReplace(r, "&nbsp;", " ");
		strReplace(r, "&hearts;", "");
		strReplace(r, "\\251", "(C)");
		strReplace(r, "&#224;", "a"); // agrave
		strReplace(r, "&#225;", "a"); // aacute
		strReplace(r, "&#228;", "a"); // auml
		strReplace(r, "&#229;", "a"); // aring
		strReplace(r, "&#231;", "c"); // ccedil
		strReplace(r, "&#232;", "e"); // egrave
		strReplace(r, "&#233;", "e"); // eacute
		strReplace(r, "&#239;", "i"); // iuml
		strReplace(r, "&#244;", "o"); // ocirc
		strReplace(r, "&#252;", "u"); // uuml
		for (int i = 230; i < 255; i++) {
			unsigned char c = (unsigned char)i;
			string ss; ss += c;
			strReplace(r, "&#" + intToString(i) + ";", ss);
		
		}
	}
	return r;
}

static bool fileExists(const string filename) {
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0;
}

static string loadStringFromFile(const string filename, const bool loadOnlyText = false) {
	string res;
	if (filename == "") return res;
	ifstream infile;
	infile.open(filename.c_str(), ios::in | ios::binary | ios::ate);
	if (infile.is_open()) {
		int size = (int)infile.tellg();
		char* bufferP = new char[size];
		infile.seekg (0, ios::beg);
		infile.read(bufferP, size);
		if (!infile) size = (int)infile.gcount();
		infile.close();

		if (!loadOnlyText) {
			res = string(reinterpret_cast<const char*>(bufferP), size);
		} else {
			for (int i = 0; i < size; i++) {
				if ((unsigned char)bufferP[i] >= 13 && (unsigned char)bufferP[i] <= 127)
					res += bufferP[i];
			}
		}
		delete[] bufferP;
	}
	return res;
}

static int loadLinesFromFile(const string filename, vector<string>& lines, bool bIgnoreEmptyLines = false) {
	string res;
	lines.clear();
	if (filename == "") return -1;
	string s = loadStringFromFile(filename);

	stringstream ss(stringstream::in | stringstream::out);
	ss << s;
	string line, section, token, val;

	while (getline(ss, line)) {
		line = strTrim(line, "\r\n");
		if (line != "" || !bIgnoreEmptyLines)
			lines.push_back(line);
	}
	return 0;
}

static int saveStringToFile(string s, const string filename, const bool unifyLineEndings = false) {
	ofstream myfile(filename.c_str(), ios::out | ios::binary);
	if (myfile.is_open()) {
		if (unifyLineEndings) {
			s = strReplace(s, "\r\n", "\n");
			s = strReplace(s, "\r", "\n");
			s = strReplace(s, "\n", "\r\n");
		}
		myfile.write(s.c_str(), s.size());
		myfile.close();
		return 0;
	}
	return -1;
}

// hex string/buffer conversion

static inline string bufferToHexString(const unsigned char* buf, const unsigned int length) {
	stringstream ss;
	for (size_t i = 0; length > i; ++i)
			ss << hex << setw(2) << setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(buf[i]));
	return ss.str();
}

static inline void hexBufferToBuffer(const unsigned char* inbuf, const unsigned int length, unsigned char* outbuf) {
	unsigned int ocnt = 0;
	for (size_t i = 0; i < length; i += 2) {
		size_t s = 0;
		stringstream ss;
		ss << std::hex << inbuf[i] << inbuf[i + 1];
		ss >> s;
		outbuf[ocnt++] = (static_cast<unsigned char>(s));
	}
}

static inline void hexStringToBuffer(const string hexstr, unsigned char* outbuf) {
	hexBufferToBuffer((const unsigned char*)hexstr.c_str(), (int)hexstr.length(), outbuf);
}

static string c2s(const char* c) {
	string n = string(c);
	int np = (int)n.find('\0');
	if (np > 0) n = n.substr(0, np);
	return strTrim(n);
}

}

