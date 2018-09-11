// value_test.cpp : main project file.

#include "stdafx.h"
#include <string>

#include "mpDefines.h"
#include "mpTypes.h"
#include "mpValue.h"

//--- MathUtils -----------------------------------------------------------------------------------
#include "muMatrix.h"
#include "muMatrixTest.h"


MUP_NAMESPACE_START
  //-----------------------------------------------------------------------------------------------
  const char_type *g_sCmdCode[] = { _T("BRCK. OPEN     "),
                                    _T("BRCK. CLOSE    "),
                                    _T("IDX OPEN       "),
                                    _T("IDX CLOSE      "),
                                    _T("ARG_SEP        "),
                                    _T("IF             "),
                                    _T("ELSE           "),
                                    _T("ENDIF          "),
                                    _T("JMP            "),
                                    _T("VAR            "),
                                    _T("VAL            "),
                                    _T("FUNC           "),
                                    _T("OPRT_BIN       "),
                                    _T("OPRT_IFX       "),
                                    _T("OPRT_PFX       "),
                                    _T("END            "),
                                    _T("SCRIPT_GOTO    "),
                                    _T("SCRIPT_LABEL   "),
                                    _T("SCRIPT_FOR     "),
                                    _T("SCRIPT_IF      "),
                                    _T("SCRIPT_ELSE    "),
                                    _T("SCRIPT_ELSEIF  "),
                                    _T("SCRIPT_ENDIF   "),
                                    _T("SCRIPT_NEWLINE "),
                                    _T("SCRIPT_FUNCTION"),
                                    _T("UNKNOWN        ") };
MUP_NAMESPACE_END

using namespace mup;
using namespace mu;

int main(int /* argc */, char ** /*argv*/)
{
  MatrixTest mt;
  mt.Run();

  return 0;
}
