"""AndroidSerial's C++ and the Java it calls agree on the names JNI binds and the values they share.

Both sides build with a mismatch. For a method or field, C++ only logs "Java method not found",
"Java field not found" or "Failed to register native methods for" at runtime. A result code or
resource error reason that Java numbers differently reaches C++ as another one, a result code
outside the range C++ checks reaches it as a plain failure, and C++ truncates a read that is
larger than the payload it accepts. A flow control or control line that the two sides number
differently reaches the other side as another one, C++ takes a stable port name for a device node
unless it starts as C++ expects, and a number for data bits, stop bits or parity that is not the
USB serial library's sets the port up as something else.
"""

from __future__ import annotations

import math
import re
import textwrap
from dataclasses import dataclass
from typing import TYPE_CHECKING

import pytest

from ..._helpers import REPO_ROOT

if TYPE_CHECKING:
    from collections.abc import Iterable, Mapping

# ci-scripts.yml checks these files out and runs when one changes (test_ci_scripts_workflow.py).
CPP_SOURCE = "src/Android/Serial/AndroidSerialJni.cc"
CPP_HEADER = "src/Android/Serial/AndroidSerialJni.h"
CPP_PORT_HEADER = "src/Android/Serial/AndroidSerialPort.h"
CPP_PORT_INFO_SOURCE = "src/Android/Serial/AndroidSerialPortInfo.cc"
JAVA_SOURCE = "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialManager.java"
READER_SOURCE = "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialReader.java"
PORT_INFO_SOURCE = "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialPortInfo.java"
PORT_NAME_SOURCE = "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialPortName.java"
RESOURCE_ERROR_SOURCE = (
    "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialResourceError.java"
)
RESULT_SOURCE = "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialResult.java"
LIBRARY_TEST_SOURCE = (
    "android/src/test/java/org/mavlink/qgroundcontrol/serial/QGCUsbSerialLibraryContractTest.java"
)

_LITERAL_OR_COMMENT = re.compile(r'"(?:\\.|[^"\\\n])*"|/\*.*?\*/|//[^\n]*', re.DOTALL)
# C++ declares a Java method it calls with the JNI types of what the method returns and of what it
# takes, as in `JavaMethod<jint, jstring, jlong> open{"open"};`, and a Java field it reads with
# the field's JNI type, as in `JavaField<jint> vendorId{"vendorId"};`. Qt derives the JNI signature
# from those types (QtJniTypes::methodSignature), and a call compiles only with arguments of them.
_CPP_DECLARATION = (
    r"\b{template}\s*<(?P<types>[^<>;{{}}()]*)>\s*(?P<handle>\w+)\s*"
    r'\{{\s*"(?P<name>\w+)"\s*\}}'
)
_CPP_METHOD = re.compile(_CPP_DECLARATION.format(template="JavaMethod"))
_CPP_FIELD = re.compile(_CPP_DECLARATION.format(template="JavaField"))
# The Java class behind a C++ type of such a declaration: QtJniTypes::QGCUsbSerialPortInfo is the
# class that Q_DECLARE_JNI_CLASS(QGCUsbSerialPortInfo, "org/.../QGCUsbSerialPortInfo") names.
_CPP_JNI_CLASS = re.compile(
    r'\bQ_DECLARE_JNI_CLASS\s*\(\s*(?P<type>\w+)\s*,\s*"(?P<name>[\w/$]+)"\s*\)'
)
# A JNI signature that C++ writes by hand after the name of its method. A pair that opens a brace
# initializer is a JNINativeMethod, {"nativeDevicesChanged", "()V", function}; any other names a
# Java method C++ calls, as in GetStaticMethodID(javaClass, "open", "(Ljava/lang/String;J)I").
_CPP_HANDWRITTEN_METHOD = re.compile(
    r'(?P<native>\{\s*)?"(?P<name>\w+)"\s*,\s*"(?P<signature>\([^"]*)"'
)
_JNI_SIGNATURE_LITERAL = re.compile(r'"\([\w/$;\[]*\)[\w/$;\[]+"')
# C++ registers a function as a native method with Q_JNI_NATIVE_SCOPED_METHOD(function, Scope),
# under the Java name that Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(function, name) gives it.
_CPP_NATIVE_REGISTRATION = re.compile(r"\bQ_JNI_NATIVE_(?:SCOPED_)?METHOD\s*\(\s*(?P<function>\w+)")
_CPP_NATIVE_NAME = re.compile(
    r"\bQ_DECLARE_JNI_NATIVE_METHOD(?:_IN_CURRENT_SCOPE)?\s*\(\s*(?P<function>\w+)\s*"
    r"(?:,\s*(?P<name>\w+)\s*)?\)"
)
# Qt derives the JNI signature it registers such a function under from the types of the function
# itself, which is declared like `static void jniDevicesChanged(JNIEnv* env, jclass, jlong token)`:
# the parameters after the JNI environment and the Java class are those of the Java method.
_CPP_NATIVE_FUNCTION = re.compile(
    r"\b(?P<returns>\w+)\s+(?P<name>\w+)\s*\(\s*JNIEnv\s*\*\s*\w*\s*,\s*j(?:class|object)\b\s*\w*"
    r"\s*(?P<parameters>(?:,[^()]*)?)\)"
)
# A JNI type that C++ writes by hand after the name of its field, as in
# GetFieldID(javaClass, "vendorId", "I").
_CPP_HANDWRITTEN_FIELD = re.compile(
    r'"(?P<name>\w+)"\s*,\s*"(?P<type>\[*(?:[ZBCSIJFD]|L[\w/$]+;))"'
)

_JAVA_PACKAGE = re.compile(r"^\s*package\s+([\w.]+)\s*;", re.MULTILINE)
_JAVA_IMPORT = re.compile(r"^\s*import\s+([\w.]+)\.(\w+)\s*;", re.MULTILINE)
_JAVA_TYPE_DECLARATION = re.compile(r"\b(?:class|interface|enum)\s+(\w+)")
# java.lang, which Java imports implicitly; any other class a source names without declaring or
# importing it belongs to the source's own package.
_JAVA_LANG = ("CharSequence", "Class", "Integer", "Long", "Object", "String", "Throwable")
# Packages start lower case and classes upper case, so `a.b.Outer.Inner` splits without a lookup.
_JAVA_CLASS = re.compile(
    r"(?P<package>(?:[a-z_]\w*\.)*)(?P<outer>[A-Z]\w*)(?P<nested>(?:\.[A-Z]\w*)*)"
)
# Type arguments and parameters; the character set keeps a match inside one declaration.
_JAVA_GENERICS = re.compile(r"<[\w\s,.?&\[\]]*>")
_JAVA_METHOD = re.compile(
    r"(?P<modifiers>(?:\b(?:public|protected|private|abstract|static|final|synchronized|native"
    r"|strictfp|default)\s+)+)"
    r"(?P<returns>[\w.]+(?:\s*\[\s*\])*)\s+(?P<name>\w+)\s*\((?P<parameters>[^)]*)\)"
)
# A field C++ can read of any instance, declared like `public final int vendorId;`.
_JAVA_FIELD = re.compile(r"\bpublic\s+final\s+(?P<type>[\w.]+(?:\s*\[\s*\])*)\s+(?P<name>\w+)\s*;")
# A value Java shares with C++, declared like `static final int NOT_OPEN = -4;` with any modifiers.
_JAVA_INT_CONSTANT = re.compile(r"\bint\s+(?P<name>[A-Z][A-Z\d_]*)\s*=(?P<value>[^;]*);")
# Java numbers the constants of one of the library's enums in a method declared like
# `private static int flowControlCode(final UsbSerialPort.FlowControl flowControl)`, whose body is
# `return switch (flowControl) { case NONE -> 0; case RTS_CTS -> 1; };`.
_JAVA_SWITCH = (
    r"\b{method}\s*\([^()]*\)\s*\{{\s*return\s+switch\s*\([^()]*\)\s*\{{(?P<cases>[^{{}}]*)\}}"
)
_JAVA_SWITCH_CASE = re.compile(r"\bcase\s+(?P<name>\w+)\s*->(?P<value>[^;]*);")
# A Java unit test states the number of a constant of the library's UsbSerialPort for data bits,
# stop bits or parity as `assertEquals(3, UsbSerialPort.STOPBITS_1_5);`, and fails where the
# library has another.
_JAVA_LIBRARY_CONSTANT = re.compile(
    r"\bassertEquals\s*\((?P<value>[^,()]*),\s*UsbSerialPort\s*\.\s*"
    r"(?P<name>(?:DATABITS|STOPBITS|PARITY)_[A-Z\d_]+)\s*\)"
)
# The values C++ shares with Java are the enumerators of an enum, declared like
# `enum class Result : int { Ok = 0, NotOpen = -4 };`. This follows the name of the enum.
_CPP_ENUM_BODY = r"\b[^{;]*\{(?P<enumerators>[^}]*)\}"
# The value is empty where C++ numbers an enumerator implicitly.
_CPP_ENUMERATOR = re.compile(r"(?P<name>\w+)\s*(?:=(?P<value>[^,]*))?")
# C++ takes what a Java command returns for a result only from one enumerator to another, as in
# `result > static_cast<jint>(Result::Ok) || result < static_cast<jint>(Result::AlreadyOpen)`.
_CPP_RESULT_BOUND = re.compile(
    r"\bresult\s*(?P<side>[<>])\s*static_cast\s*<\s*jint\s*>\s*\(\s*Result::(?P<name>\w+)\s*\)"
)
# C++ takes the code of a control line for an index into an array declared like
# `static constexpr QSerialPort::PinoutSignal kSignals[] = {QSerialPort::RequestToSendSignal};`.
_CPP_ARRAY = r"\b{name}\s*\[\s*\]\s*=\s*\{{(?P<elements>[^{{}};]*)\}}"
# C++ turns a value of one of QSerialPort's enums into the number it sends to Java in a function
# declared like `static std::optional<jint> toJavaParity(QSerialPort::Parity parity)`, whose
# switch lists each value as `case QSerialPort::OddParity: return 1;`.
_CPP_SWITCH = (
    r"\b{function}\s*\(\s*QSerialPort::(?P<enum>\w+)\s+(?P<operand>\w+)\s*\)\s*\{{\s*"
    r"switch\s*\(\s*(?P=operand)\s*\)\s*\{{(?P<cases>[^{{}}]*)\}}"
)
# Labels may share what they return.
_CPP_SWITCH_CASE = re.compile(r"(?P<labels>(?:\bcase\s+[\w:]+\s*)+)return\b(?P<value>[^;]*);")
_CPP_CASE_LABEL = re.compile(r"\bcase\s+(?:\w+::)*(\w+)\s*:")
# A label that the reader of a switch cannot read keeps it from reading the switch at all.
_SWITCH_LABEL = re.compile(r"\b(?:case|default)\b")
_JNI_PRIMITIVES = {
    "void": "V",
    "boolean": "Z",
    "byte": "B",
    "char": "C",
    "short": "S",
    "int": "I",
    "long": "J",
    "float": "F",
    "double": "D",
}
# The JNI descriptor that Qt gives a C++ type (QtJniTypes::Traits).
_CPP_JNI_TYPES = {
    "void": "V",
    "jstring": "Ljava/lang/String;",
    "jobject": "Ljava/lang/Object;",
    "jobjectArray": "[Ljava/lang/Object;",
    **{f"j{java}": jni for java, jni in _JNI_PRIMITIVES.items() if java != "void"},
    **{f"j{java}Array": f"[{jni}" for java, jni in _JNI_PRIMITIVES.items() if java != "void"},
}
# The constant of the library's UsbSerialPort that means what a value of QSerialPort's enums does.
_LIBRARY_CONSTANTS = {
    "Data5": "DATABITS_5",
    "Data6": "DATABITS_6",
    "Data7": "DATABITS_7",
    "Data8": "DATABITS_8",
    "OneStop": "STOPBITS_1",
    "OneAndHalfStop": "STOPBITS_1_5",
    "TwoStop": "STOPBITS_2",
    "NoParity": "PARITY_NONE",
    "OddParity": "PARITY_ODD",
    "EvenParity": "PARITY_EVEN",
    "MarkParity": "PARITY_MARK",
    "SpaceParity": "PARITY_SPACE",
}
# The line of the library's UsbSerialPort.ControlLine that a signal of QSerialPort is.
_CONTROL_LINES = {
    "RequestToSendSignal": "RTS",
    "ClearToSendSignal": "CTS",
    "DataTerminalReadySignal": "DTR",
    "DataSetReadySignal": "DSR",
    "DataCarrierDetectSignal": "CD",
    "RingIndicatorSignal": "RI",
}


@dataclass(frozen=True)
class _JavaMethod:
    name: str
    signature: str
    modifiers: frozenset[str]


def _code(source: str, *, strings: bool) -> str:
    """The source without its comments and, unless *strings*, with its string literals emptied."""

    def replacement(match: re.Match[str]) -> str:
        if not match[0].startswith('"'):
            return " "
        return match[0] if strings else '""'

    return _LITERAL_OR_COMMENT.sub(replacement, source)


def _cpp_jni_type(cpp_type: str, classes: dict[str, str]) -> str:
    """JNI descriptor that Qt derives from a C++ type such as ``jlong`` or ``jbyteArray``.

    *classes* are the Java classes that the source names with Q_DECLARE_JNI_CLASS: a type of
    one, such as ``QtJniTypes::QGCUsbSerialPortInfo``, is an object of it, and ``[]`` after a
    type makes an array of it. Raises ValueError for a type that is no JNI type, so that a type
    this test cannot read fails it.
    """
    if cpp_type.endswith("[]"):
        return f"[{_cpp_jni_type(cpp_type[:-2].strip(), classes)}"
    namespace, _, name = cpp_type.rpartition("::")
    if namespace == "QtJniTypes" and name in classes:
        return f"L{classes[name]};"
    if cpp_type not in _CPP_JNI_TYPES:
        raise ValueError(f"unknown C++ type {cpp_type!r}")
    return _CPP_JNI_TYPES[cpp_type]


def _cpp_signature(returns: str, arguments: Iterable[str], classes: dict[str, str]) -> str:
    """JNI signature that Qt derives from the return and argument types of a method.

    A type _cpp_jni_type() cannot read replaces the signature with ``<unknown C++ type ...>``,
    which no Java signature equals.
    """
    try:
        descriptors = [_cpp_jni_type(cpp_type, classes) for cpp_type in arguments]
        return f"({''.join(descriptors)}){_cpp_jni_type(returns, classes)}"
    except ValueError as error:
        return f"<{error}>"


def _cpp_declarations(source: str, declaration: re.Pattern[str]) -> list[tuple[str, str, str]]:
    """(C++ name, Java name, JNI signature) of each Java method C++ declares with _CPP_METHOD.

    With _CPP_FIELD for *declaration*, of each Java field instead, whose signature is its JNI
    type.
    """
    source = _code(source, strings=True)
    classes = {match["type"]: match["name"] for match in _CPP_JNI_CLASS.finditer(source)}
    declared = []
    for match in declaration.finditer(source):
        returns, *arguments = (cpp_type.strip() for cpp_type in match["types"].split(","))
        signature = _cpp_signature(returns, arguments, classes)
        if declaration is _CPP_FIELD:
            # A field is declared like a method that takes nothing and returns its type.
            signature = signature.removeprefix("()")
        declared.append((match["handle"], match["name"], signature))
    return declared


def _cpp_handwritten_methods(source: str, *, native: bool) -> list[tuple[str, str]]:
    """(name, JNI signature) of the Java methods C++ calls by a signature written by hand.

    With *native*, of the native methods it registers under one instead.
    """
    return [
        (match["name"], match["signature"])
        for match in _CPP_HANDWRITTEN_METHOD.finditer(_code(source, strings=True))
        if bool(match["native"]) == native
    ]


def _cpp_native_signature(returns: str, parameters: str) -> str:
    """JNI signature that Qt derives from the return and parameter types of a native function.

    *parameters* are those after the JNI environment and the Java class, such as
    ``, jlong token, jbyteArray data``.
    """
    declarations = [part.split() for part in parameters.split(",") if part.strip()]
    types = [next(word for word in words if word != "const") for words in declarations]
    return _cpp_signature(returns, types, {})


def _cpp_natives(source: str) -> list[tuple[str, str]]:
    """(Java name, JNI signature) of each function C++ registers as a native method.

    The name is the one Q_DECLARE_JNI_NATIVE_METHOD gives the function or, as for Qt, the
    function's own. The signature is the one Qt derives from each declaration of the function; a
    function this test cannot find gets ``<no C++ function ...>`` instead, which no Java
    signature equals.
    """
    source = _code(source, strings=False)
    names = {match["function"]: match["name"] for match in _CPP_NATIVE_NAME.finditer(source)}
    declared: dict[str, set[str]] = {}
    for match in _CPP_NATIVE_FUNCTION.finditer(source):
        signature = _cpp_native_signature(match["returns"], match["parameters"])
        declared.setdefault(match["name"], set()).add(signature)
    return [
        (names.get(function) or function, signature)
        for function in _CPP_NATIVE_REGISTRATION.findall(source)
        for signature in sorted(
            declared.get(function, {f"<no C++ function {function}(JNIEnv*, jclass, ...)>"})
        )
    ]


def _cpp_unnamed_signatures(source: str) -> list[str]:
    """JNI signature literals that no method name precedes, which nothing can check."""
    return _JNI_SIGNATURE_LITERAL.findall(
        _CPP_HANDWRITTEN_METHOD.sub("", _code(source, strings=True))
    )


def _java_classes(source: str) -> tuple[dict[str, str], str]:
    """Binary names of the classes the source declares, imports or gets from java.lang.

    Also returns the prefix of its own package, whose other classes it names without an import.
    """
    classes = {name: f"java/lang/{name}" for name in _JAVA_LANG}
    classes.update(
        (name, f"{package}.{name}".replace(".", "/"))
        for package, name in _JAVA_IMPORT.findall(source)
    )
    package = _JAVA_PACKAGE.search(source)
    prefix = f"{package[1].replace('.', '/')}/" if package else ""
    declared = _JAVA_TYPE_DECLARATION.findall(source)
    for name in declared:
        classes[name] = prefix + (name if name == declared[0] else f"{declared[0]}${name}")
    return classes, prefix


def _jni_type(java_type: str, classes: dict[str, str], package: str) -> str:
    """JNI descriptor of a Java type such as ``long``, ``byte[]`` or ``QGCUsbSerialResult...``.

    Raises ValueError for anything that is neither a primitive nor a class name, so that a
    type this test cannot read fails it.
    """
    dimensions = java_type.count("[") + java_type.count("...")
    name = re.sub(r"\[|\]|\.\.\.|\s", "", java_type)
    if name in _JNI_PRIMITIVES:
        return "[" * dimensions + _JNI_PRIMITIVES[name]
    match = _JAVA_CLASS.fullmatch(name)
    if not match:
        raise ValueError(f"unknown Java type {name!r}")
    if match["package"]:
        outer = match["package"].replace(".", "/") + match["outer"]
    else:
        outer = classes.get(match["outer"], package + match["outer"])
    return "[" * dimensions + f"L{outer}{match['nested'].replace('.', '$')};"


def _parameter_type(parameter: str) -> str:
    """Type of a parameter declaration such as ``final byte[] data``."""
    words = [word for word in parameter.split() if word != "final" and not word.startswith("@")]
    return "".join(words[:-1])


def _java_methods(source: str) -> list[_JavaMethod]:
    """Every method the Java source declares with a modifier, with its JNI signature.

    A type _jni_type() cannot read replaces the signature with ``<unknown Java type ...>``,
    which no C++ signature equals.
    """
    source = _code(source, strings=False)
    # Nested type arguments take one pass per level.
    while (erased := _JAVA_GENERICS.sub("", source)) != source:
        source = erased
    classes, package = _java_classes(source)
    methods = []
    for match in _JAVA_METHOD.finditer(source):
        parameters = [part for part in match["parameters"].split(",") if part.strip()]
        try:
            arguments = [_jni_type(_parameter_type(part), classes, package) for part in parameters]
            signature = f"({''.join(arguments)}){_jni_type(match['returns'], classes, package)}"
        except ValueError as error:
            signature = f"<{error}>"
        methods.append(_JavaMethod(match["name"], signature, frozenset(match["modifiers"].split())))
    return methods


def _signatures(methods: Iterable[tuple[str, str]]) -> dict[str, set[str]]:
    by_name: dict[str, set[str]] = {}
    for name, signature in methods:
        by_name.setdefault(name, set()).add(signature)
    return by_name


def _listed(signatures: set[str]) -> str:
    return ", ".join(sorted(signatures))


def _disagreement(name: str, verb: str, cpp: set[str], kind: str, java: set[str]) -> str:
    declared = f"{kind} {_listed(java)}" if java else f"no {kind} method of that name"
    return f"{name}: C++ {verb} {_listed(cpp)}, but Java declares {declared}"


def _misnamed(declared: Iterable[tuple[str, str, str]], kind: str) -> list[str]:
    """Describe each declaration that C++ names for another Java method or field than it binds."""
    return [
        f"{handle}: C++ declares it for the Java {kind} {name}, so what uses {handle} gets {name}"
        for handle, name, _ in declared
        if handle != name
    ]


def _binding_problems(cpp_source: str, java_source: str) -> list[str]:
    """Describe each disagreement between the C++ method declarations and the Java ones."""
    declared_in_cpp = _cpp_declarations(cpp_source, _CPP_METHOD)
    bound = _signatures((name, signature) for _, name, signature in declared_in_cpp)
    handwritten = _cpp_handwritten_methods(cpp_source, native=True)
    registered = _signatures([*_cpp_natives(cpp_source), *handwritten])
    methods = _java_methods(java_source)
    public_static = _signatures(
        (method.name, method.signature)
        for method in methods
        if {"public", "static"} <= method.modifiers
    )
    native = _signatures(
        (method.name, method.signature) for method in methods if "native" in method.modifiers
    )

    # Methods this test stopped finding would have nothing to disagree with.
    problems = [
        f"C++ has the JNI signature {literal} without a method name before it, so nothing checks it"
        for literal in _cpp_unnamed_signatures(cpp_source)
    ]
    if not bound:
        problems.append("C++ declares no Java method with its JNI types")
    if not registered:
        problems.append("C++ registers no native method")
    problems.extend(
        f"{name}: C++ registers the JNI signature {signature} written by hand, so nothing checks "
        "it against the parameters of its function"
        for name, signature in handwritten
    )
    problems.extend(
        f"{name}: C++ binds the JNI signature {signature} written by hand, so nothing checks it "
        "against the arguments of its calls"
        for name, signature in _cpp_handwritten_methods(cpp_source, native=False)
    )
    problems.extend(_misnamed(declared_in_cpp, "method"))
    declared_natives = len(re.findall(r"\bnative\b", _code(java_source, strings=False)))
    read_natives = sum("native" in method.modifiers for method in methods)
    if declared_natives != read_natives:
        problems.append(
            f"Java declares {declared_natives} native methods, of which this test can read "
            f"{read_natives}"
        )
    for name, signatures in bound.items():
        declared = public_static.get(name, set())
        if not signatures <= declared:
            problems.append(
                _disagreement(name, "binds", signatures - declared, "public static", declared)
            )
    for name, signatures in registered.items():
        declared = native.get(name, set())
        if signatures != declared:
            problems.append(_disagreement(name, "registers", signatures, "native", declared))
    problems.extend(
        f"{name}: Java declares native {_listed(signatures)}, but C++ does not register it, "
        "so its first call throws UnsatisfiedLinkError"
        for name, signatures in native.items()
        if name not in registered
    )
    return problems


def _field_problems(cpp_source: str, java_source: str) -> list[str]:
    """Describe each field C++ reads that the Java class does not declare with that type."""
    declared_in_cpp = _cpp_declarations(cpp_source, _CPP_FIELD)
    read = {name: jni_type for _, name, jni_type in declared_in_cpp}
    # Fields this test stopped finding would have nothing to disagree with.
    problems = [
        f"{match['name']}: C++ reads the JNI type {match['type']} written by hand, so nothing "
        "checks it against the type its value is read as"
        for match in _CPP_HANDWRITTEN_FIELD.finditer(_code(cpp_source, strings=True))
    ]
    if not read:
        problems.append("C++ declares no Java field with its JNI type")
    problems.extend(_misnamed(declared_in_cpp, "field"))
    java_source = _code(java_source, strings=False)
    classes, package = _java_classes(java_source)
    declared = {}
    for match in _JAVA_FIELD.finditer(java_source):
        try:
            declared[match["name"]] = _jni_type(match["type"], classes, package)
        except ValueError as error:
            declared[match["name"]] = f"<{error}>"
    problems.extend(
        f"{name}: C++ reads {jni_type}, but Java declares "
        + (
            f"public final {declared[name]}"
            if name in declared
            else "no public final field of that name"
        )
        for name, jni_type in read.items()
        if declared.get(name) != jni_type
    )
    return problems


def _integer(expression: str) -> int | None:
    """Value of an integer literal or of a product of them such as ``16 * 1024``, else None."""
    try:
        return math.prod(int(factor, 0) for factor in expression.split("*"))
    except ValueError:
        return None


def _camel_case(constant: str) -> str:
    """The name C++ gives a Java constant: ``DeviceNotFound`` for ``DEVICE_NOT_FOUND``."""
    return "".join(word.capitalize() for word in constant.split("_"))


def _cpp_enumerators(header: str, enum: str) -> dict[str, int | None]:
    """Value of each enumerator of a C++ enum, None where this test cannot read it.

    An enumerator without a value has the one after that of the enumerator before it, or 0.
    """
    declaration = re.search(
        rf"\benum\s+(?:class\s+)?{enum}{_CPP_ENUM_BODY}", _code(header, strings=False)
    )
    enumerators: dict[str, int | None] = {}
    implicit: int | None = 0
    for name, value in _CPP_ENUMERATOR.findall(declaration["enumerators"] if declaration else ""):
        number = _integer(value) if value else implicit
        enumerators[name] = number
        implicit = None if number is None else number + 1
    return enumerators


def _java_int_constants(source: str) -> dict[str, int | None]:
    """Value of each int constant the Java source declares, None where _integer() cannot read it."""
    constants = _JAVA_INT_CONSTANT.findall(_code(source, strings=False))
    return {name: _integer(value) for name, value in constants}


def _java_switch_codes(source: str, method: str) -> dict[str, int | None]:
    """Code that a Java method gives each constant of an enum in a switch like _JAVA_SWITCH.

    None where _integer() cannot read the code, and nothing unless this reads every case.
    """
    switch = re.search(_JAVA_SWITCH.format(method=method), _code(source, strings=False))
    cases = switch["cases"] if switch else ""
    codes = {name: _integer(value) for name, value in _JAVA_SWITCH_CASE.findall(cases)}
    return codes if len(codes) == len(_SWITCH_LABEL.findall(cases)) else {}


def _value_problems(
    cpp: Mapping[str, int | None],
    java: Mapping[str, int | None],
    constants: Mapping[str, str] | None = None,
) -> list[str]:
    """Describe each value that C++ and Java do not share.

    *cpp* has the value of each C++ name, *java* that of each Java constant, and *constants* the
    Java constant of a C++ name: without it, the constant that _camel_case() turns into the name.
    """
    problems = [
        f"{name}: this test cannot read its value in {language}"
        for language, values in (("C++", cpp), ("Java", java))
        for name, value in values.items()
        if value is None
    ]
    if problems:
        return problems
    if constants is None:
        constants = {_camel_case(constant): constant for constant in java}
    for name, value in cpp.items():
        constant = constants.get(name)
        if constant is None or constant not in java:
            missing = constant or "constant of that name"
            problems.append(f"{name}: C++ has {value}, but Java declares no {missing}")
        elif java[constant] != value:
            problems.append(f"{name}: C++ has {value}, but Java has {constant} = {java[constant]}")
    names = {constant: name for name, constant in constants.items()}
    problems.extend(
        f"{constant}: Java has {value}, but C++ declares no {names.get(constant) or 'name for it'}"
        for constant, value in java.items()
        if names.get(constant) not in cpp
    )
    return problems


def _shared_value_problems(cpp_header: str, enum: str, java_source: str) -> list[str]:
    """Describe each value that a C++ enum class and a Java class's int constants do not share.

    AndroidSerial::Result has the constants of QGCUsbSerialResult, and
    AndroidSerial::ResourceErrorReason those of QGCUsbSerialResourceError.
    """
    cpp = _cpp_enumerators(cpp_header, enum)
    java = _java_int_constants(java_source)
    # Values this test stopped finding or reading would have nothing to disagree with.
    problems = []
    if not cpp:
        problems.append(f"C++ declares no enumerator of enum class {enum}")
    if not java:
        problems.append("Java declares no int constant")
    return problems or _value_problems(cpp, java)


def _flow_control_problems(cpp_header: str, java_source: str) -> list[str]:
    """Describe each flow control that C++ and Java give different codes.

    AndroidSerial::FlowControl numbers them for C++, and flowControlCode() in QGCUsbSerialManager
    for Java, also where it looks up the flow control of a code that C++ sent.
    """
    cpp = _cpp_enumerators(cpp_header, "FlowControl")
    java = _java_switch_codes(java_source, "flowControlCode")
    problems = []
    if not cpp:
        problems.append("C++ declares no enumerator of enum class FlowControl")
    if not java:
        problems.append("Java declares no flowControlCode() with a switch this test can read")
    return problems or _value_problems(cpp, java)


def _cpp_array(source: str, name: str) -> list[str]:
    """Elements of a C++ array declared like _CPP_ARRAY, each without its scope."""
    declaration = re.search(_CPP_ARRAY.format(name=name), _code(source, strings=False))
    elements = declaration["elements"].split(",") if declaration else []
    return [element.rpartition("::")[2].strip() for element in elements if element.strip()]


def _control_line_problems(cpp_source: str, java_source: str) -> list[str]:
    """Describe each control line that C++ and Java give different codes.

    Java sends the code that controlLineCode() in QGCUsbSerialManager gives a line, and C++ takes
    it for the signal at that index of kSignals in AndroidSerialJni.cc.
    """
    cpp = {signal: code for code, signal in enumerate(_cpp_array(cpp_source, "kSignals"))}
    java = _java_switch_codes(java_source, "controlLineCode")
    problems = []
    if not cpp:
        problems.append("C++ declares no kSignals with elements this test can read")
    if not java:
        problems.append("Java declares no controlLineCode() with a switch this test can read")
    return problems or _value_problems(cpp, java, _CONTROL_LINES)


def _result_range_problems(cpp_source: str, cpp_header: str) -> list[str]:
    """Describe each result that C++ takes for no result when a Java command returns it.

    callJavaCommand() in AndroidSerialJni.cc turns such a value into Result::Failed.
    """
    codes = _cpp_enumerators(cpp_header, "Result")
    bounds = dict(_CPP_RESULT_BOUND.findall(_code(cpp_source, strings=False)))
    greatest = codes.get(bounds.get(">", ""))
    least = codes.get(bounds.get("<", ""))
    if greatest is None or least is None:
        return ["C++ checks what Java returns against no range of results this test can read"]
    return [
        f"{name}: C++ takes {value} from Java for no result, only {least} to {greatest}"
        for name, value in codes.items()
        if value is not None and not least <= value <= greatest
    ]


def _constant(source: str, name: str) -> int | None:
    """Value of a constant declared like ``constexpr jsize kLimit = 16 * 1024;``.

    None if the source does not declare it or _integer() cannot read its value.
    """
    declaration = re.search(rf"\b{name}\s*=(?!=)([^;]*);", _code(source, strings=False))
    return _integer(declaration[1]) if declaration else None


def _read_buffer_problems(cpp_source: str, java_source: str) -> list[str]:
    """Describe why a read that fills Java's buffer may not reach C++ whole."""
    limit = _constant(cpp_source, "kMaxNativePayloadBytes")
    size = _constant(java_source, "READ_BUF_SIZE")
    problems = []
    if limit is None:
        problems.append("C++ declares no kMaxNativePayloadBytes with a value this test can read")
    if size is None:
        problems.append("Java declares no READ_BUF_SIZE with a value this test can read")
    if limit is not None and size is not None and size > limit:
        problems.append(
            f"Java passes reads of up to READ_BUF_SIZE = {size} bytes to C++, which truncates "
            f"them to kMaxNativePayloadBytes = {limit}"
        )
    return problems


def _string_constant(source: str, name: str) -> str | None:
    """Text of a string constant, None if the source does not declare it with one string literal.

    C++ declares it like ``constexpr QLatin1String kPrefix("usb:");`` and Java like
    ``static final String PREFIX = "usb:";``.
    """
    declaration = re.search(
        rf'\b{name}\s*[=({{]\s*"((?:\\.|[^"\\\n])*)"\s*[)}}]?\s*;', _code(source, strings=True)
    )
    return declaration[1] if declaration else None


def _stable_name_prefix_problems(cpp_source: str, java_source: str) -> list[str]:
    """Describe why C++ may not tell a stable port name that Java built from a device node."""
    cpp = _string_constant(cpp_source, "kStableUsbNamePrefix")
    java = _string_constant(java_source, "STABLE_NAME_PREFIX")
    problems = []
    if cpp is None:
        problems.append("C++ declares no kStableUsbNamePrefix with a text this test can read")
    if java is None:
        problems.append("Java declares no STABLE_NAME_PREFIX with a text this test can read")
    if cpp is not None and java is not None and cpp != java:
        problems.append(
            f'Java starts a stable port name with STABLE_NAME_PREFIX = "{java}", but C++ takes a '
            f'name for one that starts with kStableUsbNamePrefix = "{cpp}"'
        )
    return problems


def _cpp_switch_values(source: str, function: str, port_header: str) -> dict[str, int | None]:
    """Number that a C++ function like _CPP_SWITCH sends for each value of a QSerialPort enum.

    Where it sends the value itself, as ``static_cast<jint>(dataBits)``, the number is the one
    the enumerator has in *port_header*. None where this test cannot read the number, and nothing
    unless it reads every label of the switch.
    """
    switch = re.search(_CPP_SWITCH.format(function=function), _code(source, strings=False))
    if not switch:
        return {}
    itself = re.compile(rf"\s*static_cast\s*<\s*jint\s*>\s*\(\s*{switch['operand']}\s*\)\s*")
    enumerators = _cpp_enumerators(port_header, switch["enum"])
    values: dict[str, int | None] = {}
    for case in _CPP_SWITCH_CASE.finditer(switch["cases"]):
        for name in _CPP_CASE_LABEL.findall(case["labels"]):
            if itself.fullmatch(case["value"]):
                values[name] = enumerators.get(name)
            else:
                values[name] = _integer(case["value"])
    return values if len(values) == len(_SWITCH_LABEL.findall(switch["cases"])) else {}


def _java_library_constants(source: str) -> dict[str, int | None]:
    """Number a Java unit test states of each constant that _JAVA_LIBRARY_CONSTANT reads.

    None where _integer() cannot read it.
    """
    stated = _JAVA_LIBRARY_CONSTANT.finditer(_code(source, strings=False))
    return {match["name"]: _integer(match["value"]) for match in stated}


def _serial_parameter_problems(cpp_source: str, port_header: str, java_test: str) -> list[str]:
    """Describe each number C++ sends for data bits, stop bits or parity that is not the library's.

    Java hands the numbers to the library as they are. This test cannot read the library, so it
    compares them with the numbers that the Java unit test states of the library's constants.
    """
    cpp: dict[str, int | None] = {}
    problems = []
    for function in ("toJavaDataBits", "toJavaStopBits", "toJavaParity"):
        sent = _cpp_switch_values(cpp_source, function, port_header)
        if not sent:
            problems.append(f"C++ declares no {function}() with a switch this test can read")
        cpp.update(sent)
    java = _java_library_constants(java_test)
    if not java:
        problems.append("Java states no number of UsbSerialPort for data bits, stop bits or parity")
    return problems or _value_problems(cpp, java, _LIBRARY_CONSTANTS)


def test_cpp_and_java_agree_on_the_usb_serial_jni_methods() -> None:
    problems = _binding_problems(
        (REPO_ROOT / CPP_SOURCE).read_text(encoding="utf-8"),
        (REPO_ROOT / JAVA_SOURCE).read_text(encoding="utf-8"),
    )
    assert not problems, f"{CPP_SOURCE} and {JAVA_SOURCE} disagree:\n" + "\n".join(problems)


def test_cpp_and_java_agree_on_the_usb_serial_port_info_fields() -> None:
    problems = _field_problems(
        (REPO_ROOT / CPP_SOURCE).read_text(encoding="utf-8"),
        (REPO_ROOT / PORT_INFO_SOURCE).read_text(encoding="utf-8"),
    )
    assert not problems, f"{CPP_SOURCE} and {PORT_INFO_SOURCE} disagree:\n" + "\n".join(problems)


def test_cpp_and_java_agree_on_the_usb_serial_result_codes() -> None:
    problems = _shared_value_problems(
        (REPO_ROOT / CPP_HEADER).read_text(encoding="utf-8"),
        "Result",
        (REPO_ROOT / RESULT_SOURCE).read_text(encoding="utf-8"),
    )
    assert not problems, f"{CPP_HEADER} and {RESULT_SOURCE} disagree:\n" + "\n".join(problems)


def test_cpp_takes_every_usb_serial_result_code_from_java() -> None:
    problems = _result_range_problems(
        (REPO_ROOT / CPP_SOURCE).read_text(encoding="utf-8"),
        (REPO_ROOT / CPP_HEADER).read_text(encoding="utf-8"),
    )
    assert not problems, f"{CPP_SOURCE} and {CPP_HEADER} disagree:\n" + "\n".join(problems)


def test_cpp_and_java_agree_on_the_usb_serial_resource_error_reasons() -> None:
    problems = _shared_value_problems(
        (REPO_ROOT / CPP_HEADER).read_text(encoding="utf-8"),
        "ResourceErrorReason",
        (REPO_ROOT / RESOURCE_ERROR_SOURCE).read_text(encoding="utf-8"),
    )
    assert not problems, f"{CPP_HEADER} and {RESOURCE_ERROR_SOURCE} disagree:\n" + "\n".join(
        problems
    )


def test_java_usb_serial_read_buffer_fits_the_payload_cpp_accepts() -> None:
    problems = _read_buffer_problems(
        (REPO_ROOT / CPP_SOURCE).read_text(encoding="utf-8"),
        (REPO_ROOT / READER_SOURCE).read_text(encoding="utf-8"),
    )
    assert not problems, f"{CPP_SOURCE} and {READER_SOURCE} disagree:\n" + "\n".join(problems)


def test_cpp_and_java_agree_on_the_usb_serial_flow_control_codes() -> None:
    problems = _flow_control_problems(
        (REPO_ROOT / CPP_HEADER).read_text(encoding="utf-8"),
        (REPO_ROOT / JAVA_SOURCE).read_text(encoding="utf-8"),
    )
    assert not problems, f"{CPP_HEADER} and {JAVA_SOURCE} disagree:\n" + "\n".join(problems)


def test_cpp_and_java_agree_on_the_usb_serial_control_line_codes() -> None:
    problems = _control_line_problems(
        (REPO_ROOT / CPP_SOURCE).read_text(encoding="utf-8"),
        (REPO_ROOT / JAVA_SOURCE).read_text(encoding="utf-8"),
    )
    assert not problems, f"{CPP_SOURCE} and {JAVA_SOURCE} disagree:\n" + "\n".join(problems)


def test_cpp_and_java_agree_on_the_prefix_of_a_stable_usb_serial_port_name() -> None:
    problems = _stable_name_prefix_problems(
        (REPO_ROOT / CPP_PORT_INFO_SOURCE).read_text(encoding="utf-8"),
        (REPO_ROOT / PORT_NAME_SOURCE).read_text(encoding="utf-8"),
    )
    assert not problems, f"{CPP_PORT_INFO_SOURCE} and {PORT_NAME_SOURCE} disagree:\n" + "\n".join(
        problems
    )


def test_cpp_sends_the_usb_serial_librarys_numbers_for_data_bits_stop_bits_and_parity() -> None:
    problems = _serial_parameter_problems(
        (REPO_ROOT / CPP_SOURCE).read_text(encoding="utf-8"),
        (REPO_ROOT / CPP_PORT_HEADER).read_text(encoding="utf-8"),
        (REPO_ROOT / LIBRARY_TEST_SOURCE).read_text(encoding="utf-8"),
    )
    assert not problems, (
        f"{CPP_SOURCE}, with {CPP_PORT_HEADER}, and {LIBRARY_TEST_SOURCE} disagree:\n"
        + "\n".join(problems)
    )


_SERIAL = "Lorg/mavlink/qgroundcontrol/serial/"
# Declarations wrap as clang-format and the Java sources wrap them.
_CPP = textwrap.dedent("""\
    Q_DECLARE_JNI_CLASS(QGCUsbSerialPortInfo,
                        "org/mavlink/qgroundcontrol/serial/QGCUsbSerialPortInfo")

    template <typename Ret, typename... Args>
    struct JavaMethod : JavaBinding<jmethodID>
    {
        static constexpr auto kSignature = QtJniTypes::methodSignature<Ret, Args...>();
    };

    struct JniMethodCache
    {
        JavaMethod<QtJniTypes::QGCUsbSerialPortInfo[]> availablePorts{"availablePorts"};
        JavaMethod<jint, jstring, jlong> open{"open"};
        JavaMethod<jbyteArray, jint, jint, jint> read{"read"};
        // JavaMethod<void> removed{"removed"};
        JavaMethod<jint, jint>
            getDataTerminalReadySupport{"getDataTerminalReadySupport"};
    };

    static bool callJavaBool(const JavaMethod<jboolean, jlong>& method, PortHandle handle);

    static jboolean jniDeviceNewData(JNIEnv* env, jclass javaClass, jlong token, jbyteArray data);
    // static void jniRemoved(JNIEnv* env, jclass javaClass);
    static void jniDeviceResourceError(JNIEnv* env, jclass javaClass, jlong token, jint reason,
                                       jstring detail);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceNewData, nativeDeviceNewData)
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceResourceError,
                                                 nativeDeviceResourceError)

    void setNativeMethods()
    {
        const JNINativeMethod javaMethods[]{
            Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceNewData, AndroidSerial),
            // Q_JNI_NATIVE_SCOPED_METHOD(jniRemoved, AndroidSerial),
            Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceResourceError, AndroidSerial),
        };
    }

    static jboolean jniDeviceNewData(JNIEnv* env, jclass, jlong token, jbyteArray data)
    {
        return deliverNewData(token, data);
    }

    static void jniDeviceResourceError(JNIEnv*, jclass, jlong token, jint reason, jstring detail)
    {
    }
    """)
_JAVA = textwrap.dedent("""\
    package org.mavlink.qgroundcontrol.serial;

    import android.content.Context;

    public final class QGCUsbSerialManager {
        private static native boolean nativeDeviceNewData(long token, byte[] data);
        private static native void nativeDeviceResourceError(long token, int reason, String detail);

        /** Not bound from C++, unlike {@code public static int stale(int portId)}. */
        public static void initialize(final Context context) {}

        public static QGCUsbSerialPortInfo[] availablePorts() {
            return sDevices.availablePorts();
        }

        public static int open(final String deviceName, final long token) {
            return findPort(deviceName, token);
        }

        public static byte[] read(final int portId, final int length,
                                  final int timeoutMs) {
            return NO_DATA;
        }

        public static int getDataTerminalReadySupport(final int portId) {
            return 1;
        }
    }
    """)


def test_agreeing_sources_have_no_problems() -> None:
    assert _cpp_declarations(_CPP, _CPP_METHOD) == [
        (
            "availablePorts",
            "availablePorts",
            "()[Lorg/mavlink/qgroundcontrol/serial/QGCUsbSerialPortInfo;",
        ),
        ("open", "open", "(Ljava/lang/String;J)I"),
        ("read", "read", "(III)[B"),
        ("getDataTerminalReadySupport", "getDataTerminalReadySupport", "(I)I"),
    ]
    assert _cpp_handwritten_methods(_CPP, native=False) == []
    assert _cpp_handwritten_methods(_CPP, native=True) == []
    assert _cpp_natives(_CPP) == [
        ("nativeDeviceNewData", "(J[B)Z"),
        ("nativeDeviceResourceError", "(JILjava/lang/String;)V"),
    ]
    assert _binding_problems(_CPP, _JAVA) == []


@pytest.mark.parametrize(
    ("cpp", "java", "problem"),
    [
        pytest.param(
            _CPP.replace("JavaMethod<jint, jstring, jlong> open", "JavaMethod<jint, jstring> open"),
            _JAVA,
            "open: C++ binds (Ljava/lang/String;)I, but Java declares public static "
            "(Ljava/lang/String;J)I",
            id="missing-argument",
        ),
        pytest.param(
            _CPP.replace("<jint, jstring, jlong> open", "<jint, jstring, jint> open"),
            _JAVA,
            "open: C++ binds (Ljava/lang/String;I)I, but Java declares public static "
            "(Ljava/lang/String;J)I",
            id="mismatched-argument-type",
        ),
        pytest.param(
            _CPP.replace("<jint, jstring, jlong> open", "<jint, QString, qint64> open"),
            _JAVA,
            "open: C++ binds <unknown C++ type 'QString'>, but Java declares public static "
            "(Ljava/lang/String;J)I",
            id="unknown-cpp-type-of-a-method",
        ),
        pytest.param(
            # Qt derives an array of java.lang.Object from it, which Java does not return.
            _CPP.replace("QtJniTypes::QGCUsbSerialPortInfo[]", "jobjectArray"),
            _JAVA,
            "availablePorts: C++ binds ()[Ljava/lang/Object;, but Java declares public static "
            f"()[{_SERIAL}QGCUsbSerialPortInfo;",
            id="array-of-no-class",
        ),
        pytest.param(
            _CPP.replace("serial/QGCUsbSerialPortInfo", "QGCUsbSerialPortInfo"),
            _JAVA,
            "availablePorts: C++ binds ()[Lorg/mavlink/qgroundcontrol/QGCUsbSerialPortInfo;, but "
            f"Java declares public static ()[{_SERIAL}QGCUsbSerialPortInfo;",
            id="mismatched-class",
        ),
        pytest.param(
            _CPP.replace(
                "Q_DECLARE_JNI_CLASS(QGCUsbSerialPortInfo,", "Q_DECLARE_JNI_CLASS(PortInfo,"
            ),
            _JAVA,
            "availablePorts: C++ binds <unknown C++ type 'QtJniTypes::QGCUsbSerialPortInfo'>, but "
            f"Java declares public static ()[{_SERIAL}QGCUsbSerialPortInfo;",
            id="undeclared-class",
        ),
        pytest.param(
            _CPP.replace('open{"open"}', 'close{"open"}'),
            _JAVA,
            "close: C++ declares it for the Java method open, so what uses close gets open",
            id="declaration-named-for-another-method",
        ),
        pytest.param(
            _CPP
            + 'const MethodDef defs[] = {{&s_methods.open, "open", "(Ljava/lang/String;J)I"}};\n',
            _JAVA,
            "open: C++ binds the JNI signature (Ljava/lang/String;J)I written by hand, so nothing "
            "checks it against the arguments of its calls",
            id="handwritten-method-signature",
        ),
        pytest.param(
            _CPP,
            _JAVA.replace("public static byte[] read(", "public static byte[] readBytes("),
            "read: C++ binds (III)[B, but Java declares no public static method of that name",
            id="missing-java-method",
        ),
        pytest.param(
            _CPP,
            _JAVA.replace("public static byte[] read(", "static byte[] read("),
            "read: C++ binds (III)[B, but Java declares no public static method of that name",
            id="java-method-not-public",
        ),
        pytest.param(
            _CPP.replace("Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceNewData, AndroidSerial),", "", 1),
            _JAVA,
            "nativeDeviceNewData: Java declares native (J[B)Z, but C++ does not register it, so "
            "its first call throws UnsatisfiedLinkError",
            id="unregistered-native",
        ),
        pytest.param(
            _CPP.replace("jlong token, jbyteArray data", "jlong token, jstring data"),
            _JAVA,
            "nativeDeviceNewData: C++ registers (JLjava/lang/String;)Z, but Java declares native "
            "(J[B)Z",
            id="mismatched-native-parameter",
        ),
        pytest.param(
            _CPP.replace("static jboolean jniDeviceNewData(", "static void jniDeviceNewData("),
            _JAVA,
            "nativeDeviceNewData: C++ registers (J[B)V, but Java declares native (J[B)Z",
            id="mismatched-native-return",
        ),
        pytest.param(
            _CPP.replace(
                "jlong token, jbyteArray data", "jlong token, jbyteArray data, jint length", 1
            ),
            _JAVA,
            "nativeDeviceNewData: C++ registers (J[B)Z, (J[BI)Z, but Java declares native (J[B)Z",
            id="native-declarations-differ",
        ),
        pytest.param(
            _CPP.replace("jstring detail", "QString detail"),
            _JAVA,
            "nativeDeviceResourceError: C++ registers <unknown C++ type 'QString'>, but Java "
            "declares native (JILjava/lang/String;)V",
            id="unknown-cpp-type",
        ),
        pytest.param(
            _CPP.replace("static jboolean jniDeviceNewData(", "static jboolean jniNewData("),
            _JAVA,
            "nativeDeviceNewData: C++ registers <no C++ function jniDeviceNewData(JNIEnv*, jclass, "
            "...)>, but Java declares native (J[B)Z",
            id="native-function-not-found",
        ),
        pytest.param(
            _CPP.replace(
                "Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceNewData, AndroidSerial),",
                '{"nativeDeviceNewData", "(J[B)Z", reinterpret_cast<void*>(jniDeviceNewData)},',
                1,
            ),
            _JAVA,
            "nativeDeviceNewData: C++ registers the JNI signature (J[B)Z written by hand, so "
            "nothing checks it against the parameters of its function",
            id="handwritten-native-signature",
        ),
        pytest.param(
            _CPP,
            _JAVA.replace("long token, byte[] data", "long token, String data"),
            "nativeDeviceNewData: C++ registers (J[B)Z, but Java declares native "
            "(JLjava/lang/String;)Z",
            id="mismatched-native-signature",
        ),
        pytest.param(
            _CPP,
            _JAVA.replace("native boolean nativeDeviceNewData", "native void nativeDeviceNewData"),
            "nativeDeviceNewData: C++ registers (J[B)Z, but Java declares native (J[B)V",
            id="mismatched-native-return-in-java",
        ),
        pytest.param(
            _CPP,
            _JAVA.replace("native boolean nativeDeviceNewData", "boolean nativeDeviceNewData"),
            "nativeDeviceNewData: C++ registers (J[B)Z, but Java declares no native method of "
            "that name",
            id="native-not-declared",
        ),
        pytest.param(
            _CPP,
            _JAVA.replace(
                "final int portId, final int length,", "final uint portId, final int length,"
            ),
            "read: C++ binds (III)[B, but Java declares public static <unknown Java type 'uint'>",
            id="unknown-java-type",
        ),
        pytest.param(
            _CPP,
            _JAVA.replace(
                "{\n", "{\n    private static native @Keep void nativeDevicesChanged();\n", 1
            ),
            "Java declares 3 native methods, of which this test can read 2",
            id="unreadable-native",
        ),
    ],
)
def test_reports_each_disagreement(cpp: str, java: str, problem: str) -> None:
    assert _binding_problems(cpp, java) == [problem]


def test_reports_methods_it_cannot_find_in_cpp() -> None:
    java = "public final class QGCUsbSerialManager {}"
    assert _binding_problems("void setNativeMethods() {}", java) == [
        "C++ declares no Java method with its JNI types",
        "C++ registers no native method",
    ]
    # A function that no macro gives a Java name is registered under its own, as by Qt.
    unnamed_native = _CPP.replace(
        "Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceNewData, nativeDeviceNewData)",
        "Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceNewData)",
    )
    assert _binding_problems(unnamed_native, _JAVA) == [
        "jniDeviceNewData: C++ registers (J[B)Z, but Java declares no native method of that name",
        "nativeDeviceNewData: Java declares native (J[B)Z, but C++ does not register it, so its "
        "first call throws UnsatisfiedLinkError",
    ]
    unnamed = _CPP + 'static constexpr const char* kOpenSignature = "(Ljava/lang/String;J)I";\n'
    assert _binding_problems(unnamed, _JAVA) == [
        'C++ has the JNI signature "(Ljava/lang/String;J)I" without a method name before it, '
        "so nothing checks it"
    ]


_JAVA_CLASS_HEAD = textwrap.dedent("""\
    package org.mavlink.qgroundcontrol.serial;

    import android.content.Context;
    import com.hoho.android.usbserial.driver.UsbSerialPort;
    import java.util.List;
    import java.util.Map;

    public final class QGCUsbSerialManager {
        interface NativeCallbacks {}
    """)


@pytest.mark.parametrize(
    ("declaration", "signature"),
    [
        ("static native void f();", "()V"),
        ("public static boolean f(final int a, final long b, boolean c)", "(IJZ)Z"),
        ("public static String[] f(final byte[] a, int[] b)", "([B[I)[Ljava/lang/String;"),
        ("static int[][] f(String... a)", "([Ljava/lang/String;)[[I"),
        (
            "static <T> List<T> f(Map<String, List<T>> a, java.nio.ByteBuffer b)",
            "(Ljava/util/Map;Ljava/nio/ByteBuffer;)Ljava/util/List;",
        ),
        (
            "public static synchronized Context f(@NonNull final Context a)",
            "(Landroid/content/Context;)Landroid/content/Context;",
        ),
        # A class of the source's own package needs no import.
        ("public static QGCUsbSerialResult[] f(final long a)", f"(J)[{_SERIAL}QGCUsbSerialResult;"),
        (
            "static void f(NativeCallbacks a, QGCUsbSerialDevices.UsbHost b)",
            f"({_SERIAL}QGCUsbSerialManager$NativeCallbacks;{_SERIAL}QGCUsbSerialDevices$UsbHost;)V",
        ),
        (
            "static UsbSerialPort.ControlLine f()",
            "()Lcom/hoho/android/usbserial/driver/UsbSerialPort$ControlLine;",
        ),
        ("static native void f(unsigned a);", "<unknown Java type 'unsigned'>"),
    ],
)
def test_derives_jni_signatures_from_java_declarations(declaration: str, signature: str) -> None:
    (method,) = _java_methods(_JAVA_CLASS_HEAD + declaration)
    assert (method.name, method.signature) == ("f", signature)


@pytest.mark.parametrize(
    ("declaration", "signature"),
    [
        ("static void f(JNIEnv* env, jclass javaClass);", "()V"),
        ("static void f(JNIEnv*, jclass, jlong a, jbyteArray b, jint c)\n{\n}", "(J[BI)V"),
        (
            "static jboolean f(JNIEnv *env, jobject thiz, const jstring a,\n    jintArray b);",
            "(Ljava/lang/String;[I)Z",
        ),
        ("static void f(JNIEnv* env, jclass, QString a);", "<unknown C++ type 'QString'>"),
        ("static QString f(JNIEnv* env, jclass);", "<unknown C++ type 'QString'>"),
        ("static void f(jlong a);", "<no C++ function f(JNIEnv*, jclass, ...)>"),
    ],
)
def test_derives_jni_signatures_from_cpp_native_functions(declaration: str, signature: str) -> None:
    registration = (
        "\nconst JNINativeMethod methods[]{Q_JNI_NATIVE_SCOPED_METHOD(f, AndroidSerial)};"
    )
    assert _cpp_natives(declaration + registration) == [("f", signature)]


@pytest.mark.parametrize(
    ("declaration", "signature"),
    [
        ('JavaMethod<void> f{"f"};', "()V"),
        ('JavaMethod<jint, jlong> f{"f"};', "(J)I"),
        (
            'JavaMethod<jbyteArray, jlong, jintArray,\n           jboolean, jstring>\n    f{"f"};',
            "(J[IZLjava/lang/String;)[B",
        ),
        (
            'JavaMethod<QtJniTypes::Info[], QtJniTypes::Info, jint[]> f{"f"};',
            f"({_SERIAL}Info;[I)[{_SERIAL}Info;",
        ),
        ('JavaMethod<jobjectArray, jobject> f{"f"};', "(Ljava/lang/Object;)[Ljava/lang/Object;"),
        # Qt takes neither: a jlong is no qint64 on a 64-bit ABI, and the class is not declared.
        ('JavaMethod<jint, qint64> f{"f"};', "<unknown C++ type 'qint64'>"),
        ('JavaMethod<QtJniTypes::Other[]> f{"f"};', "<unknown C++ type 'QtJniTypes::Other'>"),
        ('JavaMethod<Info> f{"f"};', "<unknown C++ type 'Info'>"),
    ],
)
def test_derives_jni_signatures_from_cpp_method_declarations(
    declaration: str, signature: str
) -> None:
    jni_class = 'Q_DECLARE_JNI_CLASS(Info, "org/mavlink/qgroundcontrol/serial/Info")\n'
    assert _cpp_declarations(jni_class + declaration, _CPP_METHOD) == [("f", "f", signature)]


_CPP_FIELDS = textwrap.dedent("""\
    template <typename T>
    struct JavaField : JavaBinding<jfieldID>
    {
        static constexpr auto kSignature = QtJniTypes::fieldSignature<T>();
    };

    struct JniPortInfoFields
    {
        JavaField<jstring> name{"name"};
        // JavaField<jint> removed{"removed"};
        JavaField<jstring>
            manufacturerName{"manufacturerName"};
        JavaField<jint> vendorId{"vendorId"};
    };

    static quint16 usbIdField(JNIEnv* env, jobject object, const JavaField<jint>& field);
    """)
_JAVA_FIELDS = textwrap.dedent("""\
    package org.mavlink.qgroundcontrol.serial;

    public final class QGCUsbSerialPortInfo {
        /** C++ reads it, unlike {@code public final int stale;}. */
        public final String name;
        public final String manufacturerName;
        public final int vendorId;
        public final int[] unread;

        QGCUsbSerialPortInfo(final String name) {}
    }
    """)


def test_agreeing_sources_have_no_field_problems() -> None:
    assert _cpp_declarations(_CPP_FIELDS, _CPP_FIELD) == [
        ("name", "name", "Ljava/lang/String;"),
        ("manufacturerName", "manufacturerName", "Ljava/lang/String;"),
        ("vendorId", "vendorId", "I"),
    ]
    assert _field_problems(_CPP_FIELDS, _JAVA_FIELDS) == []


@pytest.mark.parametrize(
    ("cpp", "java", "problem"),
    [
        pytest.param(
            _CPP_FIELDS,
            _JAVA_FIELDS.replace("String name;", "String portName;"),
            "name: C++ reads Ljava/lang/String;, but Java declares no public final field of that "
            "name",
            id="missing-java-field",
        ),
        pytest.param(
            _CPP_FIELDS,
            _JAVA_FIELDS.replace("public final int vendorId;", "public final long vendorId;"),
            "vendorId: C++ reads I, but Java declares public final J",
            id="mismatched-type",
        ),
        pytest.param(
            _CPP_FIELDS,
            _JAVA_FIELDS.replace(
                "public final int vendorId;", "public static final int vendorId = 0;"
            ),
            "vendorId: C++ reads I, but Java declares no public final field of that name",
            id="static-java-field",
        ),
        pytest.param(
            _CPP_FIELDS,
            _JAVA_FIELDS.replace("public final int vendorId;", "public final uint vendorId;"),
            "vendorId: C++ reads I, but Java declares public final <unknown Java type 'uint'>",
            id="unknown-java-type",
        ),
        pytest.param(
            _CPP_FIELDS.replace("JavaField<jint> vendorId", "JavaField<jlong> vendorId"),
            _JAVA_FIELDS,
            "vendorId: C++ reads J, but Java declares public final I",
            id="mismatched-cpp-type",
        ),
        pytest.param(
            _CPP_FIELDS.replace("JavaField<jint> vendorId", "JavaField<quint16> vendorId"),
            _JAVA_FIELDS,
            "vendorId: C++ reads <unknown C++ type 'quint16'>, but Java declares public final I",
            id="unknown-cpp-type",
        ),
        pytest.param(
            _CPP_FIELDS.replace('name{"name"}', 'stableName{"name"}'),
            _JAVA_FIELDS,
            "stableName: C++ declares it for the Java field name, so what uses stableName gets "
            "name",
            id="declaration-named-for-another-field",
        ),
        pytest.param(
            _CPP_FIELDS + 'env->GetFieldID(portInfoClass, "vendorId", "I");\n',
            _JAVA_FIELDS,
            "vendorId: C++ reads the JNI type I written by hand, so nothing checks it against the "
            "type its value is read as",
            id="handwritten-field-type",
        ),
        pytest.param(
            "static void cachePortInfoFieldIds(QJniEnvironment& env) {}",
            _JAVA_FIELDS,
            "C++ declares no Java field with its JNI type",
            id="no-cpp-fields",
        ),
    ],
)
def test_reports_each_field_disagreement(cpp: str, java: str, problem: str) -> None:
    assert _field_problems(cpp, java) == [problem]


_CPP_RESULTS = textwrap.dedent("""\
    namespace AndroidSerial {
    enum class FlowControl
    {
        None = 0,
        RtsCts
    };

    /// How an operation on a port went.
    enum class Result : int
    {
        Ok = 0,
        Failed = -1,    ///< The call could not be made.
        IoError = -2,   ///< The device did not carry it out.
        // Removed = -3,
        NotOpen = -4    ///< No port is open under this handle.
    };

    enum class ResourceErrorReason : int
    {
        DeviceDetached = 1,
        ReadFailed = 2,
    };
    }  // namespace AndroidSerial
    """)
_JAVA_RESULTS = textwrap.dedent("""\
    package org.mavlink.qgroundcontrol.serial;

    final class QGCUsbSerialResult {
        static final int OK = 0;
        /** It failed for a reason not listed here, unlike {@code static final int STALE = -5;}. */
        static final int FAILED = -1;
        static final int IO_ERROR = -2;
        static final int NOT_OPEN = -4;

        private QGCUsbSerialResult() {}
    }
    """)
_JAVA_REASONS = textwrap.dedent("""\
    package org.mavlink.qgroundcontrol.serial;

    final class QGCUsbSerialResourceError {
        @Retention(RetentionPolicy.SOURCE)
        @IntDef({DEVICE_DETACHED, READ_FAILED})
        @interface Reason {}

        static final int DEVICE_DETACHED = 1;
        static final int READ_FAILED = 2;

        private QGCUsbSerialResourceError() {}
    }
    """)


def test_agreeing_sources_have_no_result_code_problems() -> None:
    assert _cpp_enumerators(_CPP_RESULTS, "Result") == {
        "Ok": 0,
        "Failed": -1,
        "IoError": -2,
        "NotOpen": -4,
    }
    assert _java_int_constants(_JAVA_RESULTS) == {
        "OK": 0,
        "FAILED": -1,
        "IO_ERROR": -2,
        "NOT_OPEN": -4,
    }
    assert _shared_value_problems(_CPP_RESULTS, "Result", _JAVA_RESULTS) == []


def test_compares_each_enum_class_with_its_own_java_constants() -> None:
    assert _cpp_enumerators(_CPP_RESULTS, "ResourceErrorReason") == {
        "DeviceDetached": 1,
        "ReadFailed": 2,
    }
    assert _java_int_constants(_JAVA_REASONS) == {"DEVICE_DETACHED": 1, "READ_FAILED": 2}
    assert _shared_value_problems(_CPP_RESULTS, "ResourceErrorReason", _JAVA_REASONS) == []
    assert _shared_value_problems(
        _CPP_RESULTS, "ResourceErrorReason", _JAVA_REASONS.replace("FAILED = 2", "FAILED = 3")
    ) == ["ReadFailed: C++ has 2, but Java has READ_FAILED = 3"]


@pytest.mark.parametrize(
    ("cpp", "java", "problem"),
    [
        pytest.param(
            _CPP_RESULTS,
            _JAVA_RESULTS.replace("IO_ERROR = -2", "IO_ERROR = -3"),
            "IoError: C++ has -2, but Java has IO_ERROR = -3",
            id="mismatched-value",
        ),
        pytest.param(
            _CPP_RESULTS,
            _JAVA_RESULTS.replace("    static final int NOT_OPEN = -4;\n", ""),
            "NotOpen: C++ has -4, but Java declares no constant of that name",
            id="missing-java-constant",
        ),
        pytest.param(
            _CPP_RESULTS.replace("    IoError = -2,", ""),
            _JAVA_RESULTS,
            "IO_ERROR: Java has -2, but C++ declares no IoError",
            id="missing-cpp-enumerator",
        ),
        pytest.param(
            # C++ gives it the value after that of Failed.
            _CPP_RESULTS.replace("IoError = -2,", "IoError,"),
            _JAVA_RESULTS,
            "IoError: C++ has 0, but Java has IO_ERROR = -2",
            id="implicit-cpp-value",
        ),
        pytest.param(
            _CPP_RESULTS.replace("IoError = -2,", "IoError = Failed - 1,"),
            _JAVA_RESULTS,
            "IoError: this test cannot read its value in C++",
            id="unreadable-cpp-value",
        ),
        pytest.param(
            _CPP_RESULTS,
            _JAVA_RESULTS.replace("IO_ERROR = -2", "IO_ERROR = FAILED - 1"),
            "IO_ERROR: this test cannot read its value in Java",
            id="unreadable-java-value",
        ),
        pytest.param(
            _CPP_RESULTS.replace("enum class Result", "enum class Outcome"),
            _JAVA_RESULTS,
            "C++ declares no enumerator of enum class Result",
            id="no-cpp-enumerators",
        ),
        pytest.param(
            _CPP_RESULTS,
            _JAVA_RESULTS.replace("static final int", "static final long"),
            "Java declares no int constant",
            id="no-java-constants",
        ),
    ],
)
def test_reports_each_result_code_disagreement(cpp: str, java: str, problem: str) -> None:
    assert _shared_value_problems(cpp, "Result", java) == [problem]


_CPP_RESULT_RANGE = textwrap.dedent("""\
    template <typename... Args>
    static Result callJavaCommand(const JavaMethod<jint, Args...>& method, Args... args)
    {
        const jint result = callJava(env, method, static_cast<jint>(Result::Failed), args...);
        if (result > static_cast<jint>(Result::Ok) || result < static_cast<jint>(Result::NotOpen)) {
            // Not result < static_cast<jint>(Result::IoError), which would take NotOpen for no result.
            return Result::Failed;
        }

        return static_cast<Result>(result);
    }
    """)


def test_range_of_every_result_code_has_no_problems() -> None:
    assert _result_range_problems(_CPP_RESULT_RANGE, _CPP_RESULTS) == []


@pytest.mark.parametrize(
    ("cpp", "problem"),
    [
        pytest.param(
            _CPP_RESULT_RANGE.replace("(Result::NotOpen))", "(Result::IoError))"),
            "NotOpen: C++ takes -4 from Java for no result, only -2 to 0",
            id="result-below-the-range",
        ),
        pytest.param(
            _CPP_RESULT_RANGE.replace("result <", "code <"),
            "C++ checks what Java returns against no range of results this test can read",
            id="no-range",
        ),
    ],
)
def test_reports_each_result_range_problem(cpp: str, problem: str) -> None:
    assert _result_range_problems(cpp, _CPP_RESULTS) == [problem]


_CPP_READ = textwrap.dedent("""\
    static void jniDeviceNewData(JNIEnv* env, jobject, jlong token, jbyteArray data)
    {
        // constexpr jsize kMaxNativePayloadBytes = 1024;
        constexpr jsize kMaxNativePayloadBytes = 16 * 1024;

        const jsize len = env->GetArrayLength(data);
        const jsize cappedLen = (len > kMaxNativePayloadBytes) ? kMaxNativePayloadBytes : len;
    }
    """)
_JAVA_READ = textwrap.dedent("""\
    final class QGCUsbSerialReader {
        // Not READ_BUF_SIZE = 32768, which exceeds kMaxNativePayloadBytes in AndroidSerialJni.cc.
        private static final int READ_BUF_SIZE = 2048;

        void start() {
            driverPort.setReadQueue(2, READ_BUF_SIZE);
        }
    }
    """)


def test_read_buffer_up_to_the_native_payload_has_no_problems() -> None:
    assert _read_buffer_problems(_CPP_READ, _JAVA_READ) == []
    assert _read_buffer_problems(_CPP_READ, _JAVA_READ.replace("= 2048", "= 16384")) == []


@pytest.mark.parametrize(
    ("cpp", "java", "problem"),
    [
        pytest.param(
            _CPP_READ,
            _JAVA_READ.replace("= 2048", "= 16385"),
            "Java passes reads of up to READ_BUF_SIZE = 16385 bytes to C++, which truncates them "
            "to kMaxNativePayloadBytes = 16384",
            id="buffer-over-payload",
        ),
        pytest.param(
            _CPP_READ.replace("kMaxNativePayloadBytes", "kMaxPayloadBytes"),
            _JAVA_READ,
            "C++ declares no kMaxNativePayloadBytes with a value this test can read",
            id="missing-cpp-constant",
        ),
        pytest.param(
            _CPP_READ,
            _JAVA_READ.replace("= 2048", "= 2 * KIBIBYTE"),
            "Java declares no READ_BUF_SIZE with a value this test can read",
            id="unreadable-java-value",
        ),
    ],
)
def test_reports_each_read_buffer_problem(cpp: str, java: str, problem: str) -> None:
    assert _read_buffer_problems(cpp, java) == [problem]


_CPP_FLOW_CONTROL = textwrap.dedent("""\
    /// The flow control a USB serial driver may have.
    enum class FlowControl
    {
        None = 0,
        RtsCts,
        // DtrDsr,
        XonXoff,
    };
    """)
_JAVA_CODES = textwrap.dedent("""\
    public final class QGCUsbSerialManager {
        // Not `case NONE -> 1;`, which C++ takes for RTS_CTS.
        private static int flowControlCode(final UsbSerialPort.FlowControl flowControl) {
            return switch (flowControl) {
                case NONE -> 0;
                case RTS_CTS -> 1;
                case XON_XOFF -> 2;
            };
        }

        private static int controlLineCode(final UsbSerialPort.ControlLine controlLine) {
            return switch (controlLine) {
                case RTS -> 0;
                case CTS -> 1;
                case DTR -> 2;
            };
        }

        public static int setFlowControl(final long handle, final int flowControl) {
            for (final UsbSerialPort.FlowControl target : UsbSerialPort.FlowControl.values()) {
                if (flowControlCode(target) == flowControl) {
                    return command(handle, "set flow control", port -> port.setFlowControl(target));
                }
            }
            return QGCUsbSerialResult.UNSUPPORTED;
        }

        public static int[] getControlLines(final long handle) {
            return query(handle, "get control lines", new int[0],
                    port -> port.controlLines().stream().mapToInt(
                            QGCUsbSerialManager::controlLineCode).toArray());
        }
    }
    """)


def test_agreeing_sources_have_no_flow_control_problems() -> None:
    assert _cpp_enumerators(_CPP_FLOW_CONTROL, "FlowControl") == {
        "None": 0,
        "RtsCts": 1,
        "XonXoff": 2,
    }
    assert _cpp_enumerators("enum Parity { NoParity, EvenParity = 2, OddParity };", "Parity") == {
        "NoParity": 0,
        "EvenParity": 2,
        "OddParity": 3,
    }
    assert _java_switch_codes(_JAVA_CODES, "flowControlCode") == {
        "NONE": 0,
        "RTS_CTS": 1,
        "XON_XOFF": 2,
    }
    assert _flow_control_problems(_CPP_FLOW_CONTROL, _JAVA_CODES) == []


@pytest.mark.parametrize(
    ("cpp", "java", "problems"),
    [
        pytest.param(
            # C++ numbers the enumerators after it one higher.
            _CPP_FLOW_CONTROL.replace("// DtrDsr,", "DtrDsr,"),
            _JAVA_CODES,
            [
                "DtrDsr: C++ has 2, but Java declares no constant of that name",
                "XonXoff: C++ has 3, but Java has XON_XOFF = 2",
            ],
            id="cpp-enumerator-before-others",
        ),
        pytest.param(
            _CPP_FLOW_CONTROL,
            _JAVA_CODES.replace("case RTS_CTS -> 1;", "case RTS_CTS -> 4;"),
            ["RtsCts: C++ has 1, but Java has RTS_CTS = 4"],
            id="mismatched-java-code",
        ),
        pytest.param(
            _CPP_FLOW_CONTROL,
            _JAVA_CODES.replace("case RTS_CTS -> 1;", "case RTS_CTS -> flowControl.ordinal();"),
            ["RTS_CTS: this test cannot read its value in Java"],
            id="unreadable-java-code",
        ),
        pytest.param(
            _CPP_FLOW_CONTROL,
            _JAVA_CODES.replace("case XON_XOFF -> 2;", "default -> 2;"),
            ["Java declares no flowControlCode() with a switch this test can read"],
            id="unreadable-java-case",
        ),
        pytest.param(
            _CPP_FLOW_CONTROL,
            _JAVA_CODES.replace("int flowControlCode(", "int flowControlNumber("),
            ["Java declares no flowControlCode() with a switch this test can read"],
            id="no-java-switch",
        ),
        pytest.param(
            _CPP_FLOW_CONTROL.replace("enum class FlowControl", "enum class Flow"),
            _JAVA_CODES,
            ["C++ declares no enumerator of enum class FlowControl"],
            id="no-cpp-enumerators",
        ),
    ],
)
def test_reports_each_flow_control_problem(cpp: str, java: str, problems: list[str]) -> None:
    assert _flow_control_problems(cpp, java) == problems


_CPP_CONTROL_LINES = textwrap.dedent("""\
    static QSerialPort::PinoutSignals callJavaControlLines(PortHandle handle)
    {
        // Not kSignals[] = {QSerialPort::ClearToSendSignal}, which Java numbers 1.
        static constexpr QSerialPort::PinoutSignal kSignals[] = {
            QSerialPort::RequestToSendSignal, QSerialPort::ClearToSendSignal,
            QSerialPort::DataTerminalReadySignal,
        };

        QSerialPort::PinoutSignals data = QSerialPort::PinoutSignals();
        for (const int line : callJavaIntArray(handle)) {
            if (line < 0 || line >= static_cast<int>(std::size(kSignals))) {
                continue;
            }
            data |= kSignals[line];
        }

        return data;
    }
    """)


def test_agreeing_sources_have_no_control_line_problems() -> None:
    assert _cpp_array(_CPP_CONTROL_LINES, "kSignals") == [
        "RequestToSendSignal",
        "ClearToSendSignal",
        "DataTerminalReadySignal",
    ]
    assert _java_switch_codes(_JAVA_CODES, "controlLineCode") == {"RTS": 0, "CTS": 1, "DTR": 2}
    assert _control_line_problems(_CPP_CONTROL_LINES, _JAVA_CODES) == []


@pytest.mark.parametrize(
    ("cpp", "java", "problems"),
    [
        pytest.param(
            _CPP_CONTROL_LINES.replace(
                "QSerialPort::RequestToSendSignal, QSerialPort::ClearToSendSignal,",
                "QSerialPort::ClearToSendSignal, QSerialPort::RequestToSendSignal,",
            ),
            _JAVA_CODES,
            [
                "ClearToSendSignal: C++ has 0, but Java has CTS = 1",
                "RequestToSendSignal: C++ has 1, but Java has RTS = 0",
            ],
            id="cpp-order",
        ),
        pytest.param(
            _CPP_CONTROL_LINES,
            _JAVA_CODES.replace("case DTR -> 2;", "case DTR -> 3;"),
            ["DataTerminalReadySignal: C++ has 2, but Java has DTR = 3"],
            id="mismatched-java-code",
        ),
        pytest.param(
            _CPP_CONTROL_LINES.replace("        QSerialPort::DataTerminalReadySignal,\n", ""),
            _JAVA_CODES,
            ["DTR: Java has 2, but C++ declares no DataTerminalReadySignal"],
            id="missing-cpp-signal",
        ),
        pytest.param(
            _CPP_CONTROL_LINES,
            _JAVA_CODES.replace("case DTR -> 2;", "case BREAK -> 2;"),
            [
                "DataTerminalReadySignal: C++ has 2, but Java declares no DTR",
                "BREAK: Java has 2, but C++ declares no name for it",
            ],
            id="line-without-a-signal",
        ),
        pytest.param(
            _CPP_CONTROL_LINES.replace("kSignals[] = {\n", "kSignals[3] = {\n"),
            _JAVA_CODES,
            ["C++ declares no kSignals with elements this test can read"],
            id="no-cpp-array",
        ),
    ],
)
def test_reports_each_control_line_problem(cpp: str, java: str, problems: list[str]) -> None:
    assert _control_line_problems(cpp, java) == problems


_CPP_PORT_INFO = textwrap.dedent("""\
    namespace {
    // Not kStableUsbNamePrefix("serial:"), which no name that Java builds starts with.
    constexpr QLatin1String kStableUsbNamePrefix("usb:");
    }  // namespace

    bool QSerialPortInfo::isUsbLocation(const QString& systemLocation)
    {
        return systemLocation.startsWith(QLatin1String("/dev/bus/usb/")) ||
               systemLocation.startsWith(kStableUsbNamePrefix);
    }
    """)
_JAVA_PORT_NAME = textwrap.dedent("""\
    record QGCUsbSerialPortName(String deviceName, int portIndex) {
        private static final String PORT_SUFFIX = "#p";
        private static final String STABLE_NAME_PREFIX = "usb:";

        static String stableDeviceName(final UsbDevice device, final String serialNumber) {
            return String.format(Locale.ROOT, "%s%04x:%04x:%s", STABLE_NAME_PREFIX,
                    device.getVendorId(), device.getProductId(), serialNumber);
        }

        boolean isStable() {
            return deviceName.startsWith(STABLE_NAME_PREFIX);
        }
    }
    """)


def test_agreeing_sources_have_no_stable_name_prefix_problems() -> None:
    assert _string_constant(_CPP_PORT_INFO, "kStableUsbNamePrefix") == "usb:"
    assert _string_constant(_JAVA_PORT_NAME, "STABLE_NAME_PREFIX") == "usb:"
    assert _stable_name_prefix_problems(_CPP_PORT_INFO, _JAVA_PORT_NAME) == []


@pytest.mark.parametrize(
    ("cpp", "java", "problem"),
    [
        pytest.param(
            _CPP_PORT_INFO,
            _JAVA_PORT_NAME.replace('STABLE_NAME_PREFIX = "usb:"', 'STABLE_NAME_PREFIX = "USB:"'),
            'Java starts a stable port name with STABLE_NAME_PREFIX = "USB:", but C++ takes a name '
            'for one that starts with kStableUsbNamePrefix = "usb:"',
            id="mismatched-prefix",
        ),
        pytest.param(
            _CPP_PORT_INFO.replace('kStableUsbNamePrefix("usb:")', 'kStablePrefix("usb:")'),
            _JAVA_PORT_NAME,
            "C++ declares no kStableUsbNamePrefix with a text this test can read",
            id="missing-cpp-constant",
        ),
        pytest.param(
            _CPP_PORT_INFO,
            _JAVA_PORT_NAME.replace(
                'STABLE_NAME_PREFIX = "usb:"', 'STABLE_NAME_PREFIX = "usb" + SEP'
            ),
            "Java declares no STABLE_NAME_PREFIX with a text this test can read",
            id="unreadable-java-text",
        ),
    ],
)
def test_reports_each_stable_name_prefix_problem(cpp: str, java: str, problem: str) -> None:
    assert _stable_name_prefix_problems(cpp, java) == [problem]


_CPP_PARAMETERS = textwrap.dedent("""\
    static std::optional<jint> toJavaDataBits(QSerialPort::DataBits dataBits)
    {
        switch (dataBits) {
            case QSerialPort::Data7:
            case QSerialPort::Data8:
                return static_cast<jint>(dataBits);
        }

        return std::nullopt;
    }

    static std::optional<jint> toJavaStopBits(QSerialPort::StopBits stopBits)
    {
        switch (stopBits) {
            case QSerialPort::OneStop:
                return 1;
            // case QSerialPort::OneAndHalfStop: return 3;
            case QSerialPort::TwoStop:
                return 2;
        }

        return std::nullopt;
    }

    static std::optional<jint> toJavaParity(QSerialPort::Parity parity)
    {
        switch (parity) {
            case QSerialPort::NoParity:
                return 0;
            case QSerialPort::OddParity:
                return 1;
        }

        return std::nullopt;
    }

    Result setParameters(QSerialPort::DataBits dataBits, QSerialPort::Parity parity)
    {
        const std::optional<jint> javaDataBits = toJavaDataBits(dataBits);
        const std::optional<jint> javaParity = toJavaParity(parity);
    }
    """)
# QSerialPort numbers odd parity 3, which is not what C++ sends for it.
_CPP_PORT = textwrap.dedent("""\
    class QSerialPort : public QIODevice
    {
    public:
        enum DataBits
        {
            Data7 = 7,
            Data8 = 8
        };
        Q_ENUM(DataBits)

        enum Parity
        {
            NoParity = 0,
            OddParity = 3
        };
        Q_ENUM(Parity)
    };
    """)
_JAVA_LIBRARY_TEST = textwrap.dedent("""\
    public class QGCUsbSerialLibraryContractTest {
        @Test
        public void dataBitsStopBitsAndParity_haveTheNumbersThatCppSends() {
            assertEquals(7, UsbSerialPort.DATABITS_7);
            assertEquals(8, UsbSerialPort.DATABITS_8);
            assertEquals(1, UsbSerialPort.STOPBITS_1);
            // assertEquals(3, UsbSerialPort.STOPBITS_1_5);
            assertEquals(2, UsbSerialPort.STOPBITS_2);
            assertEquals(0, UsbSerialPort.PARITY_NONE);
            assertEquals(1, UsbSerialPort.PARITY_ODD);
            assertEquals(UsbSerialPort.FlowControl.NONE, port.getFlowControl());
        }
    }
    """)


def test_agreeing_sources_have_no_serial_parameter_problems() -> None:
    assert _cpp_switch_values(_CPP_PARAMETERS, "toJavaDataBits", _CPP_PORT) == {
        "Data7": 7,
        "Data8": 8,
    }
    assert _cpp_switch_values(_CPP_PARAMETERS, "toJavaStopBits", _CPP_PORT) == {
        "OneStop": 1,
        "TwoStop": 2,
    }
    assert _cpp_switch_values(_CPP_PARAMETERS, "toJavaParity", _CPP_PORT) == {
        "NoParity": 0,
        "OddParity": 1,
    }
    assert _java_library_constants(_JAVA_LIBRARY_TEST) == {
        "DATABITS_7": 7,
        "DATABITS_8": 8,
        "STOPBITS_1": 1,
        "STOPBITS_2": 2,
        "PARITY_NONE": 0,
        "PARITY_ODD": 1,
    }
    assert _serial_parameter_problems(_CPP_PARAMETERS, _CPP_PORT, _JAVA_LIBRARY_TEST) == []


@pytest.mark.parametrize(
    ("cpp", "port", "java", "problem"),
    [
        pytest.param(
            _CPP_PARAMETERS.replace("return 2;", "return 3;"),
            _CPP_PORT,
            _JAVA_LIBRARY_TEST,
            "TwoStop: C++ has 3, but Java has STOPBITS_2 = 2",
            id="mismatched-cpp-number",
        ),
        pytest.param(
            _CPP_PARAMETERS,
            _CPP_PORT.replace("Data7 = 7,", "Data7 = 9,"),
            _JAVA_LIBRARY_TEST,
            "Data7: C++ has 9, but Java has DATABITS_7 = 7",
            id="mismatched-enumerator-that-cpp-sends-as-it-is",
        ),
        pytest.param(
            _CPP_PARAMETERS,
            _CPP_PORT,
            _JAVA_LIBRARY_TEST.replace(
                "(1, UsbSerialPort.PARITY_ODD)", "(3, UsbSerialPort.PARITY_ODD)"
            ),
            "OddParity: C++ has 1, but Java has PARITY_ODD = 3",
            id="mismatched-java-number",
        ),
        pytest.param(
            _CPP_PARAMETERS,
            _CPP_PORT,
            _JAVA_LIBRARY_TEST.replace("        assertEquals(2, UsbSerialPort.STOPBITS_2);\n", ""),
            "TwoStop: C++ has 2, but Java declares no STOPBITS_2",
            id="missing-java-constant",
        ),
        pytest.param(
            _CPP_PARAMETERS.replace("return 2;", "return kTwoStopBits;"),
            _CPP_PORT,
            _JAVA_LIBRARY_TEST,
            "TwoStop: this test cannot read its value in C++",
            id="unreadable-cpp-number",
        ),
        pytest.param(
            _CPP_PARAMETERS,
            _CPP_PORT.replace("Data7 = 7,\n        Data8 = 8\n", "Data7 = 7\n"),
            _JAVA_LIBRARY_TEST,
            "Data8: this test cannot read its value in C++",
            id="no-enumerator-for-what-cpp-sends-as-it-is",
        ),
        pytest.param(
            _CPP_PARAMETERS.replace("case QSerialPort::TwoStop:", "default:"),
            _CPP_PORT,
            _JAVA_LIBRARY_TEST,
            "C++ declares no toJavaStopBits() with a switch this test can read",
            id="unreadable-cpp-label",
        ),
        pytest.param(
            _CPP_PARAMETERS.replace(
                "toJavaParity(QSerialPort::Parity parity)", "toParity(int parity)"
            ),
            _CPP_PORT,
            _JAVA_LIBRARY_TEST,
            "C++ declares no toJavaParity() with a switch this test can read",
            id="no-cpp-switch",
        ),
        pytest.param(
            _CPP_PARAMETERS,
            _CPP_PORT,
            _JAVA_LIBRARY_TEST.replace("UsbSerialPort.", "CdcAcmSerialPort."),
            "Java states no number of UsbSerialPort for data bits, stop bits or parity",
            id="no-java-constants",
        ),
    ],
)
def test_reports_each_serial_parameter_problem(
    cpp: str, port: str, java: str, problem: str
) -> None:
    assert _serial_parameter_problems(cpp, port, java) == [problem]
