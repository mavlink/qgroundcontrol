Add-Type -TypeDefinition @'
    using System;
    using System.Runtime.InteropServices;

    public class NativeMethods
    {
        [DllImport("kernel32.dll", EntryPoint="MoveFileW", SetLastError=true,
                   CharSet=CharSet.Unicode, ExactSpelling=true,
                   CallingConvention=CallingConvention.StdCall)]
        public static extern bool MoveFile(string lpExistingFileName, string lpNewFileName);
    }
'@

Get-ChildItem ".\symbols" -recurse | ForEach-Object {[NativeMethods]::MoveFile($_.FullName,[io.path]::combine((Split-Path $_.FullName -Parent),$_.Name.ToLower()))}