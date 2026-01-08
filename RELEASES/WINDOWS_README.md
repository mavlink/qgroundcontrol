Windows build artifact — usage and required files
===============================================

This repository includes a GitHub Actions workflow that builds a native Windows release, bundles the Qt runtime via `windeployqt`, and uploads a zip artifact named `garudxgcs-windows.zip`.

How to trigger the build

- From the repository page on GitHub: Actions → Build Windows Release → Run workflow (or push to `master`).

Downloading the artifact

1. After the workflow finishes, open the workflow run details in Actions.
2. In the run summary, expand the `Artifacts` section and download `garudxgcs-windows`.
3. Extract the ZIP to a folder on a Windows machine.

What the ZIP contains (typical)

- `garudxgcs.exe` — the application executable
- Qt DLLs: `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `Qt6Qml.dll`, `Qt6Quick.dll`, etc.
- Platform plugin: `platforms\qwindows.dll`
- QML modules and their folders (e.g., `qmldir`, `QtQuick` modules)
- Any additional plugins (imageformats, platforms, sqldrivers)

How to run on Windows

1. Extract the ZIP to `C:\qgroundcontrol` (example).
2. Double-click `garudxgcs.exe` or run from PowerShell:

```powershell
cd C:\qgroundcontrol
.\garudxgcs.exe
```

Troubleshooting

- If the app fails to start, run it from PowerShell to capture console errors. Missing DLL messages indicate some runtime libraries weren't bundled; rerun the workflow or run `windeployqt` locally.
- If you need a variant (MSVC vs MinGW), edit `.github/workflows/windows-build.yml` and adjust the build steps accordingly.

If you want, I can:

- Add an explicit step to include third-party redistributables (OpenSSL, etc.) if required.
- Provide a self-extracting installer (NSIS) in the workflow.
