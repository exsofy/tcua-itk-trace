#include <string.h>
#include <stdlib.h>

#include <tc/tc.h>
#include <mld\journal\journal.h>
#include <tc/emh.h>

#include "XFY_TRACE.h"


// global object definition for tracing
CXFYTrace g_cXFYTrace;

// Global Version ID string
#define     XFY_TRACE_MAX_VERSION_ID 128
const char *g_szVersionID[XFY_TRACE_MAX_VERSION_ID];
int         g_nVersionID       = 0;


/////////////////////////////////////////////////////////////////////////
// CXFYTrace
//

const char *aszValuePrefix[] = { "V ", "E ", "I ", "? ",
                                 "! ", "O ", "U ", "R ", "X ", "  " };

static int g_iOutMode = 0;             // global static variable
const char* CXFYTrace::s_pszNULL = NULL;   // global static variable
const int* CXFYTrace::s_piNULL = NULL;     // global static variable


//-----------------------------------------------------------------------
// Constructor : CXFYTrace
// Description : Initialize member variables with default or values from
//               environmet
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
CXFYTrace::CXFYTrace ()
{

#ifdef _DEBUG  
  m_iActFlag = eOI_ALL;  // full tracing in debug
#else
  m_iActFlag = 0;   // no tracing in release
#endif

  time_t actTime = time(NULL);
  tm     tmAct;
  
	localtime_s(&tmAct, &actTime);
  strftime ( m_szOutFile, _countof(m_szOutFile), "xfy_itk_XFY_TRACE_%Y%m%d_%H%M%S.log", &tmAct );

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
  char   *pszTraceFile;
	size_t lTraceFile;
	if ( ( _dupenv_s ( &pszTraceFile, &lTraceFile, "XFY_TRACE_FILE" ) == 0 ) && ( pszTraceFile != NULL ) )
  {
    g_iOutMode |= eOM_DEBUGING;  // debugging activated

    transFileName ( pszTraceFile, m_szOutFile, _MAX_PATH, false );
		free ( pszTraceFile );
  }

  char   *pszTraceFlags;
	size_t lTraceFlags;
	if ( ( _dupenv_s ( &pszTraceFlags, &lTraceFlags, "XFY_TRACE_FLAGS" ) == 0 ) && ( pszTraceFlags != NULL ) )
  {
    m_iActFlag = atoi ( pszTraceFlags );
		free ( pszTraceFlags );
	}

  char   *pszTraceFlush;
	size_t lTraceFlush;
	if ( ( _dupenv_s ( &pszTraceFlush, &lTraceFlush, "XFY_TRACE_FLUSH" ) == 0 ) && ( pszTraceFlush != NULL ) )
	{
		m_iUseFlush = atoi ( pszTraceFlush );
	}

  char   *pszTraceFunction;
	size_t lTraceFunction;
	if ( ( _dupenv_s ( &pszTraceFunction, &lTraceFunction, "XFY_TRACE_FUNCTION" ) == 0 ) && ( pszTraceFunction != NULL ) )
  {
    m_iUseTraceFunction = true;
    char *pszEqual = strrchr ( pszTraceFunction, '=' );
    if ( pszEqual == NULL )
    {
      m_iTraceFunctionFlag = m_iActFlag;
      strcpy_s ( m_szTraceFunction, 255, pszTraceFunction );
    }
    else 
    {
      m_iTraceFunctionFlag = atoi ( pszEqual + 1 );
      strncpy_s ( m_szTraceFunction, 255, pszTraceFunction, pszEqual - pszTraceFunction );
      m_szTraceFunction[pszEqual - pszTraceFunction] = '\0';
    }
  }

  if ( m_iActFlag != 0 || m_iTraceFunctionFlag )
  {
    printf ( "XFY Tracing activated on level %d into ", 
             m_iActFlag );
    switch ( g_iOutMode ) {
    case eOM_JOURNALING :
      printf ( "journal_file\n" );
      break;
    case eOM_DEBUGING :
      printf ( "\n%s\n", m_szOutFile );
      break;
    case eOM_JOURNALING | eOM_DEBUGING :
      printf ( "journal_file + \n%s\n", m_szOutFile );
      break;
    }
    if ( m_iTraceFunctionFlag )
    {
      printf ( "    Tracing within function %s on level %d\n",
               m_szTraceFunction, m_iTraceFunctionFlag );
    }
    else
    {
      ITK_set_journalling ( true );
    }
  }
}

//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : OutFile
// Description : Get output stream, if file not opened yet, open it
// Returns     : FILE Pointer to output stream
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void* CXFYTrace::getOutputFile()
{
  if ( m_pFile != NULL )
  {
    if ( m_iUseFlush )
      fflush ( (FILE*)m_pFile );
    return m_pFile;
  }

  if ( (strcmp ( m_szOutFile, "" ) == 0) || (strncmp ( m_szOutFile, "stdout", 6 )) == 0 )
  {
    m_pFile = stdout;
  }
  else if ( strcmp ( m_szOutFile, "stderr" ) == 0 )
  {
    m_pFile = stderr;
  }
  else { 

    char szWOutFile[_MAX_PATH];     // output file

    transFileName ( m_szOutFile, szWOutFile, _MAX_PATH, true );

    if ( ( fopen_s ( (FILE**)&m_pFile, szWOutFile, "w" ) == 0 ) && ( m_pFile == NULL ) )
    {
      fprintf ( stderr,
                "Error: Trace File %s not opened. Standart output is used.",
                szWOutFile );
      m_pFile = stdout;
    }
  }
  putFileBreak();
  return m_pFile;
}


//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : putFileBreak
// Description : Create Start/End marker with time
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void CXFYTrace::putFileBreak()
{
  if ( m_pFile != NULL ) 
  {
    char szTime[80];
    time_t tNow = time(NULL);
		tm     tmNow;
		localtime_s ( &tmNow, &tNow );
    strftime ( szTime, sizeof ( szTime ) - 1, "%Y-%m-%d %H:%M:%S", &tmNow );
    fprintf ( (FILE*)m_pFile,
              "*******************************************************\n"
              "* EXSOFY Trace log %33.33s *\n"
              "*    %-48.48s *\n",
              szTime, m_szAppName );
    for ( int iVer = 0; iVer < g_nVersionID; iVer++ )
    {
      fprintf ( (FILE*)m_pFile, "*    %s", g_szVersionID[iVer] );
    }
    fprintf ( (FILE*)m_pFile, "*******************************************************\n" );
  }
}


//-----------------------------------------------------------------------
// Object      : CXFYTrace
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
const char * CXFYTrace::functionStart ( const char *fceName )
{

  if ( m_iFceLevel <= 0 )
  { // if in top level function
    if ( ITK_ask_journalling() ) // dynamic change of journaling
    { 
      g_iOutMode |= eOM_JOURNALING;
      m_iActFlag |= eOI_JOURNAL;
    }
    else
    {
      g_iOutMode &= ~eOM_JOURNALING;
      m_iActFlag &= ~eOI_JOURNAL;
    }
    m_iFceLevel = 0; // reset to zero
  }

  // check for activating of function tracing
  if ( m_iUseTraceFunction && m_iStopLevel < 0 )
  {
    if ( strcmp ( fceName, m_szTraceFunction ) == 0 )
    {
      m_iSavedFlag = m_iActFlag;
      m_iActFlag = m_iTraceFunctionFlag;
      m_iStopLevel = m_iFceLevel;
      m_iJournalingFlag = ITK_ask_journalling ();
    }
  }

  if ( !showEntry() )
    return NULL;

  if ( g_iOutMode & eOM_DEBUGING )
  {
    int iSpaces = 2 * m_iFceLevel++;
    fprintf ( (FILE*)getOutputFile(), "%*s--> %s\n", iSpaces, "", fceName );
  }

  if ( g_iOutMode & eOM_JOURNALING )
  {
    JOURNAL_routine_start( fceName );
  }

  return fceName;
}

void CXFYTrace::finishFunctionHeader () 
{
  // at the start of function activate header
  JOURNAL_routine_call();
}


//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : functionEnd
// Description : Put Function end marker into logfile
//               and decrease function level
// Parameter
//   fceName   I : Function name
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void CXFYTrace::functionEnd ( const char *fceName,
                              const time_t lStartTime,
                              const void *MemoryStruct
                            )
{

  if ( g_iOutMode & eOM_DEBUGING )
  {
    if ( m_iFceLevel > 0 ) m_iFceLevel--;
    int iSpaces = 2 * m_iFceLevel;
    fprintf ( (FILE*)getOutputFile(), "%*s<-- %s (%2.2f sec) %s\n",
                                iSpaces, "", fceName,
                                lStartTime == 0 ? 0 : ( clock() - lStartTime ) / 1.0 / CLOCKS_PER_SEC,
                                ""/*szMemoryMessage*/
                                );
  }

  if ( g_iOutMode & eOM_JOURNALING )
  {
    JOURNAL_routine_end();
  }

  // check for deactivating of function tracing
  if ( m_iUseTraceFunction && m_iStopLevel == m_iFceLevel ) 
  {
    m_iActFlag = m_iSavedFlag;
    m_iStopLevel = -1;
    ITK_set_journalling ( m_iJournalingFlag );
  }

}

//-----------------------------------------------------------------------
// Object      : CXFYTrace
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
size_t CXFYTrace::transFileName ( const char *pszFrom, char *pszTo,
                                  const int iBufferLen, const int isTransAll = true ) const
{
  char szBuf[1024];
  char *pszSaver = pszTo;

  strncpy_s ( szBuf, _countof(szBuf), pszFrom, 1023 ); // avoid problems with self-translation
  const char *pszRunner = szBuf;

  if ( pszFrom == pszTo )
  {

  }

  while ( *pszRunner != '\0' )
  {
    if ( *pszRunner == '*' )
    {
      size_t iFreeSpace = iBufferLen - ( pszSaver - pszTo );
      switch ( *++pszRunner )
      {
      case 'D' :
        {
          time_t actTime = time(NULL);
					tm     tmAct;
					localtime_s(&tmAct,&actTime);
          if ( strftime ( pszSaver, iFreeSpace,
                          "%Y%m%d_%H%M%S", &tmAct ) > 0 )
            pszSaver += strlen ( pszSaver );
        }
        break;         
      case 'P' :
        {
          char szProgEnv[] = "itk";

          size_t iAddLen = strlen ( szProgEnv );
          if ( iAddLen < iFreeSpace )
          {
            strcpy_s ( pszSaver, iFreeSpace, szProgEnv );
            pszSaver += iAddLen;
          }
        }
        break;         
      case 'U' :
        {
#ifdef _WIN32
          size_t dwSize = iFreeSpace;
          
//          if ( GetUserName ( pszSaver, &dwSize ) != 0 )
          {
//            pszSaver += dwSize - 1;
          }
#endif
        }
      break;
      case 'A' : // only for trans all run
        if ( isTransAll )
        {
          if ( ( m_szAppName != NULL ) && ( (int)strlen ( m_szAppName ) < iFreeSpace ) )
          {
            pszSaver += sprintf_s ( pszSaver, iFreeSpace, "%s", m_szAppName );
          }
        }
        else
        {
          if ( iFreeSpace > 2 )
          {
            *pszSaver++ = '*';
            *pszSaver++ = *pszRunner;
          }
        }
        break;
      case 'C' : 
        {
#ifdef _WIN32
          size_t dwSize = iFreeSpace;

//          if ( GetComputerName ( pszSaver, &dwSize ) != 0 )
          {
//            pszSaver += dwSize - 1;
          }
#endif
        }
        break;
      default :
        fprintf ( stderr, "Unsupported Tracing Option *%c at position %d.\n", *pszRunner, pszRunner - szBuf );
      }
      pszRunner++;
    }
    else
    {
      if ( pszSaver - pszTo < iBufferLen )
        *pszSaver++ = *pszRunner++;
    }
  }

  *pszSaver = 0;

  return strlen ( pszTo );
}

int CXFYTrace::putFceReturns ( const int &Value, const CXFYTraceFce *pOutItem ) \
{ 
  if ( g_cXFYTrace.showParam() )
  {
    if ( g_iOutMode & eOM_DEBUGING )
    {
      fprintf( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%sValue = %d \n", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT_T_RET], Value);
    }
    if ( g_iOutMode & eOM_JOURNALING )
    { 
      JOURNAL_return_value (Value);
      ((CXFYTraceFce *)pOutItem)->closeJournalling();
    }
  }
  return Value;
}

#define ReturnsValSimple(ptype,pform) \
ptype CXFYTrace::putFceReturns ( const ptype &Value, const CXFYTraceFce *pOutItem ) \
{ if ( g_cXFYTrace.showParam() ) {\
    if ( g_iOutMode & eOM_DEBUGING ) { \
      fprintf( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%sValue = " pform "\n", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT_T_RET], Value); }\
  } return (ptype)Value;\
}

#ifndef _SUN
ReturnsValSimple ( bool, "%d" );
#endif

ReturnsValSimple ( unsigned int, "%u" );
ReturnsValSimple ( long, "%ld" );
ReturnsValSimple ( unsigned long, "%lu" );
ReturnsValSimple ( double, "%lf" );
ReturnsValSimple ( float, "%f" );

ReturnsValSimple ( char, "%c" );

#undef ReturnsValSimple

#define ReturnsValPtrSimple(ptype,pform) \
ptype * CXFYTrace::putFceReturns ( const ptype *const &Value, const CXFYTraceFce *pOutItem ) \
{ if ( g_cXFYTrace.showParam() ) { \
    if ( g_iOutMode & eOM_DEBUGING ) { \
      fprintf( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%sValue(%p) = ", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT_T_RET], Value );\
      if ( Value != NULL ) fprintf( (FILE*)g_cXFYTrace.getOutputFile(), pform"\n", *Value);\
      else                 fprintf( (FILE*)g_cXFYTrace.getOutputFile(), "NULL\n" ); } }\
    if ( g_iOutMode & eOM_JOURNALING ) { \
    } \
  return (ptype *)Value;\
}

ReturnsValPtrSimple ( int, "%d" );
ReturnsValPtrSimple ( unsigned int, "%u" );
ReturnsValPtrSimple ( long, "%ld" );
ReturnsValPtrSimple ( unsigned long, "%lu" );
ReturnsValPtrSimple ( double, "%lf" );
ReturnsValPtrSimple ( float, "%f" );

#undef ReturnsValPtrSimple

#define ReturnsValDefSimple(ptype,pform) \
ptype CXFYTrace::putFceReturns ( const ptype const &Value, const CXFYTraceFce *pOutItem ) \
{ if ( g_cXFYTrace.showParam() ) { \
    if ( g_iOutMode & eOM_DEBUGING ) { \
      fprintf( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%sValue(%p) = ", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT_T_RET], Value );\
      if ( Value != NULL ) fprintf( (FILE*)g_cXFYTrace.getOutputFile(), pform"\n", Value);\
      else                 fprintf( (FILE*)g_cXFYTrace.getOutputFile(), "NULL\n" ); } }\
    if ( g_iOutMode & eOM_JOURNALING ) { \
    } \
  return (ptype)Value;\
}

ReturnsValDefSimple ( void *, "%p" );
ReturnsValDefSimple ( char *, "%s" );

#undef ReturnsValDefSimple


void CXFYTrace::doFceThrows ( const char *pszMsg ) \
{ if ( g_cXFYTrace.showEntry() ) { \
    if ( g_iOutMode & eOM_DEBUGING ) { \
      fprintf( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s %s\n",\
      g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT_V_EX], pszMsg); }\
    if ( g_iOutMode & eOM_JOURNALING ) { \
      ;
    } \
  } \
}


//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : putErrorMessage
// Description : Put a translated message into the output
// Parameter
//   iMessage   I : 
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void CXFYTrace::putErrorMessage ( const int iMessage )
{
    if ( g_iOutMode & eOM_DEBUGING ) {
		int nFails;                                                    
		const int *aiSeverities = NULL;                                                   
		const int *aiFails = NULL;                                                   
		const char **asTexts = NULL;                                        
		EMH_ask_errors( &nFails, &aiSeverities, &aiFails, &asTexts ); 
		if ( nFails > 0 && aiFails[nFails-1] == iMessage ) {                                           
			// from error stack
			fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "  Teamcenter ERROR: %d [%s]\n", aiFails[nFails-1], asTexts[nFails-1] );
		}                                                             
		else {                      
			char* pszError = NULL;
			EMH_ask_error_text ( iMessage, &pszError ); 
			if ( pszError != NULL ) {
				fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "*** Error %d: %s\n", iMessage, pszError );
				MEM_free ( pszError );
			}
		}
	}
}


//-----------------------------------------------------------------------
// Object      : CXFYTrace
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
int CXFYTrace::putErrorReturns ( const int &Value, const CXFYTraceFce *pOutItem )
{ 
  putErrorMessage ( Value );
//  if ( g_cXFYTrace.ShowParam() )
//    fprintf( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%sValue = " pform "\n", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT_T_RET], Value);
  return Value;
}

//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : putMessage
// Description : Print a message into output - like printf
// Parameter
//   format   I : formater string
//   ...      I : Values list
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
int CXFYTrace::putMessage ( const char *format, ... ) const
{
  if ( !showMessage() ) return 0;

  if ( g_iOutMode & eOM_DEBUGING )
  {
    va_list arglist;
    va_start ( arglist, format );
    int iLen = fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s", g_cXFYTrace.getLevel(), "" );
    iLen += vfprintf ( (FILE*)g_cXFYTrace.getOutputFile(), format, arglist );
    va_end ( arglist );
    return iLen;
  }

  if ( g_iOutMode & eOM_JOURNALING )
  {
    va_list arglist;
    va_start ( arglist, format );
#ifdef _WIN32
    char sOut[512] = { '\0' };
    int iLen = _vsnprintf_s ( sOut, _countof(sOut), 512, format, arglist );
#else
    char sOut[8192] = { '\0' };
    int iLen = vsprintf ( sOut, format, arglist );
#endif
    va_end ( arglist );

    JOURNAL_comment ( sOut );
    return iLen;
  }

  return 0;
}

int CXFYTrace::putMessageVA ( const char *format, va_list arglist ) const
{
  if ( !showMessage() ) return 0;

  if ( g_iOutMode & eOM_DEBUGING )
  {
    int iLen = fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s", g_cXFYTrace.getLevel(), "" );
    iLen += vfprintf ( (FILE*)g_cXFYTrace.getOutputFile(), format, arglist );
    return iLen;
  }

  return 0;
}

//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : ReportFceCall
// Description : Reports a call of a function
//   ufName   I : Name of called function
//   fileName I : Name of sourcefile
//   lineNum  I : number of source code line
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 11.04.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
int CXFYTrace::reportFceCall ( const char *ufName, const char *fileName, const int lineNum,
                               const int iRetVal )
{
	if ( g_iOutMode & eOM_DEBUGING )
	{
		if (iRetVal == 0 && showUfCall() )
		{
			fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s\n", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT_UF], ufName );
		}
		if (iRetVal) {
			time_t t = time(0); // obtain the current time_t value
			tm now;
			char tmdescr[200]={0};
			if ( ( localtime_s (&now, &t ) != 0  ) || ( strftime(tmdescr, sizeof(tmdescr)-1, ", at %H:%M:%S", &now)  <= 0 ) ){
				// error occured, empty string
				*tmdescr = 0;
			}

			fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "*** %s line : %d%s\n*** %s\n",
				fileName, lineNum, tmdescr, ufName);
			putErrorMessage ( iRetVal );
		}
	}

  if ( g_iOutMode & eOM_JOURNALING )
  {
    ;
  }

  return(iRetVal);
}

//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : ReportUFCall
// Description : Trace a call of UF void function
//   ufName   I : Name of called function
//   fileName I : Name of sourcefile
//   lineNum  I : number of source code line
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void CXFYTrace::reportFceCall ( const char *ufName, const char *fileName, const int lineNum )
{
  if ( !showUfCall() )
    return;

  if ( g_iOutMode & eOM_DEBUGING )
  {
    fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s\n", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT_UF], ufName );
  }

  if ( g_iOutMode & eOM_JOURNALING )
  {
    ;
  }
}


//-----------------------------------------------------------------------
// Destructor  : CXFYTrace
// Description : If logfile opened, put end marker and close it
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
CXFYTrace::~CXFYTrace ()
{
  if ( m_pFile ) 
  {
    if ( g_iOutMode & eOM_DEBUGING )
    {
      putFileBreak();
    }
  }
  if ( (m_pFile != NULL) && (m_pFile != stdout) && (m_pFile != stderr) )
    fclose ( (FILE*)m_pFile );
}


/////////////////////////////////////////////////////////////////////////
// CXFYTraceFce
//

//-----------------------------------------------------------------------
// Constructor : CXFYTraceFce
// Description : Put function name into tracelog and save it
// Parameter
//   fceName   I : Function name
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
CXFYTraceFce::CXFYTraceFce( const char *fceName )
{
  m_pszFceName = g_cXFYTrace.functionStart( fceName );
  m_iOutCount = 0;
  m_lStart = clock();

  m_bJournallingClosed = false;
}


const int CXFYTrace::getOutMode ()  const
{
  return g_iOutMode;
};


//-----------------------------------------------------------------------
// Macros for overloaded TraveValue function with simple type parameter
// and pointer to simple type parameter
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
#ifdef IMAN_VERSION
#define DO_JOURNALING(mode,jourtype,jourpref) \
  if ( mode & CXFYTrace::eOM_JOURNALING ) switch ( eVT ) \
    { case eVT_IO : JOURNAL_output_argument ( pszName ); \
      case eVT_I : JOURNAL_##jourtype##_in ( jourpref value ); break; \
      case eVT_O : JOURNAL_##jourtype##_out ( pszName, jourpref value ); break; \
      default :;  }
#else
#define DO_JOURNALING(mode,jourtype,jourpref)
#endif

#define TraceSimple(fcename,partype,parformat,jourtype,jourpref) \
static void TraceVal_##fcename (const char *pszName, const partype *value, int iDeep ) \
{/*  if ( g_cXFYTrace.getOutMode() & CXFYTrace::eOM_DEBUGING ) */{\
     CXFYTrace::putVariable ( pszName, *value, CXFYTrace::eVT_O_RET ); } \
} \
void CXFYTrace::putVariable ( const char *pszName, const partype &value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem ) \
{ DO_JOURNALING (g_cXFYTrace.getOutMode(),##jourtype, ##jourpref); \
  if ( eVT != eVT_O ) { \
    if ( g_cXFYTrace.getOutMode() & CXFYTrace::eOM_DEBUGING ) {\
      fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s = "parformat" ("#partype")\n", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value ); }\
    } \
  if ( eVT == eVT_O || eVT == eVT_IO ) \
    pOutItem->registerOutputParam ( pszName, (void *)(&value), (TRACEVALUE)TraceVal_##fcename ); } \
static void TraceVal_p_##fcename ( const char *pszName, const partype **value, int iDeep ) \
{ CXFYTrace::putVariable ( pszName, *value, CXFYTrace::eVT_O_RET ); } \
void CXFYTrace::putVariable ( const char *pszName, const partype *const &value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem, int iDeep ) \
{ if ( eVT != eVT_O ) {\
    if ( g_cXFYTrace.getOutMode() & CXFYTrace::eOM_DEBUGING ) {\
      if ( value == NULL ) fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s = %p ("#partype" *)   *%s = undefined\n", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value, pszName); \
      else fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s = %p ("#partype" *)   *%s = "parformat"\n", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value, pszName, *value ); \
   }}\
  if ( eVT == eVT_O || eVT == eVT_IO ) \
    pOutItem->registerOutputParam ( pszName, (void **)&(value), (TRACEVALUE)TraceVal_p_##fcename );\
}; 


TraceSimple ( short, short, "%hd", nyi, & );
TraceSimple ( int, int, "%d", integer, (const int) );
#ifndef _SUN
TraceSimple ( bool, bool, "%d", logical, (const logical) );
#endif
TraceSimple ( long, long, "%ld", nyi, & );
TraceSimple ( unsigned_short, unsigned short, "hu%", nyi, & );
TraceSimple ( unsigned_int, unsigned int, "%u", nyi, & );
TraceSimple ( unsigned_long, unsigned long, "%lu", nyi, & );
TraceSimple ( float, float, "%f", nyi, & );
TraceSimple ( double, double, "%lf", double, (const double) );

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
static void TraceVal_char ( const char *pszName, const char * value, int iDeep ) 
{
  CXFYTrace::putVariable ( pszName, *value, CXFYTrace::eVT_O_RET );
}

//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//            I : Variable value of type character
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void CXFYTrace::putVariable ( const char *pszName, const char &value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem )
{
  if ( eVT != eVT_O )
  {
    if ( g_iOutMode & eOM_DEBUGING )
    {
      fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s = %c (char)\n",
                g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value );
    }
    if ( g_iOutMode & eOM_JOURNALING )
    {
      DO_JOURNALING ( g_iOutMode, char, (const char));
    }
  }
  if ( eVT == eVT_O || eVT == eVT_IO )
    pOutItem->registerOutputParam ( pszName, (void *)&value, (TRACEVALUE)TraceVal_char );
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
//static void TraceVal_obj ( const char *pszName, CXFYTraceObject *traceObject, int iDeep ) 
//{
//  CXFYTrace::putVariable ( pszName, *traceObject, CXFYTrace::eVT_O_RET, iDeep );
//}

//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//            I : Variable value of type character
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
/*void CXFYTrace::putVariable ( const char *pszName, CXFYTraceObject &traceObject, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem, int iDeep )
{
  if ( eVT != eVT_O )
  {
    if ( g_iOutMode & eOM_DEBUGING )
    {
      traceObject.putLinesToOutput ( g_cXFYTrace.getOutputFile(),
                                     g_cXFYTrace.getLevel(), aszValuePrefix[eVT],
                                     pszName );
    }
    if ( g_iOutMode & eOM_JOURNALING )
    {
#ifdef IMAN_VERSION
      traceObject.putToJournal ( pszName, eVT );
#endif
    }
  }
  if ( eVT == eVT_O || eVT == eVT_IO )
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
static void TraceVal_str ( const char *pszName, const char **value, int iDeep )
{
  CXFYTrace::putVariable ( pszName, *value, CXFYTrace::eVT_O_RET );
}

#ifdef _WIN32
void CXFYTrace::putVariable ( const char *pszName, char * &value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem, int iDeep )
{
  putVariable ( pszName, (const char*&)value, eVT, pOutItem, iDeep );
}
#endif

#ifndef _AIX
void CXFYTrace::putVariable ( const char *pszName, char *const &value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem, int iDeep )
{
  putVariable ( pszName, (const char*&)value, eVT, pOutItem, iDeep );
}
#endif

//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//             I : Variable value of type zero terminated string
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void CXFYTrace::putVariable ( const char *pszName, const char *const &value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem, int iDeep )
{
  if ( eVT != eVT_O )
  {
    if ( value == NULL )
    {
      if ( g_iOutMode & eOM_DEBUGING )
      {
        fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s = %p (char *) len=undefined\n%*s  *%s = undefined\n",
                  g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value,
                  g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT_EMPTY], pszName );
      }
      if ( g_iOutMode & eOM_JOURNALING )
      {
        DO_JOURNALING ( g_iOutMode, string, (const char *) );
//        JOURNAL_string_out(pszName, "NULL");
      }
    }
    else
    {
      if ( g_iOutMode & eOM_DEBUGING )
      {
        fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s = %p (char *)\n%*s%s*%s = \"%s\", len=%d\n",
                     g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value,
                     g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT_EMPTY], pszName, value, strlen(value) );
      }
      if ( g_iOutMode & eOM_JOURNALING )
      {
        DO_JOURNALING ( g_iOutMode, string, (const char *) );
      }
    }
  }
  if ( eVT == eVT_O || eVT == eVT_IO )
    pOutItem->registerOutputParam ( pszName, (void **)&value, (TRACEVALUE)TraceVal_str );
}

#ifdef IMAN_VERSION
#undef DO_JOURNALING
#endif


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
static void TraceVal_pstr ( const char *pszName, const char ***value, int iDeep )
{
  CXFYTrace::putVariable ( pszName, *value, CXFYTrace::eVT_O_RET, NULL, iDeep );
}

#ifdef _WIN32
void CXFYTrace::putVariable ( const char *pszName, char **&value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem, int iDeep )
{
  putVariable ( pszName, (const char**&)value, eVT, pOutItem, iDeep );
}
#endif

void CXFYTrace::putVariable ( const char *pszName, char **const &value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem, int iDeep )
{
  putVariable ( pszName, (const char**&)value, eVT, pOutItem, iDeep );
}

//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//             I : Variable value of type zero terminated string
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void CXFYTrace::putVariable ( const char *pszName, const char ** const &value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem, int iDeep )
{
  if ( eVT != eVT_O )
  {
    if ( value == NULL )
    {
      if ( g_iOutMode & eOM_DEBUGING )
      {
        fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s&%s = %p (char *) len=undefined\n%*s  *%s = undefined\n",
                  g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value, g_cXFYTrace.getLevel(), "", pszName );
      }
      if ( g_iOutMode & eOM_JOURNALING )
      {
#ifdef IMAN_VERSION
        JOURNAL_string_out(pszName, "NULL");
#endif
      }
    }
    else if ( *value == NULL )
    {
      if ( g_iOutMode & eOM_DEBUGING )
      {
        fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s = %p (char *) len=undefined\n%*s  *%s = undefined\n",
                  g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, *value, g_cXFYTrace.getLevel(), "", pszName );
      }
      if ( g_iOutMode & eOM_JOURNALING )
      {
#ifdef IMAN_VERSION
        JOURNAL_string_out(pszName, "NULL");
#endif
      }
    }
    else
    {
      if ( g_iOutMode & eOM_DEBUGING )
      {
        fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s = %p (char **)\n",
                     g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, *value );
      }
      if ( g_iOutMode & eOM_JOURNALING )
      {
#ifdef IMAN_VERSION
          JOURNAL_address_out(pszName, value);
#endif
      }
/* value changed to *value. used by holetabel program. check it with iman */
      char **pszArray = ((char **) value);
      if ( pszArray == NULL )
      {
      }
      else
      {
        if ( g_iOutMode & eOM_DEBUGING )
        {           
//          fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s*%s = %p (char *) count=%d\n",
//                    g_cXFYTrace.getLevel() + 2, "", aszValuePrefix[eVT_EMPTY], pszName, *pszArray, iDeep );
                    
          if ( iDeep < 0 )
          {
            fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s**%s = \"%s\", len=%d\n",
                      g_cXFYTrace.getLevel() + 2, "", aszValuePrefix[eVT_EMPTY], pszName, *pszArray, strlen(*pszArray) );
          }
          else
          {
            for ( int iI = 0; iI < iDeep; iI++ )
            {
              if ( pszArray[iI] == NULL )
              {
                fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s*%s[%d] = NULL\n",
                          g_cXFYTrace.getLevel() + 2, "", aszValuePrefix[eVT_EMPTY], pszName, iI );
              }
              else
              {
                fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s*%s[%d] = \"%s\", len=%d\n",
                          g_cXFYTrace.getLevel() + 2, "", aszValuePrefix[eVT_EMPTY], pszName, iI, pszArray[iI], strlen(pszArray[iI]) );
              }
            }
          }
        }
        if ( g_iOutMode & eOM_JOURNALING )
        {
#ifdef IMAN_VERSION
          JOURNAL_string_array_out(pszName, iDeep, (char **)value);
#endif
        }
      }
    }
  }
  if ( eVT == eVT_O || eVT == eVT_IO )
  {
    if ( pOutItem != NULL )
    {
      pOutItem->registerOutputParam ( pszName, (void ***)&value, (TRACEVALUE)TraceVal_pstr );
    }
  }
}

#ifdef IMAN_VERSION

#ifdef _NOT_USED

#ifdef MOTIF_UIF

//-----------------------------------------------------------------------
// Function    : TraceVal_UIF_message
// Description : Put a UIF_message variable into tracelog
// Parameter
//   pszName   I : Parameter name
//   value     I : Pointer to Parameter value
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
static void TraceVal_UIF_message ( const char *pszName, const UIF_message_t *value, int iDeep )
{
  CXFYTrace::putVariable ( pszName, *value, CXFYTrace::eVT_O_RET );
}

//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//             I : Variable value of type zero terminated string
//-----------------------------------------------------------------------
// Written by   Svatos Coufal on 30.03.00
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void CXFYTrace::putVariable ( const char *pszName, const UIF_message_s &value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem )
{
  if ( eVT != eVT_O )
  {
    fprintf ( g_cXFYTrace.getOutputFile(), "%*s%s%s = %p UIF_message_t [%p,%u,\"%s\",%d,%p,%p]\n",
              g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, &value, value.gizmo, value.tag,
                                          ( value.obj_type == NULL ) ? "null" : value.obj_type,
                                          value.message, value.system_data, value.user_data );
  }
  if ( eVT == eVT_O || eVT == eVT_IO ) \
    pOutItem->AddOutputParam ( pszName, (void *)&value, (TRACEVALUE)TraceVal_UIF_message );
}

#endif

#endif

#endif

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
void TraceVal_p_void ( const char *pszName, const void** value, int iDeep )
{
  CXFYTrace::putVariable ( pszName, *value, CXFYTrace::eVT_O_RET );
}

//-----------------------------------------------------------------------
// Object      : CXFYTrace
// Method      : TraceVal
// Description : Returns the trace function
// Parameter
//   pszName   I : Variable name
//   value     I : Variable value of type void pointer
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void CXFYTrace::putVariable ( const char *pszName, const void *const &value, eVALUE_TYPE eVT, CXFYTraceFce *pOutItem ) 
{
  if ( eVT != eVT_O )
  {
    if ( g_iOutMode & eOM_DEBUGING )
    {
      fprintf ( (FILE*)g_cXFYTrace.getOutputFile(), "%*s%s%s = %p (void*)\n", g_cXFYTrace.getLevel(), "", aszValuePrefix[eVT], pszName, value );
    }
    if ( g_iOutMode & eOM_JOURNALING )
    {
      JOURNAL_address_out ( pszName, value);
    }
 }
  if ( eVT == eVT_O || eVT == eVT_IO ) \
    pOutItem->registerOutputParam ( pszName, (void *)&value, (TRACEVALUE)TraceVal_p_void );
}



//-----------------------------------------------------------------------
// Destructor  : CXFYTraceFce
// Description : Put saved function name into tracelog
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
CXFYTraceFce::~CXFYTraceFce()
{
  if ( g_cXFYTrace.showParam() )
  {
    for ( int i = 0; i < m_iOutCount; i++ ) 
      m_asOutList[i].fx( m_asOutList[i].pszName, m_asOutList[i].pvalue, m_asOutList[i].elements );

    if ( ( !m_bJournallingClosed )  && ( g_iOutMode & CXFYTrace::eOM_JOURNALING ) )
    {
      g_cXFYTrace.putFceReturns ( (const int)0, this );
    }
  }

  if ( ( g_cXFYTrace.showEntry() ) &&
       m_pszFceName )
    g_cXFYTrace.functionEnd ( m_pszFceName, m_lStart, NULL/*&m_MemoryData*/ );
}

//-----------------------------------------------------------------------
// Object      : CXFYTraceFce
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
void CXFYTraceFce::registerOutputParam ( const char *pszName,
                                         void *pvalue, TRACEVALUE fx )
{
  if ( m_iOutCount < 8 ) 
  {
    m_asOutList[m_iOutCount].pszName = pszName;
    m_asOutList[m_iOutCount].pvalue = pvalue;
    m_asOutList[m_iOutCount].fx = fx;
    m_asOutList[m_iOutCount].elements = -1;
    m_iOutCount++;
  }
  else
  {
    printf ( "Maximal count of output parameter reached\n" );
  }
}

//-----------------------------------------------------------------------
// Object      : CXFYTraceFce
// Method      : setOutputArraySize
// Description : Set count of items in a parameter output array
// Parameter
//   iSize    I : count of items
//   pvalue   I : first array item
//-----------------------------------------------------------------------
// Changes : 
// Date        By      Reason
//-----------------------------------------------------------------------
void CXFYTraceFce::setOutputArraySize ( const char *pszName, const int iSize )
{
  for ( int iI = 0; iI < m_iOutCount; iI++ )
  {
    if ( strcmp ( m_asOutList[iI].pszName, pszName ) == 0 )
    {
      m_asOutList[iI].elements = iSize;
      return;
    }
  }
}
