VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   2250
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   3855
   LinkTopic       =   "Form1"
   ScaleHeight     =   2250
   ScaleWidth      =   3855
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox tb 
      Height          =   1995
      Left            =   120
      MultiLine       =   -1  'True
      TabIndex        =   0
      Top             =   120
      Width           =   3615
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Dim myCon As Long
Dim myDataCon As Long

Private Sub Form_Load()
    Dim i As Long
    Dim x As Byte
    Dim ctype, addrH, addrL As Long
    Me.Show
    DoEvents
    
    ' Connect to the XBee on COM1 with a baud rate of 57600
    ' The XBee should be in API mode 2 (ATAP2)
    If xbee_setupDebug("COM8", 57600, "xbee.log") <> 0 Then
        MsgBox "Error while setting up the local XBee module", vbCritical, "xbee_setup()"
        End
    End If
    xbee_logit "Hello!"
    
    ' Enable callbacks, this only needs to be done ONCE
    ' The window handle provided must remain in memory (dont unload the form - callbacks will automatically be disabled)
    xbee_enableCallbacks Me.hWnd
    
    ' Create a Remote AT connection to a node using 64-bit addressing
    myCon = xbee_newcon_64bit(&H30, xbee_64bitRemoteAT, &H13A200, &H404B75DE)
    xbee_enableACKwait myCon
    
    myDataCon = xbee_newcon_64bit(&H31, xbee_64bitData, &H13A200, &H404B75DE)
    
    ' Setup callbacks
    xbee_attachCallback myCon, AddressOf Module1.callback1
    xbee_attachCallback myDataCon, AddressOf Module1.callback2
    
    ' Send the AT command NI (Node Identifier)
    tb.text = "Sending 'ATNI'..."
    xbee_sendstring myCon, "NI"
End Sub

