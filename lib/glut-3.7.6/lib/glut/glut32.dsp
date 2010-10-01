# Microsoft Developer Studio Project File - Name="glut32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=glut32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "glut32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "glut32.mak" CFG="glut32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "glut32 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "glut32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "glut32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLUT32_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLUT32_EXPORTS" /U "GLUT_USE_SGI_OPENGL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib glu32.lib opengl32.lib /nologo /dll /machine:I386 /nodefaultlib:"glut32.lib"
# Begin Special Build Tool
TargetDir=.\Release
SOURCE="$(InputPath)"
PostBuild_Desc=Copying libraries, headers & dll's...
PostBuild_Cmds=copy $(TARGETDIR)\glut32.dll %WINDIR%\SYSTEM	copy $(TARGETDIR)\glut32.lib "$(MSDevDir)\..\..\VC98\lib"	copy ..\..\include\GL\glut.h "$(MSDevDir)\..\..\VC98\include\GL"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "glut32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLUT32_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLUT32_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib glu32.lib opengl32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"glut32.lib" /pdbtype:sept
# Begin Special Build Tool
TargetDir=.\Debug
SOURCE="$(InputPath)"
PostBuild_Desc=Copying libraries, headers & dll's...
PostBuild_Cmds=copy $(TARGETDIR)\glut32.dll %WINDIR%\SYSTEM32	copy $(TARGETDIR)\glut32.lib "$(MSDevDir)\..\..\VC98\lib"	copy ..\..\include\GL\glut.h "$(MSDevDir)\..\..\VC98\include\GL"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "glut32 - Win32 Release"
# Name "glut32 - Win32 Debug"
# Begin Source File

SOURCE=.\glut.def
# End Source File
# Begin Source File

SOURCE=..\..\include\Gl\glut.h
# End Source File
# Begin Source File

SOURCE=.\glut.ico
# End Source File
# Begin Source File

SOURCE=.\glut.rc
# End Source File
# Begin Source File

SOURCE=.\glut_8x13.c
# End Source File
# Begin Source File

SOURCE=.\glut_9x15.c
# End Source File
# Begin Source File

SOURCE=.\glut_bitmap.c
# End Source File
# Begin Source File

SOURCE=.\glut_bwidth.c
# End Source File
# Begin Source File

SOURCE=.\glut_cindex.c
# End Source File
# Begin Source File

SOURCE=.\glut_cmap.c
# End Source File
# Begin Source File

SOURCE=.\glut_cursor.c
# End Source File
# Begin Source File

SOURCE=.\glut_dials.c
# End Source File
# Begin Source File

SOURCE=.\glut_dstr.c
# End Source File
# Begin Source File

SOURCE=.\glut_event.c
# End Source File
# Begin Source File

SOURCE=.\glut_ext.c
# End Source File
# Begin Source File

SOURCE=.\glut_fcb.c
# End Source File
# Begin Source File

SOURCE=.\glut_fullscrn.c
# End Source File
# Begin Source File

SOURCE=.\glut_gamemode.c
# End Source File
# Begin Source File

SOURCE=.\glut_get.c
# End Source File
# Begin Source File

SOURCE=.\glut_glxext.c
# End Source File
# Begin Source File

SOURCE=.\glut_hel10.c
# End Source File
# Begin Source File

SOURCE=.\glut_hel12.c
# End Source File
# Begin Source File

SOURCE=.\glut_hel18.c
# End Source File
# Begin Source File

SOURCE=.\glut_init.c
# End Source File
# Begin Source File

SOURCE=.\glut_input.c
# End Source File
# Begin Source File

SOURCE=.\glut_joy.c
# End Source File
# Begin Source File

SOURCE=.\glut_key.c
# End Source File
# Begin Source File

SOURCE=.\glut_keyctrl.c
# End Source File
# Begin Source File

SOURCE=.\glut_keyup.c
# End Source File
# Begin Source File

SOURCE=.\glut_mesa.c
# End Source File
# Begin Source File

SOURCE=.\glut_modifier.c
# End Source File
# Begin Source File

SOURCE=.\glut_mroman.c
# End Source File
# Begin Source File

SOURCE=.\glut_overlay.c
# End Source File
# Begin Source File

SOURCE=.\glut_roman.c
# End Source File
# Begin Source File

SOURCE=.\glut_shapes.c
# End Source File
# Begin Source File

SOURCE=.\glut_space.c
# End Source File
# Begin Source File

SOURCE=.\glut_stroke.c
# End Source File
# Begin Source File

SOURCE=.\glut_swap.c
# End Source File
# Begin Source File

SOURCE=.\glut_swidth.c
# End Source File
# Begin Source File

SOURCE=.\glut_tablet.c
# End Source File
# Begin Source File

SOURCE=.\glut_teapot.c
# End Source File
# Begin Source File

SOURCE=.\glut_tr10.c
# End Source File
# Begin Source File

SOURCE=.\glut_tr24.c
# End Source File
# Begin Source File

SOURCE=.\glut_util.c
# End Source File
# Begin Source File

SOURCE=.\glut_vidresize.c
# End Source File
# Begin Source File

SOURCE=.\glut_warp.c
# End Source File
# Begin Source File

SOURCE=.\glut_win.c
# End Source File
# Begin Source File

SOURCE=.\glut_winmisc.c
# End Source File
# Begin Source File

SOURCE=.\glutbitmap.h
# End Source File
# Begin Source File

SOURCE=.\include\Gl\glutf90.h
# End Source File
# Begin Source File

SOURCE=.\glutint.h
# End Source File
# Begin Source File

SOURCE=.\glutstroke.h
# End Source File
# Begin Source File

SOURCE=.\glutwin32.h
# End Source File
# Begin Source File

SOURCE="..\..\README-win32.txt"
# End Source File
# Begin Source File

SOURCE=.\stroke.h
# End Source File
# Begin Source File

SOURCE=.\win32_glx.c
# End Source File
# Begin Source File

SOURCE=.\win32_glx.h
# End Source File
# Begin Source File

SOURCE=.\win32_menu.c
# End Source File
# Begin Source File

SOURCE=.\win32_util.c
# End Source File
# Begin Source File

SOURCE=.\win32_winproc.c
# End Source File
# Begin Source File

SOURCE=.\win32_x11.c
# End Source File
# Begin Source File

SOURCE=.\win32_x11.h
# End Source File
# End Target
# End Project
