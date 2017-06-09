#ifndef __XFY_TRACE_H_INCLUDED_
#define __XFY_TRACE_H_INCLUDED_

#include <time.h>
#include <stdarg.h>

typedef void (*TRACEVALUE)(const char *, void*, int);

typedef struct {
    TRACEVALUE fx;
    void * pvalue;
    int    elements;
    const char *pszName;  } SOutputParamm;


// scope object for every tracing function
class CXFYTraceFce
{

// Constructors
public:  
  CXFYTraceFce( const char *fceName );

// Operations
public:
  void registerOutputParam ( const char *pszName, void *pvalue, TRACEVALUE fx );
  void setOutputArraySize ( const char *pszName, const int iSize );
  void closeJournalling() { m_bJournallingClosed = true; };

// Implementation
public:
  ~CXFYTraceFce();

protected:
  const char * m_pszFceName;

private:
  SOutputParamm m_asOutList[8];
  int m_iOutCount;
  time_t m_lStart;

protected:
  bool m_bJournallingClosed; // if the function returned value yet

};


// global object for tracing
class CXFYTrace 
{
public:
  enum eVALUE_TYPE { eVT_V = 0, eVT_V_END, eVT_I, eVT_O, eVT_IO, eVT_O_RET, eVT_UF, eVT_T_RET,
                     eVT_V_EX,
                     eVT_EMPTY, 
                     eVT_COUNT /*must be last*/ };
  enum eOUTPUT_MODE { eOM_DEBUGING = 0x01, eOM_JOURNALING = 0x02 };

private:
  enum eOutputItem { eOI_FCE_CALL = 0x0001, eOI_ENTRY    = 0x0002, eOI_PARAM  = 0x0004,
                     eOI_VALUES  = 0x0008, eOI_MESSAGES = 0x0010, eOI_MEMORY = 0x0020,
                     eOI_JOURNAL = 0x4000,
                     eOI_ALL = eOI_FCE_CALL | eOI_ENTRY    | eOI_PARAM  | 
                               eOI_VALUES  | eOI_MESSAGES | eOI_MEMORY };

// Constructors

public:
  CXFYTrace();
  ~CXFYTrace ();

// Attributes
public:
  void* getOutputFile();

  const int getLevel() { return m_iFceLevel << 1; };
  static const char *getHelpMessage ();

  static const char *s_pszNULL;
  static const int  *s_piNULL;

  static const eVALUE_TYPE translateValueType ( const char *pszType );
  
// Operations
public:
  const char * functionStart ( const char *pszName );
  void functionEnd ( const char *pszName, const time_t lStartTime, const void *pMemoryStruct );

  void setAppName( const char * pszName );
  static void setAppVersion ( const char *AppVersion );

  int putMessage ( const char *format, ... ) const;
  int putMessageVA ( const char *format, va_list ap ) const;
  int reportFceCall ( const char *ufName, const char *fileName, const int lineNum, const int iRetVal );
  void reportFceCall ( const char *ufName, const char *fileName, const int lineNum );

  const int showMessage () const { return m_iActFlag & ( eOI_MESSAGES | eOI_JOURNAL ); };
  const int showValues ()  const { return m_iActFlag & ( eOI_VALUES   | eOI_JOURNAL ); };
  const int showParam ()   const { return m_iActFlag & ( eOI_PARAM    | eOI_JOURNAL ); };
  const int showEntry ()   const { return m_iActFlag & ( eOI_ENTRY    | eOI_JOURNAL ); };
  const int showUfCall ()  const { return m_iActFlag & ( eOI_FCE_CALL | eOI_JOURNAL ); };
  const int showMemory ()  const { return m_iActFlag & ( eOI_MEMORY   | eOI_JOURNAL ); };
  const int getOutMode ()  const;

  const char* getTraceFileName() const { return m_szOutFile; };

  static void finishFunctionHeader();  // ends the function header in journal file

#define XFY_TRACE_VAL(ptype) static void putVariable ( const char *Name, const ptype &Value, CXFYTrace::eVALUE_TYPE eVT = eVT_V, CXFYTraceFce *pzrhFce = NULL  )
#define XFY_TRACE_PVAL(ptype) static void putVariable ( const char *Name, const ptype *const &Value, CXFYTrace::eVALUE_TYPE eVT = eVT_V, CXFYTraceFce *pzrhFce = NULL, int iDeep = -1 )

  XFY_TRACE_VAL ( char );
  XFY_TRACE_PVAL ( char );

  XFY_TRACE_VAL ( short );
  XFY_TRACE_PVAL ( short );

  XFY_TRACE_VAL ( int );
  XFY_TRACE_PVAL ( int );

#ifndef _SUN
  XFY_TRACE_VAL ( bool );
  XFY_TRACE_PVAL ( bool );
#endif

  XFY_TRACE_VAL ( long );
  XFY_TRACE_PVAL ( long );

  XFY_TRACE_VAL ( unsigned short );
  XFY_TRACE_PVAL ( unsigned short );

  XFY_TRACE_VAL ( unsigned int );
  XFY_TRACE_PVAL ( unsigned int );

  XFY_TRACE_VAL ( unsigned long );
  XFY_TRACE_PVAL ( unsigned long );

  XFY_TRACE_VAL ( float );
  XFY_TRACE_PVAL ( float );

  XFY_TRACE_VAL ( double );
  XFY_TRACE_PVAL ( double );

#undef XFY_TRACE_VAL
#undef XFY_TRACE_PVAL

static void putVariable ( const char *Name, const void *const &Value, CXFYTrace::eVALUE_TYPE eVT = eVT_V, CXFYTraceFce *pzrhFce = NULL );

#ifndef _AIX
  static void putVariable ( const char *Name, char *const &Value, CXFYTrace::eVALUE_TYPE eVT = eVT_V, CXFYTraceFce *pzrhFce = NULL, int iDeep = -1  );
#endif
static void putVariable ( const char *Name, char ** const &Value, CXFYTrace::eVALUE_TYPE eVT = eVT_V, CXFYTraceFce *pzrhFce = NULL, int iDeep = -1 );
static void putVariable ( const char *Name, const char ** const &Value, CXFYTrace::eVALUE_TYPE eVT = eVT_V, CXFYTraceFce *pzrhFce = NULL, int iDeep = -1 );
#ifdef _WIN32
static void putVariable ( const char *Name, char * &Value, CXFYTrace::eVALUE_TYPE eVT = eVT_V, CXFYTraceFce *pzrhFce = NULL, int iDeep = -1  );
static void putVariable ( const char *Name, char ** &Value, CXFYTrace::eVALUE_TYPE eVT = eVT_V, CXFYTraceFce *pzrhFce = NULL, int iDeep = -1 );
#endif

//static void putTraceVal ( const char *Name, CXFYTraceObject &Object, CXFYTrace::eVALUE_TYPE eVT = eVT_V, CXFYTraceFce *pzrhFce = NULL, int iDeep = -1 );

#define RETURNS_VAL(ptype) ptype putFceReturns ( const ptype &Value, const CXFYTraceFce *pzrhFce = NULL )
#define RETURNS_PVAL(ptype) ptype putFceReturns ( const ptype const &Value, const CXFYTraceFce *pzrhFce = NULL )

  RETURNS_PVAL ( void* );

#ifndef _SUN
  RETURNS_VAL ( bool );
  RETURNS_PVAL ( bool* );
#endif

  RETURNS_VAL ( int );
  RETURNS_PVAL ( int* );
  RETURNS_VAL ( unsigned int );
  RETURNS_PVAL ( unsigned int* );

  RETURNS_VAL ( long );
  RETURNS_PVAL ( long* );
  RETURNS_VAL ( unsigned long );
  RETURNS_PVAL ( unsigned long* );

  RETURNS_VAL ( double );
  RETURNS_PVAL ( double* );
  RETURNS_VAL ( float );
  RETURNS_PVAL ( float* );

  RETURNS_VAL ( char );
  RETURNS_PVAL ( char* );

#undef  RETURNS_VAL
#undef  RETURNS_PVAL

  // print out the exception message
  void doFceThrows ( const char *Msg );

  // print out error code translation
  int putErrorReturns ( const int &Value, const CXFYTraceFce *pOutItem = NULL );

// Implementations
protected:
  int  m_iActFlag;           // active debug flag
  char m_szOutFile[256];     // output file
  char m_szAppName[32];      // application name

  void *m_pFile;             // output file handle

  int  m_iUseTraceFunction;  // function tracking is active
  char m_szTraceFunction[256];   // function for tracing
  int  m_iTraceFunctionFlag; // flags for function tracing
  int  m_iSavedFlag;         // flags to be switched back
  int  m_iJournalingFlag;    // jornaling flag before entry of tracking function
  int  m_iStopLevel;         // level to set function tracking back

  int  m_iUseFlush;          // use flush after every file access
    
  void putFileBreak();       // draw stars and time into output file
                             // put a translated message into the output
  void putErrorMessage ( const int iMessage );

  // File Name Syntax Parse
  size_t  transFileName ( const char *pszFrom, char *pszTo,
                          const int iBufferLen, const int isTransAll ) const;


private:
  int  m_iFceLevel;          // level of the current function
};

// global object definition for tracing
extern CXFYTrace g_cXFYTrace;


#endif /* __XFY_TRACE_H_INCLUDED_ */