#pragma once

#include <QtGlobal>

#if defined(QGCPLUGIN_LIBRARY)
#define QGCPLUGIN_EXPORT Q_DECL_EXPORT
#else
#define QGCPLUGIN_EXPORT Q_DECL_IMPORT
#endif
