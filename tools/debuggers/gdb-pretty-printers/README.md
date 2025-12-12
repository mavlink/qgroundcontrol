# GDB Pretty Printers for Qt 6

This directory contains GDB pretty printers for displaying Qt types in a human-readable format.

## Setup

### Quick Setup (per session)

In GDB:

```gdb
source /path/to/qgroundcontrol/tools/gdb-pretty-printers/qt6.py
```

### Permanent Setup

Add to your `~/.gdbinit`:

```gdb
python
import sys
sys.path.insert(0, '/path/to/qgroundcontrol/tools/gdb-pretty-printers')
from qt6 import register_qt_printers
register_qt_printers(None)
end
```

### VS Code Setup

The `.vscode/launch.json` in this repository is already configured to load these printers automatically.

## Supported Types

- **Strings**: QString, QByteArray
- **Containers**: QList, QVector, QMap, QHash
- **Geometry**: QPoint, QPointF, QSize, QSizeF, QRect, QRectF
- **Other**: QVariant, QUrl, QSharedPointer

## Example Output

```
(gdb) print myString
$1 = "Hello, World!"

(gdb) print myList
$2 = QList<int> (size=3)
  [0] = 1
  [1] = 2
  [2] = 3

(gdb) print myPoint
$3 = (100, 200)

(gdb) print myRect
$4 = [(0, 0) 800x600]
```

## LLDB Support

For LLDB (macOS), Qt provides built-in formatters. You can also use:

```bash
# In ~/.lldbinit
command script import /path/to/qt6/plugins/lldb/qt6lldb.py
```

Or check if your Qt installation includes LLDB formatters in `$QT_ROOT/plugins/lldb/`.
