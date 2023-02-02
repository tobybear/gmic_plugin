/*
 #
 #  File        : gmic_plugin.h
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
#pragma once

// version, name and description for the plugin
#define	PLUGIN_MAJOR_VERSION		3
#define	PLUGIN_MINOR_VERSION		0
#define	PLUGIN_BUILD_VERSION		0
#define PLUGIN_DESCRIPTION	"Wrapper for the G'MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com) and Frederic Devernay."

// AE plugin needs these with spaces to replace them with proper effect names in the batch tool!
#define PLUGIN_NAME					"G'MIC Generic Plugin           "
#define PLUGIN_CATEGORY				"G'MIC                          "
#define PLUGIN_UNIQUEID				"eu.gmic.gmic_generic           "

//#define PLUGIN_NAME					"##GMIC_NAME####################"
//#define PLUGIN_CATEGORY				"##GMIC_CAT#####################"
//#define PLUGIN_UNIQUEID				"##GMIC_ID######################"

#define MULTITHREADED_SEQDATA

#define	_PF_OutFlag2_AE13_5_THREADSAFE 4194304
#define _PF_OutFlag2_SUPPORTS_GET_FLATTENED_SEQUENCE_DATA 8388608
#define _PF_OutFlag2_SUPPORTS_THREADED_RENDERING 134217728

#ifdef MULTITHREADED_SEQDATA
#define _PF_OutFlag2_MUTABLE_RENDER_SEQUENCE_DATA_SLOWER 0
#else
#define _PF_OutFlag2_MUTABLE_RENDER_SEQUENCE_DATA_SLOWER 268435456
#endif

#define FX_OUT_FLAGS PF_OutFlag_NON_PARAM_VARY + PF_OutFlag_DEEP_COLOR_AWARE + PF_OutFlag_I_DO_DIALOG + PF_OutFlag_SEND_UPDATE_PARAMS_UI + PF_OutFlag_USE_OUTPUT_EXTENT + PF_OutFlag_CUSTOM_UI
#define FX_OUT_FLAGS2 PF_OutFlag2_SUPPORTS_SMART_RENDER + PF_OutFlag2_FLOAT_COLOR_AWARE + PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG + _PF_OutFlag2_AE13_5_THREADSAFE + _PF_OutFlag2_SUPPORTS_GET_FLATTENED_SEQUENCE_DATA + _PF_OutFlag2_SUPPORTS_THREADED_RENDERING 
//+ _PF_OutFlag2_MUTABLE_RENDER_SEQUENCE_DATA_SLOWER

#define MAX_NOF_THREADS 4

// generic includes
#ifndef PIPL

#if !defined(OFX_PLUGIN) && !defined(AE_PLUGIN) && !defined(FREI0R_PLUGIN)
#error No target plugin SDK defined!
#endif

#ifdef WIN32
#include <Windows.h>
#include <ShlObj.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#endif

#include <cassert>
#include <cmath>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
using namespace std;

#include "gmic_libc.h"
#include "RFX_StringUtils.h"
#include "RFX_Parameter.h"
#include "gmic_utils.h"
using namespace reduxfx;

// #include "..\..\ThirdParty\TLog\TLog.h"

// specific settings and includes for OFX SDK
#ifdef OFX_PLUGIN
#define IMG_FLIP_Y
#define CONVERT_INT_TO_FLOAT
#define PLANAR_BUFFER
#include "RFX_OFX_Utils.h"
#include "webrequest.h"
#endif

// specific settings and includes for Adobe After Effects/Premiere Pro SDK
#ifdef AE_PLUGIN
#define CONVERT_INT_TO_FLOAT
#define PLANAR_BUFFER
#include "RFX_AE_Utils.h"
#endif

// specific settings and includes for frei0r SDK
#ifdef FREI0R_PLUGIN
GlobalData frei0rGlobalData;
#include "RFX_frei0r_Utils.h"
#endif

// include placed at the end here as pthreads unfortunately redefines int64 types, which leads to conflicts with other libraries on Windows
#include <pthread.h> 
#endif

