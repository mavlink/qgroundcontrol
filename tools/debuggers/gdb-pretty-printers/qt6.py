"""
GDB Pretty Printers for Qt 6 Types

This module provides human-readable display of Qt types in GDB.

Usage in GDB:
    source /path/to/qgroundcontrol/tools/debuggers/gdb-pretty-printers/qt6.py

Usage in .gdbinit:
    python
    import sys
    sys.path.insert(0, '/path/to/qgroundcontrol/tools/debuggers/gdb-pretty-printers')
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
        size = int(d["size"])
        data_ptr = d["ptr"]
        if size == 0 or not data_ptr:
            return ""
        mem = gdb.selected_inferior().read_memory(int(data_ptr), size * 2)
        return mem.tobytes().decode("utf-16-le", errors="replace")
    except Exception as e:
        return f"<QString error: {e}>"


def _qbytearray_to_str(val):
    """Convert a QByteArray to a Python string."""
    try:
        d = val["d"]
        size = int(d["size"])
        data_ptr = d["ptr"]
        if size == 0 or not data_ptr:
            return ""
        mem = gdb.selected_inferior().read_memory(int(data_ptr), size)
        return mem.tobytes().decode("utf-8", errors="replace")
    except Exception as e:
        return f"<QByteArray error: {e}>"


def _number(val):
    """Unwrap the QCheckedInt that Qt 6.12's integer QPoint/QSize/QRect members use."""
    try:
        return val["m_i"]
    except gdb.error:
        return val


def _raw_pointer(val):
    """Unwrap Qt::totally_ordered_wrapper, which holds shared-data pointers since Qt 6.8."""
    if val.type.strip_typedefs().code == gdb.TYPE_CODE_PTR:
        return val
    return val["ptr"]


def _atomic_int(val):
    """Read a QBasicAtomicInt; its std::atomic<int> member has the layout of a plain int."""
    return int(val["_q_value"].address.cast(gdb.lookup_type("int").pointer()).dereference())


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
    """Pretty printer for QMap<K, V>, shown through its std::map."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            data = _raw_pointer(self.val["d"]["d"])
            if not data:
                return "QMap (empty)"
            return data.dereference()["m"]
        except Exception:
            return "QMap"


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
    """Pretty printer for QVariant: the metatype name, plus the value when GDB knows that type."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            d = self.val["d"]
            interface = int(d["packedType"]) << 2
            if interface == 0:
                return "QVariant(Invalid)"
            interface_type = gdb.lookup_type("QtPrivate::QMetaTypeInterface").pointer()
            name = gdb.Value(interface).cast(interface_type).dereference()["name"].string()
        except Exception:
            return "QVariant"
        try:
            if int(d["is_shared"]):
                shared = d["data"]["shared"]
                address = int(shared) + int(shared["offset"])
            else:
                address = int(d["data"]["data"].address)
            value = gdb.Value(address).cast(gdb.lookup_type(name).pointer()).dereference()
            return f"QVariant({name}) = {value.format_string()}"
        except Exception:
            return f"QVariant({name})"


class QPointPrinter:
    """Pretty printer for QPoint/QPointF."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            x = _number(self.val["xp"])
            y = _number(self.val["yp"])
            return f"({x}, {y})"
        except Exception:
            return "QPoint"


class QSizePrinter:
    """Pretty printer for QSize/QSizeF."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            w = _number(self.val["wd"])
            h = _number(self.val["ht"])
            return f"{w}x{h}"
        except Exception:
            return "QSize"


class QRectPrinter:
    """Pretty printer for QRect/QRectF."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            x1 = _number(self.val["x1"])
            y1 = _number(self.val["y1"])
            x2 = _number(self.val["x2"])
            y2 = _number(self.val["y2"])
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
    """Pretty printer for QUrl (scheme, host and path)."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            d = self.val["d"]
            if d == 0:
                return "QUrl(empty)"

            scheme = _qstring_to_str(d["scheme"])
            host = _qstring_to_str(d["host"])
            path = _qstring_to_str(d["path"])
            url = f"{scheme}:" if scheme else ""
            if host:
                url += f"//{host}"
            return f"QUrl({url}{path})"
        except Exception:
            return "QUrl"


class QSharedPointerPrinter:
    """Pretty printer for QSharedPointer<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            d = self.val["d"]
            value = _raw_pointer(self.val["value"])

            if value == 0:
                return "QSharedPointer(nullptr)"

            if d != 0:
                strong = _atomic_int(d["strongref"])
                weak = _atomic_int(d["weakref"])
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
