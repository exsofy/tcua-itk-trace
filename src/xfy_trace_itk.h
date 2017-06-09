#include <stdio.h>

#include "EXSOFY/XFY_TRACE.h"

#ifdef XFY_TRACE_NOT_ACTIVE

// Non trace expansion
#define XFY_TRACE_MSG
#define XFY_TRACE_FCE(FceName)
#define XFY_TRACE_RETURN(r) return r
#define XFY_TRACE_ERROR(r) return r

#define XFY_TRACE_PARAM_1(p1,o1)
#define XFY_TRACE_PARAM_2(p1,o1,p2,o2)
#define XFY_TRACE_PARAM_4(p1,o1,p2,o2,p3,o3,p4,o4)
#define XFY_TRACE_PARAM_9(p1,o1,p2,o2,p3,o3,p4,o4,p5,o5,p6,o6,p7,o7,p8,o8,p9,o9)

#define XFY_TRACE_MSG(a,b,c)

//#define XFY_CALL_RNZ(f) { int xfy0O0O0O = XFY_CALL ( f ); if ( xfy0O0O0O != 0 ) { printf ( "%d:%s\n", xfy0O0O0O, #f ); XFY_TRACE_RETURN ( xfy0O0O0O ); } }
//#define XFY_CALL_RNZ(f) { int xfy0O0O0O = XFY_CALL ( f ); if ( xfy0O0O0O != 0 ) XFY_XFY_TRACE_RETURN ( xfy0O0O0O ); }

#define XFY_CALL(f) f

#define XFY_CALL_EX(f) { int xfy0O0O0O = XFY_CALL ( f ); if ( xfy0O0O0O != 0 ) { printf ( "%d:%s\n", xfy0O0O0O, #f ); } }


#else

#define XFY_TRACE_MSG g_cXFYTrace.putMessage
#define XFY_TRACE_FCE(FceName) CXFYTraceFce cXFYTraceFce(#FceName)

#define XFY_TRACE_RETURN(RetValue) return (g_cXFYTrace.putFceReturns(RetValue,&cXFYTraceFce))
#define XFY_TRACE_ERROR(RetValue) return(g_cXFYTrace.putErrorReturns(RetValue,&cXFYTraceFce))

#define XFY_TRACE_PARAM_P(P,T) g_cXFYTrace.putVariable(#P,P,CXFYTrace::eVT_##T,&cXFYTraceFce)
#define XFY_TRACE_PARAM_0() if ( g_cXFYTrace.showParam() ) { g_cXFYTrace.finishFunctionHeader(); }
#define XFY_TRACE_PARAM_F(P1,T1) if ( g_cXFYTrace.showParam() ) { XFY_TRACE_PARAM_P(P1,T1); }
#define XFY_TRACE_PARAM_1(P1,T1) if ( g_cXFYTrace.showParam() ) { XFY_TRACE_PARAM_P(P1,T1); g_cXFYTrace.finishFunctionHeader(); }
#define XFY_TRACE_PARAM_2(P1,T1,P2,T2) if ( g_cXFYTrace.showParam() ) { XFY_TRACE_PARAM_P(P1,T1); XFY_TRACE_PARAM_P(P2,T2); g_cXFYTrace.finishFunctionHeader();}
#define XFY_TRACE_PARAM_3(P1,T1,P2,T2,P3,T3) if ( g_cXFYTrace.showParam() ) { XFY_TRACE_PARAM_P(P1,T1); XFY_TRACE_PARAM_P(P2,T2); XFY_TRACE_PARAM_P(P3,T3); g_cXFYTrace.finishFunctionHeader();}
#define XFY_TRACE_PARAM_4(P1,T1,P2,T2,P3,T3,P4,T4) if ( g_cXFYTrace.showParam() ) { XFY_TRACE_PARAM_P(P1,T1); XFY_TRACE_PARAM_P(P2,T2); XFY_TRACE_PARAM_P(P3,T3);  XFY_TRACE_PARAM_P(P4,T4); g_cXFYTrace.finishFunctionHeader();} 
#define XFY_TRACE_PARAM_5(P1,T1,P2,T2,P3,T3,P4,T4,P5,T5) if ( g_cXFYTrace.showParam() ) { XFY_TRACE_PARAM_P(P1,T1); XFY_TRACE_PARAM_P(P2,T2); XFY_TRACE_PARAM_P(P3,T3); \
                                                                                      XFY_TRACE_PARAM_P(P4,T4); XFY_TRACE_PARAM_P(P5,T5); g_cXFYTrace.finishFunctionHeader();}
#define XFY_TRACE_PARAM_6(P1,T1,P2,T2,P3,T3,P4,T4,P5,T5,P6,T6) if ( g_cXFYTrace.showParam() ) { XFY_TRACE_PARAM_P(P1,T1); XFY_TRACE_PARAM_P(P2,T2); XFY_TRACE_PARAM_P(P3,T3); \
                                                                                      XFY_TRACE_PARAM_P(P4,T4); XFY_TRACE_PARAM_P(P5,T5); XFY_TRACE_PARAM_P(P6,T6); g_cXFYTrace.finishFunctionHeader();}
#define XFY_TRACE_PARAM_7(P1,T1,P2,T2,P3,T3,P4,T4,P5,T5,P6,T6,P7,T7) if ( g_cXFYTrace.showParam() ) { XFY_TRACE_PARAM_P(P1,T1); XFY_TRACE_PARAM_P(P2,T2); XFY_TRACE_PARAM_P(P3,T3); \
                                                                                      XFY_TRACE_PARAM_P(P4,T4); XFY_TRACE_PARAM_P(P5,T5); XFY_TRACE_PARAM_P(P6,T6); XFY_TRACE_PARAM_P(P7,T7); g_cXFYTrace.finishFunctionHeader();}
#define XFY_TRACE_PARAM_8(P1,T1,P2,T2,P3,T3,P4,T4,P5,T5,P6,T6,P7,T7,P8,T8) if ( g_cXFYTrace.showParam() ) { XFY_TRACE_PARAM_P(P1,T1); XFY_TRACE_PARAM_P(P2,T2); XFY_TRACE_PARAM_P(P3,T3); \
                                                                                      XFY_TRACE_PARAM_P(P4,T4); XFY_TRACE_PARAM_P(P5,T5); XFY_TRACE_PARAM_P(P6,T6); XFY_TRACE_PARAM_P(P7,T7); XFY_TRACE_PARAM_P(P8,T8); g_cXFYTrace.finishFunctionHeader();}

#define XFY_CALL(X) (g_cXFYTrace.reportFceCall(#X, __FILE__, __LINE__, (X)))

#define XFY_CALL_EX(f) { int xfy0O0O0O = XFY_CALL ( f ); if ( xfy0O0O0O != 0 ) { printf ( "%d:%s\n", xfy0O0O0O, #f ); } }

#endif

// Independent calls, used by trace and notrace
#ifdef __cplusplus
#define XFY_CALL_RNZ(X) { int rc0O0O0O = XFY_CALL ( X ); if ( rc0O0O0O != 0 ) XFY_TRACE_RETURN ( rc0O0O0O ); }
#define CALL_RNZ(X) { int rc0O0O0O = X; if ( rc0O0O0O != 0 ) XFY_TRACE_RETURN ( rc0O0O0O ); }
#define XFY_CALL_RNZ_P(X,V) { int rc0O0O0O = XFY_CALL ( X ); if ( rc0O0O0O != 0 ) XFY_TRACE_RETURN ( V ); }
#define XFY_CALL_RNZ_U(X,V) { int rc0O0O0O = XFY_CALL ( X ); if ( rc0O0O0O != 0 ) return ( V ); }
#define CALL_RNZ_P(X,V) { int rc0O0O0O = X; if ( rc0O0O0O != 0 ) XFY_TRACE_RETURN ( V ); }
#define CALL_RNZ_U(X,V) { int rc0O0O0O = X; if ( rc0O0O0O != 0 ) return ( V ); }
#else
#define XFY_CALL_RNZ(X) { int rc0O0O0O = XFY_CALL ( X ); if ( rc0O0O0O != 0 ) XFY_TRACE_RETURN_INT ( rc0O0O0O ); }
#define CALL_RNZ(X) { int rc0O0O0O = X; if ( rc0O0O0O != 0 ) XFY_TRACE_RETURN_INT ( rc0O0O0O ); }
#endif
#define XFY_CALL_RNZ_V(X) { int rc0O0O0O = XFY_CALL ( X ); if ( rc0O0O0O != 0 ) return; }
#define CALL_RNZ_V(X) { int rc0O0O0O = X; if ( rc0O0O0O != 0 ) return; }

#define XFY_USE_JNZ		int xfyRetCode = ITK_ok;
#define XFY_JNZ_VALUE	xfyRetCode
#define XFY_CALL_JNZ(X,L) { xfyRetCode = XFY_CALL ( X ); if ( xfyRetCode != 0 ) goto L; };
#define CALL_JNZ(X,L) { int xfyRetCode = X; if ( xfyRetCode != 0 ) goto L; }
#define XFY_TRACE_JNZ_RETURN XFY_TRACE_RETURN(xfyRetCode)


