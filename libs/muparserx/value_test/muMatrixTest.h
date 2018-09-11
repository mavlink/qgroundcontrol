#ifndef MU_MATRIX_TEST_H
#define MU_MATRIX_TEST_H

#include <vector>
#include <string>
#include <iostream>

#include "muMatrix.h"

#ifndef _T 
#define _T
#endif

namespace mu
{
  class MatrixTest
  {
  public:
      MatrixTest();

     void Run();

  private:
      typedef int (MatrixTest::*testfun_type)();

      int m_nTests;
      std::vector<testfun_type> m_vTests;

#if defined(_UNICODE)
      std::wostream *m_stream;
#else
      std::ostream *m_stream;
#endif

      int TestInitialization();
      int TestAddSub();
      int TestMul();
      
      void AddTest(testfun_type pFun);

      int Eval(const Matrix<double> &m1, const Matrix<double> &m2)
      {
        m_nTests++;
        int nFail = 0;
        try
        {
          if (m1.GetDim()!=m2.GetDim())
            throw MatrixError("invalid Matrix dimentsion");

          if (m1.GetRows()!=m2.GetRows())
            throw MatrixError("invalid Matrix rows");

          if (m1.GetCols()!=m2.GetCols())
            throw MatrixError("invalid Matrix rows");

          for (int i=0; i<m1.GetRows(); ++i)
          {
            for (int j=0; j<m1.GetCols(); ++j)
            {
              if (m1.At(i,j)!=m2.At(i,j))
                throw MatrixError("invalid Matrix value");
            }
          }

          nFail = 0;
        }
        catch(...)
        {
          *m_stream << "\nincorrect result:\n" << m1.ToString() << "\nexpected:\n" << m2.ToString() << "\n";
          nFail = 1;
        }

        return nFail;
      }
  };
}

#endif