/*
 #
 #  File        : gmicpluginbatcher.cpp
 #
 #  Description : The sources for a tool that creates effect plugins 
 #                for various hosts from a binary template file
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
#include <direct.h>
#include <Windows.h>
#include <conio.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#define _mkdir mkdir
#endif

#define PIPL
#define AE_PLUGIN
#include "../gmic_plugin.h"
#undef PIPL
#undef AE_PLUGIN

#include "../gmic_utils.h"

#include "../RFX_StringUtils.h"
using namespace reduxfx;
using namespace std;

static int loadBufferFromFile(const string filename, unsigned char** bufferP, unsigned int& bufferSize, const bool piggybackMode = false) {
	bufferSize = 0;
	*bufferP = NULL;
	if (filename == "") return -1;
	ifstream infile;
	infile.open(filename.c_str(), ios::in | ios::binary | ios::ate);
	if (infile.is_open()) {
		if (piggybackMode) {
			infile.seekg(0, ios::end);
			unsigned int flen = (unsigned int)infile.tellg();
			int ofs = (int)flen - sizeof(int);
			infile.seekg(ofs, ios::beg);
			infile.read((char*)(&bufferSize), sizeof(int));
			ofs = (int)flen - (int)bufferSize - sizeof(int);
			if (ofs < 0 || bufferSize == 0) {
				infile.close();
				return -1;
			}
			infile.seekg(ofs, ios::beg);
		} else {
			bufferSize = (unsigned int)infile.tellg();
			infile.seekg(0, ios::beg);
		}
		*bufferP = new unsigned char[bufferSize];
		infile.read((char*)*bufferP, bufferSize);
		if (!infile) bufferSize = (unsigned int)infile.gcount();
		
		infile.close();
		return 0;
	}
	return -1;
}

static int saveBufferToFile(const unsigned char** bufferP, const unsigned int bufferSize, const string filename, const bool piggybackMode = false) {
	ofstream outfile;
	if (piggybackMode) {
		outfile.open(filename.c_str(), ios::out | ios::binary | ios::app);
	} else {
		outfile.open(filename.c_str(), ios::out | ios::binary);
	}
	if (outfile.is_open()) {
		outfile.write((char*)(*bufferP), bufferSize);
		if (piggybackMode) outfile.write((char*)&bufferSize, sizeof(int));
		outfile.close();			
		return 0;
	}
	return -1;
}

static unsigned int replacebin(unsigned char* p, long length, string s1, string s2, bool addNull = true, bool onlyFirst = false) {
	unsigned int r = 0;
	for (unsigned int i = 0; i < length - s1.length(); i++) {
		bool bad = false;
		for (unsigned int j = 0; j < s1.length(); j++) {
			if ((unsigned char)p[i + j] != (unsigned char)s1[j]) { bad = true; break; }
		}
		if (!bad) {
			r++;
			for (unsigned int j = 0; j < s2.length(); j++) p[i + j] = s2[j];
			if (addNull) p[i + s2.length()] = 0;
			if (i > 0 && p[i - 1] == '\x1f') {
				//only for PIPL!
				p[i - 1] = min((int)s2.length(), 255);
			}
			if (onlyFirst) return r;
		}
	}
	return r;
}

static void patchbin(string pf, string pfout, const gmicFilter& f) {
	unsigned char* p = NULL;
	unsigned int len = 0;
	loadBufferFromFile(pf, &p, len);
	string fcode = serializeFilter(f) + "$$$";
	string fcode_src = string(GMIC_CODE);
	if (fcode.size() > fcode_src.size()) {
		// fcode = fcode.substr(0, fcode_src.size());
		printf("ERROR: filter source code for '%s' is too big!", f.name.c_str());
		delete[] p;
		return;
	}
	replacebin(p, len, fcode_src, fcode, false);

	// TODO: check if strings longer than 31 characters cause problems
	// for example some unique IDs are no longer unique if shortened!
	string cat = "G'MIC - " + f.category;
	replacebin(p, len, PLUGIN_NAME, f.name.substr(0, 31));
	replacebin(p, len, PLUGIN_CATEGORY, cat.substr(0, 31));
	replacebin(p, len, PLUGIN_UNIQUEID, f.uniqueId.substr(0, 31));

	saveBufferToFile((const unsigned char**)&p, len, pfout);
	delete[] p;
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("ERROR: invalid parameters!\nUsage: gmicpluginbatcher FILTERFILE TEMPLATEFILE [SELECTFILE]\n");
		return 1;
	}
	string filterFile = string(argv[1]);
	string templateFile = string(argv[2]);
	string selectFile;
	vector<string> selectList;
	if (argc > 3) {
		selectFile = string(argv[3]);
	}
	if (!fileExists(filterFile)) {
		printf("ERROR: filter file '%s' not found!\n", filterFile.c_str());
		return 1;
	}
	if (!fileExists(templateFile)) {
		printf("ERROR: template file '%s' not found!\n", templateFile.c_str());
		return 1;
	}
	if (selectFile != "") {
		if (!fileExists(selectFile)) {
			printf("ERROR: select file '%s' not found!\n", selectFile.c_str());
			return 1;
		}
		string l = loadStringFromFile(selectFile);
		if (l != "") {
			l = strReplace(l, "\r\n", "\n");
			l = strReplace(l, "\r", "\n");
			l = strReplace(l, "\t", "\a");
			strSplit(l, '\n', selectList, false);
		}

	}
	string f;
	vector<gmicFilter> filters;
	if (strLowercase(filterFile).find(".json") < 0) {
		vector<string> lines;
		loadLinesFromFile(filterFile, lines, true);
		for (int i = 0; i < (int)lines.size(); i++) {
			gmicFilter& gf = deserializeFilter(lines[i]);
			if (gf.name != "") {
				filters.push_back(gf);
			}
		}
	} else {
		f = loadStringFromFile(filterFile);
		parseFilters(f, filters);
	}
	applySelect(selectList, filters);
	printf("Found %d filters\n", filters.size());
	string outFile;
	for (int i = 0; i < (int)filters.size(); i++) {
		int pos = (int)templateFile.rfind(".");
		if (pos < 0) {
			outFile = templateFile + "_" + filters[i].category + "_" +  filters[i].name;
		} else {
			outFile = templateFile.substr(0, pos) + "_" + filters[i].category + "_" + filters[i].name + templateFile.substr(pos);
		}
		patchbin(templateFile, outFile, filters[i]);
		printf("Creating %s\n", outFile.c_str());
	}
	return 0;
}
