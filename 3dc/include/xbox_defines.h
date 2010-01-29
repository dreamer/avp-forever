#ifndef _XBOX_DEFINES_H_
#define _XBOX_DEFINES_H_

#ifdef __cplusplus
extern "C" { 
#endif

typedef int DPID;
#define D3DLOCK_DISCARD	0
#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox

#ifdef __cplusplus
};
#endif

#endif // #define _XBOX_DEFINES_H_