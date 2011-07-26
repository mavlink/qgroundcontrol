Attribute VB_Name = "Module1"
Public Function callback1(ByVal con As Long, ByRef pkt As xbee_pkt) As Long
    ' Check the returned status, if it isnt 0 then an error occured
    If pkt.status <> 0 Then
        Form1.tb.Text = Form1.tb.Text & vbNewLine & "An error occured (" & pkt.status & ")"
        Exit Function
    End If
    
    ' Display the Node Identifier
    Form1.tb.Text = Form1.tb.Text & vbNewLine & "Node Identifier:" & StrConv(pkt.data, vbUnicode)
    Form1.tb.SelStart = Len(Form1.tb.Text)
End Function

Public Function callback2(ByVal con As Long, ByRef pkt As xbee_pkt) As Long
    ' Display the data
    Form1.tb.Text = Form1.tb.Text & vbNewLine & "Rx:" & StrConv(pkt.data, vbUnicode)
    Form1.tb.SelStart = Len(Form1.tb.Text)
End Function

