# GDB Pretty Printers for Qt 6

This directory contains GDB pretty printers for displaying Qt types in a human-readable format.

## Setup

### Quick Setup (per session)

In GDB:

```gdb
source /path/to/qgroundcontrol/tools/debuggers/gdb-pretty-printers/qt6.py
```

### Permanent Setup

Add to your `~/.gdbinit`:

```gdb
python
import sys
sys.path.insert(0, '/path/to/qgroundcontrol/tools/debuggers/gdb-pretty-printers')
from qt6 import register_qt_printers
register_qt_printers(None)
end
```

### VS Code Setup

The tracked launch template (`.vscode/launch.default.json`, installed as `launch.json` by
`just vscode`) only enables GDB's built-in pretty-printing. Use the `~/.gdbinit` setup above to load
these printers in VS Code GDB sessions.

## Supported Types

- **Strings**: QString, QByteArray
- **Containers**: QList, QVector, QMap, QHash
- **Geometry**: QPoint, QPointF, QSize, QSizeF, QRect, QRectF
- **Other**: QVariant, QUrl, QSharedPointer

## Example Output

```text
(gdb) print myString
$1 = "Hello, World!"

(gdb) print myList
$2 = QList<...> (size=3) = {1, 2, 3}

(gdb) print myPoint
$3 = (100, 200)

(gdb) print myRect
$4 = [(0, 0) - (799, 599)]

(gdb) print myVariant
$5 = QVariant(int) = 42

(gdb) print myMap
$6 = std::map with 2 elements = {[1] = 2, [3] = 4}
```

QMap prints through GDB's libstdc++ `std::map` printer. QVariant prints the stored value when GDB
has debug info for its type, and only the type name otherwise; QUrl needs QtCore debug symbols.

## LLDB Support

These printers are GDB-only; no LLDB formatters are provided here. Qt Creator's debugger ships its
own Qt type helpers for LLDB (`share/qtcreator/debugger/lldbbridge.py` in the Qt Creator install).
