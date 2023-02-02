/*
#
#  File        : gmic_plugin.cpp
#
#  Description : The sources for a plugin that can be compiled for both 
#                various hosts allowing them to interface the G'MIC library     
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

#include "gmic_plugin.h"
#include <atomic>

#define PARAM_COMMAND (globalDataP->nofParams - 10)
#define PARAM_OUTPUT (globalDataP->nofParams - 8)
#define PARAM_RESIZE (globalDataP->nofParams - 7)
#define PARAM_NOALPHA (globalDataP->nofParams - 6)
#define PARAM_PREVIEW (globalDataP->nofParams - 5)
#define PARAM_SRAND (globalDataP->nofParams - 4)
#define PARAM_ANIMSEED (globalDataP->nofParams - 3)
#define PARAM_VERBOSITY (globalDataP->nofParams - 2)

class ThreadData {
public:
	pthread_t tid;
	string cmd;
	atomic<int> result;
	atomic<bool> doProcess;
	atomic<bool> doExit;
	atomic<bool> initialized;
//	bool busy;
	unsigned int nofImages;
	gmic_interface_image images[MAX_NOF_LAYERS];
	gmic_interface_options options;

	ThreadData() {
		initialized = false;
		doProcess = false;
		doExit = false;
		// busy = false;
		options.custom_commands = NULL;
		options.error_message_buffer = new char[256];
		options.error_message_buffer[0] = '\0';
		options.ignore_stdlib = false;
		options.p_is_abort = NULL;
		options.p_progress = NULL;
#ifdef FREI0R_PLUGIN
		options.output_format = E_FORMAT_BYTE;
		options.interleave_output = true;
#else
		options.output_format = E_FORMAT_FLOAT;
		options.interleave_output = false;
#endif
		options.no_inplace_processing = true;
	}

	virtual ~ThreadData() {
		delete[] options.error_message_buffer;
	}
};

static string getDesktopFolder() {
#if defined(_WIN32)
	HRESULT hr;
	char path[1024];
	hr = SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, path);
	string sFile = string((const char*)&path[0]);
#else
	struct passwd *pw = getpwuid(getuid());
	const char* homedir = pw->pw_dir;
	string sFile = string(homedir) + "/Desktop";
#endif
	return sFile;
}

class GmicGlobalData {
public:
	ThreadData tdata[MAX_NOF_THREADS];
	atomic<bool> busy[MAX_NOF_THREADS];
	mutex threadlock;
	atomic<bool> initialized;

	GmicGlobalData() {
		initialized = false;

		string rc_path = get_gmic_rc_path();
		string logfile_path = rc_path + "/gmic_lib.log";
		FILE* logfile = fopen(logfile_path.c_str(), "w");
		fclose(logfile);

		// TLog::SetFile(getDesktopFolder() + "/gmic_plugin.log");
		
//		freopen(string(getDesktopFolder() + "/gmic_plugin2.log").c_str(), "w", stdout);
//		freopen(string(getDesktopFolder() + "/gmic_plugin2.log").c_str(), "w", stderr);

		#ifndef OFX_PLUGIN
		string code = GMIC_CODE;
		code = string(code.c_str()); // shorten string until first 0 byte
		filter = deserializeFilter(code);
		#endif
	}

	~GmicGlobalData() {
		for (int i = 0; i < MAX_NOF_THREADS; i++) {
			if (tdata[i].initialized && !tdata[i].doExit) {
				tdata[i].doExit = true;
				pthread_detach(tdata[i].tid);
				tdata[i].initialized = false;
			}
		}
	}

#ifdef OFX_PLUGIN
	vector<gmicFilter> filters;
	mutex initLock;

	string getGmicVersion() {
		unsigned int nofImg = 0;
		gmic_interface_image img;
		gmic_interface_options options;
		memset(&options, 0, sizeof(gmic_interface_options));
		options.output_format = E_FORMAT_BYTE;
		options.no_inplace_processing = true;
		gmic_call("('$_version')", &nofImg, &img, &options);
		string version = "320";
		if (nofImg == 1) {
			if (img.width > 2) version = string((const char*)img.data, img.width);
			if (img.data) gmic_delete_external((float*)img.data);
		}
		// LOG << "GMIC " << version;
		return version;
	}

	void init() {
		// LOG << "plugin global data init";
		if (initialized) {
			// LOG << "plugin global data init end - already initialized";
			return;
		}
		initLock.lock();
		try {
			if (initialized) {
				initLock.unlock();
				return;
			}
			string rc_path = get_gmic_rc_path();
			string gmicVersion = getGmicVersion(); // intToString(gmic_get_version())
			unsigned int nof = 0;
			gmic_interface_image img;
			gmic_interface_options options;
			memset(&options, 0, sizeof(gmic_interface_options));
			options.output_format = E_FORMAT_BYTE;
			options.no_inplace_processing = true;

			string updateFile = rc_path + "update" + gmicVersion + ".gmic";
			requestData r;
			string result;

			// check if filter selection file exists (this file defines what filters are reported to the host and in what categories)
			// if it doesn't exist, create a new file containing all the filters
			vector<string> selectList;
			string listFile = rc_path + "gmic_ofx.txt";
			string l = loadStringFromFile(listFile);
			if (l != "") {
				l = strReplace(l, "\r\n", "\n");
				l = strReplace(l, "\r", "\n");
				l = strReplace(l, "\t", "\a");
				strSplit(l, '\n', selectList, false);
			}

			// check if JSON file exists (either a custom one called gmic_ofx.json, or if not, the official updateXXX.json)
			string filterFile = rc_path + "gmic_ofx.json";
			if (!fileExists(filterFile)) filterFile = rc_path + "update" + gmicVersion + ".json";
			if (!fileExists(filterFile)) {
				
				r.url = "https://gmic.eu/update" + gmicVersion + ".json";				
				request(r, result);
				if (result != "") saveStringToFile(result, filterFile);

				r.url = "https://gmic.eu/update" + gmicVersion + ".gmic";
				request(r, result);
				if (result != "") saveStringToFile(result, updateFile);
			}

			string effectContent = loadStringFromFile(filterFile);
			if (!effectContent.empty()) {

				// parse the JSON into our internal structure
				parseFilters(effectContent, selectList, filters);
				gmicFilter filter = deserializeFilter(GMIC_CODE);
				filters.push_back(filter);

				// check if G'MIC update file exists, if not, download it
				// this is necessary since the JSON is built against the updated filter set
				if (!fileExists(updateFile)) {
					r.url = "https://gmic.eu/update" + gmicVersion + ".gmic";
					request(r, result);
					if (result != "") saveStringToFile(result, updateFile);
				}
			}
		} catch(...) {
		}
		initialized = true;
		initLock.unlock();
	}
#else
	gmicFilter filter;
#endif
};
GmicGlobalData gmicGlobalData;

class MyGlobalData {
public:
	MyGlobalData() { }
	~MyGlobalData() { }
};

class MySequenceData {
public:
	MySequenceData() { }
	~MySequenceData() { }
};

void* createCustomGlobalData() { return new MyGlobalData(); }
void destroyCustomGlobalData(void* customGlobalDataP) { delete (MyGlobalData*)customGlobalDataP; }
void* createCustomSequenceData() { return new MySequenceData(); }
void destroyCustomSequenceData(void* customSequenceDataP) { delete (MySequenceData*)customSequenceDataP; }
void* flattenCustomSequenceData(void* customUnflatSequenceDataP, int& flatSize) { return NULL; }
void* unflattenCustomSequenceData(void* customSequenceDataP, int flatSize) { return new MySequenceData(); }

int pluginSetdown(GlobalData* /*globalDataP*/, ContextData* /*contextDataP*/) { 
	void* exit_status = NULL;
	for (int i = 0; i < MAX_NOF_THREADS; i++) {
		if (gmicGlobalData.tdata[i].initialized && !gmicGlobalData.tdata[i].doExit) {
			gmicGlobalData.tdata[i].doExit = true;
			pthread_join(gmicGlobalData.tdata[i].tid, &exit_status);
			gmicGlobalData.tdata[i].initialized = false;
		}
	}
	return 0; 
}

int pluginSetup(GlobalData* globalDataP, ContextData* /*contextDataP*/) {
	globalDataP->scale = 255.f;
	globalDataP->buttonName = "Reload";

	MyGlobalData* myGlobalDataP = (MyGlobalData*)globalDataP->customGlobalDataP;
#ifdef OFX_PLUGIN
	gmicFilter filter = gmicGlobalData.filters[globalDataP->pluginIndex];
#else
	gmicFilter& filter = gmicGlobalData.filter;
#endif	

	globalDataP->inplaceProcessing = false;

#ifdef OFX_PLUGIN
	globalDataP->param[0] = Parameter(kOfxImageEffectSimpleSourceClipName, "", PT_LAYER);
#else
	globalDataP->param[0] = Parameter("Input", "", PT_LAYER);
#endif

	bool previewOn = false;
	int p = 1;
	string group;
	for (int i = 0; i < (int)filter.param.size(); i++) {
		int t = PT_FLOAT;
		int flags = 0;
		bool h = false;
		float d1 = 0.f;
		float d2 = 0.f;
		float d3 = 0.f;
		float d4 = 0.f;
		const gmicParameter& param = filter.param[i];
		std::string name = param.name;
		const string& minValue = param.minValue;
		const string& maxValue = param.maxValue;
		const string& defaultValue = param.defaultValue;
		string text = param.text;
		const string& paramType = param.paramType;
		if (paramType == "color") {
			vector<string> r;
			t = PT_COLOR;
			strSplit(defaultValue + "|||", '|', r, true);
			flags = 0;
			if (r.size() > 0) d1 = (float)atof(r[0].c_str()) / 255.f;
			if (r.size() > 1) d2 = (float)atof(r[1].c_str()) / 255.f;
			if (r.size() > 2) d3 = (float)atof(r[2].c_str()) / 255.f;
			if (r.size() > 3) { 
				d4 = (float)atof(r[3].c_str()) / 255.f;
				if (d4 == 0) d4 = 1.;
				flags = 1;
			}
		} else if (paramType == "point") {
			vector<string> r;
			t = PT_POINT;
			strSplit(defaultValue + "||", '|', r, true);
			d1 = (float)atof(r[0].c_str());
			d2 = (float)atof(r[1].c_str());
#ifdef OFX_PLUGIN
			// erase any occurence of " (%)" in the string, because the displayed parameter will be in pixels
			size_t start_pos = name.find(" (%)");
			if (start_pos != string::npos) {
				name.erase(start_pos, 4);
			}
#else
			d1 /= 100.f;
			d2 /= 100.f;
			flags = 1;
#endif
		} else {
			d1 = (float)atof(defaultValue.c_str());
			d2 = d1; d3 = d1; d4 = d1;
			if (paramType == "bool") {
				t = PT_BOOL;
			} else if (paramType == "button" || paramType == "unknown") {
				t = PT_BUTTON;
			} else if (paramType == "choice") {
				t = PT_SELECT;
				if (strLowercase(name) == "preview type") previewOn = true;
			} else if (paramType == "value") {
				h = true;
				t = PT_CONST;
			} else if (paramType == "file" || paramType == "fileout") {
				t = PT_TEXT;
				flags = 2;
			} else if (paramType == "filein") {
				t = PT_TEXT;
				text = defaultValue;
				flags = 5;
			} else if (paramType == "float") {
				t = PT_FLOAT;
			} else if (paramType == "folder") {
				t = PT_TEXT;
				text = defaultValue;
				flags = 3;
			} else if (paramType == "int") {
				t = PT_INT;
			} else if (paramType == "note" || paramType == "link") {
				string note = strRemoveXmlTags(strTrim(text, " \t\r\n'\""));
				note = note.substr(0, 100);
				bool validParamAhead = false;
				for (int j = i + 1; j < filter.param.size(); j++) {
					if (filter.param[j].paramType != "note" && filter.param[j].paramType != "link" && filter.param[j].paramType != "separator") {
						validParamAhead = true;
						break;
					}
				}
				if (paramType == "note" && group == "" && i > 0 && validParamAhead && filter.param[i-1].paramType == "separator") {
					group = note;
					t = PT_TOPIC_START;
					name = note;
				} else if ((int)text.find("span fore") > 0) {
					if (group != "") {
						globalDataP->param[p] = Parameter(group, "", PT_TOPIC_END);
						p++;
					}
					group = note;
					t = PT_TOPIC_START;
					name = note;
				} else {
					flags = 4;
					filter.notes += text + "<br>";//note + '\n';
					h = true;
					t = PT_NONE;
				}
			} else if (paramType == "text") {
				text = defaultValue;
				t = PT_TEXT;
			} else if (paramType == "separator") {
				if (group != "") {
					// if we are currently in a group, close the group
					t = PT_TOPIC_END;
					name = group;
					group = "";
				} else {
					t = PT_SEPARATOR;
				}
			} else if (paramType == "input") {
				t = PT_LAYER;
			} else {
				assert(false);
			}
		}

		if (t != PT_NONE) {
			globalDataP->param[p] = Parameter(name, "", t, (float)atof(minValue.c_str()), (float)atof(maxValue.c_str()), d1, d2, d3, d4, text);
			globalDataP->param[p].flags = flags;
			if (h) globalDataP->param[p].displayStatus = DS_HIDDEN;
			p++;
		}
	}

	if (group != "") {
		globalDataP->param[p] = Parameter(group, "", PT_TOPIC_END);
		p++;
	}

	if (filter.multiLayer) {
		globalDataP->param[p] = Parameter("Add. Layer 1", "", PT_LAYER);
		++p;
		globalDataP->param[p] = Parameter("Add. Layer 2", "", PT_LAYER);
		++p;
		globalDataP->param[p] = Parameter("Add. Layer 3", "", PT_LAYER);
		++p;
		globalDataP->param[p] = Parameter("Add. Layer 4", "", PT_LAYER);
		++p;
	}	
	globalDataP->param[p] = Parameter("Command", "", PT_TEXT, 0, 0, 0, 0, 0, 0, "-blur 2");
#ifndef FREI0R_PLUGIN
	if (strTrim(filter.name) != "G'MIC") globalDataP->param[p].displayStatus = DS_HIDDEN;
#endif
	++p;

	globalDataP->param[p] = Parameter("Advanced Options", "", PT_TOPIC_START);
	globalDataP->param[p].flags = 1;
	++p;
	globalDataP->param[p] = Parameter("Output Layer", "", PT_SELECT, 0, 10, 10, 0, 0, 0, "Merged|Layer 0 (Bottom)|Layer 1|Layer 2|Layer 3|Layer 4|Layer 5|Layer 6|Layer 7|Layer 8|Layer 9 (Top)|");
	++p;
	globalDataP->param[p] = Parameter("Resize Mode", "", PT_SELECT, 0, 5, 1, 0, 0, 0, "Fixed (Inplace)|Dynamic|Downsample 1/2|Downsample 1/4|Downsample 1/8|Downsample 1/16");
	++p;
	globalDataP->param[p] = Parameter("Ignore Alpha", "", PT_BOOL, 0, 1, 1, 0, 0, 0, "");
	++p;
	globalDataP->param[p] = Parameter("Preview/Draft Mode", "", PT_BOOL, 0, 1, previewOn?1.f:0.f, 0, 0, 0, "");
	if (filter.command == filter.preview_command) {
		globalDataP->param[p].displayStatus = DS_HIDDEN;
	}
	++p;
	globalDataP->param[p] = Parameter("Global Random Seed", "", PT_INT, 0, 1<<24, 0, 0, 0, 0, "");
	++p;
	globalDataP->param[p] = Parameter("Animate Random Seed", "", PT_BOOL, 0, 1, 0, 0, 0, 0, "");
	++p;
	globalDataP->param[p] = Parameter("Log Verbosity", "", PT_SELECT, 0, 4, 0, 0, 0, 0, "Off|Level 1|Level 2|Level 3|Level 4|Level 5|Level 6|Level 7|Level 8|Level 9|Level 10");
	++p;
	globalDataP->param[p] = Parameter("Advanced Options", "", PT_TOPIC_END);
	++p;
	globalDataP->nofParams = p;

	string d = filter.notes;
	/*
	d = strRemoveXmlTags(d, true);
	d = strReplace(d, "<", "(");
	d = strReplace(d, ">", ")");
	*/
	d = strReplace(d, "\xc2\xa0", " ");
	d = strReplace(d, "  ", " ");
	d = strReplace(d, "\n ", "\n");
	d = strReplace(d, "\r ", "\n");
	d = strReplace(d, "\\n ", "\\n");
	d = strReplace(d, "\\r ", "\\n");
	globalDataP->pluginInfo.description = d + "\\n\\n" + c2s(PLUGIN_DESCRIPTION);

	return 0;
}

static string gmicCommand(WorldData* worldDataP, SequenceData* sequenceDataP, GlobalData* globalDataP) {
	MyGlobalData* myGlobalDataP = (MyGlobalData*)globalDataP->customGlobalDataP;
#ifdef OFX_PLUGIN
	string cmd = PAR_VAL(PARAM_PREVIEW) > 0 ? gmicGlobalData.filters[globalDataP->pluginIndex].preview_command:gmicGlobalData.filters[globalDataP->pluginIndex].command;
#else
	string cmd = PAR_VAL(PARAM_PREVIEW) > 0 ? gmicGlobalData.filter.preview_command:gmicGlobalData.filter.command;
#endif
	// LOG << "gmicCommand *" << cmd << "*" << gmicGlobalData.filter.preview_command << "*" << gmicGlobalData.filter.command;
	if (cmd == "") {
		cmd = PAR_TXT(PARAM_COMMAND);
		for (int i = 0; i < 8; i++) {
			float f = PAR_VAL(i + 1);
			string p = floatToString(f);
			string t = "A";
			t[0] += i;
			cmd = strReplace(cmd, "$" + t, p);
		}
	} else {
		cmd = "-" + strTrim(cmd, " \t\r\n") + " ";
		for (int i = 1; i < globalDataP->nofParams - 10; i++) {
//if ((int)(strLowercase(globalDataP->param[i].paramName).find("preview")) >= 0) break;
//			LOG << "param " << i << ": " << PAR_TYPE(i) << ", " << globalDataP->param[i].paramName << ", " << PAR_VAL(i);
			if (PAR_TYPE(i) == PT_INT || PAR_TYPE(i) == PT_BOOL || PAR_TYPE(i) == PT_SELECT) {
				cmd += intToString((int)PAR_VAL(i)) + ",";
			} else if (PAR_TYPE(i) == PT_FLOAT) {
				cmd += floatToString(PAR_VAL(i)) + ",";
			} else if (PAR_TYPE(i) == PT_NONE) {
				continue;
			} else if (PAR_TYPE(i) == PT_TEXT) {
				if (i == PARAM_COMMAND && globalDataP->param[PARAM_COMMAND].displayStatus == DS_HIDDEN) continue;
				if (globalDataP->param[i].flags == 4) {
					// label/note
					continue;
				}
				string s = PAR_TXT(i);
				cmd += "\"" + s + "\",";
			} else if (PAR_TYPE(i) == PT_POINT) {
//				LOG << "point: " << i << " " << globalDataP->param[i].flags << " " << PAR_CH(i, 0) << " " << worldDataP->outWorld.width;
				if (globalDataP->param[i].flags == 1) {
					float x = PAR_CH(i, 0);
					float y = PAR_CH(i, 1);
#ifdef OFX_PLUGIN
					if (x > 1.f) x /= (worldDataP->outWorld.width / worldDataP->downsample_x);
					if (y > 1.f) y /= (worldDataP->outWorld.height / worldDataP->downsample_y);
#else
					if (x > 1.f) x /= worldDataP->outWorld.width;
					if (y > 1.f) y /= worldDataP->outWorld.height;
#endif

#if defined(IMG_FLIP_Y) && defined(OFX_PLUGIN)
					y = ((y - 0.5f) * -1.f) + 0.5f;
#endif
					cmd += 
						floatToString((100.f * x)) + "," +
						floatToString((100.f * y)) + ",";
				} else {
					cmd += 
						floatToString((PAR_CH(i, 0))) + "," +
						floatToString((PAR_CH(i, 1))) + ",";
				}
			} else if (PAR_TYPE(i) == PT_COLOR) {
				cmd += 
					intToString((int)(255.f * PAR_CH(i, 0))) + "," +
					intToString((int)(255.f * PAR_CH(i, 1))) + "," +
					intToString((int)(255.f * PAR_CH(i, 2))) + ",";
				if (globalDataP->param[i].flags == 0) {
					cmd += "255,";
				} else {
					cmd += intToString((int)(255.f * PAR_CH(i, 3))) + ",";
				}
			} else if (PAR_TYPE(i) == PT_CONST) {
				vector<string> r;
				string s = PAR_TXT(i);
				cmd += s + ",";
			}
		}
		cmd = cmd.substr(0, cmd.size() - 1);
		cmd = strReplace(cmd, "\r", " ");
		cmd = strReplace(cmd, "\n", " ");
		for (int i = 0; i < cmd.size(); i++) {
			if (cmd[i] < 32) cmd[i] = 32;
		}
	}
	return cmd;
}

int pluginParamChange(int index, SequenceData* sequenceDataP, GlobalData* globalDataP, ContextData* /*contextDataP*/) {
	if (index == PARAM_RESIZE) {
		globalDataP->inplaceProcessing = PAR_VAL(index) < 1.f;
	}
	return 0;
}

// #define pmtx
#ifdef pmtx
mutex proc_mtx;
#endif
mutex gmic_cmd_mtx;

void* runCmd(void* arg) {
	ThreadData* td = (ThreadData*)arg;
	do {
		if (td->doProcess) {
//			printf("start cmd %s\n", td->cmd.c_str());
//			LOG << "START " << td->cmd;
			gmic_cmd_mtx.lock();
			try {
				td->result = gmic_call(td->cmd.c_str(), &(td->nofImages), &(td->images[0]), &(td->options));
			} catch(...) {
			}
			gmic_cmd_mtx.unlock();
//			LOG << "STOP " << td->cmd;
//			printf("stop cmd %s, result = %d\n", td->cmd.c_str(), td->result);
			td->doProcess = false;
		}
		Sleep(100);
	} while (!td->doExit);
	return NULL;
}

int pluginProcess(WorldData* worldDataP, SequenceData* sequenceDataP, GlobalData* globalDataP, ContextData* contextDataP) {	
	int err = 0;
	bool abort = false;
	int index = -1;
#ifdef pmtx
	proc_mtx.lock();
#endif
	try {

	// MyGlobalData* myGlobalDataP = (MyGlobalData*)globalDataP->customGlobalDataP;

	do {  
//		gmicGlobalData.threadlock.lock();
		for (int j = 0; j < MAX_NOF_THREADS; j++) {
//			if (!gmicGlobalData.tdata[j].busy) {
			if (!gmicGlobalData.busy[j]) {
				index = j;
				gmicGlobalData.busy[j] = true;
//				gmicGlobalData.tdata[j].busy = true;
				break;
			}
		}
//		gmicGlobalData.threadlock.unlock();
		if (index < 0) Sleep(100);
	} while (index < 0);

	ThreadData& td = gmicGlobalData.tdata[index];
	td.options.no_inplace_processing = !globalDataP->inplaceProcessing;
	td.options.p_is_abort = &abort;
	td.nofImages = 0;
	for (int i = 0; i < globalDataP->nofInputs; i++) {
		if (!worldDataP->inWorld[i].data || !sequenceDataP->inputConnected[i]) break;
		td.nofImages++;
	}
	if (td.nofImages == 0) {
		/*
		gmicGlobalData.threadlock.lock();
		td.busy = false;
		gmicGlobalData.threadlock.unlock();
		*/
		gmicGlobalData.busy[index] = false;
#ifdef pmtx
		proc_mtx.unlock();
#endif
		return err;
	}

	for (unsigned int i = 0; i < td.nofImages; i++) {
		td.images[i].width = worldDataP->inWorld[i].width;
		td.images[i].height = worldDataP->inWorld[i].height;
		td.images[i].data = worldDataP->inWorld[i].data;
		td.images[i].spectrum = 4;
		td.images[i].depth = 1;
		td.images[i].name[0] = '\0';
#ifdef FREI0R_PLUGIN
		td.images[i].format = E_FORMAT_BYTE;
		td.images[i].is_interleaved = true;
#else
		td.images[i].format = E_FORMAT_FLOAT;
		td.images[i].is_interleaved = false;
#endif
		strncpy(td.images[i].name, string("input" + intToString(i)).c_str(), sizeof(td.images[i].name));
	}

	td.cmd = gmicCommand(worldDataP, sequenceDataP, globalDataP);
	if (PAR_VAL(PARAM_PREVIEW) > 0) {
		td.cmd = "_preview_area_width,_preview_area_height:=w,h " + td.cmd;
	}

	string rs = intToString(worldDataP->inWorld[0].width) + "," + intToString(worldDataP->inWorld[0].height);
	if (PAR_VAL(PARAM_NOALPHA) > 0.f) {
		td.cmd = strReplace(td.cmd, "\"", "\\\"");
		td.cmd = "apply_channels \"" + td.cmd + "\",rgb";
//		td.cmd = "apply_channels \"" + td.cmd + " resize " + rs  + "\",rgb";
	}

	if (!globalDataP->inplaceProcessing) {
		float ds = PAR_VAL(PARAM_RESIZE);
		if (ds >= 2.f) {
			ds = pow(2.f, ds - 1.f);
			td.cmd = " resize " + intToString(worldDataP->inWorld[0].width / (int)ds) + "," + intToString(worldDataP->inWorld[0].height / (int)ds) + " " + td.cmd;
		}
		td.cmd += " resize " + rs;
	}

	int verbosity = (int)PAR_VAL(PARAM_VERBOSITY) - 1;
	int seed = (int)PAR_VAL(PARAM_SRAND);
	int animated_seed = (int)PAR_VAL(PARAM_ANIMSEED);
	if (animated_seed != 0) {
		seed += (int)worldDataP->time;
	}
	td.options.use_logfile = verbosity > 0;
	td.cmd = "-v " + intToString(verbosity) + " -srand " + intToString(seed) + " " + td.cmd;
	if (PAR_VAL(PARAM_OUTPUT) == 0.f) {
		td.cmd += " -gui_merge_layers";
	}

#ifdef SINGLE_THREADED
	td.doProcess = false;
	td.result = gmic_call(td.cmd.c_str(), &(td.nofImages), &(td.images[0]), &(td.options));
#else
	if (!td.initialized) {
		td.initialized = true;
		pthread_attr_t tattr;
		size_t size = 16000000;
		int ret = pthread_attr_init(&tattr);
		ret = pthread_attr_setstacksize(&tattr, size);
		ret = pthread_create(&td.tid, &tattr, &runCmd, &td);
		pthread_attr_destroy(&tattr);
	}
	td.doProcess = true;

	do {
#if defined OFX_PLUGIN
		if (!abort && gEffectHost->abort(contextDataP->instance)) {
			abort = true;
//			LOG << "ABORT!";
//			printf("ABORT!\n");
			// break;
		}
#elif defined AE_PLUGIN
		if (!abort && PF_ABORT(contextDataP->in_data) != 0) {
			abort = true; 
//			LOG << "ABORT!";
//			printf("ABORT!\n");
			// break;
		}
#endif
		Sleep(50);
	} while (td.doProcess);
#endif

	string msg = string(td.options.error_message_buffer);
	if (!abort) {
		if (td.result == 0) {
			if (!globalDataP->inplaceProcessing) {
				int idx = (int)PAR_VAL(PARAM_OUTPUT);
				if (idx > 0) {
					idx -= 1;
					//idx = td.nofImages - idx;
					//if (idx < 0) idx = 0;
					if (idx >= (int)td.nofImages) idx = td.nofImages - 1;
				}

				if ((unsigned)worldDataP->outWorld.width == td.images[idx].width && (unsigned)worldDataP->outWorld.height == td.images[idx].height) {
					size_t sizePixels = worldDataP->outWorld.width * worldDataP->outWorld.height;
					unsigned char* src = (unsigned char*)td.images[idx].data;
					unsigned char* dst = (unsigned char*)worldDataP->outWorld.data;

					#ifdef FREI0R_PLUGIN
					if (td.images[idx].spectrum == 1) { // copy r to g/b, set a = 1
						for (int i = 0; i < sizePixels; i++) {
							unsigned char r = *src++;
							*dst++ = r;
							*dst++ = r;
							*dst++ = r;
							*dst++ = 255;
						}
					} else if (td.images[idx].spectrum == 2) { // set a = g, copy r to g/b
						for (int i = 0; i < sizePixels; i++) {
							unsigned char r = *src++;
							unsigned char g = *src++;
							*dst++ = r;
							*dst++ = r;
							*dst++ = r;
							*dst++ = g;
						}
					} else if (td.images[idx].spectrum == 3) { // set a = 1
						for (int i = 0; i < sizePixels; i++) {
							unsigned char r = *src++;
							unsigned char g = *src++;
							unsigned char b = *src++;
							*dst++ = r;
							*dst++ = g;
							*dst++ = b;
							*dst++ = 255;
						}
					} else {
						memcpy(dst, src, sizePixels * 4);
					}
					#else
					size_t sizeBytes = sizePixels * sizeof(float);
					if (td.images[idx].spectrum == 1) { // copy s.r to d.g/d.b, set d.a = 1
						float* p = (float*)dst; // d.r
						memcpy(p, src, sizeBytes); // s.r->d.r
						p = (float*)dst + sizePixels; // d.g
						memcpy(p, src, sizeBytes); // s.r->d.g
						p = (float*)dst + sizePixels * 2; // d.b
						memcpy(p, src, sizeBytes); // s.r->d.b
						p = (float*)dst + sizePixels * 3; // d.a
						std::fill(p, p + sizePixels, 255.f); // d.a = 1
					} else if (td.images[idx].spectrum == 2) { // copy s.r to d.g/d.b, set d.a = s.g, 
						float* p = (float*)dst; // d.r
						memcpy(p, src, sizeBytes); // s.r->d.r
						p = (float*)dst + sizePixels; // d.g
						memcpy(p, src, sizeBytes); // s.r->d.g
						p = (float*)dst + sizePixels * 2; // d.b
						memcpy(p, src, sizeBytes); // s.r->d.b
						p = (float*)dst + sizePixels * 3; // d.a
						float* p2 = (float*)src + sizePixels; // s.g
						memcpy(p, p2, sizeBytes); // s.g->d.a
					} else if (td.images[idx].spectrum == 3) { // set a = 1
						memcpy(dst, src, sizeBytes * 3);
						float* p = (float*)dst + sizePixels * 3; // d.a
						std::fill(p, p + sizePixels, 255.f); // d.a = 1
					} else {
						memcpy(dst, src, sizeBytes * 4);
					}
					#endif
				} else {
					td.result = -1;
					msg = "The image produced by G'MIC has the wrong size.";
				}
			}
		}
		if (td.result != 0) {
//			LOG << msg;
#if defined OFX_PLUGIN
			if (gMessageSuite) {
				gMessageSuite->message(contextDataP->instance, kOfxMessageError, "G'MIC Error", msg.c_str());
			} else {
				cout << "ERROR: " << msg << endl;
			}
#elif defined AE_PLUGIN
			msg = msg.substr(0, 255);
			strcpy(contextDataP->out_data->return_msg, msg.c_str());
			err = PF_Err_INTERNAL_STRUCT_DAMAGED;
#endif
		}
	}

	for (unsigned int i = 0; i < td.nofImages; i++) {
		bool found = false;
		for (int j = 0; j < globalDataP->nofInputs; j++) {
			if (td.images[i].data == worldDataP->inWorld[j].data) {
				found = true;
				break;
			}
		}
		if (!found) gmic_delete_external((float*)td.images[i].data);
	}
	} catch(...) {
	}
/*
	gmicGlobalData.threadlock.lock();
	td.busy = false;
	gmicGlobalData.threadlock.unlock();
*/
	gmicGlobalData.busy[index] = false;
#ifdef pmtx
	proc_mtx.unlock();
#endif
	if (abort) return -1;
	return err;
}

#if defined OFX_PLUGIN

void getPluginInfo(int pluginIndex, PluginInfo& pluginInfo) {
	string n = gmicGlobalData.filters[pluginIndex].name;
	n = strReplace(n, " & ", " And ");
	pluginInfo.name = strTrim(n);
	pluginInfo.identifier = strTrim(gmicGlobalData.filters[pluginIndex].uniqueId);
	
	n = gmicGlobalData.filters[pluginIndex].category;
	n = strReplace(n, " & ", " And ");
	pluginInfo.category = strTrim(n);

	pluginInfo.description = strTrim(gmicGlobalData.filters[pluginIndex].notes + " \n \n" + c2s(PLUGIN_DESCRIPTION));
	pluginInfo.major_version = PLUGIN_MAJOR_VERSION;
	pluginInfo.minor_version = PLUGIN_MINOR_VERSION;
}

int getNofPlugins() {
	gmicGlobalData.init();
	return (int)gmicGlobalData.filters.size();
}

#elif defined AE_PLUGIN

extern "C" DllExport PF_Err EntryPointFunc(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* param[], PF_LayerDef* outputP, void* extraP) {
	if (cmd == PF_Cmd_DO_DIALOG) {
		GlobalData* globalDataP = ((AE_GlobalData*)PF_LOCK_HANDLE(in_data->global_data))->globalDataP;
		pluginSetup(globalDataP, NULL);
		PF_UNLOCK_HANDLE(in_data->global_data);
		return PF_Err_NONE;
	} else {
		return pluginMain(cmd, in_data, out_data, param, outputP, extraP); 
	}
};

#elif defined FREI0R_PLUGIN

	
#endif
