# Microsoft Developer Studio Project File - Name="glsmap" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=glsmap - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "glsmap.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "glsmap.mak" CFG="glsmap - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "glsmap - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "glsmap - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "glsmap - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "glsmap - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "glsmap - Win32 Release"
# Name "glsmap - Win32 Debug"
# Begin Source File

SOURCE=.\glsmapint.h
# End Source File
# Begin Source File

SOURCE=.\smap_buildsmap.c
# End Source File
# Begin Source File

SOURCE=.\smap_context.c
# End Source File
# Begin Source File

SOURCE=.\smap_create.c
# End Source File
# Begin Source File

SOURCE=.\smap_destroy.c
# End Source File
# Begin Source File

SOURCE=.\smap_drawmesh.c
# End Source File
# Begin Source File

SOURCE=.\smap_flag.c
# End Source File
# Begin Source File

SOURCE=.\smap_get.c
# End Source File
# Begin Source File

SOURCE=.\smap_getfunc.c
# End Source File
# Begin Source File

SOURCE=.\smap_gettexdim.c
# End Source File
# Begin Source File

SOURCE=.\smap_gettexobj.c
# End Source File
# Begin Source File

SOURCE=.\smap_getvec.c
# End Source File
# Begin Source File

SOURCE=.\smap_makemesh.c
# End Source File
# Begin Source File

SOURCE=.\smap_nearfar.c
# End Source File
# Begin Source File

SOURCE=.\smap_origin.c
# End Source File
# Begin Source File

SOURCE=.\smap_render.c
# End Source File
# Begin Source File

SOURCE=.\smap_rvec2st.c
# End Source File
# Begin Source File

SOURCE=.\smap_set.c
# End Source File
# Begin Source File

SOURCE=.\smap_setfunc.c
# End Source File
# Begin Source File

SOURCE=.\smap_setvec.c
# End Source File
# Begin Source File

SOURCE=.\smap_texdim.c
# End Source File
# Begin Source File

SOURCE=.\smap_texobj.c
# End Source File
# End Target
# End Project
