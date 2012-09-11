#-- uncomment this to enable debugging
#DEBUG:=/Zi /DDEBUG /MTd
#LDBUG:=/DEBUG

#-- you may need to edit these lines if your installation is different
VCPath:=C:\Program Files\Microsoft Visual Studio 10.0\VC
SDKPath:=C:\Program Files\Microsoft SDKs\Windows\v7.1

#!! if using visual studio 2010, you may need to run the following in a shell,
#     and then within the same shell run `make -f win32.makefile`
#   C:\Program Files\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat

###### YOU SHOULD NOT CHANGE BELOW THIS LINE ######
SHELL:=cmd
DEBUG?=/MT

SRCS:=api.c

CC:="${VCPath}\bin\cl.exe"
LINK:="${VCPath}\bin\link.exe"
RC:="${SDKPath}\bin\rc.exe"

.PHONY: all new clean 

all: .\lib\libxbee.dll

new: clean all

clean:
	-rmdir /Q /S lib
	-rmdir /Q /S obj

.\obj:
	mkdir obj

.\lib:
	mkdir lib  

.\lib\libxbee.dll: .\lib .\obj\api.obj .\obj\win32.res
	${LINK} ${LDBUG} /nologo /DLL /MAP:lib\libxbee.map /DEF:xsys\win32.def \
		"/LIBPATH:${SDKPath}\Lib" "/LIBPATH:${VCPath}\lib" \
		/OUT:.\lib\libxbee.dll .\obj\api.obj .\obj\win32.res

.\obj\api.obj: .\obj api.c api.h xbee.h
	${CC} ${DEBUG} /nologo "/I${SDKPath}\Include" "/I${VCPath}\include" /RTCs /Gz /c /Fd.\lib\libxbee.pdb /Fo.\obj\api.obj ${SRCS}

.\obj\win32.res: .\xsys\win32.rc
	${RC} "/I${SDKPath}\Include" "/I${VCPath}\include" /n /fo.\obj\win32.res .\xsys\win32.rc 
