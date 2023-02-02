/*
 #
 #  File        : RFX_frei0r_Utils.h
 #
 #  Description : A self-contained header file with helper functions to make    
 #                the frei0r SDK a bit easier to use with plugins
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

#pragma once

#include <cmath>
#include "frei0r.h"
#include <cstdlib>
#include <string>
#include "RFX_Parameter.h"
using namespace std;
using namespace reduxfx;

struct ContextData {
	void* extraP;
};

extern GlobalData frei0rGlobalData;

extern void* createCustomGlobalData();
extern void destroyCustomGlobalData(void* customGlobalDataP);
extern void* createCustomSequenceData();
extern void destroyCustomSequenceData(void* customSequenceDataP);
extern void* flattenCustomSequenceData(void* customUnflatSequenceDataP, int& flatSize);
extern void* unflattenCustomSequenceData(void* customFlatSequenceDataP, int flatSize);

extern int pluginParamChange(int index, SequenceData* sequenceDataP, GlobalData* globalDataP, ContextData* contextDataP);
extern int pluginSetup(GlobalData* globalDataP, ContextData* contextDataP);
extern int pluginSetdown(GlobalData* globalDataP, ContextData* contextDataP);
extern int pluginProcess(WorldData* worldDataP, SequenceData* sequenceDataP, GlobalData* globalDataP, ContextData* contextDataP);

// private instance data type
struct MyInstanceData {
	int width, height;
	SequenceData* sequenceDataP;
};

int f0r_init() {
	pluginSetup(&frei0rGlobalData, NULL);
	frei0rGlobalData.nofInputs = 1;
	return 1;
}

void f0r_deinit() {
	pluginSetdown(&frei0rGlobalData, NULL);
}

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name = "gmic";//sfrei0rGlobalData.pluginInfo.name.c_str();
	info->author = "";
	info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
	info->color_model = F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version = FREI0R_MAJOR_VERSION;
	info->major_version = frei0rGlobalData.pluginInfo.major_version;
	info->minor_version = frei0rGlobalData.pluginInfo.major_version;
	info->explanation = frei0rGlobalData.pluginInfo.description.c_str();
	int numParams = 0;
	for (int i = 0; i < frei0rGlobalData.nofParams; i++) {
		int pt = frei0rGlobalData.param[i].paramType;
		if (pt == PT_BOOL || pt == PT_SELECT || pt == PT_INT || pt == PT_FLOAT || pt == PT_COLOR || pt == PT_POINT || pt == PT_TEXT || pt == PT_CONST) numParams++;
	}
	info->num_params = numParams;
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	int numParams = -1;
	for (int i = 0; i < frei0rGlobalData.nofParams; i++) {
		int pt = frei0rGlobalData.param[i].paramType;
		int ptype = -1;
		if (pt == PT_BOOL) ptype = F0R_PARAM_BOOL;
		else if (pt == PT_SELECT || pt == PT_INT || pt == PT_FLOAT || pt == PT_CONST) ptype = F0R_PARAM_DOUBLE;
		else if (pt == PT_COLOR) ptype = F0R_PARAM_COLOR;
		else if (pt == PT_POINT) ptype = F0R_PARAM_POSITION;
		else if (pt == PT_TEXT) ptype = F0R_PARAM_STRING;
		if (ptype >= 0) numParams++;
		if (numParams == param_index) {
			info->name = frei0rGlobalData.param[i].paramName.c_str();
			info->type = ptype;
			info->explanation = "";
			return;
		}
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	// make private instance data
	MyInstanceData* myData = new MyInstanceData;
	myData->sequenceDataP = new SequenceData();
	for (int i = 0; i < frei0rGlobalData.nofParams; i++) {
		myData->sequenceDataP->textValue[i] = frei0rGlobalData.param[i].text;
		for (int j = 0; j < 4; j++) {
			myData->sequenceDataP->floatValue[i][j] = frei0rGlobalData.param[i].defaultValue[j];
		}
	}
	myData->sequenceDataP->contextP = NULL;
	myData->sequenceDataP->inputConnected[0] = true;
	myData->width = width;
	myData->height = height;
	return (f0r_instance_t)myData;
}

void f0r_destruct(f0r_instance_t instance) {
	// get my instance data
	MyInstanceData* myData = (MyInstanceData*)instance;
	if (myData && myData->sequenceDataP) {
		if (myData->sequenceDataP->customSequenceDataP) {
			destroyCustomSequenceData(myData->sequenceDataP->customSequenceDataP);
			myData->sequenceDataP->customSequenceDataP = NULL;
		}
		delete (SequenceData*)myData->sequenceDataP;
		myData->sequenceDataP = NULL;
	}

	// and delete it
	if (myData) delete myData;
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	MyInstanceData* myData = (MyInstanceData*)instance;
	int realIndex = -1;
	for (int i = 0; i < frei0rGlobalData.nofParams; i++) {
		int pt = frei0rGlobalData.param[i].paramType;
		if (pt == PT_BOOL || pt == PT_SELECT || pt == PT_INT || pt == PT_FLOAT || pt == PT_COLOR || pt == PT_POINT || pt == PT_TEXT || pt == PT_CONST) realIndex++;
		if (realIndex == param_index) {
			if (pt == PT_BOOL || pt == PT_SELECT || pt == PT_INT || pt == PT_FLOAT || pt == PT_CONST) {
				myData->sequenceDataP->floatValue[i][0] = *((double*)param);
			} else if (pt == PT_COLOR) {
				f0r_param_color_t c = *((f0r_param_color_t*)param);
				myData->sequenceDataP->floatValue[i][0] = c.r / 255.f;
				myData->sequenceDataP->floatValue[i][1] = c.g / 255.f;
				myData->sequenceDataP->floatValue[i][2] = c.b / 255.f;
				myData->sequenceDataP->floatValue[i][3] = 1.f;
			} else if (pt == PT_POINT) {
				f0r_param_position_t p = *((f0r_param_position_t*)param);
				myData->sequenceDataP->floatValue[i][0] = p.x;
				myData->sequenceDataP->floatValue[i][1] = p.y;
			} else if (pt == PT_TEXT) {
				char* sval = (*(char**)param);
				myData->sequenceDataP->textValue[i] = string(sval);
			}
			return;
		}
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	MyInstanceData* myData = (MyInstanceData*)instance;
	int realIndex = -1;
	for (int i = 0; i < frei0rGlobalData.nofParams; i++) {
		int pt = frei0rGlobalData.param[i].paramType;
		if (pt == PT_BOOL || pt == PT_SELECT || pt == PT_INT || pt == PT_FLOAT || pt == PT_COLOR || pt == PT_POINT || pt == PT_TEXT || pt == PT_CONST) realIndex++;
		if (realIndex == param_index) {
			if (pt == PT_BOOL || pt == PT_SELECT || pt == PT_INT || pt == PT_FLOAT || pt == PT_CONST) {
				*((double*)param) = myData->sequenceDataP->floatValue[i][0];
			} else if (pt == PT_COLOR) {
				f0r_param_color_t c;
				c.r = myData->sequenceDataP->floatValue[i][0] * 255;
				c.g = myData->sequenceDataP->floatValue[i][1] * 255;
				c.b = myData->sequenceDataP->floatValue[i][2] * 255;
				*((f0r_param_color_t*)param) = c; 
			} else if (pt == PT_POINT) {
				f0r_param_position_t p;
				p.x = myData->sequenceDataP->floatValue[i][0];
				p.y = myData->sequenceDataP->floatValue[i][1];
				*((f0r_param_position_t*)param) = p;
			} else if (pt == PT_TEXT) {
				*((char**)param) = (char*)myData->sequenceDataP->textValue[i].c_str();
			}
			return;
		}
	}
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe) {
	MyInstanceData* myData = (MyInstanceData*)instance;
	WorldData wd;
	wd.inWorld[0].bitDepth = 8;
	wd.inWorld[0].width = myData->width;
	wd.inWorld[0].height = myData->height;
	wd.inWorld[0].pixelFormat = PF_RGBA;
	wd.inWorld[0].rowBytes = myData->width * 4;
	wd.outWorld = wd.inWorld[0];
	wd.inWorld[0].data = (void*)inframe;
	wd.outWorld.data = (void*)outframe;
	int procResult = pluginProcess(&wd, myData->sequenceDataP, &frei0rGlobalData, NULL);
}
