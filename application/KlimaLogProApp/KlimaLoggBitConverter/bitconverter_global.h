#ifndef BITCONVERTER_GLOBAL_H
#define BITCONVERTER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(BITCONVERTER_LIBRARY)
#  define BITCONVERTERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define BITCONVERTERSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // BITCONVERTER_GLOBAL_H