#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <unidefs.h>
#ifdef _WIN32
	#include <share.h>
#else
	#include <limits.h>
#endif

#include <mld/journal/journal.h>
#include <tc/emh.h>

#include "xfy_trace.h"

// global object definition for tracing
#ifdef XFYLIB
	#   if defined(__lint)
	#   elif defined(_WIN32)
		__declspec(dllexport)
	#else
	#endif
#endif
XFY::Trace XFY::g_XFYTrace;

#if defined (_WIN32)
	#define LOCALTIME_IDPD	localtime_s
	#define STRCPY_IDPD strcpy_s
	#define STRNCPY_IDPD strncpy_s
#else
	#define LOCALTIME_IDPD(P1,P2)	memcpy (P1, localtime (P2),  sizeof ( struct tm ) );
	#define STRCPY_IDPD(DEST,MAX,SRC) strcpy (DEST,SRC)
	#define STRNCPY_IDPD(DEST,MAX,SRC,SIZE) strncpy (DEST,SRC,SIZE)
	#define _countof(a) (sizeof(a)/sizeof(*(a)))
	#define _MAX_PATH PATH_MAX
#endif


// Global Version ID string
#define     XFY_TRACE_MAX_VERSION_ID 128
const char *g_szVersionID[XFY_TRACE_MAX_VERSION_ID];
int g_nVersionID = 0;

/////////////////////////////////////////////////////////////////////////
// XFY::Trace
//

const char *aszValuePrefix[XFY::Trace::eVT_COUNT] = { "V ", "E ", "I ", "? ",
		"! ", "I ", "? ", "! ", "N ", "O ", "C ", "R ", "X ", "  " };

static int g_iOutMode = 0;             // global static variable
const char* XFY::Trace::s_pszNULL = NULL;   // global static variable
const int* XFY::Trace::s_piNULL = NULL;     // global static variable

//-----------------------------------------------------------------------
// Constructor : XFY::Trace
// Description : Initialize member variables with default or values from
//               environment
//-----------------------------------------------------------------------
XFY::Trace::Trace() {

#ifdef _DEBUG  
	m_iActFlag = eOI_ALL;  // full tracing in debug
#else
	m_iActFlag = 0;   // no tracing in release
#endif

	time_t actTime = time(NULL);
	tm tmAct;

	LOCALTIME_IDPD(&tmAct, &actTime);
	strftime(m_szOutFile, _countof(m_szOutFile),
			"xfy_itk_XFY_TRACE_%Y%m%d_%H%M%S.log", &tmAct);

	m_pFile = NULL;

	m_iUseTraceFunction = false;
	*m_szTraceFunction = 0;
	m_iTraceFunctionFlag = 0;
	m_iSavedFlag = 0;

	m_iJournalingFlag = 0;
	m_iStopLevel = -1;

	m_iUseFlush = 0;
	m_iFceLevel = 0;

// load environment for current settings
	char *pszTraceFile = NULL;
	char *pszTraceFlags = NULL;
	char *pszTraceFlush = NULL;
	char *pszTraceFunction = NULL;

#if defined (_WIN32)
	size_t lTraceFile;
	if ((_dupenv_s(&pszTraceFile, &lTraceFile, "XFY_TRACE_FILE") == 0) &&
#else
	pszTraceFile = getenv ( "XFY_TRACE_FILE" );
	if (
#endif
		 (pszTraceFile != NULL)) {
		g_iOutMode |= eOM_DEBUGING;  // debugging activated

		transFileName(pszTraceFile, m_szOutFile, _MAX_PATH, false);
	}

#if defined (_WIN32)
	size_t lTraceFlags;
	if ((_dupenv_s(&pszTraceFlags, &lTraceFlags, "XFY_TRACE_FLAGS") == 0) &&
#else
	pszTraceFlags = getenv ("XFY_TRACE_FLAGS");
	if (
#endif
			(pszTraceFlags != NULL)) {
		m_iActFlag = atoi(pszTraceFlags);
	}

#if defined (_WIN32)
	size_t lTraceFlush;
	if ((_dupenv_s(&pszTraceFlush, &lTraceFlush, "XFY_TRACE_FLUSH") == 0) &&
#else
	pszTraceFlush = getenv ( "XFY_TRACE_FLUSH");
	if (
#endif
			 (pszTraceFlush != NULL)) {
		m_iUseFlush = atoi(pszTraceFlush);
	}

#if defined (_WIN32)
	size_t lTraceFunction;
	if ((_dupenv_s(&pszTraceFunction, &lTraceFunction, "XFY_TRACE_FUNCTION")
			== 0) &&
#else
	pszTraceFunction = getenv ( "XFY_TRACE_FUNCTION" );
	if (
#endif
			(pszTraceFunction != NULL)) {
		m_iUseTraceFunction = true;
		char *pszEqual = strrchr(pszTraceFunction, '=');
		if (pszEqual == NULL) {
			m_iTraceFunctionFlag = m_iActFlag;
			STRCPY_IDPD(m_szTraceFunction, 255, pszTraceFunction);
		} else {
			m_iTraceFunctionFlag = atoi(pszEqual + 1);
			STRNCPY_IDPD(m_szTraceFunction, 255, pszTraceFunction,
					pszEqual - pszTraceFunction);
			m_szTraceFunction[pszEqual - pszTraceFunction] = '\0';
		}
	}

#if defined (_WIN32)
	if ( pszTraceFile != NULL ) free (pszTraceFile);
	if ( pszTraceFlags != NULL ) free(pszTraceFlags);
	if ( pszTraceFlush != NULL ) free(pszTraceFlush);
#else
	// unix does not allocate memory by getenv
#endif

	if (m_iActFlag != 0 || m_iTraceFunctionFlag) {
		printf("XFY Tracing activated on level %d into ", m_iActFlag);
		switch (g_iOutMode) {
		case eOM_JOURNALING:
			printf("journal_file\n");
			break;
		case eOM_DEBUGING:
			printf("\n%s\n", m_szOutFile);
			break;
		case eOM_JOURNALING | eOM_DEBUGING:
			printf("journal_file + \n%s\n", m_szOutFile);
			break;
		}
		if (m_iTraceFunctionFlag) {
			printf("    Tracing within function %s on level %d\n",
					m_szTraceFunction, m_iTraceFunctionFlag);
		} else {
			ITK_set_journalling(true);
		}
	}
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : OutFile
// Description : Get output stream, if file not opened yet, open it
// Returns     : FILE Pointer to output stream
//-----------------------------------------------------------------------
void* XFY::Trace::getOutputFile() {
	if (m_pFile != NULL) {
		if (m_iUseFlush)
			fflush((FILE*) m_pFile);
		return m_pFile;
	}

	if ((strcmp(m_szOutFile, "") == 0)
			|| (strncmp(m_szOutFile, "stdout", 6)) == 0) {
		m_pFile = stdout;
	} else if (strcmp(m_szOutFile, "stderr") == 0) {
		m_pFile = stderr;
	} else {

		char szWOutFile[_MAX_PATH];     // output file

		transFileName(m_szOutFile, szWOutFile, _MAX_PATH, true);

#ifdef WIN32
		// try to open in read shared mode
		m_pFile = _fsopen ( szWOutFile, "w", _SH_DENYWR );
		if (m_pFile == NULL)
		{
			if ( fopen_s((FILE**) &m_pFile, szWOutFile, "w") == 0)
#else
		{
			if ((m_pFile = fopen( szWOutFile, "w") )== NULL)
#endif
			{
				fprintf(stderr,
						"Error: Trace File %s not opened. Standart output is used.",
						szWOutFile);
				m_pFile = stdout;
			}
		}
	}
	putFileBreak();
	return m_pFile;
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : putFileBreak
// Description : Create Start/End marker with time
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::Trace::putFileBreak() {
	if (m_pFile != NULL) {
		char szTime[80];
		time_t tNow = time(NULL);
		tm tmNow;
		LOCALTIME_IDPD(&tmNow, &tNow);
		strftime(szTime, sizeof(szTime) - 1, "%Y-%m-%d %H:%M:%S", &tmNow);
		fprintf((FILE*) m_pFile,
				"*******************************************************\n"
						"* EXSOFY Trace log %33.33s *\n"
						"*    %-48.48s *\n", szTime, m_szAppName);
		for (int iVer = 0; iVer < g_nVersionID; iVer++) {
			fprintf((FILE*) m_pFile, "*    %s", g_szVersionID[iVer]);
		}
		fprintf((FILE*) m_pFile,
				"*******************************************************\n");
	}
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : functionStart
// Description : Put Function start marker into logfile
//               and increase function level
// Parameter
//   fceName   I : Function name 
// Returns     : Pointer tu the function name, when traced,
//               otherwise NULL
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
const char * XFY::Trace::functionStart(const char *fceName) {

	if (m_iFceLevel <= 0) { // if in top level function
		if (ITK_ask_journalling()) // dynamic change of journaling
		{
			g_iOutMode |= eOM_JOURNALING;
			m_iActFlag |= eOI_JOURNAL;
		} else {
			g_iOutMode &= ~eOM_JOURNALING;
			m_iActFlag &= ~eOI_JOURNAL;
		}
		m_iFceLevel = 0; // reset to zero
	}

	// check for activating of function tracing
	if (m_iUseTraceFunction && m_iStopLevel < 0) {
		if (strcmp(fceName, m_szTraceFunction) == 0) {
			m_iSavedFlag = m_iActFlag;
			m_iActFlag = m_iTraceFunctionFlag;
			m_iStopLevel = m_iFceLevel;
			m_iJournalingFlag = ITK_ask_journalling();
		}
	}

	if (!showEntry())
		return NULL;

	if (g_iOutMode & eOM_DEBUGING) {
		int iSpaces = 2 * m_iFceLevel++;
		fprintf((FILE*) getOutputFile(), "%*s--> %s\n", iSpaces, "", fceName);
	}

	if (g_iOutMode & eOM_JOURNALING) {
		JOURNAL_routine_start(fceName);
	}

	return fceName;
}

void XFY::Trace::finishFunctionHeader() {
	// at the start of function activate header
	JOURNAL_routine_call();
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : functionEnd
// Description : Put Function end marker into logfile
//               and decrease function level
// Parameter
//   fceName   I : Function name
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
#pragma warning(disable: 4100)
void XFY::Trace::functionEnd(const char *fceName, const time_t lStartTime,
		const void *MemoryStruct) {

	if (g_iOutMode & eOM_DEBUGING) {
		if (m_iFceLevel > 0)
			m_iFceLevel--;
		int iSpaces = 2 * m_iFceLevel;
		fprintf((FILE*) getOutputFile(), "%*s<-- %s (%2.2f sec) %s\n", iSpaces,
				"", fceName,
				lStartTime == 0 ?
						0 : (clock() - lStartTime) / 1.0 / CLOCKS_PER_SEC, ""/*szMemoryMessage*/
				);
	}

	if (g_iOutMode & eOM_JOURNALING) {
		JOURNAL_routine_end();
	}

	// check for deactivating of function tracing
	if (m_iUseTraceFunction && m_iStopLevel == m_iFceLevel) {
		m_iActFlag = m_iSavedFlag;
		m_iStopLevel = -1;
		ITK_set_journalling(m_iJournalingFlag);
	}

}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : transFileName
// Description : File Name Syntax Parser
// Parameter
//   pszFrom      I : Coded Name
//   pszTo        O : Translated name
//   iBufferLen   I : Maximal buffer Length
//   isTransAll   I : Translate all attributes
// Returns     : 
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
size_t XFY::Trace::transFileName(const char *pszFrom, char *pszTo,
		const int iBufferLen, const int isTransAll = true) const {
	char szBuf[1024];
	char *pszSaver = pszTo;

	STRNCPY_IDPD( szBuf, _countof(szBuf), pszFrom, 1023); // avoid problems with self-translation
	const char *pszRunner = szBuf;

	if (pszFrom == pszTo) {

	}

	while (*pszRunner != '\0') {
		if (*pszRunner == '*') {
			size_t iFreeSpace = iBufferLen - (pszSaver - pszTo);
			switch (*++pszRunner) {
			case 'D': {
				time_t actTime = time(NULL);
				tm tmAct;
				LOCALTIME_IDPD(&tmAct, &actTime);
				if (strftime(pszSaver, iFreeSpace, "%Y%m%d_%H%M%S", &tmAct) > 0)
					pszSaver += strlen(pszSaver);
			}
				break;
			case 'P': {
				char szProgEnv[] = "itk";

				size_t iAddLen = strlen(szProgEnv);
				if (iAddLen < iFreeSpace) {
					STRCPY_IDPD(pszSaver, iFreeSpace, szProgEnv);
					pszSaver += iAddLen;
				}
			}
				break;
			case 'U': {
#ifdef _WIN32
//          size_t dwSize = iFreeSpace;

//          if ( GetUserName ( pszSaver, &dwSize ) != 0 )
				{
//            pszSaver += dwSize - 1;
				}
#endif
			}
				break;
			case 'A': // only for trans all run
				if (isTransAll) {
					if ((m_szAppName != NULL)
							&& ((int) strlen(m_szAppName) < iFreeSpace)) {
						pszSaver +=
								snprintf(pszSaver,iFreeSpace, "%s",
										m_szAppName);
					}
				} else {
					if (iFreeSpace > 2) {
						*pszSaver++ = '*';
						*pszSaver++ = *pszRunner;
					}
				}
				break;
			case 'C': {
#ifdef _WIN32
//          size_t dwSize = iFreeSpace;

//          if ( GetComputerName ( pszSaver, &dwSize ) != 0 )
				{
//            pszSaver += dwSize - 1;
				}
#endif
			}
				break;
			default:
				fprintf(stderr,
						"Unsupported Tracing Option *%c at position %td.\n",
						*pszRunner, pszRunner - szBuf);
				break;
			}
			pszRunner++;
		} else {
			if (pszSaver - pszTo < iBufferLen)
				*pszSaver++ = *pszRunner++;
		}
	}

	*pszSaver = 0;

	return strlen(pszTo);
}

int XFY::Trace::putFceReturns(const int &Value, const XFY::TraceFce *pOutItem, const int fromLine ) {
	if (XFY::g_XFYTrace.showParam()) {
		if (g_iOutMode & eOM_DEBUGING) {
			fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
					"%*s%sValue = %d", XFY::g_XFYTrace.getLevel(), "",
					aszValuePrefix[eVT_T_RET], Value);
			if ( fromLine >= 0 ) fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), ", line=%d\n", fromLine );
			else                 fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), "\n" );
		}
		if (g_iOutMode & eOM_JOURNALING) {
			JOURNAL_return_value(Value);
			((XFY::TraceFce *) pOutItem)->closeJournalling();
		}
	}
	return Value;
}

#define ReturnsValSimple(ptype,pform) \
ptype XFY::Trace::putFceReturns ( const ptype &Value, const XFY::TraceFce *pOutItem, const int fromLine ) \
{ if ( XFY::g_XFYTrace.showParam() ) {\
    if ( g_iOutMode & eOM_DEBUGING ) { \
      fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%sValue = " pform, XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT_T_RET], Value); \
      if ( fromLine >= 0 ) fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), ", line=%d\n", fromLine );\
      else fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), "\n" ); }\
  } return (ptype)Value;\
}

#ifndef _SUN
ReturnsValSimple( bool, "%d");
#endif

ReturnsValSimple( unsigned int, "%u");
ReturnsValSimple( long, "%ld");
ReturnsValSimple( unsigned long, "%lu");
ReturnsValSimple( unsigned long long, "%llu");
ReturnsValSimple( double, "%lf");
ReturnsValSimple( float, "%f");

ReturnsValSimple( char, "%c");

#undef ReturnsValSimple

#define ReturnsValPtrSimple(ptype,pform) \
ptype * XFY::Trace::putFceReturns ( const ptype *const &Value, const XFY::TraceFce *pOutItem, const int fromLine  ) \
{ if ( XFY::g_XFYTrace.showParam() ) { \
    if ( g_iOutMode & eOM_DEBUGING ) { \
      fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%sValue(%p) = ", XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT_T_RET], Value );\
      if ( Value != NULL ) fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), pform, *Value);\
      else                 fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), "NULL" ); \
      if ( fromLine >= 0 ) fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), ", line=%d\n", fromLine ); \
      else                 fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), "\n" ); } }\
    if ( g_iOutMode & eOM_JOURNALING ) { \
    } \
  return (ptype *)Value;\
}

ReturnsValPtrSimple( int, "%d");
ReturnsValPtrSimple( unsigned int, "%u");
ReturnsValPtrSimple( long, "%ld");
ReturnsValPtrSimple( unsigned long, "%lu");
ReturnsValPtrSimple( unsigned long long, "%llu");
ReturnsValPtrSimple( double, "%lf");
ReturnsValPtrSimple( float, "%f");

#undef ReturnsValPtrSimple

#define ReturnsValDefSimple(ptype,pform) \
ptype XFY::Trace::putFceReturns ( const ptype const &Value, const XFY::TraceFce *pOutItem, const int fromLine ) \
{ if ( XFY::g_XFYTrace.showParam() ) { \
    if ( g_iOutMode & eOM_DEBUGING ) { \
      fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%sValue(%p) = ", XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT_T_RET], Value );\
      if ( Value != NULL ) fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), pform, Value);\
      else                 fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), "NULL" ); \
      if ( fromLine >= 0 )  fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), ", line=%d\n", fromLine ); \
      else                 fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), "\n" ); } }\
    if ( g_iOutMode & eOM_JOURNALING ) { \
    } \
  return (ptype)Value;\
}

ReturnsValDefSimple( void *, "%p");
ReturnsValDefSimple( char *, "%s");

#undef ReturnsValDefSimple

void XFY::Trace::doFceThrows(const char *pszMsg) {
	if (XFY::g_XFYTrace.showEntry()) {
		if (g_iOutMode & eOM_DEBUGING) {
			fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(), "%*s%s %s\n",
					XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT_V_EX],
					pszMsg);
		}\

		if (g_iOutMode & eOM_JOURNALING) {
			;
		}
	}
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : putErrorMessage
// Description : Put a translated message into the output
// Parameter
//   iMessage   I : 
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::Trace::putErrorMessage(const int iMessage) {
	if (g_iOutMode & eOM_DEBUGING) {
		int nFails;
		const int *aiSeverities = NULL;
		const int *aiFails = NULL;
		const char **asTexts = NULL;
		EMH_ask_errors(&nFails, &aiSeverities, &aiFails, &asTexts);
		if (nFails > 0 && aiFails[nFails - 1] == iMessage) {
			// from error stack
			fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
					"  Teamcenter ERROR: %d [%s]\n", aiFails[nFails - 1],
					asTexts[nFails - 1]);
		} else {
			char* pszError = NULL;
			EMH_ask_error_text(iMessage, &pszError);
			if (pszError != NULL) {
				fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
						"*** Error %d: %s\n", iMessage, pszError);
				MEM_free(pszError);
			}
		}
	}
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : ErrorReturns
// Description : Puts the message string of Value and returns
// Parameter
//   Value      I : 
//   pOutItem   I : 
// Returns     : 
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
int XFY::Trace::putErrorReturns(const int &Value,
		const XFY::TraceFce *pOutItem, const int /*fromLine*/ ) {
	putErrorMessage(Value);
//  if ( XFY::g_XFYTrace.ShowParam() )
//    fprintf( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%sValue = " pform "\n", XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT_T_RET], Value);
	return Value;
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : putMessage
// Description : Print a message into output - like printf
// Parameter
//   format   I : formater string
//   ...      I : Values list
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
int XFY::Trace::putMessage(const char *format, ...) const {
	if (!showMessage())
		return 0;

	if (g_iOutMode & eOM_DEBUGING) {
		va_list arglist;
		va_start(arglist, format);
		int iLen = fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(), "%*s",
				XFY::g_XFYTrace.getLevel(), "");
		iLen += vfprintf((FILE*) XFY::g_XFYTrace.getOutputFile(), format,
				arglist);
		va_end(arglist);
		return iLen;
	}

	if (g_iOutMode & eOM_JOURNALING) {
		va_list arglist;
		va_start(arglist, format);
#ifdef _WIN32
		char sOut[512] = {'\0'};
		int iLen = _vsnprintf_s ( sOut, _countof(sOut), 512, format, arglist );
#else
		char sOut[8192] = { '\0' };
		int iLen = vsprintf(sOut, format, arglist);
#endif
		va_end(arglist);

		JOURNAL_comment(sOut);
		return iLen;
	}

	return 0;
}

int XFY::Trace::putMessageVA(const char *format, va_list arglist) const {
	if (!showMessage())
		return 0;

	if (g_iOutMode & eOM_DEBUGING) {
		int iLen = fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(), "%*s",
				XFY::g_XFYTrace.getLevel(), "");
		iLen += vfprintf((FILE*) XFY::g_XFYTrace.getOutputFile(), format,
				arglist);
		return iLen;
	}

	return 0;
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : ReportFceCall
// Description : Reports a call of a function
//   ufName   I : Name of called function
//   fileName I : Name of sourcefile
//   lineNum  I : number of source code line
//   iRetVal  I : function return code
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 11.04.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
int XFY::Trace::reportFceCall(const char *ufName, const char *fileName,
		const int lineNum, const int iRetVal) {
	char tmdescr[200] = { 0 };

	if ( iRetVal != ITK_ok ) {
		// error occurred
		time_t t = time(0); // obtain the current time_t value
		tm now;
#if defined (_WIN32)
		if ((LOCALTIME_IDPD(&now, &t) != 0) ||
#else
		LOCALTIME_IDPD(&now,&t);
		if (
#endif
				(strftime(tmdescr, sizeof(tmdescr) - 1, ", at %H:%M:%S",
						&now) <= 0)) {
			// error occurred, empty string
			*tmdescr = 0;
		}
	}

	if (g_iOutMode & eOM_DEBUGING) {
		if (iRetVal == 0 && showUfCall()) {
			fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(), "%*s%s%s\n",
					XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT_UF],
					ufName);
		}
		if (iRetVal != ITK_ok ) {

			fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
					"*** %s line : %d%s\n*** %s\n", fileName, lineNum, tmdescr,
					ufName);
			putErrorMessage(iRetVal);
		}
	}

	if (g_iOutMode & eOM_JOURNALING) {
		;
	}

	if (iRetVal != ITK_ok ) {
		// every error in syslog
		TC_write_syslog( " %s returns [%d]\n", ufName, iRetVal );
		int nFails;
		const int *aiSeverities = NULL;
		const int *aiFails = NULL;
		const char **asTexts = NULL;
		EMH_ask_errors( &nFails, &aiSeverities, &aiFails, &asTexts );
		if ( nFails > 0 ) {
			TC_write_syslog( "  Teamcenter ERROR: %d [%s]\n", aiFails[nFails-1], asTexts[nFails-1]);
		}
		else {
			char *s = NULL;
			EMH_ask_error_text (iRetVal, &s);
			TC_write_syslog( "  Teamcenter ERROR: [%s]\n", s);
			if (s != 0) MEM_free (s);
		}
		TC_write_syslog( "  in file [%s], line [%d]%s\n\n", fileName, lineNum, tmdescr );
	}

	return (iRetVal);
}


//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : ReportUFCall
// Description : Trace a call of UF void function
//   ufName   I : Name of called function
//   fileName I : Name of sourcefile
//   lineNum  I : number of source code line
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::Trace::reportFceCall(const char *ufName, const char *fileName,
		const int lineNum) {
	if (!showUfCall())
		return;

	if (g_iOutMode & eOM_DEBUGING) {
		fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(), "%*s%s%s\n",
				XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT_UF], ufName);
	}

	if (g_iOutMode & eOM_JOURNALING) {
		;
	}
}

//-----------------------------------------------------------------------
// Destructor  : XFY::Trace
// Description : If logfile opened, put end marker and close it
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
XFY::Trace::~Trace() {
	if (m_pFile) {
		if (g_iOutMode & eOM_DEBUGING) {
			putFileBreak();
		}
	}
	if ((m_pFile != NULL) && (m_pFile != stdout) && (m_pFile != stderr))
		fclose((FILE*) m_pFile);
}

/////////////////////////////////////////////////////////////////////////
// XFY::TraceFce
//

//-----------------------------------------------------------------------
// Constructor : XFY::TraceFce
// Description : Put function name into tracelog and save it
// Parameter
//   fceName   I : Function name
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
XFY::TraceFce::TraceFce(const char *fceName) {
	m_pszFceName = XFY::g_XFYTrace.functionStart(fceName);
	m_iOutCount = 0;
	m_lStart = clock();
	m_retValue = ITK_ok;

	m_bJournallingClosed = false;
}

const int XFY::Trace::getOutMode() const {
	return g_iOutMode;
}
;

//-----------------------------------------------------------------------
// Macros for overloaded TraveValue function with simple type parameter
// and pointer to simple type parameter
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
#ifdef XXX
#define DO_JOURNALING(mode,jourtype,jourpref) \
  if ( mode & XFY::Trace::eOM_JOURNALING ) switch ( eVT ) \
    { case eVT_IO : case eVT_ION :JOURNAL_output_argument ( pszName ); \
      case eVT_I : case eVT_IN : JOURNAL_##jourtype##_in ( jourpref value ); break; \
      case eVT_O : case eVT_ON : JOURNAL_##jourtype##_out ( pszName, jourpref value ); break; \
      default :;  }

#define TraceSimple(fcename,partype,parformat,jourtype,jourpref) \
static void TraceVal_##fcename (const char *pszName, const partype *value, int iDeep ) \
{/*  if ( XFY::g_XFYTrace.getOutMode() & XFY::Trace::eOM_DEBUGING ) */{\
     XFY::Trace::putVariable ( pszName, *value, XFY::Trace::eVT_O_RET ); } \
} \
void XFY::Trace::putVariable ( const char *pszName, const partype &value, eVALUE_TYPE eVT, XFY::TraceFce *pOutItem ) \
{ DO_JOURNALING (XFY::g_XFYTrace.getOutMode(),##jourtype, ##jourpref); \
  if ( eVT != eVT_O && eVT != eVT_ON ) { \
    if ( XFY::g_XFYTrace.getOutMode() & XFY::Trace::eOM_DEBUGING ) {\
      fprintf ( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%s%s = " parformat " (" #partype ")\n", XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value ); }\
    } \
  if ( eVT == eVT_O || eVT == eVT_ON || eVT == eVT_IO || eVT == eVT_ION ) \
    pOutItem->registerOutputParam ( pszName, (void *)(&value), (TRACEVALUE)TraceVal_##fcename ); } \
static void TraceVal_p_##fcename ( const char *pszName, const partype **value, int iDeep ) \
{ XFY::Trace::putVariable ( pszName, *value, XFY::Trace::eVT_O_RET ); } \
void XFY::Trace::putVariable ( const char *pszName, const partype *const &value, eVALUE_TYPE eVT, XFY::TraceFce *pOutItem, int iDeep ) \
{ if ( eVT != eVT_O && eVT != eVT_ON ) {\
    if ( XFY::g_XFYTrace.getOutMode() & XFY::Trace::eOM_DEBUGING ) {\
      if ( value == NULL ) fprintf ( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%s%s = %p (" #partype " *)   *%s = undefined\n", XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value, pszName); \
      else fprintf ( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%s%s = %p (" #partype " *)   *%s = " parformat "\n", XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value, pszName, *value ); \
   }}\
  if ( eVT == eVT_O || eVT == eVT_ON || eVT == eVT_IO || eVT == eVT_ION ) \
    pOutItem->registerOutputParam ( pszName, (void **)&(value), (TRACEVALUE)TraceVal_p_##fcename );\
}; 
#endif

#define DO_JOURNALING(mode,jourtype,jourpref) \
  if ( mode & XFY::Trace::eOM_JOURNALING ) switch ( eVT ) \
    { case eVT_IO : case eVT_ION :JOURNAL_output_argument ( pszName ); \
      case eVT_I : case eVT_IN : JOURNAL_##jourtype##_in ( jourpref value ); break; \
      case eVT_O : case eVT_ON : JOURNAL_##jourtype##_out ( pszName, jourpref value ); break; \
      default :;  }

#define TraceSimple(fcename,partype,parformat,jourtype,jourpref) \
static void TraceVal_##fcename (const char *pszName, const partype *value, int iDeep ) \
{/*  if ( XFY::g_XFYTrace.getOutMode() & XFY::Trace::eOM_DEBUGING ) */{\
     XFY::Trace::putVariable ( pszName, *value, XFY::Trace::eVT_O_RET ); } \
} \
void XFY::Trace::putVariable ( const char *pszName, const partype &value, eVALUE_TYPE eVT, XFY::TraceFce *pOutItem ) \
{ /*DO_JOURNALING (XFY::g_XFYTrace.getOutMode(),##jourtype, ##jourpref); */\
  if ( eVT != eVT_O && eVT != eVT_ON ) { \
    if ( XFY::g_XFYTrace.getOutMode() & XFY::Trace::eOM_DEBUGING ) {\
      fprintf ( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%s%s = " parformat " (" #partype ")\n", XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value ); }\
    } \
  if ( eVT == eVT_O || eVT == eVT_ON || eVT == eVT_IO || eVT == eVT_ION ) \
    pOutItem->registerOutputParam ( pszName, (void *)(&value), (TRACEVALUE)TraceVal_##fcename ); } \
static void TraceVal_p_##fcename ( const char *pszName, const partype **value, int iDeep ) \
{ XFY::Trace::putVariable ( pszName, *value, XFY::Trace::eVT_O_RET ); } \
void XFY::Trace::putVariable ( const char *pszName, const partype *const &value, eVALUE_TYPE eVT, XFY::TraceFce *pOutItem, int iDeep ) \
{ if ( eVT != eVT_O && eVT != eVT_ON ) {\
    if ( XFY::g_XFYTrace.getOutMode() & XFY::Trace::eOM_DEBUGING ) {\
      if ( value == NULL ) fprintf ( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%s%s = %p (" #partype " *)   *%s = undefined\n", XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value, pszName); \
      else fprintf ( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%s%s = %p (" #partype " *)   *%s = " parformat "\n", XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value, pszName, *value ); \
   }}\
  if ( eVT == eVT_O || eVT == eVT_ON || eVT == eVT_IO || eVT == eVT_ION ) \
    pOutItem->registerOutputParam ( pszName, (void **)&(value), (TRACEVALUE)TraceVal_p_##fcename );\
};

TraceSimple( short, short, "%hd", nyi, &);
TraceSimple( int, int, "%d", integer, (const int));
#ifndef _SUN
TraceSimple( bool, bool, "%d", logical, (const logical));
#endif
TraceSimple( long, long, "%ld", nyi, &);
TraceSimple( unsigned_short, unsigned short, "%hu", nyi, &);
// tag_t needs separate implementation of unsigned int
// TraceSimple ( unsigned_int, unsigned int, "%u", nyi, & );
TraceSimple( unsigned_long, unsigned long, "%lu", nyi, &);
TraceSimple( unsigned_long_long, unsigned long long, "%llu", nyi, &);
TraceSimple( float, float, "%f", nyi, &);
TraceSimple( double, double, "%lf", double, (const double));

#undef TraceSimple

//-----------------------------------------------------------------------
// Function    : TraceVal_char
// Description : Put a character variable into tracelog
// Parameter
//   pszName   I : Parameter name
//   value     I : Pointer to Parameter value
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
static void TraceVal_char(const char *pszName, const char * value, int iDeep) {
	XFY::Trace::putVariable(pszName, *value, XFY::Trace::eVT_O_RET);
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//            I : Variable value of type character
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::Trace::putVariable(const char *pszName, const char &value,
		eVALUE_TYPE eVT, XFY::TraceFce *pOutItem) {
	if (eVT != eVT_O && eVT != eVT_ON) {
		if (g_iOutMode & eOM_DEBUGING) {
			fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
					"%*s%s%s = %c (char)\n", XFY::g_XFYTrace.getLevel(), "",
					aszValuePrefix[eVT], pszName, value);
		}
		if (g_iOutMode & eOM_JOURNALING) {
			DO_JOURNALING( g_iOutMode, char, (const char));
		}
	}
	if (eVT == eVT_O || eVT == eVT_ON || eVT == eVT_IO || eVT == eVT_ION)
		pOutItem->registerOutputParam(pszName, (void *) &value,
				(TRACEVALUE) TraceVal_char);
}

//-----------------------------------------------------------------------
// Function    : TraceVal_unsigned_int
// Description : Put an unsigned it variable into tracelog
// Parameter
//   pszName   I : Parameter name
//   value     I : Pointer to Parameter value
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes :
// Date        By      Reason
//-----------------------------------------------------------------------
static void TraceVal_unsigned_int(const char *pszName,
		const unsigned int * value, int iDeep) {
	XFY::Trace::putVariable(pszName, *value, XFY::Trace::eVT_O_RET);
}

//-----------------------------------------------------------------------
// Function    : TraceVal_unsigned_int
// Description : Put an unsigned it variable into tracelog
// Parameter
//   pszName   I : Parameter name
//   value     I : Pointer to Parameter value
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes :
// Date        By      Reason
//-----------------------------------------------------------------------
static void TraceVal_tag_t(const char *pszName, const tag_t * value,
		int iDeep) {
	XFY::Trace::putVariable(pszName, *value, XFY::Trace::eVT_O_RET);
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//            I : Variable value of type unsigned int
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes :
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::Trace::putVariable(const char *pszName, const unsigned int &value,
		eVALUE_TYPE eVT, XFY::TraceFce *pOutItem) {
	if (eVT != eVT_O && eVT != eVT_ON) {
		if (g_iOutMode & eOM_DEBUGING) {
			fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
					"%*s%s%s = %u (%s)", XFY::g_XFYTrace.getLevel(), "",
					aszValuePrefix[eVT], pszName, value,
					( eVT == eVT_IN || eVT == eVT_ION ) ? "uint" : "tag_t");
			if ( eVT == eVT_I || eVT == eVT_IO ) {
				if ( g_iOutMode & eOM_JOURNALING ) {
					// journaling active, not use ITK call
					fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
							"\n" );
				} else {
					if ( value != NULLTAG ) {
						char * uid = NULL;
						POM_tag_to_uid( value, &uid );
						fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
								" <%s>\n", uid );
						MEM_free ( uid );
					} else {
						fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
								" <NULLTAG>\n" );
					}
				}
			} else {
				fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
						"\n" );
			}
		}
		if (g_iOutMode & eOM_JOURNALING) {
			if (eVT == eVT_IN) {
				DO_JOURNALING( g_iOutMode, nyi, &);
			} else {
				DO_JOURNALING( g_iOutMode, tag, (const tag_t));
			}
		}
	}
	if (eVT == eVT_O || eVT == eVT_IO)
		pOutItem->registerOutputParam(pszName, (void *) &value,
				(TRACEVALUE) TraceVal_tag_t);
	else if (eVT == eVT_ON || eVT == eVT_ION)
		pOutItem->registerOutputParam(pszName, (void *) &value,
				(TRACEVALUE) TraceVal_unsigned_int);
}

//-----------------------------------------------------------------------
// Function    : TraceVal_unsigned_int
// Description : Put an unsigned it variable into tracelog
// Parameter
//   pszName   I : Parameter name
//   value     I : Pointer to Parameter value
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes :
// Date        By      Reason
//-----------------------------------------------------------------------
static void TraceVal_p_unsigned_int(const char *pszName,
		const unsigned int ** value, int iDeep) {
	XFY::Trace::putVariable(pszName, *value, XFY::Trace::eVT_O_RET);
}

//-----------------------------------------------------------------------
// Function    : TraceVal_unsigned_int
// Description : Put an unsigned it variable into tracelog
// Parameter
//   pszName   I : Parameter name
//   value     I : Pointer to Parameter value
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes :
// Date        By      Reason
//-----------------------------------------------------------------------
static void TraceVal_p_tag_t(const char *pszName, const tag_t ** value,
		int iDeep) {
	XFY::Trace::putVariable(pszName, *value, XFY::Trace::eVT_O_RET);
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//            I : Variable value of type unsigned int
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes :
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::Trace::putVariable(const char *pszName,
		const unsigned int * const &value, eVALUE_TYPE eVT,
		XFY::TraceFce *pOutItem, int iDeep) {
	if (eVT != eVT_O && eVT != eVT_ON) {
		if (g_iOutMode & eOM_DEBUGING) {
			fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
					"%*s%s%s = %p (%s *)   *%s = %u",
					XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT],
					pszName, value, ( eVT == eVT_IN || eVT == eVT_ION ) ? "uint" : "tag_t", pszName,
					value != NULL ? *value : 0 );
			if ( ( eVT == eVT_I || eVT == eVT_IO ) && value != NULL ) {
				char * uid = NULL;
				POM_tag_to_uid( *value, &uid );
				fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
						" <%s>\n", uid );
				MEM_free ( uid );
			} else {
				fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
						"\n" );
			}
		}
	}
	if (eVT == eVT_O || eVT == eVT_IO)
		pOutItem->registerOutputParam(pszName, (void *) &value,
				(TRACEVALUE) TraceVal_p_tag_t);
	if (eVT == eVT_ON || eVT == eVT_ION)
		pOutItem->registerOutputParam(pszName, (void *) &value,
				(TRACEVALUE) TraceVal_p_unsigned_int);
}

//-----------------------------------------------------------------------
// Function    : TraceVal_obj
// Description : Put a character variable into tracelog
// Parameter
//   pszName   I : Parameter name
//   value     I : Pointer to Parameter value
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
//static void TraceVal_obj ( const char *pszName, XFY::TraceObject *traceObject, int iDeep ) 
//{
//  XFY::Trace::putVariable ( pszName, *traceObject, XFY::Trace::eVT_O_RET, iDeep );
//}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//            I : Variable value of type character
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
/*void XFY::Trace::putVariable ( const char *pszName, XFY::TraceObject &traceObject, eVALUE_TYPE eVT, XFY::TraceFce *pOutItem, int iDeep )
 {
 if ( eVT != eVT_O && eVT != eVT_ON )
 {
 if ( g_iOutMode & eOM_DEBUGING )
 {
 traceObject.putLinesToOutput ( XFY::g_XFYTrace.getOutputFile(),
 XFY::g_XFYTrace.getLevel(), aszValuePrefix[eVT],
 pszName );
 }
 if ( g_iOutMode & eOM_JOURNALING )
 {
 traceObject.putToJournal ( pszName, eVT );
 }
 }
 if ( eVT == eVT_O || eVT == eVT_ON || eVT == eVT_IO || eVT == eVT_ION )
 pOutItem->AddOutputParam ( pszName, (void *)&traceObject, (TRACEVALUE)TraceVal_obj );
 }
 */
//-----------------------------------------------------------------------
// Function    : TraceVal_str
// Description : Put a string variable into tracelog
// Parameter
//   pszName   I : Parameter name
//   value     I : Pointer to Parameter value
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
static void TraceVal_str(const char *pszName, const char **value, int iDeep) {
	XFY::Trace::putVariable(pszName, *value, XFY::Trace::eVT_O_RET);
}

#ifdef _WIN32
void XFY::Trace::putVariable ( const char *pszName, char * &value, eVALUE_TYPE eVT, XFY::TraceFce *pOutItem, int iDeep )
{
	putVariable ( pszName, (const char*&)value, eVT, pOutItem, iDeep );
}
#endif

#ifndef _AIX
void XFY::Trace::putVariable(const char *pszName, char * const &value,
		eVALUE_TYPE eVT, XFY::TraceFce *pOutItem, int iDeep) {
	putVariable(pszName, (const char*&) value, eVT, pOutItem, iDeep);
}
#endif

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//             I : Variable value of type zero terminated string
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::Trace::putVariable(const char *pszName, const char * const &value,
		eVALUE_TYPE eVT, XFY::TraceFce *pOutItem, int iDeep) {
	if (eVT != eVT_O && eVT != eVT_ON) {
		if (value == NULL) {
			if (g_iOutMode & eOM_DEBUGING) {
				fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
						"%*s%s%s = %p (char *) len=undefined\n%*s%s*%s = undefined\n",
						XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT],
						pszName, value, XFY::g_XFYTrace.getLevel(), "",
						aszValuePrefix[eVT_EMPTY], pszName);
			}
			if (g_iOutMode & eOM_JOURNALING) {
				DO_JOURNALING( g_iOutMode, string, (const char *));
//        JOURNAL_string_out(pszName, "NULL");
			}
		} else {
			if (g_iOutMode & eOM_DEBUGING) {
				size_t stringLen = strlen ( value );
				fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
						"%*s%s%s = %p (char *)\n%*s%s*%s = \"%.128s\"%s, len=%zu\n",
						XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT],
						pszName, value, XFY::g_XFYTrace.getLevel(), "",
						aszValuePrefix[eVT_EMPTY], pszName, value,
						stringLen > 128 ? "..." : "",
						stringLen);
			}
			if (g_iOutMode & eOM_JOURNALING) {
				DO_JOURNALING( g_iOutMode, string, (const char *));
			}
		}
	}
	if (eVT == eVT_O || eVT == eVT_ON || eVT == eVT_IO || eVT == eVT_ION)
		pOutItem->registerOutputParam(pszName, (void **) &value,
				(TRACEVALUE) TraceVal_str);
}

#undef DO_JOURNALING

//-----------------------------------------------------------------------
// Function    : TraceVal_str
// Description : Put a string variable into tracelog
// Parameter
//   pszName   I : Parameter name
//   value     I : Pointer to Parameter value
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
static void TraceVal_pstr(const char *pszName, const char ***value, int iDeep) {
	XFY::Trace::putVariable(pszName, *value, XFY::Trace::eVT_O_RET, NULL,
			iDeep);
}

#ifdef _WIN32
void XFY::Trace::putVariable ( const char *pszName, char **&value, eVALUE_TYPE eVT, XFY::TraceFce *pOutItem, int iDeep )
{
	putVariable ( pszName, (const char**&)value, eVT, pOutItem, iDeep );
}
#endif

void XFY::Trace::putVariable(const char *pszName, char ** const &value,
		eVALUE_TYPE eVT, XFY::TraceFce *pOutItem, int iDeep) {
	putVariable(pszName, (const char**&) value, eVT, pOutItem, iDeep);
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//             I : Variable value of type zero terminated string
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::Trace::putVariable(const char *pszName, const char ** const &value,
		eVALUE_TYPE eVT, XFY::TraceFce *pOutItem, int iDeep) {
	if (eVT != eVT_O && eVT != eVT_ON) {
		if (value == NULL) {
			if (g_iOutMode & eOM_DEBUGING) {
				fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
						"%*s%s&%s = %p (char *) len=undefined\n%*s  *%s = undefined\n",
						XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT],
						pszName, value, XFY::g_XFYTrace.getLevel(), "",
						pszName);
			}
			if (g_iOutMode & eOM_JOURNALING) {
				JOURNAL_string_out(pszName, "NULL");
			}
		} else if (*value == NULL) {
			if (g_iOutMode & eOM_DEBUGING) {
				fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
						"%*s%s%s = %p (char *) len=undefined\n%*s  *%s = undefined\n",
						XFY::g_XFYTrace.getLevel(), "", aszValuePrefix[eVT],
						pszName, *value, XFY::g_XFYTrace.getLevel(), "",
						pszName);
			}
			if (g_iOutMode & eOM_JOURNALING) {
				JOURNAL_string_out(pszName, "NULL");
			}
		} else {
			if (g_iOutMode & eOM_DEBUGING) {
				fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
						"%*s%s%s = %p (char **)\n", XFY::g_XFYTrace.getLevel(),
						"", aszValuePrefix[eVT], pszName, *value);
			}
			if (g_iOutMode & eOM_JOURNALING) {
				JOURNAL_address_out(pszName, value);
			}
			/* value changed to *value. used by holetabel program. check it with iman */
			char **pszArray = ((char **) value);
			if (pszArray == NULL) {
			} else {
				if (g_iOutMode & eOM_DEBUGING) {
//          fprintf ( (FILE*)XFY::g_XFYTrace.getOutputFile(), "%*s%s*%s = %p (char *) count=%d\n",
//                    XFY::g_XFYTrace.getLevel() + 2, "", aszValuePrefix[eVT_EMPTY], pszName, *pszArray, iDeep );

					if (iDeep < 0) {
						size_t stringLen = strlen(*pszArray);
						fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
								"%*s%s**%s = \"%.128s\"%s, len=%zu\n",
								XFY::g_XFYTrace.getLevel() + 2, "",
								aszValuePrefix[eVT_EMPTY], pszName, *pszArray,
								stringLen > 128 ? "..." : "",
								stringLen);
					} else {
						for (int iI = 0; iI < iDeep; iI++) {
							if (pszArray[iI] == NULL) {
								fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
										"%*s%s*%s[%d] = NULL\n",
										XFY::g_XFYTrace.getLevel() + 2, "",
										aszValuePrefix[eVT_EMPTY], pszName, iI);
							} else {
								fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
										"%*s%s*%s[%d] = \"%s\", len=%zu\n",
										XFY::g_XFYTrace.getLevel() + 2, "",
										aszValuePrefix[eVT_EMPTY], pszName, iI,
										pszArray[iI], strlen(pszArray[iI]));
							}
						}
					}
				}
				if (g_iOutMode & eOM_JOURNALING) {
					JOURNAL_string_array_out(pszName, iDeep, (char **) value);
				}
			}
		}
	}
	if (eVT == eVT_O || eVT == eVT_ON || eVT == eVT_IO || eVT == eVT_ION) {
		if (pOutItem != NULL) {
			pOutItem->registerOutputParam(pszName, (void ***) &value,
					(TRACEVALUE) TraceVal_pstr);
		}
	}
}

//-----------------------------------------------------------------------
// Function    : TraceVal_p_void
// Description : Put a void pointer variable into tracelog
// Parameter
//   pszName   I : Parameter name
//   value     I : Pointer to Parameter value
// Global      : NONE
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void TraceVal_p_void(const char *pszName, const void** value, int iDeep) {
	XFY::Trace::putVariable(pszName, *value, XFY::Trace::eVT_O_RET);
}

//-----------------------------------------------------------------------
// Object      : XFY::Trace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//   pszName   I : Variable name
//   value     I : Variable value of type void pointer
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::Trace::putVariable(const char *pszName, const void * const &value,
		eVALUE_TYPE eVT, XFY::TraceFce *pOutItem) {
	if (eVT != eVT_O && eVT != eVT_ON) {
		if (g_iOutMode & eOM_DEBUGING) {
			fprintf((FILE*) XFY::g_XFYTrace.getOutputFile(),
					"%*s%s%s = %p (void*)\n", XFY::g_XFYTrace.getLevel(), "",
					aszValuePrefix[eVT], pszName, value);
		}
		if (g_iOutMode & eOM_JOURNALING) {
			JOURNAL_address_out(pszName, value);
		}
	}
	if (eVT == eVT_O || eVT == eVT_ON || eVT == eVT_IO || eVT == eVT_ION)
		pOutItem->registerOutputParam(pszName, (void *) &value,
				(TRACEVALUE) TraceVal_p_void);
}

//-----------------------------------------------------------------------
// Destructor  : XFY::TraceFce
// Description : Put saved function name into tracelog
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
XFY::TraceFce::~TraceFce() {
	if (XFY::g_XFYTrace.showParam()) {
		for (int i = 0; i < m_iOutCount; i++) {
			m_asOutList[i].fx(m_asOutList[i].pszName, m_asOutList[i].pvalue,
					m_asOutList[i].elements);
		}

		if ((!m_bJournallingClosed)
				&& (g_iOutMode & XFY::Trace::eOM_JOURNALING)) {
			XFY::g_XFYTrace.putFceReturns((const int) 0, this);
		}
	}

	if ((XFY::g_XFYTrace.showEntry()) && m_pszFceName)
		XFY::g_XFYTrace.functionEnd(m_pszFceName, m_lStart,
				NULL/*&m_MemoryData*/);
}

//-----------------------------------------------------------------------
// Object      : XFY::TraceFce
// Method      : registerOutputParam
// Description : Register function output parameter.
//               Output parameter is saved and traced by destructor.
// Parameter
//   fx        I : Pointer to the traced function
//   pszName   I : Name od variable
//   pvalue    I : Pointer to the value
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::TraceFce::registerOutputParam(const char *pszName, void *pvalue,
		TRACEVALUE fx) {
	if (m_iOutCount < 8) {
		m_asOutList[m_iOutCount].pszName = pszName;
		m_asOutList[m_iOutCount].pvalue = pvalue;
		m_asOutList[m_iOutCount].fx = fx;
		m_asOutList[m_iOutCount].elements = -1;
		m_iOutCount++;
	} else {
		printf("Maximal count of output parameter reached\n");
	}
}

//-----------------------------------------------------------------------
// Object      : XFY::TraceFce
// Method      : setOutputArraySize
// Description : Set count of items in a parameter output array
// Parameter
//   iSize    I : count of items
//   pvalue   I : first array item
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void XFY::TraceFce::setOutputArraySize(const char *pszName, const int iSize) {
	for (int iI = 0; iI < m_iOutCount; iI++) {
		if (strcmp(m_asOutList[iI].pszName, pszName) == 0) {
			m_asOutList[iI].elements = iSize;
			return;
		}
	}
}
