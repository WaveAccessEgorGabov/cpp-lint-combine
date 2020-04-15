#ifndef __LINTWRAPPER_LINTWRAPPERUTILS_H__
#define __LINTWRAPPER_LINTWRAPPERUTILS_H__

#include <string>
#include "LinterWrapperBase.h"

static LinterWrapperBase * linterWrapperFactory( const std::string & linterName, const std::string & linterOptions,
                                                 const std::string & yamlFilePath );

LinterWrapperBase * parseCommandLine( int argc, char ** argv );

#endif //__LINTWRAPPER_LINTWRAPPERUTILS_H__
