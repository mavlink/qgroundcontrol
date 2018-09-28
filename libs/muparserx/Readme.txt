#########################################################################
#                                                                       #
#               __________                                 ____  ___    #
#    _____  __ _\______   \_____ _______  ______ __________\   \/  /    #
#   /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     /     #
#  |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \     #
#  |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \    #
#        \/                     \/           \/     \/           \_/    #
#                                                                       #
#  Copyright (C) 2016, Ingo Berg                                        #
#                                                                       #
#  Web:     http://muparserx.beltoforion.de                             #
#  SVN:     http://muparserx.googlecode.com/svn/trunk                   #
#  e-mail:  muparserx@beltoforion.de                                    #
#                                                                       #
#  The code contains contributions made by the following people:        #
#           Martin Rotter (https://github.com/martinrotter)             #
#           Josh Blum     (https://github.com/guruofquality)            #
#           and others                                                  #
#                                                                       #
#  This software is distributed under the terms of the                  #
#  BSD - Clause 2 "Simplified" or "FreeBSD" Licence (BSD-2-Clause)      #
#                                                                       #
#########################################################################


#########################################################################
#                                                                       #
#  Version history                                                      #
#                                                                       #
#########################################################################

V4.0.7 (20160331)
-----------------
Bugfixes:
  - Issue 68: 	Assertion fails (i.e "abs(-3)>abs(2)")
  - untracked issue: cbrt function did not work properly
  - new functions: atan2, reminder, fmod
  
V4.0.5 (20151120)
-----------------

Changes:
  - New Noncomplex Functions:	cbrt  - Cubic root
                                hypot - Length of a 2d vector
                                pow   - Power function
  - New complex functions:      pow   - Power function
  - Value construction and assignment from int is supported again (removed
    when going from in 3.x to 4.x).

V4.0.4 (20151015)
-----------------

Bugfixes:
  - Issue 59, 60, 61, 63: Various segfaults/assertions for unexpected 
                          input        
  - Issue 55, 56, 57, 58: Various issued related to a failure to detect 
                          missing brackets.

V4.0.0 (20150622)
-----------------

API changes:
  - removed value and variable constructors from integer types. There was 
    some confusion about the extend of support for integers (see Issue 
    #36). Internally muparserx is always using floating point values even 
    when dealing with integer numbers. There is no point in the API 
    pretending to have real integer support.

V3.0.2 (20140531)
-----------------

Syntax rule changes:
  - Semicolon removed as term separator (was not meant to be in the release)
  - postfix operator identifiers are searched only at locations that could 
    contain such an operator. (This will prevent conflicts with variable names.)
  
Bugfixes:
  - untracked issue: Identifiers of uninitialized variables could be lost in 
    error messages.
  - Issue 33: Value::operator+= and Value::operator-= do not work properly
              when used with complex values. 
  
V3.0.1 (20140527)
-----------------

Bugfixes:
  - untracked issue: Index operator did not work properly when applied to non 
                     variable types. The bug only occured when used together 
                     with an assignment operator.
  
V3.0.0 (20140525)
-----------------

Warning: The library requires a C++11 compliant compiler!

Syntax rule changes:
  - Curly brackets removed from unit postfix operator identifiers. (I need them 
    for on the fly array construction)
  - in place array construction like: {1,2,3} to create a row vector
  - Comma can no longer be used to separate terms in an expression. 
  - '#' added for comments
  
API Changes:
  - C++11 features introduced in the code
  - Functions defining variables, constants, operators or functions will now 
    throw an exception if a a token with a similar identifier already exists. 
    Their API changed to take a managed pointer instead of a raw pointer.
  - Hooks for customizing error messages added; German translations added
  - Functions for undefining variables, constants, functions and operators
    added.
  - Functions for querying the presence of variables, constants, functions and 
    operators added.

Changes:
  - added "zeros" function for creating matrices initialized to all zero added
  - added "eye" function for creating an idendity matrices
  - added "size" function for determining matrix dimensions
  - factorial operator added
  - floating point data type can now be selected with the "MUP_FLOAT_TYPE" macro
    in mpDefines.h

Bugfixes:
  - Issue 14: Precedence of Unary Operators
  - Issue 17: Wrong result on complex power
  - Issue 20: Library crash when " " is calculated
  - Issue 23: min, max and sum functions return values when called without parameters
  - Issue 26: bugfixes for "<<" and ">>" operators.
  - Issue 27: Querying multiple results of comma separated expressions did not work 
              (multiple results are no longer supported)
  - Issue 31: m_nPosExpr incorrect value after Eval() or GetExprVar()
  - untracked issue: compiling with UNICODE did not work
  - untracked issue: Column number of matrices were not reported correctly
  - untracked issue: expressions ending with newline could not be evaluated

V2.1.6 (20121221; Mayan calendar doomsday edition)
--------------------------------------------------

Bugfixes:
  - Issue 16: Wrong operator precedence. Some binary operators had incorrect 
              precedence values.
  - untracked issue: "Unexpected variable" errors could report incorrect expression 
                     positions

V2.1.5 (20121102)
-----------------

Bugfixes:
  - Issue 13: Unpredictable behaviour when using backslash character in strings; 
              Fixed by adding support for more escape sequences ("\n", "\r", "\t", 
              "\"", "\\")

V2.1.4 (20121010)
-----------------
Changes / Additions:
  - Added casting opertors to the Value class
  - Added project for qt creator

Bugfixes:
  - Issue 8: 	Crash on invalid expression in Release 
  - Issue 11:	Roots of negative numbers computed incorrectly
  - untracked issue: Fixed a problem caused by changes in the behaviour of tellg in GCC 4.6

V2.1.3 (20120715)
-----------------
Bugfixes:
  - Issue 7: 	Memory leak in GetExprVar


V2.1.2 (20120221)
-----------------
Changes:
  - License changed to the BSD-2-Clause licence

Bugfixes:
  - Compiler warnings for gcc (gcc 4.3.4) fixed
  - Issue 4 fixed: 	memory exception in mpValReader.cpp

V2.1.1 (20120201)
-----------------
Bugfixes:
  - Complex power operations could introduce small imaginary values in the result 
    even if both arguments were real numbers. 
    (i.e. the result of -2^8 was something like "-8 + 9e-19i")

V2.1.0 (20111016)
-----------------
Bugfixes:
  - Issue 1: Assertion when using a function with multiple arguments 
             in the same expression twice with different number of arguments.
             (Reference: http://code.google.com/p/muparserx/issues/detail?id=1)
  - Issue 2: DblValReader::IsValue broken for numbers at the end of the string

V2.0.0 (20111009)
-----------------

Changes:
- data type changed to a matrix type instead of a vector type
- Multidimensional index operator added
  old:
	"m[1][2] = 1"
  new:
	"m[1,2] = 1"

- internal change: type identifier for matrices is now 'm', formerly 'a' was used to 
  indicate arrays. An arrays are now seen as subsets of matrices, there is no special 
  type for arrays.

Bugfixes:
- Matrix addition and subtraction work properly now.


V1.10.2 (20110911)
------------------
Bugfixes:
- Fix for changed behaviour of tellg in GCC 4.6.1. A Space is now appended 
  to every expression in order to avoid problems.


V1.10 (20110310)
----------------
Warning:
The API of this version is not backwards compatible and requires minor 
changes to client code!

Changes:
- Static type checking removed entirely
  (All type checking must be made at runtime by the callbacks themself)
- Optimizer removed. The optimizer had only limited effect since it only
  implemented a very simple constant folding mechanism. It collided with 
  the new if-then-else logic and had to go. It will probably be reintroduced
  in one of the next versions using a different implementation.
- Expressions can now span multiple lines. This only makes sense when used
  together with the assignment operator i.e.:

		a=1
		b=2
		c=3
		sin(a+b+c)


Bugfixes:
- Nested if-then-else did not work properly
- Sign operators extended to work with arrays
- Operators "==" and "!=" did not work properly for arrays
- Relational operators "<", ">", "<=", ">=" did not work for complex numbers
- GCC makefile was broken in V1.09
- complex multiplication did not work correctly; 
  was:
	 (a+ib)(c+id)=>re=ac-bd;im=ad-bc 
  instead of:
         (a+ib)(c+id)=>re=ac-bd;im=ad+bc 
- Expressions with parameterless functions like a=foo() did not evaluate properly.


V1.09 (20101120)
----------------
Changes:
- Performance increased by factor 4 due to introducing a simple memory pool for value items.
- C++ like if-then-else operator added ( "(a<b) ? c : d")

Bugfixes:
- Memory leak fixed which prevented operator and funtion items from beeing released.


V1.08 (20100902)
----------------
Changes:
- Implicit creation of unknown variables at parser runtime is now possibe
  (i.e. expressions like "a=0"; with a beeing a undefined variable).
- Callbacks can now be organized in packages
- Callbacks split into a complex and a non complex package
- Complex power operator added

Bugfixes:
- Assignment operators did not work properly for complex values
- Complex sign operator fixed in order not to mess up 0 by
  multiplying it with -1 (-0 and 0 ar not the same according to
  IEEE754) 


V1.07 (20100818)
----------------
Changes:
- Parsing performance improved by 20 - 30 % due to removing unnecessary 
  copy constructor calls when returning the final result.
- License changed from GPLv3 to LGPLv3
- Assignment to vector elements is now possible (i.e.: va[1]=9)

Bugfixes:
- The Value type could not properly handle matrices (vector of vector)
  uses the reverse polish notation of an expression for the evaluation.
- Error messages did not display the proper type id's when compiled 
  with UNICODE support.


V1.06 (20100710)
----------------
- Parsing performance improved by factor 2-3 due to a change which
  uses the reverse polish notation of an expression for the evaluation.


V1.05 (20100530)
----------------
- The parser now handles the associativity of binary operators properly
- Parsing performance improved by factor 7 due to caching tokens 
  once an expression is parsed. Successive evaluations will use 
  the cached tokens instead of parsing from string again.


V1.04 (20100516):
-----------------
- Querying of expression variables implemented
- Bugfix for incorrect evaluation of expressions using the index 
  operator added
- nil values removed
- Support for functions without parameters added
- UNICODE support added


V1.03:
------
- basic functions rewritten to complex valued functions
- Unit postfix operators added to the standard setup


V1.02:
------
- Index operator added
- addition/subtraction of vectors added


V1.01:
------
- Complex numbers added with support for basic binary operators
- Vector type added with support for basic operations (v*v, v+v)
- Variable class changed to take a pointer to a value class instead
  of base types like int or double.
