# QtCreator Integration for QGroundControl

IDE support for QGC-specific patterns: snippets, Lua extension, and external tool integration.

## Directory Structure

```
qtcreator/
├── snippets/
│   └── qgc-cpp.xml          # C++ snippets for Fact/MAVLink patterns
├── lua/
│   └── QGCTools/            # Lua extension (Qt Creator 14+)
└── plugin/
    └── ...                  # Native C++ plugin (see plugin/README.md)
```

**Related tools:**
- [tools/lsp/](../lsp/) - Language Server for real-time diagnostics
- [tools/analyzers/](../analyzers/) - Static analysis (vehicle null-check)
- [tools/locators/](../locators/) - CLI search for Facts/MAVLink
- [tools/generators/](../generators/) - Code generators (FactGroup)

---

## 1. QtCreator Snippets

Pre-built snippets for common QGC patterns.

**Installation:**
```bash
# Linux
cp snippets/qgc-cpp.xml ~/.config/QtProject/qtcreator/snippets/

# macOS
cp snippets/qgc-cpp.xml ~/Library/Application\ Support/QtProject/qtcreator/snippets/

# Windows
copy snippets\qgc-cpp.xml %APPDATA%\QtProject\qtcreator\snippets\
```

**Available snippets (type trigger + Tab):**

| Trigger | Description |
|---------|-------------|
| `factprop` | Q_PROPERTY declaration for a Fact |
| `factmember` | Fact member variable with initialization |
| `factaccessor` | Fact getter method |
| `factinit` | _addFact() call in constructor |
| `factfull` | Complete Fact declaration (all parts) |
| `mavhandle` | MAVLink message handler function |
| `mavswitch` | switch on message.msgid |
| `mavcase` | Single case for MAVLink switch |
| `mavdecode` | MAVLink message decode pattern |
| `vehiclenull` | Safe vehicle null check pattern |
| `vehiclenullv` | Safe vehicle null check (void return) |
| `paramnull` | Safe parameter access with null check |
| `factgroupctor` | FactGroup constructor |
| `handlemsg` | handleMessage() override |

---

## 2. Lua Extension (Qt Creator 14+)

Full IDE integration with menu actions, keyboard shortcuts, and dialogs.

**Requirements:**
- Qt Creator 14.0.0 or later
- Python 3.10+

**Installation:**
```bash
# Linux
mkdir -p ~/.local/share/QtProject/qtcreator/lua/extensions
cp -r lua/QGCTools ~/.local/share/QtProject/qtcreator/lua/extensions/

# macOS
mkdir -p ~/Library/Application\ Support/QtProject/qtcreator/lua/extensions
cp -r lua/QGCTools ~/Library/Application\ Support/QtProject/qtcreator/lua/extensions/

# Windows
xcopy /E /I lua\QGCTools %APPDATA%\QtProject\qtcreator\lua\extensions\QGCTools
```

Restart Qt Creator. Access via **Tools → QGC**.

**Features:**

| Menu Action | Shortcut | Description |
|-------------|----------|-------------|
| Search Facts... | `Ctrl+Shift+F` | Search for Fact names in codebase |
| Search FactGroups... | `Ctrl+Shift+G` | Search for FactGroup classes |
| Search MAVLink Usage... | `Ctrl+Shift+M` | Find MAVLink message handlers |
| Check Null Safety (File) | `Ctrl+Shift+N` | Analyze current file for null issues |
| Check Null Safety (Project) | - | Analyze entire src/ directory |
| Generate FactGroup... | `Ctrl+Shift+Alt+G` | Interactive FactGroup generator wizard |

**LSP Integration:**

The Lua extension automatically registers the [QGC Language Server](../lsp/) for real-time diagnostics. Requires: `pip install pygls lsprotocol`

---

## 3. External Tools Integration

Add via **Tools → External → Configure**:

### MAVLink Message Lookup
- **Executable**: `xdg-open`
- **Arguments**: `https://mavlink.io/en/messages/common.html#%{CurrentSelection}`

### QGC Fact Locator
- **Executable**: `python3`
- **Arguments**: `%{CurrentProject:Path}/tools/locators/qgc_locator.py fact %{CurrentDocument:Selection}`

### QGC MAVLink Locator
- **Executable**: `python3`
- **Arguments**: `%{CurrentProject:Path}/tools/locators/qgc_locator.py mavlink %{CurrentDocument:Selection}`

---

## 4. MAVLink Scaling Factors Reference

Common scaling factors used in QGC MAVLink handlers:

| Field Pattern | Scaling | Example |
|---------------|---------|---------|
| `lat`, `lon` (degE7) | `* 1e-7` | `gpsRawInt.lat * 1e-7` |
| `alt` (mm) | `/ 1000.0` | `gpsRawInt.alt / 1000.0` |
| `eph`, `epv` (cm) | `/ 100.0` | `gpsRawInt.eph / 100.0` |
| `vel` (cm/s) | `/ 100.0` | `gpsRawInt.vel / 100.0` |
| `cog` (cdeg) | `/ 100.0` | `gpsRawInt.cog / 100.0` |
| `hdg` (cdeg) | `/ 100.0` | `vfrHud.heading / 100.0` |
| `press_abs` (hPa) | `* 100.0` (Pa) | `scaledPressure.press_abs * 100.0` |
| `temperature` (cdegC) | `/ 100.0` | `scaledPressure.temperature / 100.0` |
| `roll`, `pitch`, `yaw` (rad) | `* (180.0/M_PI)` | degrees conversion |

---

## Resources

- [Qt Creator Lua Extensions](https://doc.qt.io/qtcreator-extending/index.html)
- [Creating C++ Plugins](https://doc.qt.io/qtcreator-extending/first-plugin.html)
- [Fact System Documentation](../../docs/en/qgc-dev-guide/fact_system.md)
