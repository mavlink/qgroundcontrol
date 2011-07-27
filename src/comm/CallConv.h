#ifndef _CALLCONV_H_
#define _CALLCONV_H_

#ifdef Q_OS_WIN
#define CALLTYPEXBEE __stdcall
#else // Q_OS_WIN
#define CALLTYPEXBEE
#endif // Q_OS_WIN


#endif // _CALLCONV_H_