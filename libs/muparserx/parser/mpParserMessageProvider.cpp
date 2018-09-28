#include "mpParserMessageProvider.h"

#include <cassert>
#include "mpError.h"


MUP_NAMESPACE_START

  //-------------------------------------------------------------------------------------------------
  //
  // class ParserMessageProviderBase - Base class for message providers
  //
  //-------------------------------------------------------------------------------------------------

  ParserMessageProviderBase::ParserMessageProviderBase()
    :m_vErrMsg(0)
  {}

  //-----------------------------------------------------------------------------------------------
  ParserMessageProviderBase::~ParserMessageProviderBase()
  {}

  //-----------------------------------------------------------------------------------------------
  void ParserMessageProviderBase::Init()
  {
    InitErrorMessages();
    for (int i=0; i<ecCOUNT; ++i)
    {
      if (!m_vErrMsg[i].length())
        throw std::runtime_error("Incomplete translation (at least one error code missing)");
    }
  }

  //---------------------------------------------------------------------------------------------
  string_type ParserMessageProviderBase::GetErrorMsg(EErrorCodes eError) const
  {
    int nError = (int)eError;
    return (nError<(int)m_vErrMsg.size()) ? m_vErrMsg[nError] : string_type();
  }

  //-----------------------------------------------------------------------------------------------
  //
  // class ParserMessageProviderEnglish - English Parser Messages (default)
  //
  //-----------------------------------------------------------------------------------------------

  ParserMessageProviderEnglish::ParserMessageProviderEnglish()
    :ParserMessageProviderBase()
  {}

  //-----------------------------------------------------------------------------------------------
  void ParserMessageProviderEnglish::InitErrorMessages()
  {
    m_vErrMsg.resize(ecCOUNT);

    m_vErrMsg[ecUNASSIGNABLE_TOKEN]       = _T("Undefined token \"$IDENT$\" found at position $POS$.");
    m_vErrMsg[ecINTERNAL_ERROR]           = _T("Internal error.");
    m_vErrMsg[ecUNKNOWN_ESCAPE_SEQUENCE]  = _T("Unknown escape sequence.");
    m_vErrMsg[ecINVALID_NAME]             = _T("Invalid function, variable or constant name.");
    m_vErrMsg[ecINVALID_FUN_PTR]          = _T("Invalid pointer to callback function.");
    m_vErrMsg[ecINVALID_VAR_PTR]          = _T("Invalid pointer to variable.");
    m_vErrMsg[ecUNEXPECTED_OPERATOR]      = _T("Unexpected operator \"$IDENT$\" found at position $POS$.");
    m_vErrMsg[ecUNEXPECTED_EOF]           = _T("Unexpected end of expression found at position $POS$.");
    m_vErrMsg[ecUNEXPECTED_COMMA]         = _T("Unexpected comma found at position $POS$.");
    m_vErrMsg[ecUNEXPECTED_PARENS]        = _T("Unexpected parenthesis \"$IDENT$\" found at position $POS$.");
    m_vErrMsg[ecUNEXPECTED_FUN]           = _T("Unexpected function \"$IDENT$\" found at position $POS$.");
    m_vErrMsg[ecUNEXPECTED_VAL]           = _T("Unexpected value \"$IDENT$\" found at position $POS$.");
    m_vErrMsg[ecUNEXPECTED_VAR]           = _T("Unexpected variable \"$IDENT$\" found at position $POS$.");
    m_vErrMsg[ecUNEXPECTED_STR]           = _T("Unexpected string token found at position $POS$.");
    m_vErrMsg[ecUNEXPECTED_CONDITIONAL]   = _T("The \"$IDENT$\" operator must be preceded by a closing bracket.");
    m_vErrMsg[ecUNEXPECTED_NEWLINE]       = _T("Unexprected newline.");
    m_vErrMsg[ecMISSING_PARENS]           = _T("Missing parenthesis.");
    m_vErrMsg[ecMISSING_ELSE_CLAUSE]      = _T("If-then-else operator is missing an else clause.");
    m_vErrMsg[ecMISPLACED_COLON]          = _T("Misplaced colon at position $POS$.");
    m_vErrMsg[ecTOO_MANY_PARAMS]          = _T("Too many parameters passed to function \"$IDENT$\".");
    m_vErrMsg[ecTOO_FEW_PARAMS]           = _T("Too few parameters passed to function \"$IDENT$\".");
    m_vErrMsg[ecDIV_BY_ZERO]              = _T("Division by zero occurred.");
    m_vErrMsg[ecDOMAIN_ERROR]             = _T("The value passed as argument to function/operator \"$IDENT$\" is not part of its domain.");
    m_vErrMsg[ecNAME_CONFLICT]            = _T("Name conflict.");
    m_vErrMsg[ecOPT_PRI]                  = _T("Invalid value for operator priority (must be greater or equal to zero).");
    m_vErrMsg[ecBUILTIN_OVERLOAD]         = _T("Binary operator identifier conflicts with a built in operator.");
    m_vErrMsg[ecUNTERMINATED_STRING]      = _T("Unterminated string starting at position $POS$.");
    m_vErrMsg[ecSTRING_EXPECTED]          = _T("String function called with a non string type of argument.");
    m_vErrMsg[ecVAL_EXPECTED]             = _T("Numerical function called with a non value type of argument.");
    m_vErrMsg[ecTYPE_CONFLICT]            = _T("Value \"$IDENT$\" is of type '$TYPE1$'. There is no implicit conversion to type '$TYPE2$'.");
    m_vErrMsg[ecTYPE_CONFLICT_FUN]        = _T("Argument $ARG$ of function/operator \"$IDENT$\" is of type '$TYPE1$' whereas type '$TYPE2$' was expected.");
    m_vErrMsg[ecTYPE_CONFLICT_IDX]        = _T("Index to \"$IDENT$\" must be a positive integer value. '$TYPE1$' is not an acceptable type.");
    m_vErrMsg[ecGENERIC]                  = _T("Parser error.");
    m_vErrMsg[ecINVALID_TYPE]             = _T("Invalid argument type.");
    m_vErrMsg[ecINVALID_TYPECAST]         = _T("Value type conversion from type '$TYPE1$' to '$TYPE2$' is not supported!");
    m_vErrMsg[ecARRAY_SIZE_MISMATCH]      = _T("Array size mismatch.");
    m_vErrMsg[ecNOT_AN_ARRAY]             = _T("Using the index operator on the scalar variable \"$IDENT$\" is not allowed.");
    m_vErrMsg[ecUNEXPECTED_SQR_BRACKET]   = _T("Unexpected \"[]\".");
	m_vErrMsg[ecUNEXPECTED_CURLY_BRACKET] = _T("Unexpected \"{}\".");
    m_vErrMsg[ecINDEX_OUT_OF_BOUNDS]      = _T("Index to variable \"$IDENT$\" is out of bounds.");
    m_vErrMsg[ecINDEX_DIMENSION]          = _T("Index operator dimension error.");
    m_vErrMsg[ecMISSING_SQR_BRACKET]      = _T("Missing \"]\".");
	m_vErrMsg[ecMISSING_CURLY_BRACKET]    = _T("Missing \"}\".");
    m_vErrMsg[ecASSIGNEMENT_TO_VALUE]     = _T("Assignment operator \"$IDENT$\" can't be used in this context.");
    m_vErrMsg[ecEVAL]                     = _T("Can't evaluate function/operator \"$IDENT$\": $HINT$");
    m_vErrMsg[ecINVALID_PARAMETER]        = _T("Parameter $ARG$ of function \"$IDENT$\" is invalid.");
    m_vErrMsg[ecINVALID_NUMBER_OF_PARAMETERS] = _T("Invalid number of function arguments.");
    m_vErrMsg[ecOVERFLOW]                     = _T("Possible arithmetic overflow occurred in function/operator \"$IDENT$\".");
    m_vErrMsg[ecMATRIX_DIMENSION_MISMATCH]    = _T("Matrix dimension error.");
    m_vErrMsg[ecVARIABLE_DEFINED]             = _T("Variable \"$IDENT$\" is already defined.");
    m_vErrMsg[ecCONSTANT_DEFINED]             = _T("Constant \"$IDENT$\" is already defined.");
    m_vErrMsg[ecFUNOPRT_DEFINED]              = _T("Function/operator \"$IDENT$\" is already defined.");
  }

#if defined(_UNICODE)

  //-------------------------------------------------------------------------------------------------
  //
  // class ParserMessageProviderGerman - German translations of Parser Messages 
  //
  //-------------------------------------------------------------------------------------------------

  ParserMessageProviderGerman::ParserMessageProviderGerman()
    :ParserMessageProviderBase()
  {}

  //-----------------------------------------------------------------------------------------------
  void ParserMessageProviderGerman::InitErrorMessages()
  {
    m_vErrMsg.resize(ecCOUNT);

    m_vErrMsg[ecUNASSIGNABLE_TOKEN]       = _T("Unbekanntes Token \"$IDENT$\" an Position $POS$ gefunden.");
    m_vErrMsg[ecINTERNAL_ERROR]           = _T("Interner Fehler.");
    m_vErrMsg[ecUNKNOWN_ESCAPE_SEQUENCE]  = _T("Unbekannte Escape-Sequenz.");
    m_vErrMsg[ecINVALID_NAME]             = _T("Ungültiger Funktions-, Variablen- oder Konstantenbezeichner.");
    m_vErrMsg[ecINVALID_FUN_PTR]          = _T("Ungültiger Zeiger auf eine Callback-Funktion.");
    m_vErrMsg[ecINVALID_VAR_PTR]          = _T("Ungültiger Variablenzeiger.");
    m_vErrMsg[ecUNEXPECTED_OPERATOR]      = _T("Unerwarteter Operator \"$IDENT$\" an Position $POS$.");
    m_vErrMsg[ecUNEXPECTED_EOF]           = _T("Unerwartetes Formelende an Position $POS$.");
    m_vErrMsg[ecUNEXPECTED_COMMA]         = _T("Unerwartetes Komma an Position $POS$.");
    m_vErrMsg[ecUNEXPECTED_PARENS  ]      = _T("Unerwartete Klammer \"$IDENT$\" an Position $POS$ gefunden.");
    m_vErrMsg[ecUNEXPECTED_FUN]           = _T("Unerwartete Funktion \"$IDENT$\" an Position $POS$ gefunden.");
    m_vErrMsg[ecUNEXPECTED_VAL]           = _T("Unerwarteter Wert \"$IDENT$\" an Position $POS$ gefunden.");
    m_vErrMsg[ecUNEXPECTED_VAR]           = _T("Unerwartete Variable \"$IDENT$\" an Position $POS$ gefunden.");
    m_vErrMsg[ecUNEXPECTED_STR]           = _T("Unerwarteter Text an Position $POS$ gefunden.");
    m_vErrMsg[ecUNEXPECTED_CONDITIONAL]   = _T("Der Operator \"$IDENT$\" muss stets auf eine schließenden Klammer folgen.");
    m_vErrMsg[ecUNEXPECTED_NEWLINE]       = _T("Unerwarteter Zeilenumbruch.");
    m_vErrMsg[ecMISSING_PARENS]           = _T("Fehlende Klammer.");
    m_vErrMsg[ecMISSING_ELSE_CLAUSE]      = _T("\"If-then-else\" Operator ohne \"else\" Zweig verwendet.");
    m_vErrMsg[ecMISPLACED_COLON]          = _T("Komma an unerwarteter Position $POS$ gefunden.");
    m_vErrMsg[ecTOO_MANY_PARAMS]          = _T("Der Funktion \"$IDENT$\" wurden zu viele Argumente übergeben.");
    m_vErrMsg[ecTOO_FEW_PARAMS]           = _T("Der Funktion \"$IDENT$\" wurden nicht genug Argumente übergeben.");
    m_vErrMsg[ecDIV_BY_ZERO]              = _T("Division durch Null.");
    m_vErrMsg[ecDOMAIN_ERROR]             = _T("Der Parameter der Funktion \"$IDENT$\" hat einen Wert, der nicht Teil des Definitionsbereiches der Funktion ist.");
    m_vErrMsg[ecNAME_CONFLICT]            = _T("Namenskonflikt");
    m_vErrMsg[ecOPT_PRI]                  = _T("Ungültige Operatorpriorität (muss größer oder gleich Null sein).");
    m_vErrMsg[ecBUILTIN_OVERLOAD]         = _T("Die Überladung für diesen Binäroperator steht im Widerspruch zu intern vorhanden operatoren.");
    m_vErrMsg[ecUNTERMINATED_STRING]      = _T("Die Zeichenkette an Position $POS$ wird nicht beendet.");
    m_vErrMsg[ecSTRING_EXPECTED]          = _T("Es wurde eine Zeichenkette als Funktionseingabewert erwartet.");
    m_vErrMsg[ecVAL_EXPECTED]             = _T("Numerische Funktionen können nicht mit nichtnumerischen Parametern aufgerufen werden.");
    m_vErrMsg[ecTYPE_CONFLICT]            = _T("Der Wert \"$IDENT$\" ist vom Typ '$TYPE1$' und es gibt keine passende Typkonversion in den erwarteten Typ '$TYPE2$'.");
    m_vErrMsg[ecTYPE_CONFLICT_FUN]        = _T("Das Argument $ARG$ der Funktion oder des Operators \"$IDENT$\" ist vom Typ '$TYPE1$', erwartet wurde Typ '$TYPE2$'.");
    m_vErrMsg[ecTYPE_CONFLICT_IDX]        = _T("Der Index der Variable \"$IDENT$\" muss ein positiver Ganzzahlwert sein. Der übergebene Indexwert ist vom Typ '$TYPE1$'.");
    m_vErrMsg[ecGENERIC]                  = _T("Allgemeiner Parser Fehler.");
    m_vErrMsg[ecINVALID_TYPE]             = _T("Ungültiger Funktionsargumenttyp.");
    m_vErrMsg[ecINVALID_TYPECAST]         = _T("Umwandlungen vom Typ '$TYPE1$' in den Typ '$TYPE2$' werden nicht unterstützt!");
    m_vErrMsg[ecARRAY_SIZE_MISMATCH]      = _T("Feldgrößen stimmen nicht überein.");
    m_vErrMsg[ecNOT_AN_ARRAY]             = _T("Der Indexoperator kann nicht auf die Skalarvariable \"$IDENT$\" angewandt werden.");
    m_vErrMsg[ecUNEXPECTED_SQR_BRACKET]   = _T("Eckige Klammern sind an dieser Position nicht erlaubt.");
	m_vErrMsg[ecUNEXPECTED_CURLY_BRACKET] = _T("Geschweifte Klammern sind an dieser Position nicht erlaubt.");
    m_vErrMsg[ecINDEX_OUT_OF_BOUNDS]      = _T("Indexüberschreitung bei Variablenzugriff auf \"$IDENT$\".");
    m_vErrMsg[ecINDEX_DIMENSION]          = _T("Die Operation kann nicht auf Felder angewandt werden, deren Größe unterschiedlich ist.");
    m_vErrMsg[ecMISSING_SQR_BRACKET]      = _T("Fehlendes \"]\".");
	m_vErrMsg[ecMISSING_CURLY_BRACKET]    = _T("Fehlendes \"}\".");
    m_vErrMsg[ecASSIGNEMENT_TO_VALUE]     = _T("Der Zuweisungsoperator \"$IDENT$\" kann in diesem Zusammenhang nicht verwendet werden.");
    m_vErrMsg[ecEVAL]                     = _T("Die Funktion bzw. der Operator \"$IDENT$\" kann nicht berechnet werden: $HINT$");
    m_vErrMsg[ecINVALID_PARAMETER]        = _T("Der Parameter $ARG$ der Funktion \"$IDENT$\" is ungültig.");
    m_vErrMsg[ecINVALID_NUMBER_OF_PARAMETERS] = _T("Unzulässige Zahl an Funktionsparametern.");
    m_vErrMsg[ecOVERFLOW]                     = _T("Ein arithmetische Überlauf wurde in Funktion/Operator \"$IDENT$\" entdeckt.");
    m_vErrMsg[ecMATRIX_DIMENSION_MISMATCH]    = _T("Matrixdimensionen stimmen nicht überein, Operation \"$IDENT$\" kann nicht ausgeführt werden.");
    m_vErrMsg[ecVARIABLE_DEFINED]             = _T("Die Variable \"$IDENT$\" is bereits definiert.");
    m_vErrMsg[ecCONSTANT_DEFINED]             = _T("Die Konstante \"$IDENT$\" is bereits definiert.");
    m_vErrMsg[ecFUNOPRT_DEFINED]              = _T("Ein Element mit der Bezeichnung \"$IDENT$\" ist bereits definiert.");
  }
#endif // _UNICODE

MUP_NAMESPACE_END
