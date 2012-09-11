VERSION 5.00
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Talk to Me"
   ClientHeight    =   7875
   ClientLeft      =   45
   ClientTop       =   375
   ClientWidth     =   7515
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   7875
   ScaleWidth      =   7515
   StartUpPosition =   1  'CenterOwner
   Begin VB.Timer tmr_timeout 
      Enabled         =   0   'False
      Interval        =   5000
      Left            =   3720
      Top             =   1380
   End
   Begin VB.Frame Frame2 
      Caption         =   " Actions "
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   1335
      Left            =   180
      TabIndex        =   1
      Top             =   6420
      Width           =   7215
      Begin VB.CommandButton write_settings 
         Caption         =   "Write Settings"
         Enabled         =   0   'False
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   1920
         TabIndex        =   50
         Top             =   780
         Width           =   1935
      End
      Begin VB.CommandButton set_default 
         Caption         =   "Set Default"
         Enabled         =   0   'False
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   180
         TabIndex        =   49
         Top             =   780
         Width           =   1575
      End
      Begin VB.CommandButton reset_node 
         Caption         =   "Reset Node"
         Enabled         =   0   'False
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   4020
         TabIndex        =   47
         Top             =   780
         Width           =   1575
      End
      Begin VB.CommandButton set_dest 
         Caption         =   "Set destination"
         Enabled         =   0   'False
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   1920
         TabIndex        =   46
         Top             =   300
         Width           =   1935
      End
      Begin VB.CommandButton talk_to_me 
         Caption         =   "Talk to Me"
         Enabled         =   0   'False
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   180
         TabIndex        =   45
         Top             =   300
         Width           =   1575
      End
      Begin VB.CommandButton set_ni 
         Caption         =   "Set Node Identifier"
         Enabled         =   0   'False
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   4020
         TabIndex        =   51
         Top             =   300
         Width           =   2355
      End
   End
   Begin VB.Frame Frame1 
      Caption         =   " Settings "
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   3315
      Left            =   180
      TabIndex        =   0
      Top             =   3000
      Width           =   7215
      Begin VB.Label ni 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   24
         Top             =   300
         Width           =   3915
      End
      Begin VB.Label sl 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   22
         Top             =   1020
         Width           =   3915
      End
      Begin VB.Label sh 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   20
         Top             =   780
         Width           =   3915
      End
      Begin VB.Label my 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   18
         Top             =   540
         Width           =   3915
      End
      Begin VB.Label ap 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   17
         Top             =   1260
         Width           =   3915
      End
      Begin VB.Label bd 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   16
         Top             =   1500
         Width           =   3915
      End
      Begin VB.Label ch 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   15
         Top             =   1740
         Width           =   3915
      End
      Begin VB.Label dh 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   14
         Top             =   1980
         Width           =   3915
      End
      Begin VB.Label dl 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   13
         Top             =   2220
         Width           =   3915
      End
      Begin VB.Label ia 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   11
         Top             =   2460
         Width           =   3915
      End
      Begin VB.Label vr 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   10
         Top             =   2940
         Width           =   3915
      End
      Begin VB.Label hv 
         BackStyle       =   0  'Transparent
         Caption         =   "-"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   3180
         TabIndex        =   12
         Top             =   2700
         Width           =   3915
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "NI - Node Identifier"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   17
         Left            =   180
         TabIndex        =   25
         Top             =   300
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "MY - 16-bit Address"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   13
         Left            =   180
         TabIndex        =   19
         Top             =   540
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "VR - Firmware Version"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   14
         Left            =   180
         TabIndex        =   9
         Top             =   2940
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "IA - I/O Address"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   12
         Left            =   180
         TabIndex        =   8
         Top             =   2460
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "DL - Destination Low"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   10
         Left            =   180
         TabIndex        =   6
         Top             =   2220
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "DH - Destination High"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   9
         Left            =   180
         TabIndex        =   5
         Top             =   1980
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "CH - Channel"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   8
         Left            =   180
         TabIndex        =   4
         Top             =   1740
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "BD - Interface Rate"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   7
         Left            =   180
         TabIndex        =   3
         Top             =   1500
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "AP - API Enable"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   6
         Left            =   180
         TabIndex        =   2
         Top             =   1260
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "HV - Hardware Version"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   11
         Left            =   180
         TabIndex        =   7
         Top             =   2700
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                      .....:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   29
         Left            =   180
         TabIndex        =   44
         Top             =   2700
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                ...........:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   28
         Left            =   180
         TabIndex        =   43
         Top             =   1260
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                    .......:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   27
         Left            =   180
         TabIndex        =   42
         Top             =   1500
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "             ..............:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   26
         Left            =   180
         TabIndex        =   41
         Top             =   1740
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                      .....:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   25
         Left            =   180
         TabIndex        =   40
         Top             =   1980
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                     ......:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   24
         Left            =   180
         TabIndex        =   39
         Top             =   2220
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                 ..........:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   23
         Left            =   180
         TabIndex        =   38
         Top             =   2460
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                      .....:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   22
         Left            =   180
         TabIndex        =   37
         Top             =   2940
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                    .......:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   21
         Left            =   180
         TabIndex        =   36
         Top             =   540
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                     ......:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   18
         Left            =   180
         TabIndex        =   33
         Top             =   300
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "SL - 64-bit Address (Lo)"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   16
         Left            =   180
         TabIndex        =   23
         Top             =   1020
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "SH - 64-bit Address (Hi)"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   15
         Left            =   180
         TabIndex        =   21
         Top             =   780
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                         ..:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   20
         Left            =   180
         TabIndex        =   35
         Top             =   780
         Width           =   2955
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "                         ..:"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   255
         Index           =   19
         Left            =   180
         TabIndex        =   34
         Top             =   1020
         Width           =   2955
      End
   End
   Begin VB.Timer tmr_refresh 
      Enabled         =   0   'False
      Interval        =   500
      Left            =   3240
      Top             =   1380
   End
   Begin VB.Frame Frame3 
      Caption         =   " Node List "
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2775
      Left            =   180
      TabIndex        =   26
      Top             =   120
      Width           =   7215
      Begin VB.ListBox nodelist 
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   2085
         Left            =   180
         TabIndex        =   27
         Top             =   540
         Width           =   6855
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "16-bit"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   195
         Index           =   0
         Left            =   240
         TabIndex        =   32
         Top             =   300
         Width           =   675
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "RSSI"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   195
         Index           =   3
         Left            =   3660
         TabIndex        =   30
         Top             =   300
         Width           =   435
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "@"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   195
         Index           =   4
         Left            =   4440
         TabIndex        =   29
         Top             =   300
         Width           =   135
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "Node Name"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   195
         Index           =   5
         Left            =   4740
         TabIndex        =   28
         Top             =   300
         Width           =   915
      End
      Begin VB.Label Label 
         Alignment       =   2  'Center
         BackStyle       =   0  'Transparent
         Caption         =   "-        -"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C0C0C0&
         Height          =   195
         Index           =   2
         Left            =   1020
         TabIndex        =   48
         Top             =   300
         Width           =   2355
      End
      Begin VB.Label Label 
         Alignment       =   2  'Center
         BackStyle       =   0  'Transparent
         Caption         =   "(Hi)  64-bit  (Lo)"
         BeginProperty Font 
            Name            =   "Courier New"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   195
         Index           =   1
         Left            =   1020
         TabIndex        =   31
         Top             =   300
         Width           =   2355
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim dieNow As Boolean

Private Sub Form_Load()
    Me.Show
    DoEvents
    dieNow = False
    
    ' setup libxbee
    If (xbee_setupDebugAPI("COM8", 57600, "xbee.log", "+", 250) = -1) Then
        MsgBox "libxbee setup failed...", vbCritical
        Unload Me
        End
    End If
    
    ' enable callback functions
    xbee_enableCallbacks Me.hWnd
    
    ' setup a local at connection
    atcon = xbee_newcon_simple(Asc("A"), xbee_localAT)
    xbee_enableACKwait atcon
    xbee_attachCallback atcon, AddressOf localCB
    
    ' set off the chain reaction!
    xbeesend atcon, "MY"
End Sub

Private Sub Form_Unload(Cancel As Integer)
    Static c As Integer
    dieNow = 1
    Cancel = 1
    Me.Caption = "Waiting for command to complete..."
    c = c + 1
    If (c >= 2) Then
        Cancel = 0
    End If
End Sub

Private Sub nodelist_Click()
    Dim tmp() As String
    If nodelist.ListCount = 0 Or nodelist.ListIndex = -1 Then Exit Sub
    If set_dest.Tag = "yes" Then
        nodelist.Enabled = False
    Else
        If nodelist.ListIndex = 0 Then
            remoteCon = atcon
        Else
            str2 = Split(nodelist.text, "  ")
            remoteCon = xbee_newcon_64bit(Asc("2"), xbee_64bitRemoteAT, CLng("&H" & Right(str2(1), 8)), CLng("&H" & Right(str2(2), 8)))
        End If
        setButtons False
        nodelist.Enabled = False
        tmp = Split(nodelist.List(nodelist.ListIndex), "  ")
        ni = tmp(5)
        my = tmp(0)
        sh = tmp(1)
        sl = tmp(2)
        ap = "-"
        bd = "-"
        ch = "-"
        dh = "-"
        dl = "-"
        ia = "-"
        hv = "-"
        vr = "-"
    End If
End Sub

Private Sub reset_node_Click()
    If nodelist.ListCount = 0 Or nodelist.ListIndex = -1 Then Exit Sub
    nodelist.Enabled = False
    setButtons False
    reset_node.Tag = "yes"
End Sub

Private Sub set_default_Click()
    If nodelist.ListCount = 0 Or nodelist.ListIndex = -1 Then Exit Sub
    nodelist.Enabled = False
    setButtons False
    set_default.Tag = "yes"
End Sub

Private Sub set_dest_Click()
    If nodelist.ListCount = 0 Or nodelist.ListIndex = -1 Then Exit Sub
    nodelist.Tag = nodelist.ListIndex
    setButtons False
    set_dest.Tag = "yes"
End Sub

Private Sub set_ni_Click()
    Dim newni As String
    Dim oldni As String
    oldni = Split(nodelist.text, "  ")(5)
    newni = InputBox("New node identifier:", "Set Node Identifier", oldni)
    If newni = oldni Then Exit Sub
    nodelist.Enabled = False
    setButtons False
    set_ni.Tag = newni
End Sub

Private Sub talk_to_me_Click()
    If nodelist.ListCount = 0 Or nodelist.ListIndex = -1 Then Exit Sub
    nodelist.Enabled = False
    setButtons False
    talk_to_me.Tag = "yes"
End Sub

Private Sub tmr_refresh_Timer()
    Dim str As String
    Dim str2() As String
    tmr_refresh.Enabled = False
    If atcon = 0 Then Exit Sub
    
    If (dieNow) Then
        xbee_end
        DoEvents
        xbee_disableCallbacks
        Unload Me
        End
    End If
    
    If nodelist.Enabled = False Then
        xbee_attachCallback remoteCon, AddressOf remoteCB
        If talk_to_me.Tag = "yes" Then
            str2 = Split(Form1.nodelist.text, "  ")
            xbee_attachCallback remoteCon, AddressOf setupCB_TTM
            str2 = Split(nodelist.List(0), "  ")
            str = Chr(CInt("&H" & Mid(str2(1), 9, 2)))
            str = Chr(CInt("&H" & Mid(str2(1), 7, 2))) & str
            str = Chr(CInt("&H" & Mid(str2(1), 5, 2))) & str
            str = Chr(CInt("&H" & Mid(str2(1), 3, 2))) & str
            str = "DH" & str
            xbeesend remoteCon, str
        ElseIf set_dest.Tag = "yes" Then
            str2 = Split(Form1.nodelist.text, "  ")
            xbee_attachCallback remoteCon, AddressOf setupCB_SDEST
            str2 = Split(nodelist.text, "  ")
            str = Chr(CInt("&H" & Mid(str2(1), 9, 2)))
            str = Chr(CInt("&H" & Mid(str2(1), 7, 2))) & str
            str = Chr(CInt("&H" & Mid(str2(1), 5, 2))) & str
            str = Chr(CInt("&H" & Mid(str2(1), 3, 2))) & str
            str = "DH" & str
            xbeesend remoteCon, str
        ElseIf reset_node.Tag = "yes" Then
            xbee_sendstring remoteCon, "FR"
            setButtons True
            reset_node.Tag = ""
            tmr_refresh.Enabled = True
        ElseIf write_settings.Tag = "yes" Then
            xbee_sendstring remoteCon, "WR"
            setButtons True
            write_settings.Tag = ""
            tmr_refresh.Enabled = True
        ElseIf set_default.Tag = "yes" Then
            setupCB_Default_Start
        ElseIf set_ni.Tag <> "" Then
            xbeesend remoteCon, "NI" & set_ni.Tag
            set_ni.Tag = ""
        Else
            xbeesend remoteCon, "AP"
        End If
        Exit Sub
    End If
    ' initiate network scan
    xbee_attachCallback atcon, AddressOf localCB
    xbeesend atcon, "MY"
End Sub

Private Sub tmr_timeout_Timer()
    Dim con As Long
    Dim str As String
    Dim str2() As String
    tmr_timeout.Enabled = False
    str2 = Split(tmr_timeout.Tag, Chr(1), 2)
    con = CStr(str2(0))
    str = str2(1)
    If MsgBox("Request timed out... Retry?", vbYesNo + vbQuestion, "Retry?") = vbNo Then
        setButtons True
        nodelist.Enabled = True
        tmr_refresh.Enabled = True
        Exit Sub
    End If
    xbeesend con, str
End Sub

Private Sub write_settings_Click()
    If nodelist.ListCount = 0 Or nodelist.ListIndex = -1 Then Exit Sub
    nodelist.Enabled = False
    setButtons False
    write_settings.Tag = "yes"
End Sub
