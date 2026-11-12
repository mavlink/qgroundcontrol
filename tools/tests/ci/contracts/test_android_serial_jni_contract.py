"""AndroidSerial's C++ and the Java it calls agree on the names JNI binds and the values they share.

Both sides build with a mismatch. For a method or field, C++ only logs "Java method not found",
"Java field not found" or "Failed to register native methods for" at runtime. A result code or
resource error reason that Java numbers differently reaches C++ as another one, and C++ truncates
a read that is larger than the payload it accepts.
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
    from collections.abc import Iterable

# ci-scripts.yml checks these files out and runs when one changes (test_ci_scripts_workflow.py).
CPP_SOURCE = "src/Android/qtandroidserialport/AndroidSerial.cc"
CPP_HEADER = "src/Android/qtandroidserialport/AndroidSerial.h"
JAVA_SOURCE = "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialManager.java"
READER_SOURCE = "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialReader.java"
PORT_INFO_SOURCE = "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialPortInfo.java"
RESOURCE_ERROR_SOURCE = (
    "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialResourceError.java"
)
RESULT_SOURCE = "android/src/org/mavlink/qgroundcontrol/serial/QGCUsbSerialResult.java"

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
# The values C++ shares with Java are the enumerators of an enum class, declared like
# `enum class Result : int { Ok = 0, NotOpen = -4 };`. This follows the name of the enum.
_CPP_ENUM_BODY = r"\b[^{;]*\{(?P<enumerators>[^}]*)\}"
# The value is empty where C++ numbers an enumerator implicitly.
_CPP_ENUMERATOR = re.compile(r"(?P<name>\w+)\s*(?:=(?P<value>[^,]*))?")
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
    """Value of each enumerator of a C++ enum class, None where _integer() cannot read it."""
    declaration = re.search(
        rf"\benum\s+class\s+{enum}{_CPP_ENUM_BODY}", _code(header, strings=False)
    )
    enumerators = _CPP_ENUMERATOR.findall(declaration["enumerators"]) if declaration else []
    return {name: _integer(value) for name, value in enumerators}


def _java_int_constants(source: str) -> dict[str, int | None]:
    """Value of each int constant the Java source declares, None where _integer() cannot read it."""
    constants = _JAVA_INT_CONSTANT.findall(_code(source, strings=False))
    return {name: _integer(value) for name, value in constants}


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
    problems.extend(
        f"{name}: this test cannot read its value in {language}"
        for language, codes in (("C++", cpp), ("Java", java))
        for name, value in codes.items()
        if value is None
    )
    if problems:
        return problems
    constants = {_camel_case(constant): constant for constant in java}
    for name, value in cpp.items():
        constant = constants.get(name)
        if constant is None:
            problems.append(f"{name}: C++ has {value}, but Java declares no constant of that name")
        elif java[constant] != value:
            problems.append(f"{name}: C++ has {value}, but Java has {constant} = {java[constant]}")
    problems.extend(
        f"{constant}: Java has {java[constant]}, but C++ declares no enumerator of that name"
        for name, constant in constants.items()
        if name not in cpp
    )
    return problems


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
            "IO_ERROR: Java has -2, but C++ declares no enumerator of that name",
            id="missing-cpp-enumerator",
        ),
        pytest.param(
            _CPP_RESULTS.replace("IoError = -2,", "IoError,"),
            _JAVA_RESULTS,
            "IoError: this test cannot read its value in C++",
            id="implicit-cpp-value",
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
        // Not READ_BUF_SIZE = 32768, which exceeds kMaxNativePayloadBytes in AndroidSerial.cc.
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
