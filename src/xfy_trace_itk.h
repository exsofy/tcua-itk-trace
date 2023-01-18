#ifndef XFY_TRACE_ITK_H
#define XFY_TRACE_ITK_H

#include <stdio.h>

#include "xfy/trace/xfy_trace.h"

#ifdef XFY_TRACE_NOT_ACTIVE

// Non trace expansion
#define XFY_TMSG
#define XFY_TFCE()
#define XFY_TFCE_P0()
#define XFY_TFCE_P1(p1,o1)
#define XFY_TRET(r) return r
#define XFY_TERR(r) return r


#define XFY_TPAR_1(p1,o1)
#define XFY_TPAR_2(p1,o1,p2,o2)
#define XFY_TPAR_4(p1,o1,p2,o2,p3,o3,p4,o4)
#define XFY_TPAR_9(p1,o1,p2,o2,p3,o3,p4,o4,p5,o5,p6,o6,p7,o7,p8,o8,p9,o9)

#define XFY_TRACE_MSG(a,b,c)

//#define XFY_CALL_RNZ(f) { int xfy0O0O0O = XFY_CALL ( f ); if ( xfy0O0O0O != 0 ) { printf ( "%d:%s\n", xfy0O0O0O, #f ); XFY_TRACE_RETURN ( xfy0O0O0O ); } }
//#define XFY_CALL_RNZ(f) { int xfy0O0O0O = XFY_CALL ( f ); if ( xfy0O0O0O != 0 ) XFY_XFY_TRACE_RETURN ( xfy0O0O0O ); }

#define XFY_TCALL(f) f

#else

#ifdef _WIN32
#ifndef __func__
#define __func__ __FUNCTION__
#endif
#endif

#define XFY_TMSG XFY::g_XFYTrace.putMessage
#define XFY_TRET(RetValue) return (XFY::g_XFYTrace.putFceReturns(RetValue,&cXFYTraceFce,__LINE__))
#define XFY_TERR(RetValue) return(XFY::g_XFYTrace.putErrorReturns(RetValue,&cXFYTraceFce,__LINE__))
#define XFY_TRET_OK return (XFY::g_XFYTrace.putFceReturns(ITK_ok,&cXFYTraceFce,__LINE__))

#define XFY_TPAR_REPORT(P,T) XFY::g_XFYTrace.putVariable(#P,P,XFY::Trace::eVT_##T,&cXFYTraceFce)
#define XFY_TPAR(P1,T1) if ( XFY::g_XFYTrace.showParam() ) { XFY_TPAR_REPORT(P1,T1); }
#define XFY_TPAR_0() if ( XFY::g_XFYTrace.showParam() ) { XFY::g_XFYTrace.finishFunctionHeader(); }
#define XFY_TPAR_1(P1,T1) if ( XFY::g_XFYTrace.showParam() ) { XFY_TPAR_REPORT(P1,T1); XFY::g_XFYTrace.finishFunctionHeader(); }
#define XFY_TPAR_2(P1,T1,P2,T2) if ( XFY::g_XFYTrace.showParam() ) { XFY_TPAR_REPORT(P1,T1); XFY_TPAR_REPORT(P2,T2); XFY::g_XFYTrace.finishFunctionHeader();}
#define XFY_TPAR_3(P1,T1,P2,T2,P3,T3) if ( XFY::g_XFYTrace.showParam() ) { XFY_TPAR_REPORT(P1,T1); XFY_TPAR_REPORT(P2,T2); XFY_TPAR_REPORT(P3,T3); XFY::g_XFYTrace.finishFunctionHeader();}
#define XFY_TPAR_4(P1,T1,P2,T2,P3,T3,P4,T4) if ( XFY::g_XFYTrace.showParam() ) { XFY_TPAR_REPORT(P1,T1); XFY_TPAR_REPORT(P2,T2); XFY_TPAR_REPORT(P3,T3);  XFY_TPAR_REPORT(P4,T4); XFY::g_XFYTrace.finishFunctionHeader();}
#define XFY_TPAR_5(P1,T1,P2,T2,P3,T3,P4,T4,P5,T5) if ( XFY::g_XFYTrace.showParam() ) { XFY_TPAR_REPORT(P1,T1); XFY_TPAR_REPORT(P2,T2); XFY_TPAR_REPORT(P3,T3); \
                                                                                      XFY_TPAR_REPORT(P4,T4); XFY_TPAR_REPORT(P5,T5); XFY::g_XFYTrace.finishFunctionHeader();}
#define XFY_TPAR_6(P1,T1,P2,T2,P3,T3,P4,T4,P5,T5,P6,T6) if ( XFY::g_XFYTrace.showParam() ) { XFY_TPAR_REPORT(P1,T1); XFY_TPAR_REPORT(P2,T2); XFY_TPAR_REPORT(P3,T3); \
                                                                                      XFY_TPAR_REPORT(P4,T4); XFY_TPAR_REPORT(P5,T5); XFY_TPAR_REPORT(P6,T6); XFY::g_XFYTrace.finishFunctionHeader();}

#define XFY_TFCE XFY::TraceFce cXFYTraceFce(__func__)
#define XFY_TFCE_NAME(X) XFY::TraceFce cXFYTraceFce(#X)
#define XFY_TFCE_P0() XFY_TFCE; XFY_TPAR_0();
#define XFY_TFCE_P1(P1,T1) XFY_TFCE; XFY_TPAR_1(P1,T1)
#define XFY_TFCE_P2(P1,T1,P2,T2) XFY_TFCE; XFY_TPAR_2(P1,T1,P2,T2)
#define XFY_TFCE_P3(P1,T1,P2,T2,P3,T3) XFY_TFCE; XFY_TPAR_3(P1,T1,P2,T2,P3,T3)
#define XFY_TFCE_P4(P1,T1,P2,T2,P3,T3,P4,T4) XFY_TFCE; XFY_TPAR_4(P1,T1,P2,T2,P3,T3,P4,T4)
#define XFY_TFCE_P5(P1,T1,P2,T2,P3,T3,P4,T4,P5,T5) XFY_TFCE; XFY_TPAR_5(P1,T1,P2,T2,P3,T3,P4,T4,P5,T5)
#define XFY_TFCE_P6(P1,T1,P2,T2,P3,T3,P4,T4,P5,T5,P6,T6) XFY_TFCE; XFY_TPAR_6(P1,T1,P2,T2,P3,T3,P4,T4,P5,T5,P6,T6)

#define XFY_TREP(X) (XFY::g_XFYTrace.reportFceCall(#X, __FILE__, __LINE__, (X)))
#endif

// Independent calls, used by trace and notrace
#ifdef __cplusplus
#define XFY_TCALL(X) { int rc0O0O0O = XFY_TREP ( X ); if ( rc0O0O0O != 0 ) XFY_TRET ( rc0O0O0O ); }
#define XFY_CALL(X) { int rc0O0O0O = X; if ( rc0O0O0O != 0 ) XFY_TRET ( rc0O0O0O ); }
#define XFY_TCALL_P(X,V) { int rc0O0O0O = XFY_TREP ( X ); if ( rc0O0O0O != 0 ) XFY_TRET ( V ); }
#define XFY_TCALL_V(X,V) { int rc0O0O0O = XFY_TREP ( X ); if ( rc0O0O0O != 0 ) return ( V ); }
#define XFY_CALL_P(X,V) { int rc0O0O0O = X; if ( rc0O0O0O != 0 ) XFY_TRET ( V ); }
#define XFY_CALL_V(X,V) { int rc0O0O0O = X; if ( rc0O0O0O != 0 ) return ( V ); }
#define XFY_JNZ_VALUE	cXFYTraceFce.m_retValue
#define XFY_USE_JNZ

#else
#define XFY_TCALL(X) { int rc0O0O0O = XFY_TCALL_RET ( X ); if ( rc0O0O0O != 0 ) XFY_TRET_INT ( rc0O0O0O ); }
#define XFY_TCALL_P(X,P) { int rc0O0O0O = XFY_TCALL_RET ( X ); if ( rc0O0O0O != 0 ) XFY_TRET_PTR ( P ); }
#define XFY_TCALL_V(X,V) { int rc0O0O0O = XFY_TCALL_RET ( X ); if ( rc0O0O0O != 0 ) return ( V ); }
#define XFY_CALL(X) { int rc0O0O0O = X; if ( rc0O0O0O != 0 ) XFY_TRET_INT ( rc0O0O0O ); }

#define XFY_JNZ_VALUE	xfyRetCode
#define XFY_USE_JNZ		int XFY_JNZ_VALUE = ITK_ok;
#endif


#define XFY_TCALL_N(X) { int rc0O0O0O = XFY_TREP ( X ); if ( rc0O0O0O != 0 ) return; }
#define XFY_CALL_N(X) { int rc0O0O0O = X; if ( rc0O0O0O != 0 ) return; }


// if last chain traced function was OK, call XFY chain traced function X 
#define XFY_OK_CALL(X) { if ( XFY_JNZ_VALUE == ITK_ok ) XFY_JNZ_VALUE = X; }
#define XFY_OK_TCALL(X) { if ( XFY_JNZ_VALUE == ITK_ok ) XFY_JNZ_VALUE = XFY_TREP ( X ); }
#define XFY_OK_CALL_V(X) { if ( XFY_JNZ_VALUE == ITK_ok ) {X;} }

// call ITK function X and go to label L on fail
#define XFY_TCALL_L(X,L) { XFY_JNZ_VALUE = XFY_TREP ( X ); if ( XFY_JNZ_VALUE != ITK_ok ) goto L; };
// call XFY traced function X and go to label L on fail
#define XFY_CALL_L(X,L) { XFY_JNZ_VALUE= X; if ( XFY_JNZ_VALUE != ITK_ok ) goto L; }
// return code of last XFY_CALL_L/XFY_OK_CALL
#define XFY_TRET_JNZ XFY_TRET(XFY_JNZ_VALUE)
// return error code if last XFY_CALL_L/XFY_OK_CALL failed
#define XFY_TRET_NOK { if ( XFY_JNZ_VALUE != ITK_ok ) XFY_TRET(XFY_JNZ_VALUE); }
#define XFY_NOK_TRET { if ( XFY_JNZ_VALUE != ITK_ok ) XFY_TRET(XFY_JNZ_VALUE); }
// go to label if last XFY_CALL_L/XFY_OK_CALL failed
#define XFY_NOK_L(L) { if ( XFY_JNZ_VALUE != ITK_ok ) goto L; }
// go to label if last XFY_CALL_L/XFY_OK_CALL suceeded
#define XFY_OK_L(L) { if ( XFY_JNZ_VALUE == ITK_ok ) goto L; }

#endif /* XFY_TRACE_ITK_H */
