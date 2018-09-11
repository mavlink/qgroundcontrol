#include "muMatrixTest.h"

using namespace std;


namespace mu
{
  //-----------------------------------------------------------------------------------------------
  MatrixTest::MatrixTest()
    :m_nTests(0)
    ,m_vTests()
  {
    
#if defined(_UNICODE)
    m_stream = &std::wcout;
#else
    m_stream = &std::cout;
#endif

    AddTest(&MatrixTest::TestInitialization);
    AddTest(&MatrixTest::TestAddSub);
    AddTest(&MatrixTest::TestMul);
  }

  //-----------------------------------------------------------------------------------------------
  void MatrixTest::AddTest(MatrixTest::testfun_type a_pFun)
  {
    m_vTests.push_back(a_pFun);
  }
  
  //-----------------------------------------------------------------------------------------------
  int MatrixTest::TestInitialization()
  {
    int nFail = 0;

    *m_stream << "testing matrix initialization...";

    // Array initialization
    double ref_v3[] = { 1.0, 2.0, 3.0};
    
    // 1.1) Create empty array,initialize cell by cell afterwards
    Matrix<double> v1(3);
    v1.At(0) = ref_v3[0];
    v1.At(1) = ref_v3[1];
    v1.At(2) = ref_v3[2];

    // Create a reference matrix
    double ref[3][3] = { {1,2,3}, 
                         {4,5,6}, 
                         {7,8,9} };

    // 1.) Construct empty matrix, fill afterwards cell by cell
    Matrix<double> m1(3,3);
    m1.At(0,0) = ref[0][0]; m1.At(0,1) = ref[0][1]; m1.At(0,2) = ref[0][2];
    m1.At(1,0) = ref[1][0]; m1.At(1,1) = ref[1][1]; m1.At(1,2) = ref[1][2];
    m1.At(2,0) = ref[2][0]; m1.At(2,1) = ref[2][1]; m1.At(2,2) = ref[2][2];

    // 2.) Construct matrix from multidimensional array
    Matrix<double> m2(ref);


    if (nFail==0) 
      *m_stream << "passed" << endl;
    else 
      *m_stream << "\n  failed with " << nFail << " errors" << endl;

    return nFail;  
  }

  //-----------------------------------------------------------------------------------------------
  int MatrixTest::TestAddSub()
  {
    int nFail = 0;

    *m_stream << "testing addition and subtraction...";

    Matrix<double> a,b,c,d;
    a = 1;
    b = 2;
    c = 3;
    d = 6;

    Matrix<double> va3(3,3), vb3(3,3), vc3(3,3), vd3(3,3), m44(4,4), m34(3,4), m43(4,3);

    va3.At(0,0) = 1; va3.At(0,1) = 2; va3.At(0,2) = 3;
    va3.At(1,0) = 4; va3.At(1,1) = 5; va3.At(1,2) = 6;
    va3.At(2,0) = 7; va3.At(2,1) = 8; va3.At(2,2) = 9;

    vb3.At(0,0) = 9; vb3.At(0,1) = 8; vb3.At(0,2) = 7;
    vb3.At(1,0) = 6; vb3.At(1,1) = 5; vb3.At(1,2) = 4;
    vb3.At(2,0) = 3; vb3.At(2,1) = 2; vb3.At(2,2) = 1;

    vc3.Fill(10);
    vd3.Fill(0);

    // Addition von Skalaren
    nFail += Eval(a+b, c);
    nFail += Eval(b+a, c);
    nFail += Eval(a+b+c, d);
    nFail += Eval(c-b, a);
    nFail += Eval(d-c, c);

#define THROWTEST(EXPR)                     \
    {                                       \
      m_nTests++;                           \
      try                                   \
      {                                     \
        EXPR;                               \
        nFail++;                            \
      }                                     \
      catch(...)                            \
      {                                     \
      }                                     \
    }

    // Addition von Matrizen mit nicht passenden Zeilen/Spaltenzahl
    THROWTEST(a+va3);
    THROWTEST(va3+a);
    THROWTEST(a+va3);
    THROWTEST(va3+a);
    THROWTEST(va3+m44);
    THROWTEST(va3+m34);
    THROWTEST(va3+m43);
    THROWTEST(m44+va3);
    THROWTEST(m34+va3);
    THROWTEST(m43+va3);
    
    THROWTEST(a-va3);
    THROWTEST(va3-a);
    THROWTEST(a-va3);
    THROWTEST(va3-a);
    THROWTEST(va3-m44);
    THROWTEST(va3-m34);
    THROWTEST(va3-m43);
    THROWTEST(m44-va3);
    THROWTEST(m34-va3);
    THROWTEST(m43-va3);

    // Addition von Matrizen
    nFail += Eval(va3+vb3, vc3);
    nFail += Eval(vb3+va3, vc3);
    nFail += Eval(vc3-vb3, va3);
    nFail += Eval(vc3-va3, vb3);

    if (nFail==0) 
      *m_stream << "passed" << endl;
    else 
      *m_stream << "\n  failed with " << nFail << " errors" << endl;

    return nFail;  
  }


  //-----------------------------------------------------------------------------------------------
  int MatrixTest::TestMul()
  {
    int nFail = 0;
    *m_stream << "testing multiplication...";

    Matrix<double> a,b,c,d;
    a = 1;
    b = 2;
    c = 3;
    d = 6;

    // Try assigning from array
    double buf[]= { 1,2,3 };  
    Matrix<double> va3 = buf;

    // Manual assignment
    Matrix<double> vb3(3), vc3(3), vd3(3);
    vb3.At(0) = 4, vc3.At(0) = 3, vd3.At(0) = 12;
    vb3.At(1) = 5, vc3.At(1) = 4, vd3.At(1) = 15;
    vb3.At(2) = 6, vc3.At(2) = 3, vd3.At(2) = 18;

    // Skalarprodukt
    THROWTEST(va3*vb3);
    THROWTEST(vb3*va3);

    double res1[3][3] = { { 4,  5,  6},
                          { 8, 10, 12},
                          {12, 15, 18} };
    nFail += Eval(va3 * Matrix<double>(vb3).Transpose(), res1);
    nFail += Eval(Matrix<double>(vb3).Transpose() * va3, 32);

    double res2[3][3] = { { 4,  8, 12},
                          { 5, 10, 15},
                          { 6, 12, 18} };
    nFail += Eval(vb3 * Matrix<double>(va3).Transpose(), res2);

    double ma[2][3] = { {1, 2, 3},
                        {4, 5, 6} };
    double mb[3][2] = { {6, -1},
                        {3,  2},
                        {0, -3}};
    double res3[2][2] = { {12,  -6},
                          {39, -12} };
    nFail += Eval(Matrix<double>(ma)*Matrix<double>(mb), res3);
    
    // Skalar-Vektor Multiplikatione
    nFail += Eval(c*vb3, vd3);
    nFail += Eval(vb3*c, vd3);

    // gemischte Multiplikation
    double res4[3][3] = { {12*3, 15*3, 18*3},
                          {16*3, 20*3, 24*3},
                          {12*3, 15*3, 18*3} };
    nFail += Eval(c*vc3*Matrix<double>(vb3).Transpose(), res4);
    nFail += Eval(vc3*c*Matrix<double>(vb3).Transpose(), res4);
    nFail += Eval(vc3*Matrix<double>(vb3).Transpose()*c, res4);

    if (nFail==0) 
      *m_stream << "passed" << endl;
    else 
      *m_stream << "\n  failed with " << nFail << " errors" << endl;

    return nFail;  
  }

  //-----------------------------------------------------------------------------------------------
  void MatrixTest::Run()
  {
    int nFail = 0;
    try
    {
      for (int i=0; i<(int)m_vTests.size(); ++i)
        nFail += (this->*m_vTests[i])();

      if (nFail==0) 
      {
        *m_stream << _T("Test passed (") <<  m_nTests << _T(" distinct test)") << endl;
      }
      else 
      {
        *m_stream << _T("Test failed with ") << nFail 
                  << _T(" errors (") <<  m_nTests 
                  << _T(" expressions)") << endl;
      }
    }
    catch(std::exception &e)
    {
      *m_stream << _T("\nTest failed (unexpected exception):\n");
      *m_stream << e.what() << endl;
    }
    catch(...)
    {
      *m_stream << _T("\nTest failed (unexpected exception):\n");
      *m_stream << _T("Internal error");
    }

    m_nTests = 0;
  }
}