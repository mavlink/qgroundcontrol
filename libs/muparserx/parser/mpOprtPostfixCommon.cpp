#include <limits>
#include "mpOprtPostfixCommon.h"

MUP_NAMESPACE_START

//-----------------------------------------------------------
//
// class OprtFact
//
//-----------------------------------------------------------

  OprtFact::OprtFact()
    :IOprtPostfix(_T("!"))
  {}

  //-----------------------------------------------------------
  void OprtFact::Eval(ptr_val_type& ret, const ptr_val_type *arg, int)
  {
    if (!arg[0]->IsInteger())
      throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, GetExprPos(), GetIdent(), arg[0]->GetType(), 'i', 1));

    int input = arg[0]->GetInteger();
    float_type input_long = float_type(input);

    if (input < 0) {
    throw ParserError(ErrorContext(ecDOMAIN_ERROR, GetExprPos(),
				    GetIdent()));
    }

    float_type result = 1;
    for (float_type i = 1.0; i <= input_long; i += 1.0) 
    {
      result *= i;

      // <ibg 20130225/> Only throw exceptions if IEEE 754 is not supported. The 
      //                 Prefered way of dealing with overflows is relying on: 
      //
      //                      http://en.wikipedia.org/wiki/IEEE_754-1985 
      //
      //                 If the compiler does not support IEEE 754, chances are 
      //                 you are running on a pretty fucked up system.
      //
      #pragma warning(push)
      #pragma warning(disable:4127)
      if ( !std::numeric_limits<float_type>::is_iec559 && 
           (result>std::numeric_limits<float_type>::max() || result < 1.0) )
      #pragma warning(pop)
      {
        throw ParserError(ErrorContext(ecOVERFLOW, GetExprPos(), GetIdent()));
      }
      // </ibg>
    }

    *ret = result;
  }

  //-----------------------------------------------------------
  const char_type* OprtFact::GetDesc() const
  {
    return _T("x! - Returns factorial of a non-negative integer.");
  }

  //-----------------------------------------------------------
  IToken* OprtFact::Clone() const
  {
    return new OprtFact(*this);
  }

  //-----------------------------------------------------------
  //
  // class OprtPercentage
  //
  //-----------------------------------------------------------

    OprtPercentage::OprtPercentage()
      :IOprtPostfix(_T("%"))
    {}

    //-----------------------------------------------------------
    void OprtPercentage::Eval(ptr_val_type& ret, const ptr_val_type *arg, int)
    {

      switch (arg[0]->GetType()) {
        case 'i':
        case 'f': {
          float_type input = arg[0]->GetFloat();
          *ret = input / 100.0;
          break;
        }
        default:
          throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, GetExprPos(), GetIdent(), arg[0]->GetType(), 'f', 1));
          break;
      }
    }

    //-----------------------------------------------------------
    const char_type* OprtPercentage::GetDesc() const
    {
      return _T("x% - Returns percentage of integer/float.");
    }

    //-----------------------------------------------------------
    IToken* OprtPercentage::Clone() const
    {
      return new OprtPercentage(*this);
    }

}
