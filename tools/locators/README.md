# QGC Locator CLI Tool

A command-line tool for quickly finding QGC-specific code elements like Facts, FactGroups, and MAVLink messages.

## Usage

```bash
# Find Fact names containing 'lat'
./qgc_locator.py fact lat

# Find FactGroup classes with 'gps'
./qgc_locator.py factgroup gps

# Find MAVLink message usage
./qgc_locator.py mavlink HEARTBEAT

# Find parameters in JSON metadata
./qgc_locator.py param BATT
```

## Output Format

Results are printed in tab-separated format for easy editor integration:

```
name    path:line
```

Example:
```
lat     src/Vehicle/FactGroups/VehicleGPSFactGroup.h:17
lon     src/Vehicle/FactGroups/VehicleGPSFactGroup.h:18
```

## Command Aliases

| Full Command | Aliases |
|-------------|---------|
| `fact` | `facts`, `f` |
| `factgroup` | `factgroups`, `fg` |
| `mavlink` | `mav`, `m` |
| `param` | `params`, `p` |

## QtCreator Integration

Add as External Tool via **Tools → External → Configure**:

### QGC Fact Locator
- **Executable**: `python3`
- **Arguments**: `%{CurrentProject:Path}/tools/locators/qgc_locator.py fact %{CurrentDocument:Selection}`
- **Working Directory**: `%{CurrentProject:Path}`

### QGC MAVLink Locator
- **Executable**: `python3`
- **Arguments**: `%{CurrentProject:Path}/tools/locators/qgc_locator.py mavlink %{CurrentDocument:Selection}`
- **Working Directory**: `%{CurrentProject:Path}`

## VS Code Integration

Add to `.vscode/tasks.json`:

```json
{
  "label": "QGC: Find Fact",
  "type": "shell",
  "command": "python3",
  "args": [
    "${workspaceFolder}/tools/locators/qgc_locator.py",
    "fact",
    "${selectedText}"
  ],
  "problemMatcher": []
}
```

## Vim/Neovim Integration

Add to your config:

```vim
" Find Fact under cursor
nnoremap <leader>qf :!python3 tools/locators/qgc_locator.py fact <cword><CR>

" Find MAVLink message under cursor
nnoremap <leader>qm :!python3 tools/locators/qgc_locator.py mavlink <cword><CR>
```
