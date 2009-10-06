#ifndef _avp_utilities_h_
#define _avp_utilities_h_

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

FILE *avp_fopen(const char *fileName, const char *mode);

#ifdef __cplusplus
};
#endif

#endif