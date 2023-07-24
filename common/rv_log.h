// rv_log.h  --  Ravenna Project

#ifndef RV_LOG_H
#define RV_LOG_H


#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
	void rv_log__(unsigned int log_level, char const* filename, int line, char const *format, ...);
	void log4c_log__(char const* device_category, unsigned int log_level, char const* filename, int line, char const *format, ...);

	typedef void (*rv_log_callback)(unsigned int log_level, char const* filename, int line, char const* text);
	void rv_log_set_callback__(rv_log_callback pCallback);

	#ifdef WIN32
		extern int g_bANSI_Enabled;
		void rv_log_add_console();
		int rv_log_to_file(char const* pFileName, bool append = false);
	#endif
	//extern char cColors[9][13];
#ifdef __cplusplus
}	
#endif

// to make the preprocessor directives as __LINE__ interpreted
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#ifdef WIN32
#define __RV_FILE_NAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)
#else
#define __RV_FILE_NAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)
#endif
//#define AT "In " __FILE__ ", line " TOSTRING(__LINE__) ": "


/*#ifdef __linux__
#include <syslog.h>

#define rv_log(log_level, ...) \
	{syslog(log_level, AT __VA_ARGS__);}
#define rv_log2(log_level, ...) \
		{;}
//    {syslog(log_level, __VA_ARGS__);}

#else // not __linux__*/

#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */
#define LOG_TRACE		8


// LOG4C emulation
#define LOG4C_PRIORITY_FATAL	LOG_EMERG	/** fatal */	
#define LOG4C_PRIORITY_ALERT	LOG_ALERT	/** alert */	
#define LOG4C_PRIORITY_CRIT		LOG_CRIT	/** crit */	    
#define LOG4C_PRIORITY_ERROR	LOG_ERR		/** error */	
#define LOG4C_PRIORITY_WARN		LOG_WARNING	/** warn */	    
#define LOG4C_PRIORITY_NOTICE	LOG_NOTICE	/** notice */	
#define LOG4C_PRIORITY_INFO		LOG_INFO	/** info */	    
#define LOG4C_PRIORITY_DEBUG	LOG_DEBUG	/** debug */	
#define LOG4C_PRIORITY_TRACE	LOG_TRACE	/** trace */
#define LOG4C_PRIORITY_NOTSET	9			/** notset */	
#define LOG4C_PRIORITY_UNKNOWN	10			/** unknown */

#define LOG_MAX_LEVEL LOG_DEBUG


#if UNDER_RTSS
	#include "MTAL_DP.h"
	#define rv_log(log_level, ...) { if(log_level <= LOG_MAX_LEVEL) { MTAL_DP("In %s, line %i: ", __RV_FILE_NAME__, __LINE__); MTAL_DP(__VA_ARGS__); }}
	#define log4c_log(device_category, log_level, ...) { if(log_level <= LOG_MAX_LEVEL) { MTAL_DP("In %s, line %i: ", __RV_FILE_NAME__, __LINE__);  MTAL_DP(__VA_ARGS__); MTAL_DP("\r\n"); }}

#else
	#define rv_log(log_level, format, ...)  rv_log__(log_level, __RV_FILE_NAME__, __LINE__, format, ##__VA_ARGS__)
	#define log4c_log(device_category, log_level, format, ...) log4c_log__(TOSTRING(device_category), log_level, __RV_FILE_NAME__, __LINE__, format, ##__VA_ARGS__)
#endif

#endif
