/*
JH - 18/02/98
Deal with lost surfaces and textures - restore them when the application is re-activated
*/

#ifndef _INCLUDED_ALT_TAB_H_
#define _INCLUDED_ALT_TAB_H_

#include "aw.h"

#ifdef __cplusplus
	extern "C" {
#endif

typedef void (* AT_PFN_RESTORETEXTURE) (AVPTEXTURE * pTexture, void * pUser);

#ifdef NDEBUG
	extern void ATIncludeTexture(AVPTEXTURE * pTexture, AW_BACKUPTEXTUREHANDLE hBackup);
	extern void ATIncludeTextureEx(AVPTEXTURE * pTexture, AT_PFN_RESTORETEXTURE pfnRestore, void * pUser);
#else
	extern void _ATIncludeTexture(AVPTEXTURE * pTexture, AW_BACKUPTEXTUREHANDLE hBackup, char const * pszFile, unsigned nLine, char const * pszDebugString);
	extern void _ATIncludeTextureEx(AVPTEXTURE * pTexture, AT_PFN_RESTORETEXTURE pfnRestore, void * pUser, char const * pszFile, unsigned nLine, char const * pszFuncName, char const * pszDebugString);
	#define ATIncludeTexture(p,h) _ATIncludeTexture(p,h,__FILE__,__LINE__,NULL)
	#define ATIncludeTextureEx(p,f,u) _ATIncludeTextureEx(p,f,u,__FILE__,__LINE__,#f ,NULL)
	#define ATIncludeTextureDb(p,h,d) _ATIncludeTexture(p,h,__FILE__,__LINE__,d)
	#define ATIncludeTextureExDb(p,f,u,d) _ATIncludeTextureEx(p,f,u,__FILE__,__LINE__,#f ,d)
#endif

extern void ATRemoveTexture(AVPTEXTURE * pTexture);

extern void ATOnAppReactivate();

#ifdef __cplusplus
	}
#endif

#endif /* ! _INCLUDED_ALT_TAB_H_ */
