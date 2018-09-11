#include "mpParser.h"


void Calc()
{
  char line[100];
  try
  {
    // Create value objects that will be bound to parser variables
    Value fVal = 1.11;
    Value sVal = "hello world";
    Value arr1(3, 0);
    arr1[0] = 1.11, 
    arr1[1] = 2.22;
    arr1[2] = 3.33;

    Parser  parser;
    
    // Define parser variables and bind them to their value objects
    parser.DefineVar("va", Variable(&arr1));
    parser.DefineVar("a",  Variable(&fVal));
    parser.DefineVar("sa", Variable(&sVal));

    parser.SetExpr("sin(a)+b");

    // The returned result is of type Value, value is a Variant like
    // type that can be either a boolean an integer or a floating point value
    Value result = parser.Eval();

    // Value supports C++ streaming like this:
    cout << "Result:\n" << result << "\n";

    // Or if you need the specific type use this:
    switch (result.GetType())
    {
    case 's': cout << result.GetString() << " (string)" << "\n"; break;
    case 'i': cout << result.GetInt() << " (int)" << "\n"; break;
    case 'f': cout << result.GetFloat() << " (float)" << "\n"; break;
    case 'c': cout << result.GetFloat() << "+" << result.GetImag() << "i (complex)" << "\n"; break;
    case 'b': break;
    }
  }
  catch(ParserError &e)
  {
    cout << e.GetMsg() << "\n\n";

    if (e.GetContext().Ident.length()) 
      cout << "Ident.: " << e.GetContext().Ident << "\n";

    if (e.GetExpr().length()) 
      cout << "Expr.: " << e.GetExpr() << "\n";

    if (e.GetToken().length()) 
      cout << "Token: " << e.GetToken() << "\n";

    cout << "Pos:   " << e.GetPos() << "\n";
    cout << "Errc:  " << e.GetCode() << "\n";
  }
} // Calc()