Attribute VB_Name = "Module1"
Public atcon As Long
Public remoteCon As Long

Public Sub setButtons(ByVal state As Boolean)
    Form1.talk_to_me.Tag = ""
    Form1.talk_to_me.Enabled = state
    Form1.set_dest.Tag = ""
    Form1.set_dest.Enabled = state
    Form1.reset_node.Tag = ""
    Form1.reset_node.Enabled = state
    Form1.set_default.Tag = ""
    Form1.set_default.Enabled = state
    Form1.write_settings.Tag = ""
    Form1.write_settings.Enabled = state
    Form1.set_ni.Tag = ""
    Form1.set_ni.Enabled = state
End Sub

Public Function xbeesend(ByVal con As Long, ByVal str As String) As Long
    Form1.tmr_timeout.Enabled = False
    Form1.tmr_timeout.Tag = CStr(con) & Chr(1) & str
    Form1.tmr_timeout.Enabled = True
    xbee_sendstring con, str
End Function

Public Sub setupCB_Default_Start()
    xbee_attachCallback remoteCon, AddressOf setupCB_Default
    xbee_attachCallback atcon, AddressOf setupCB_Default
    xbeesend remoteCon, "CH" & Chr(16)
End Sub
Public Function setupCB_Default(ByVal con As Long, ByRef pkt As xbee_pkt) As Long
    Dim str As String
    Dim str2() As String
    ' default values (in order of setting):
    ' CH = 10
    ' local CH = 10
    ' MY = FF
    ' T3 = 1
    ' BD = 6
    ' AP = 0
    ' RO = 1
    ' D0 = 5 (turn on rest of system)
    ' D1 = 2 (battery reading)
    ' D2 = 0
    ' D3 = 5 (reset)
    ' D4 = 4 (battery reading power)
    ' D5 = 0
    ' D6 = 0
    ' D7 = 0
    ' D8 = 0
    ' IA = 0xFFFF (accept inputs from anyone)
    ' IU = 0
    Debug.Print ArrayToString(pkt.atCmd)
    If con = atcon Then
        xbee_attachCallback con, AddressOf localCB
        xbeesend remoteCon, "MY" & Chr(255) & Chr(255)
        Exit Function
    End If
    Select Case ArrayToString(pkt.atCmd)
        Case "CH"
            xbeesend atcon, "CH" & Chr(16)
        Case "MY"
            xbeesend con, "T3" & Chr(1)
        Case "T3"
            xbeesend con, "BD" & Chr(0) & Chr(0) & Chr(0) & Chr(6)
        Case "BD"
            xbeesend con, "AP" & Chr(0)
        Case "AP"
            xbeesend con, "RO" & Chr(1)
        Case "RO"
            xbeesend con, "D0" & Chr(5)
        Case "D0"
            xbeesend con, "D1" & Chr(2)
        Case "D1"
            xbeesend con, "D2" & Chr(0)
        Case "D2"
            xbeesend con, "D3" & Chr(4)
        Case "D3"
            xbeesend con, "D4" & Chr(4)
        Case "D4"
            xbeesend con, "D5" & Chr(0)
        Case "D5"
            xbeesend con, "D6" & Chr(0)
        Case "D6"
            xbeesend con, "D7" & Chr(0)
        Case "D7"
            xbeesend con, "D8" & Chr(0)
        Case "D8"
            xbeesend con, "IA" & Chr(0) & Chr(0) & Chr(0) & Chr(0) & Chr(0) & Chr(0) & Chr(255) & Chr(255)
        Case "IA"
            xbeesend con, "IU" & Chr(0)
        Case "IU"
            Form1.set_default.Tag = ""
            Form1.tmr_refresh.Enabled = True
            Form1.tmr_timeout.Enabled = False
    End Select
End Function

Public Function setupCB_TTM(ByVal con As Long, ByRef pkt As xbee_pkt) As Long
    Dim str As String
    Dim str2() As String
    Select Case ArrayToString(pkt.atCmd)
        Case "DH"
            str2 = Split(Form1.nodelist.List(0), "  ")
            str = Chr(CInt("&H" & Mid(str2(2), 9, 2)))
            str = Chr(CInt("&H" & Mid(str2(2), 7, 2))) & str
            str = Chr(CInt("&H" & Mid(str2(2), 5, 2))) & str
            str = Chr(CInt("&H" & Mid(str2(2), 3, 2))) & str
            str = "DL" & str
            xbeesend con, str
        Case "DL"
            Form1.talk_to_me.Tag = ""
            Form1.tmr_refresh.Enabled = True
    End Select
End Function

Public Function setupCB_SDEST(ByVal con As Long, ByRef pkt As xbee_pkt) As Long
    Dim str As String
    Dim str2() As String
    Select Case ArrayToString(pkt.atCmd)
        Case "DH"
            str2 = Split(Form1.nodelist.text, "  ")
            str = Chr(CInt("&H" & Mid(str2(2), 9, 2)))
            str = Chr(CInt("&H" & Mid(str2(2), 7, 2))) & str
            str = Chr(CInt("&H" & Mid(str2(2), 5, 2))) & str
            str = Chr(CInt("&H" & Mid(str2(2), 3, 2))) & str
            str = "DL" & str
            xbeesend con, str
        Case "DL"
            Form1.set_dest.Tag = ""
            Form1.nodelist.ListIndex = Form1.nodelist.Tag
            Form1.tmr_refresh.Enabled = True
    End Select
End Function

Public Function remoteCB(ByVal con As Long, ByRef pkt As xbee_pkt) As Long
    Dim t As String
    Dim i As Long
    Debug.Print "<+>", ArrayToString(pkt.atCmd)
    Form1.tmr_timeout.Enabled = False
    Select Case ArrayToString(pkt.atCmd)
        Case "AP"
            Select Case pkt.data(0)
                Case 0
                    Form1.ap.Caption = "0 - API Disabled"
                Case 1
                    Form1.ap.Caption = "1 - API Enabled (no escapes)"
                Case 2
                    Form1.ap.Caption = "2 - API Enabled (with escapes)"
                Case Default
                    Form1.ap.Caption = "0x" & Hex(pkt.data(0)) & " - Unknown..."
            End Select
            xbeesend con, "BD"
        Case "BD"
            t = Hex(pkt.data(3))
            If (Len(t) < 2) Then t = "0" & t
            t = Hex(pkt.data(2)) & t
            If (Len(t) < 4) Then t = "0" & t
            t = Hex(pkt.data(1)) & t
            If (Len(t) < 6) Then t = "0" & t
            t = Hex(pkt.data(0)) & t
            If (Len(t) < 8) Then t = "0" & t
            i = CStr("&H" & t)
            Select Case i
                Case 0
                    Form1.bd.Caption = "0 - 1200 bps"
                Case 1
                    Form1.bd.Caption = "1 - 2400 bps"
                Case 2
                    Form1.bd.Caption = "2 - 4800 bps"
                Case 3
                    Form1.bd.Caption = "3 - 9600 bps"
                Case 4
                    Form1.bd.Caption = "4 - 19200 bps"
                Case 5
                    Form1.bd.Caption = "5 - 38400 bps"
                Case 6
                    Form1.bd.Caption = "6 - 57600 bps"
                Case 7
                    Form1.bd.Caption = "7 - 115200 bps"
                Case Default
                    Form1.bd.Caption = "0x" & Hex(i) & " - Unknwon..."
            End Select
            xbeesend con, "CH"
        Case "CH"
            t = Hex(pkt.data(0))
            If (Len(t) < 2) Then t = "0" & t
            Form1.ch.Caption = "0x" & t
            xbeesend con, "DH"
        Case "DH"
            t = Hex(pkt.data(3))
            If (Len(t) < 2) Then t = "0" & t
            t = Hex(pkt.data(2)) & t
            If (Len(t) < 4) Then t = "0" & t
            t = Hex(pkt.data(1)) & t
            If (Len(t) < 6) Then t = "0" & t
            t = Hex(pkt.data(0)) & t
            If (Len(t) < 8) Then t = "0" & t
            Form1.dh.Caption = "0x" & t
            xbeesend con, "DL"
        Case "DL"
            t = Hex(pkt.data(3))
            If (Len(t) < 2) Then t = "0" & t
            t = Hex(pkt.data(2)) & t
            If (Len(t) < 4) Then t = "0" & t
            t = Hex(pkt.data(1)) & t
            If (Len(t) < 6) Then t = "0" & t
            t = Hex(pkt.data(0)) & t
            If (Len(t) < 8) Then t = "0" & t
            Form1.dl.Caption = "0x" & t
            xbeesend con, "IA"
        Case "IA"
            t = Hex(pkt.data(7)) & t
            If (Len(t) < 2) Then t = "0" & t
            t = Hex(pkt.data(6)) & t
            If (Len(t) < 4) Then t = "0" & t
            t = Hex(pkt.data(5)) & t
            If (Len(t) < 6) Then t = "0" & t
            t = Hex(pkt.data(4)) & t
            If (Len(t) < 8) Then t = "0" & t
            t = Hex(pkt.data(3)) & t
            If (Len(t) < 10) Then t = "0" & t
            t = Hex(pkt.data(2)) & t
            If (Len(t) < 12) Then t = "0" & t
            t = Hex(pkt.data(1)) & t
            If (Len(t) < 14) Then t = "0" & t
            t = Hex(pkt.data(0)) & t
            If (Len(t) < 16) Then t = "0" & t
            Form1.ia.Caption = "0x" & t
            xbeesend con, "HV"
        Case "HV"
            t = Hex(pkt.data(1))
            If (Len(t) < 2) Then t = "0" & t
            t = Hex(pkt.data(0)) & t
            If (Len(t) < 4) Then t = "0" & t
            Form1.hv.Caption = "0x" & t
            xbeesend con, "VR"
        Case "VR"
            t = Hex(pkt.data(1))
            If (Len(t) < 2) Then t = "0" & t
            t = Hex(pkt.data(0)) & t
            If (Len(t) < 4) Then t = "0" & t
            Form1.vr.Caption = "0x" & t
            If con = atcon Then
                xbee_attachCallback con, AddressOf localCB
            End If
            setButtons True
            Form1.nodelist.Enabled = True
            Form1.tmr_refresh.Enabled = True
            Form1.tmr_timeout.Enabled = False
        Case Else
            If con = atcon Then
                xbee_attachCallback con, AddressOf localCB
            End If
            setButtons True
            Form1.nodelist.Enabled = True
            Form1.tmr_refresh.Enabled = True
            Form1.tmr_timeout.Enabled = False
    End Select
End Function

Public Function localCB(ByVal con As Long, ByRef pkt As xbee_pkt) As Long
    Dim nodeinfo As String
    Dim nodename As String
    Dim tmp As String
    Dim tmp2() As String
    Dim sh, sl As String
    Dim i, m As Integer
    Dim AT As String
    Form1.tmr_timeout.Enabled = False
    AT = ArrayToString(pkt.atCmd)
    ' handle initial stuff
    Select Case AT
        Case "MY"
            nodeinfo = "0x"
            tmp = Hex(pkt.data(0))
            If (Len(tmp) < 2) Then tmp = "0" & tmp
            nodeinfo = nodeinfo & tmp
            tmp = Hex(pkt.data(1))
            If (Len(tmp) < 2) Then tmp = "0" & tmp
            nodeinfo = nodeinfo & tmp
            If Form1.nodelist.ListCount > 0 Then
                Form1.nodelist.List(0) = nodeinfo
            Else
                Form1.nodelist.AddItem nodeinfo
            End If
            ' issue next command
            xbeesend con, "SH"
        Case "SH"
            nodeinfo = Form1.nodelist.List(0) & "  0x"
            tmp = Hex(pkt.data(0))
            If (Len(tmp) < 2) Then tmp = "0" & tmp
            nodeinfo = nodeinfo & tmp
            tmp = Hex(pkt.data(1))
            If (Len(tmp) < 2) Then tmp = "0" & tmp
            nodeinfo = nodeinfo & tmp
            tmp = Hex(pkt.data(2))
            If (Len(tmp) < 2) Then tmp = "0" & tmp
            nodeinfo = nodeinfo & tmp
            tmp = Hex(pkt.data(3))
            If (Len(tmp) < 2) Then tmp = "0" & tmp
            nodeinfo = nodeinfo & tmp
            Form1.nodelist.List(0) = nodeinfo
            ' issue next command
            xbeesend con, "SL"
        Case "SL"
            nodeinfo = Form1.nodelist.List(0) & "  0x"
            tmp = Hex(pkt.data(0))
            If (Len(tmp) < 2) Then tmp = "0" & tmp
            nodeinfo = nodeinfo & tmp
            tmp = Hex(pkt.data(1))
            If (Len(tmp) < 2) Then tmp = "0" & tmp
            nodeinfo = nodeinfo & tmp
            tmp = Hex(pkt.data(2))
            If (Len(tmp) < 2) Then tmp = "0" & tmp
            nodeinfo = nodeinfo & tmp
            tmp = Hex(pkt.data(3))
            If (Len(tmp) < 2) Then tmp = "0" & tmp
            nodeinfo = nodeinfo & tmp
            Form1.nodelist.List(0) = nodeinfo
            ' issue next command
            xbeesend con, "NI"
        Case "NI"
            nodeinfo = Form1.nodelist.List(0) & "  -***dB  *  "
            tmp = ArrayToString(pkt.data)
            nodeinfo = nodeinfo & tmp
            Form1.nodelist.List(0) = nodeinfo
            ' issue next command
            xbeesend con, "ND"
    End Select
    
    If (AT <> "ND") Then Exit Function
    If (pkt.status <> 0) Then
        MsgBox "An error occured when attempting to scan!", vbCritical
        Exit Function
    End If
    
    If (pkt.datalen = 0) Then
        ' increment the counter for each node
        For i = 0 To Form1.nodelist.ListCount - 1
            tmp2 = Split(Form1.nodelist.List(i), "  ")
            If tmp2(4) <> "+" And tmp2(4) <> "*" Then
                tmp2(4) = CInt(tmp2(4)) + 1
                If (CInt(tmp2(4)) > 9) Then tmp2(4) = "+"
            End If
            tmp = ""
            For m = LBound(tmp2) To UBound(tmp2)
                If m > 0 Then tmp = tmp & "  "
                tmp = tmp & tmp2(m)
            Next
            Form1.nodelist.List(i) = tmp
        Next
        
        ' restart the refresh timer
        Form1.tmr_refresh.Enabled = True
        Exit Function
    End If
    
    ' extract the 16-bit address
    nodeinfo = ""
    tmp = Hex(pkt.data(1))
    If (Len(tmp) < 2) Then tmp = "0" & tmp
    tmp = Hex(pkt.data(0)) & tmp
    If (Len(tmp) < 4) Then tmp = "0" & tmp
    tmp = "0x" & tmp
    nodeinfo = nodeinfo & tmp
    
    nodeinfo = nodeinfo & "  "
    
    ' extract the high portion of the 64-bit address
    nodeinfo = nodeinfo
    tmp = Hex(pkt.data(5))
    If (Len(tmp) < 2) Then tmp = "0" & tmp
    tmp = Hex(pkt.data(4)) & tmp
    If (Len(tmp) < 4) Then tmp = "0" & tmp
    tmp = Hex(pkt.data(3)) & tmp
    If (Len(tmp) < 6) Then tmp = "0" & tmp
    tmp = Hex(pkt.data(2)) & tmp
    If (Len(tmp) < 8) Then tmp = "0" & tmp
    tmp = "0x" & tmp
    nodeinfo = nodeinfo & tmp
    sh = tmp
    
    nodeinfo = nodeinfo & "  "
    
    ' extract the low portion of the 64-bit address
    nodeinfo = nodeinfo
    tmp = Hex(pkt.data(9))
    If (Len(tmp) < 2) Then tmp = "0" & tmp
    tmp = Hex(pkt.data(8)) & tmp
    If (Len(tmp) < 4) Then tmp = "0" & tmp
    tmp = Hex(pkt.data(7)) & tmp
    If (Len(tmp) < 6) Then tmp = "0" & tmp
    tmp = Hex(pkt.data(6)) & tmp
    If (Len(tmp) < 8) Then tmp = "0" & tmp
    tmp = "0x" & tmp
    nodeinfo = nodeinfo & tmp
    sl = tmp
    
    nodeinfo = nodeinfo & "  "
    
    ' extract the rssi (signal strength)
    tmp = "-" & CStr(pkt.data(10))
    If Len(tmp) < 3 Then tmp = " " & tmp
    If Len(tmp) < 4 Then tmp = " " & tmp
    tmp = tmp & "dB"
    nodeinfo = nodeinfo & tmp
    
    nodeinfo = nodeinfo & "  "
    ' add a number of scans
    nodeinfo = nodeinfo & 0
    
    nodeinfo = nodeinfo & "  "
    
    ' extract the node name
    nodename = ArrayToString(pkt.data, 11)
    nodeinfo = nodeinfo & nodename
    
    ' see if we have already got this node
    For i = 0 To Form1.nodelist.ListCount - 1
        tmp2 = Split(Form1.nodelist.List(i), "  ")
        If tmp2(1) = sh And tmp2(2) = sl Then
            Form1.nodelist.List(i) = nodeinfo
            Exit Function
        End If
    Next
    
    ' otherwise add the info to the list
    Form1.nodelist.AddItem nodeinfo
End Function
