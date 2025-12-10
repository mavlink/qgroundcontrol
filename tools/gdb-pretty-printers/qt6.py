"""
GDB Pretty Printers for Qt 6 Types

This module provides human-readable display of Qt types in GDB.

Usage in GDB:
    source /path/to/qgroundcontrol/tools/gdb-pretty-printers/qt6.py

Usage in .gdbinit:
    python
    import sys
    sys.path.insert(0, '/path/to/qgroundcontrol/tools/gdb-pretty-printers')
    from qt6 import register_qt_printers
    register_qt_printers(None)
    end

Based on KDE's Qt pretty printers, adapted for Qt 6.
"""

import gdb


def _qstring_to_str(val):
    """Convert a QString to a Python string."""
    try:
        d = val["d"]
        if d == 0:
            return '""'

        # Qt 6 QString internal structure
        size = int(d["size"])
        if size == 0:
            return '""'

        # Data is stored inline after QStringData header in Qt 6
        data_ptr = d["ptr"]
        if data_ptr == 0:
            return '""'

        # Read UTF-16 data
        inferior = gdb.selected_inferior()
        mem = inferior.read_memory(int(data_ptr), size * 2)
        return '"' + mem.tobytes().decode("utf-16-le", errors="replace") + '"'
    except Exception as e:
        return f"<QString error: {e}>"


def _qbytearray_to_str(val):
    """Convert a QByteArray to a Python string."""
    try:
        d = val["d"]
        if d == 0:
            return '""'

        size = int(d["size"])
        if size == 0:
            return '""'

        data_ptr = d["ptr"]
        if data_ptr == 0:
            return '""'

        inferior = gdb.selected_inferior()
        mem = inferior.read_memory(int(data_ptr), size)
        return '"' + mem.tobytes().decode("utf-8", errors="replace") + '"'
    except Exception as e:
        return f"<QByteArray error: {e}>"


class QStringPrinter:
    """Pretty printer for QString."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return _qstring_to_str(self.val)

    def display_hint(self):
        return "string"


class QByteArrayPrinter:
    """Pretty printer for QByteArray."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return _qbytearray_to_str(self.val)

    def display_hint(self):
        return "string"


class QListPrinter:
    """Pretty printer for QList<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            d = self.val["d"]
            size = int(d["size"])
            return f"QList<...> (size={size})"
        except Exception:
            return "QList<...>"

    def children(self):
        try:
            d = self.val["d"]
            size = int(d["size"])
            ptr = d["ptr"]

            # Get the element type
            val_type = self.val.type.template_argument(0)

            for i in range(min(size, 100)):  # Limit to 100 elements
                element = (ptr + i).dereference().cast(val_type)
                yield f"[{i}]", element
        except Exception:
            pass

    def display_hint(self):
        return "array"


class QVectorPrinter(QListPrinter):
    """Pretty printer for QVector<T> (alias of QList in Qt 6)."""

    pass


class QMapPrinter:
    """Pretty printer for QMap<K, V>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            d = self.val["d"]
            if d == 0:
                return "QMap (empty)"
            size = int(d["size"])
            return f"QMap (size={size})"
        except Exception:
            return "QMap"

    def display_hint(self):
        return "map"


class QHashPrinter:
    """Pretty printer for QHash<K, V>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            d = self.val["d"]
            if d == 0:
                return "QHash (empty)"
            size = int(d["size"])
            return f"QHash (size={size})"
        except Exception:
            return "QHash"

    def display_hint(self):
        return "map"


class QVariantPrinter:
    """Pretty printer for QVariant."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            d = self.val["d"]
            type_id = int(d["typeId"])

            # Common type IDs in Qt 6
            type_names = {
                0: "Invalid",
                1: "Bool",
                2: "Int",
                3: "UInt",
                4: "LongLong",
                5: "ULongLong",
                6: "Double",
                7: "Char",
                10: "QString",
                11: "QStringList",
                12: "QByteArray",
                13: "QBitArray",
                14: "QDate",
                15: "QTime",
                16: "QDateTime",
                17: "QUrl",
            }

            type_name = type_names.get(type_id, f"type={type_id}")
            return f"QVariant({type_name})"
        except Exception:
            return "QVariant"


class QPointPrinter:
    """Pretty printer for QPoint/QPointF."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            x = self.val["xp"]
            y = self.val["yp"]
            return f"({x}, {y})"
        except Exception:
            return "QPoint"


class QSizePrinter:
    """Pretty printer for QSize/QSizeF."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            w = self.val["wd"]
            h = self.val["ht"]
            return f"{w}x{h}"
        except Exception:
            return "QSize"


class QRectPrinter:
    """Pretty printer for QRect/QRectF."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            x1 = self.val["x1"]
            y1 = self.val["y1"]
            x2 = self.val["x2"]
            y2 = self.val["y2"]
            return f"[({x1}, {y1}) - ({x2}, {y2})]"
        except Exception:
            try:
                x = self.val["xp"]
                y = self.val["yp"]
                w = self.val["w"]
                h = self.val["h"]
                return f"[({x}, {y}) {w}x{h}]"
            except Exception:
                return "QRect"


class QUrlPrinter:
    """Pretty printer for QUrl."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            d = self.val["d"]
            if d == 0:
                return "QUrl(empty)"

            # Try to get the full URL string
            scheme = d["scheme"]
            host = d["host"]
            path = d["path"]

            parts = []
            if scheme:
                parts.append(_qstring_to_str(scheme).strip('"'))
            if host:
                parts.append("://" + _qstring_to_str(host).strip('"'))
            if path:
                parts.append(_qstring_to_str(path).strip('"'))

            return f"QUrl({(''.join(parts))})"
        except Exception:
            return "QUrl"


class QSharedPointerPrinter:
    """Pretty printer for QSharedPointer<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            d = self.val["d"]
            value = self.val["value"]

            if value == 0:
                return "QSharedPointer(nullptr)"

            if d != 0:
                strong = int(d["strongref"]["_q_value"])
                weak = int(d["weakref"]["_q_value"])
                return f"QSharedPointer(strong={strong}, weak={weak}) = {value}"

            return f"QSharedPointer = {value}"
        except Exception:
            return "QSharedPointer"


def build_qt_printer():
    """Build and return the Qt pretty printer collection."""
    pp = gdb.printing.RegexpCollectionPrettyPrinter("Qt6")

    # Core types
    pp.add_printer("QString", "^QString$", QStringPrinter)
    pp.add_printer("QByteArray", "^QByteArray$", QByteArrayPrinter)
    pp.add_printer("QVariant", "^QVariant$", QVariantPrinter)
    pp.add_printer("QUrl", "^QUrl$", QUrlPrinter)

    # Containers
    pp.add_printer("QList", "^QList<.*>$", QListPrinter)
    pp.add_printer("QVector", "^QVector<.*>$", QVectorPrinter)
    pp.add_printer("QMap", "^QMap<.*>$", QMapPrinter)
    pp.add_printer("QHash", "^QHash<.*>$", QHashPrinter)

    # Geometry
    pp.add_printer("QPoint", "^QPoint$", QPointPrinter)
    pp.add_printer("QPointF", "^QPointF$", QPointPrinter)
    pp.add_printer("QSize", "^QSize$", QSizePrinter)
    pp.add_printer("QSizeF", "^QSizeF$", QSizePrinter)
    pp.add_printer("QRect", "^QRect$", QRectPrinter)
    pp.add_printer("QRectF", "^QRectF$", QRectPrinter)

    # Smart pointers
    pp.add_printer("QSharedPointer", "^QSharedPointer<.*>$", QSharedPointerPrinter)

    return pp


def register_qt_printers(obj):
    """Register Qt pretty printers with GDB."""
    gdb.printing.register_pretty_printer(obj, build_qt_printer(), replace=True)


# Auto-register when loaded
register_qt_printers(None)
print("Qt 6 pretty printers loaded")
