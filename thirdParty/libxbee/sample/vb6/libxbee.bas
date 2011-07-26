Attribute VB_Name = "libxbee"
Option Explicit

Enum xbee_types
  xbee_unknown

  xbee_localAT
  xbee_remoteAT
  xbee_modemStatus
  xbee_txStatus
  
  ' XBee Series 1 stuff
  xbee_16bitRemoteAT
  xbee_64bitRemoteAT

  xbee_16bitData
  xbee_64bitData
  
  xbee_16bitIO
  xbee_64bitIO
  
  ' XBee Series 2 stuff
  xbee2_data
  xbee2_txStatus
End Enum

Type xbee_sample
    '# X  A5 A4 A3 A2 A1 A0 D8    D7 D6 D5 D4 D3 D2 D1 D0
    IOmask As Integer
    '# X  X  X  X  X  X  X  D8    D7 D6 D5 D4 D3 D2 D1 D0
    IOdigital As Integer
    '# X  X  X  X  X  D  D  D     D  D  D  D  D  D  D  D
    IOanalog(0 To 5) As Integer
End Type

Type xbee_pkt
    flags As Long               '# bit 0 - is64
                                '# bit 1 - dataPkt
                                '# bit 2 - txStatusPkt
                                '# bit 3 - modemStatusPkt
                                '# bit 4 - remoteATPkt
                                '# bit 5 - IOPkt
    frameID As Byte
    atCmd(0 To 1) As Byte
    
    status As Byte
    samples As Byte
    RSSI As Byte
    
    Addr16(0 To 1) As Byte
    
    Addr64(0 To 7) As Byte
    
    data(0 To 127) As Byte
    
    datalen As Long
    
    type As Long ' enum xbee_types
    
    SPARE As Long ' IGNORE THIS (is the pointer to the next packet in C... this will ALWAYS be 0 in VB)
    
    IOdata As xbee_sample
End Type

Private OldhWndHandler As Long
Private ActivehWnd As Long
Private callbackMessageID As Long
Private Callbacks As New Collection

Public Declare Sub xbee_free Lib "libxbee.dll" (ByVal ptr As Long)

Public Declare Function xbee_setup Lib "libxbee.dll" (ByVal port As String, ByVal baudRate As Long) As Long
Public Declare Function xbee_setupDebug Lib "libxbee.dll" (ByVal port As String, ByVal baudRate As Long, ByVal logfile As String) As Long
Private Declare Function xbee_setupDebugAPIRaw Lib "libxbee.dll" Alias "xbee_setupDebugAPI" (ByVal port As String, ByVal baudRate As Long, ByVal logfile As String, ByVal cmdSeq As Byte, ByVal cmdTime As Long) As Long
Private Declare Function xbee_setupAPIRaw Lib "libxbee.dll" Alias "xbee_setupAPI" (ByVal port As String, ByVal baudRate As Long, ByVal cmdSeq As Byte, ByVal cmdTime As Long) As Long

Public Declare Function xbee_end Lib "libxbee.dll" () As Long

Public Declare Function xbee_newcon_simple Lib "libxbee.dll" (ByVal frameID As Byte, ByVal conType As Long) As Long 'xbee_con *
Public Declare Function xbee_newcon_16bit Lib "libxbee.dll" (ByVal frameID As Byte, ByVal conType As Long, ByVal addr16bit As Long) As Long  'xbee_con *
Public Declare Function xbee_newcon_64bit Lib "libxbee.dll" (ByVal frameID As Byte, ByVal conType As Long, ByVal addr64bitLow As Long, ByVal addr64bitHigh As Long) As Long  'xbee_con *
Public Declare Sub xbee_enableACKwait Lib "libxbee.dll" (ByVal con As Long)
Public Declare Sub xbee_disableACKwait Lib "libxbee.dll" (ByVal con As Long)
Public Declare Sub xbee_enableDestroySelf Lib "libxbee.dll" (ByVal con As Long)

Private Declare Sub xbee_enableCallbacksRaw Lib "libxbee.dll" Alias "xbee_enableCallbacks" (ByVal hWnd As Long, ByVal uMsg As Long)
Private Declare Sub xbee_attachCallbackRaw Lib "libxbee.dll" Alias "xbee_attachCallback" (ByVal con As Long)
Private Declare Sub xbee_detachCallbackRaw Lib "libxbee.dll" Alias "xbee_detachCallback" (ByVal con As Long)
Private Declare Function xbee_runCallback Lib "libxbee.dll" (ByVal func As Long, ByVal con As Long, ByVal pkt As Long) As Long

Public Declare Sub xbee_endcon2 Lib "libxbee.dll" (ByVal con As Long)
Public Declare Sub xbee_flushcon Lib "libxbee.dll" (ByVal con As Long)

Public Declare Function xbee_senddata Lib "libxbee.dll" Alias "xbee_nsenddata" (ByVal con As Long, ByRef data As Byte, ByVal Length As Long) As Long
Private Declare Function xbee_senddata_str Lib "libxbee.dll" Alias "xbee_nsenddata" (ByVal con As Long, ByVal data As String, ByVal Length As Long) As Long

Public Declare Function xbee_getpacketRaw Lib "libxbee.dll" Alias "xbee_getpacket" (ByVal con As Long) As Long 'xbee_pkt *

Public Declare Function xbee_hasanalog Lib "libxbee.dll" (ByRef pkt As xbee_pkt, ByVal sample As Long, ByVal inputPin As Long) As Long
Public Declare Function xbee_getanalog Lib "libxbee.dll" (ByRef pkt As xbee_pkt, ByVal sample As Long, ByVal inputPin As Long, ByVal Vref As Double) As Double

Public Declare Function xbee_hasdigital Lib "libxbee.dll" (ByRef pkt As xbee_pkt, ByVal sample As Long, ByVal inputPin As Long) As Long
Public Declare Function xbee_getdigital Lib "libxbee.dll" (ByRef pkt As xbee_pkt, ByVal sample As Long, ByVal inputPin As Long) As Long

Private Declare Function xbee_svn_versionRaw Lib "libxbee.dll" Alias "xbee_svn_version" () As Long
Public Declare Sub xbee_logit Lib "libxbee.dll" (ByVal text As String)

'###########################################################################################################################################################################

Private Declare Sub CopyMemory Lib "kernel32" Alias "RtlMoveMemory" (Destination As Any, Source As Any, ByVal Length As Long)
Private Declare Function lstrlenW Lib "kernel32" (ByVal lpString As Long) As Long
Private Declare Function RegisterWindowMessage Lib "user32" Alias "RegisterWindowMessageA" (ByVal lpString As String) As Long
Private Declare Function SetWindowLong Lib "user32" Alias "SetWindowLongA" (ByVal hWnd As Long, ByVal nIndex As Long, ByVal dwNewLong As Long) As Long
Private Declare Function CallWindowProc Lib "user32" Alias "CallWindowProcA" (ByVal lpPrevWndFunc As Long, ByVal hWnd As Long, ByVal Msg As Long, ByVal wParam As Long, ByVal lParam As Long) As Long
Private Const WM_DESTROY = &H2
Private Const GWL_WNDPROC = -4

Public Function PointerToString(lngPtr As Long) As String
   Dim strTemp As String
   Dim lngLen As Long
   If lngPtr Then
      lngLen = lstrlenW(lngPtr) * 2
      If lngLen Then
         strTemp = Space(lngLen)
         CopyMemory ByVal strTemp, ByVal lngPtr, lngLen
         PointerToString = Replace(strTemp, Chr(0), "")
      End If
   End If
End Function

Public Function ArrayToString(data() As Byte, Optional lb As Integer = -1, Optional ub As Integer = -1) As String
    Dim tmp As String
    Dim i
    If lb = -1 Then lb = LBound(data)
    If ub = -1 Then ub = UBound(data)
    tmp = ""
    For i = lb To ub
        If (data(i) = 0) Then Exit For
        tmp = tmp & Chr(data(i))
    Next
    ArrayToString = tmp
End Function

Public Function xbee_pointerToPacket(lngPtr As Long) As xbee_pkt
    Dim p As xbee_pkt
    CopyMemory p, ByVal lngPtr, Len(p)
    xbee_pointerToPacket = p
End Function

Public Sub libxbee_load()
    ' this function is simply to get VB6 to call a libxbee function
    ' if you are using any C DLLs that make use of libxbee, then you should call this function first so that VB6 will load libxbee
    xbee_svn_versionRaw
End Sub

Public Function xbee_svn_version() As String
    xbee_svn_version = PointerToString(xbee_svn_versionRaw())
End Function

Public Function xbee_setupAPI(ByVal port As String, ByVal baudRate As Long, ByVal cmdSeq As String, ByVal cmdTime As Long)
    xbee_setupAPI = xbee_setupAPIRaw(port, baudRate, Asc(cmdSeq), cmdTime)
End Function

Public Function xbee_setupDebugAPI(ByVal port As String, ByVal baudRate As Long, ByVal logfile As String, ByVal cmdSeq As String, ByVal cmdTime As Long)
    xbee_setupDebugAPI = xbee_setupDebugAPIRaw(port, baudRate, logfile, Asc(cmdSeq), cmdTime)
End Function

Private Sub xbee_ensureMessageID()
    If callbackMessageID = 0 Then
        callbackMessageID = RegisterWindowMessage("libxbee")
    End If
    xbee_enableCallbacksRaw ActivehWnd, callbackMessageID
End Sub

Public Sub xbee_attachCallback(ByVal con As Long, ByVal func As Long)
    Dim t(0 To 1) As Long
    Dim c As String
    If ActivehWnd = 0 Then
        Debug.Print "Callbacks not enabled!"
        Exit Sub
    End If
    xbee_ensureMessageID
    c = CStr(con)
    t(0) = con
    t(1) = func
    On Error Resume Next
    Callbacks.Remove c
    Callbacks.Add t, c
    On Error GoTo 0
    xbee_attachCallbackRaw con
End Sub

Public Sub xbee_detachCallback(ByVal con As Long)
    If ActivehWnd = 0 Then
        Debug.Print "Callbacks not enabled!"
        Exit Sub
    End If
    On Error Resume Next
    xbee_detachCallbackRaw con
    Callbacks.Remove CStr(con)
End Sub

Public Sub xbee_enableCallbacks(ByVal hWnd As Long)
    If ActivehWnd <> 0 Then
        Debug.Print "Callbacks already enabled!"
        Exit Sub
    End If
    ActivehWnd = hWnd
    OldhWndHandler = SetWindowLong(hWnd, GWL_WNDPROC, AddressOf libxbee.xbee_messageHandler)
    xbee_ensureMessageID
End Sub

Public Sub xbee_disableCallbacks()
    Dim id As Variant
    If ActivehWnd = 0 Then
        Debug.Print "Callbacks not enabled!"
        Exit Sub
    End If
    For Each id In Callbacks
        xbee_detachCallback id(0)
    Next
    SetWindowLong ActivehWnd, GWL_WNDPROC, OldhWndHandler
    ActivehWnd = 0
    OldhWndHandler = 0
End Sub

Private Function xbee_messageHandler(ByVal hWnd As Long, ByVal uMsg As Long, ByVal wParam As Long, ByVal lParam As Long) As Long
    If uMsg = callbackMessageID Then
        Dim t As Long
        On Error Resume Next
        Err.Clear
        t = Callbacks.Item(CStr(wParam))(1)
        If Err.Number = 0 Then
            On Error GoTo 0
            xbee_messageHandler = xbee_runCallback(t, wParam, lParam)
            Exit Function
        End If
        On Error GoTo 0
        xbee_logit "Unable to match Connection with active callback!"
    End If
    xbee_messageHandler = CallWindowProc(OldhWndHandler, hWnd, uMsg, wParam, lParam)
    If uMsg = WM_DESTROY And ActivehWnd <> 0 Then
        ' Disable the MessageHandler if the form "unload" event is detected
        xbee_disableCallbacks
    End If
End Function

Public Sub xbee_endcon(ByRef con As Long)
    xbee_endcon2 con
    con = 0
End Sub

Public Function xbee_sendstring(ByVal con As Long, ByVal str As String)
    xbee_sendstring = xbee_senddata_str(con, str, Len(str))
End Function

Public Function xbee_getpacketPtr(ByVal con As Long, ByRef pkt As Long) As Integer
    Dim ptr As Long
    
    ptr = xbee_getpacketRaw(con)
    If ptr = 0 Then
        pkt = 0
        xbee_getpacketPtr = 0
        Exit Function
    End If
    
    pkt = ptr
    xbee_getpacketPtr = 1
End Function

Public Function xbee_getpacket(ByVal con As Long, ByRef pkt As xbee_pkt) As Integer
    Dim ptr As Long
    
    ptr = xbee_getpacketRaw(con)
    If ptr = 0 Then
        xbee_getpacket = 0
        Exit Function
    End If
    
    pkt = xbee_pointerToPacket(ptr)
    xbee_free ptr
    
    xbee_getpacket = 1
End Function

