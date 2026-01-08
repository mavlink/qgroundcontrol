# QGroundControl Language Server

Language Server Protocol (LSP) implementation for QGC-specific C++ patterns.

## Features

### Diagnostics

| Code | Description |
|------|-------------|
| `null-vehicle` | Unsafe `activeVehicle()` dereference without null check |
| `null-parameter` | Unsafe `getParameter()` result used without null check |

### MAVLink Completion

The server provides intelligent autocomplete for MAVLink code patterns:

**Message ID Constants** — Type `MAVLINK_MSG_ID_` to get:
- All 29+ common MAVLink message IDs (HEARTBEAT, ATTITUDE, GPS_RAW_INT, etc.)
- Message ID number and category in the completion popup
- Full documentation on hover/resolve

**Decode Functions** — Type `mavlink_msg_` to get:
- Decode functions: `mavlink_msg_heartbeat_decode(&message, &heartbeat)`
- Field getters: `mavlink_msg_heartbeat_get_type(&message)`
- Field documentation with units

**Switch Case Snippets** — In a `switch(message.msgid)` block:
- Full case statement templates with decode pattern
- Automatically includes struct declaration and decode call

### Fact JSON Completion

The server provides intelligent autocomplete for `*Fact.json` metadata files:

**Property Keys** — After `{` or `,` in the Facts array:
- All 22 Fact properties (name, type, units, min, max, etc.)
- Required properties (`name`, `type`) sorted first
- Property descriptions and type info

**Type Values** — After `"type": "`:
- All 14 valid Fact types (uint8, int32, double, string, bool, etc.)
- Type descriptions (bit widths, ranges)

**Unit Values** — After `"units": "`:
- 33+ common units (m, m/s, deg, %, mV, etc.)
- Unit descriptions

**Boolean Values** — After boolean property names:
- `true` / `false` completion

### Go-to-Definition for Facts

Navigate directly from C++ Fact references to their JSON definitions:

- **`_rollFact`** → jumps to `"name": "roll"` in VehicleFact.json
- **`headingFact()`** → finds the `heading` Fact definition
- **`QStringLiteral("pitch")`** → navigates to the Fact metadata

Works with cursor on any Fact variable or accessor in C++ code.

### Planned Features

- Hover documentation for MAVLink types (partially implemented via completion resolve)

## Installation

### Server

```bash
# Install dependencies
pip install pygls lsprotocol

# Test the server
python -m tools.lsp --tcp --verbose
```

### VS Code Extension

```bash
cd tools/lsp/vscode-extension

# Install dependencies
npm install

# Build
npm run compile

# Install locally (from VS Code: F5 to debug, or package with vsce)
```

## Usage

### With VS Code

1. Build and install the VS Code extension
2. Open the QGC workspace
3. Diagnostics appear automatically in C++ files

### With Other Editors

The server uses STDIO by default, compatible with any LSP client:

```bash
# STDIO mode (for editors)
python -m tools.lsp

# TCP mode (for debugging)
python -m tools.lsp --tcp --port 2087
```

#### Neovim (with nvim-lspconfig)

```lua
local lspconfig = require('lspconfig')
local configs = require('lspconfig.configs')

if not configs.qgc_lsp then
    configs.qgc_lsp = {
        default_config = {
            cmd = { 'python3', '-m', 'tools.lsp' },
            filetypes = { 'cpp', 'c' },
            root_dir = lspconfig.util.root_pattern('CMakeLists.txt', '.git'),
        },
    }
end

lspconfig.qgc_lsp.setup({})
```

#### Qt Creator (14.0+)

**Option A: Manual Configuration**

1. Go to **Edit → Preferences → Language Client**
2. Click **Add** to add a new language server
3. Configure:
   - **Name**: `QGC LSP`
   - **Executable**: `/usr/bin/python3` (or your Python path)
   - **Arguments**: `-m tools.lsp`
   - **Working directory**: `%{CurrentProject:Path}`
   - **MIME types**: `text/x-c++src`, `text/x-c++hdr`
   - **File patterns**: `*.cpp;*.cc;*.cxx;*.h;*.hpp;*.hxx`
   - **Startup behavior**: `Requires File`
4. Click **Apply**

**Option B: Via Lua Extension (Recommended)**

If you have the QGCTools Lua extension installed, the LSP server is automatically configured.
See `tools/qtcreator/README.md` for installation.

#### Sublime Text (with LSP package)

Add to `LSP.sublime-settings`:

```json
{
    "clients": {
        "qgc-lsp": {
            "command": ["python3", "-m", "tools.lsp"],
            "selector": "source.c++",
            "initializationOptions": {}
        }
    }
}
```

## Architecture

```
tools/lsp/
├── server.py           # Main LSP server (pygls-based)
├── mavlink_data.py     # MAVLink message definitions for completion
├── fact_schema.py      # Fact JSON schema for completion
├── __init__.py
├── __main__.py         # Entry point for `python -m tools.lsp`
├── analyzers/          # Diagnostic providers
│   └── (future: modular analyzers)
├── vscode-extension/   # VS Code client
│   ├── package.json
│   ├── tsconfig.json
│   └── src/
│       └── extension.ts
└── README.md
```

## Development

### Testing the Server

```bash
# Run in TCP mode for easy testing
python -m tools.lsp --tcp --verbose

# In another terminal, connect with netcat
echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{}}}' | nc localhost 2087
```

### Adding New Diagnostics

1. Add a new method `_check_*` in `QGCLanguageServer`
2. Call it from `analyze_document()`
3. Return `types.Diagnostic` objects with appropriate severity and message

Example:

```python
def _check_magic_numbers(self, document: TextDocument) -> list[types.Diagnostic]:
    diagnostics = []
    for idx, line in enumerate(document.lines):
        # Check for hardcoded MAVLink message IDs
        if re.search(r'case\s+\d+:', line):
            diagnostics.append(
                types.Diagnostic(
                    message="Use MAVLINK_MSG_ID_* constants instead of magic numbers",
                    severity=types.DiagnosticSeverity.Hint,
                    source="qgc-lsp",
                    code="magic-mavlink-id",
                    range=types.Range(
                        start=types.Position(line=idx, character=0),
                        end=types.Position(line=idx, character=len(line)),
                    ),
                )
            )
    return diagnostics
```

## Troubleshooting

### Server not starting

1. Check Python path: `which python3`
2. Verify pygls is installed: `python3 -c "import pygls; print(pygls.__version__)"`
3. Run server directly to see errors: `python3 -m tools.lsp --verbose`

### No diagnostics appearing

1. Ensure file is saved (some editors only sync on save)
2. Check the "QGC LSP" output channel in VS Code
3. Verify the file is a C++ file (.cpp, .cc, .h, .hpp)

## Resources

- [LSP Specification](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/)
- [pygls Documentation](https://pygls.readthedocs.io/)
- [VS Code Language Extensions](https://code.visualstudio.com/api/language-extensions/overview)
