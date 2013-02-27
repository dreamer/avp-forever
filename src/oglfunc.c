#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#include "oglfunc.h"

#if defined(_GL_STATIC)
void glWrapperPushAttrib( GLbitfield mask )
{
	// TODO.
}

void glWrapperPopAttrib( )
{
	// TODO.
}

void glWrapperPixelZoom( GLfloat xFactor, GLfloat yFactor )
{
	// TODO.
}

void glWrapperRasterPos2i( GLint x, GLint y )
{
	// TODO.
}

void glWrapperDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels )
{
	// TODO.
}

void glWrapperPushClientAttrib( GLbitfield mask)
{
	// TODO.
}

void glWrapperPopClientAttrib( )
{
	// TODO.
}

#else

PFNGLALPHAFUNCPROC		pglAlphaFunc;
PFNGLARRAYELEMENTPROC		pglArrayElement;
PFNGLBEGINPROC			pglBegin;
PFNGLBINDTEXTUREPROC		pglBindTexture;
PFNGLBLENDFUNCPROC		pglBlendFunc;
PFNGLCLEARPROC			pglClear;
PFNGLCLEARCOLORPROC		pglClearColor;
PFNGLCOLOR3FPROC		pglColor3f;
#if !defined(_PANDORA)
PFNGLCOLOR3FVPROC		pglColor3fv;
PFNGLCOLOR4FVPROC		pglColor4fv;
PFNGLTEXCOORD2FVPROC	pglTexCoord2fv;
PFNGLTEXCOORD3FPROC		pglTexCoord3f;
PFNGLTEXCOORD3FVPROC	pglTexCoord3fv;
PFNGLTEXCOORD4FPROC		pglTexCoord4f;
PFNGLTEXCOORD4FVPROC	pglTexCoord4fv;
PFNGLVERTEX2FPROC		pglVertex2f;
PFNGLVERTEX2FVPROC		pglVertex2fv;
PFNGLVERTEX4FPROC		pglVertex4f;
PFNGLVERTEX4FVPROC		pglVertex4fv;
PFNGLPOLYGONMODEPROC	pglPolygonMode;
PFNGLDRAWBUFFERPROC		pglDrawBuffer;
PFNGLDRAWRANGEELEMENTSPROC	pglDrawRangeElements;
PFNGLREADBUFFERPROC		pglReadBuffer;
PFNGLDEPTHRANGEPROC		pglDepthRange;
PFNGLVERTEX3FVPROC		pglVertex3fv;
#endif
PFNGLCOLOR3UBPROC		pglColor3ub;
PFNGLCOLOR3UBVPROC		pglColor3ubv;
PFNGLCOLOR4FPROC		pglColor4f;
PFNGLCOLOR4UBPROC		pglColor4ub;
PFNGLCOLOR4UBVPROC		pglColor4ubv;
PFNGLCOLORPOINTERPROC		pglColorPointer;
PFNGLCULLFACEPROC		pglCullFace;
PFNGLDELETETEXTURESPROC		pglDeleteTextures;
PFNGLDEPTHFUNCPROC		pglDepthFunc;
PFNGLDEPTHMASKPROC		pglDepthMask;
PFNGLDISABLEPROC		pglDisable;
PFNGLDISABLECLIENTSTATEPROC	pglDisableClientState;
PFNGLDRAWELEMENTSPROC		pglDrawElements;
PFNGLDRAWPIXELSPROC		pglDrawPixels;
PFNGLENABLEPROC			pglEnable;
PFNGLENABLECLIENTSTATEPROC	pglEnableClientState;
PFNGLENDPROC			pglEnd;
PFNGLFRONTFACEPROC		pglFrontFace;
PFNGLFRUSTUMPROC		pglFrustum;
PFNGLGENTEXTURESPROC		pglGenTextures;
PFNGLGETERRORPROC		pglGetError;
PFNGLGETFLOATVPROC		pglGetFloatv;
PFNGLGETINTEGERVPROC		pglGetIntegerv;
PFNGLGETSTRINGPROC		pglGetString;
PFNGLHINTPROC			pglHint;
PFNGLLOADIDENTITYPROC		pglLoadIdentity;
PFNGLLOADMATRIXFPROC		pglLoadMatrixf;
PFNGLMATRIXMODEPROC		pglMatrixMode;
PFNGLMULTMATRIXFPROC		pglMultMatrixf;
PFNGLNORMALPOINTERPROC		pglNormalPointer;
PFNGLORTHOPROC			pglOrtho;
PFNGLPIXELSTOREIPROC		pglPixelStorei;
PFNGLPIXELZOOMPROC		pglPixelZoom;
PFNGLPOLYGONOFFSETPROC		pglPolygonOffset;
PFNGLPOPATTRIBPROC		pglPopAttrib;
PFNGLPOPCLIENTATTRIBPROC	pglPopClientAttrib;
PFNGLPOPMATRIXPROC		pglPopMatrix;
PFNGLPUSHATTRIBPROC		pglPushAttrib;
PFNGLPUSHCLIENTATTRIBPROC	pglPushClientAttrib;
PFNGLPUSHMATRIXPROC		pglPushMatrix;
PFNGLRASTERPOS2IPROC		pglRasterPos2i;
PFNGLREADPIXELSPROC		pglReadPixels;
PFNGLROTATEFPROC		pglRotatef;
PFNGLSCALEFPROC			pglScalef;
PFNGLSHADEMODELPROC		pglShadeModel;
PFNGLTEXCOORD2FPROC		pglTexCoord2f;
PFNGLTEXCOORDPOINTERPROC	pglTexCoordPointer;
PFNGLTEXENVFPROC		pglTexEnvf;
PFNGLTEXENVFVPROC		pglTexEnvfv;
PFNGLTEXENVIPROC		pglTexEnvi;
PFNGLTEXIMAGE2DPROC		pglTexImage2D;
PFNGLTEXPARAMETERFPROC		pglTexParameterf;
PFNGLTEXPARAMETERIPROC		pglTexParameteri;
PFNGLTEXSUBIMAGE2DPROC		pglTexSubImage2D;
PFNGLTRANSLATEFPROC		pglTranslatef;
PFNGLVERTEX3FPROC		pglVertex3f;
PFNGLVERTEXPOINTERPROC		pglVertexPointer;
PFNGLVIEWPORTPROC		pglViewport;

// GL_EXT_paletted_texture
PFNGLCOLORTABLEEXTPROC			pglColorTableEXT;
PFNGLGETCOLORTABLEPARAMETERIVEXTPROC	pglGetColorTableParameterivEXT;

// GL_EXT_secondary_color
PFNGLSECONDARYCOLOR3FEXTPROC		pglSecondaryColor3fEXT;
PFNGLSECONDARYCOLOR3FVEXTPROC		pglSecondaryColor3fvEXT;
PFNGLSECONDARYCOLOR3UBEXTPROC		pglSecondaryColor3ubEXT;
PFNGLSECONDARYCOLOR3UBVEXTPROC		pglSecondaryColor3ubvEXT;
PFNGLSECONDARYCOLORPOINTEREXTPROC	pglSecondaryColorPointerEXT;

#endif

int ogl_have_paletted_texture;
int ogl_have_secondary_color;

int ogl_use_paletted_texture;
int ogl_use_secondary_color;

static void dummyfunc()
{
}

#define LoadOGLProc(type, func)						\
{									\
	if (!mode) p##func = (type) dummyfunc; else			\
	p##func = (type) SDL_GL_GetProcAddress(#func);			\
	if (p##func == NULL) {						\
		if (!ogl_missing_func) ogl_missing_func = #func;	\
	}								\
}

static int check_token(const char *string, const char *token)
{
	const char *s = string;
	int len = strlen(token);
	
	while ((s = strstr(s, token)) != NULL) {
		const char *next = s + len;
		
		if ((s == string || *(s-1) == ' ') &&
			(*next == 0 || *next == ' ')) {
			
			return 1;
		}
		
		s = next;
	}
	
	return 0;
}

void load_ogl_functions(int mode)
{
	const char* ogl_missing_func;
	const char* ext;

	ogl_missing_func = NULL;
	
#if defined(_GL_STATIC)

#else
	LoadOGLProc(PFNGLALPHAFUNCPROC, glAlphaFunc);
	LoadOGLProc(PFNGLARRAYELEMENTPROC, glArrayElement);
	LoadOGLProc(PFNGLBEGINPROC, glBegin);
	LoadOGLProc(PFNGLBINDTEXTUREPROC, glBindTexture);
	LoadOGLProc(PFNGLBLENDFUNCPROC, glBlendFunc);
	LoadOGLProc(PFNGLCLEARPROC, glClear);
	LoadOGLProc(PFNGLCLEARCOLORPROC, glClearColor);
	LoadOGLProc(PFNGLCOLOR3FPROC, glColor3f);
#if !defined(_PANDORA)
	LoadOGLProc(PFNGLCOLOR3FVPROC, glColor3fv);
	LoadOGLProc(PFNGLCOLOR4FVPROC, glColor4fv);
	LoadOGLProc(PFNGLTEXCOORD2FVPROC, glTexCoord2fv);
	LoadOGLProc(PFNGLTEXCOORD3FPROC, glTexCoord3f);
	LoadOGLProc(PFNGLTEXCOORD3FVPROC, glTexCoord3fv);
	LoadOGLProc(PFNGLTEXCOORD4FPROC, glTexCoord4f);
	LoadOGLProc(PFNGLTEXCOORD4FVPROC, glTexCoord4fv);
	LoadOGLProc(PFNGLVERTEX2FPROC, glVertex2f);
	LoadOGLProc(PFNGLVERTEX2FVPROC, glVertex2fv);
	LoadOGLProc(PFNGLVERTEX4FPROC, glVertex4f);
	LoadOGLProc(PFNGLVERTEX4FVPROC, glVertex4fv);
	LoadOGLProc(PFNGLDRAWBUFFERPROC, glDrawBuffer);
	LoadOGLProc(PFNGLPOLYGONMODEPROC, glPolygonMode);
	LoadOGLProc(PFNGLDRAWRANGEELEMENTSPROC, glDrawRangeElements);
	LoadOGLProc(PFNGLREADBUFFERPROC, glReadBuffer);
	LoadOGLProc(PFNGLDEPTHRANGEPROC, glDepthRange);
	LoadOGLProc(PFNGLVERTEX3FVPROC, glVertex3fv);
#endif
	LoadOGLProc(PFNGLCOLOR3UBPROC, glColor3ub);
	LoadOGLProc(PFNGLCOLOR3UBVPROC, glColor3ubv);
	LoadOGLProc(PFNGLCOLOR4FPROC, glColor4f);
	LoadOGLProc(PFNGLCOLOR4UBPROC, glColor4ub);
	LoadOGLProc(PFNGLCOLOR4UBVPROC, glColor4ubv);
	LoadOGLProc(PFNGLCOLORPOINTERPROC, glColorPointer);
	LoadOGLProc(PFNGLCULLFACEPROC, glCullFace);
	LoadOGLProc(PFNGLDELETETEXTURESPROC, glDeleteTextures);
	LoadOGLProc(PFNGLDEPTHFUNCPROC, glDepthFunc);
	LoadOGLProc(PFNGLDEPTHMASKPROC, glDepthMask);
	LoadOGLProc(PFNGLDISABLEPROC, glDisable);
	LoadOGLProc(PFNGLDISABLECLIENTSTATEPROC, glDisableClientState);
	LoadOGLProc(PFNGLDRAWELEMENTSPROC, glDrawElements);
	LoadOGLProc(PFNGLDRAWPIXELSPROC, glDrawPixels);
	LoadOGLProc(PFNGLENABLEPROC, glEnable);
	LoadOGLProc(PFNGLENABLECLIENTSTATEPROC, glEnableClientState);
	LoadOGLProc(PFNGLENDPROC, glEnd);
	LoadOGLProc(PFNGLFRONTFACEPROC, glFrontFace);
	LoadOGLProc(PFNGLFRUSTUMPROC, glFrustum);
	LoadOGLProc(PFNGLGENTEXTURESPROC, glGenTextures);
	LoadOGLProc(PFNGLGETERRORPROC, glGetError);
	LoadOGLProc(PFNGLGETFLOATVPROC, glGetFloatv);
	LoadOGLProc(PFNGLGETINTEGERVPROC, glGetIntegerv);
	LoadOGLProc(PFNGLGETSTRINGPROC, glGetString);
	LoadOGLProc(PFNGLHINTPROC, glHint);
	LoadOGLProc(PFNGLLOADIDENTITYPROC, glLoadIdentity);
	LoadOGLProc(PFNGLLOADMATRIXFPROC, glLoadMatrixf);
	LoadOGLProc(PFNGLMATRIXMODEPROC, glMatrixMode);
	LoadOGLProc(PFNGLMULTMATRIXFPROC, glMultMatrixf);
	LoadOGLProc(PFNGLNORMALPOINTERPROC, glNormalPointer);
	LoadOGLProc(PFNGLORTHOPROC, glOrtho);
	LoadOGLProc(PFNGLPIXELSTOREIPROC, glPixelStorei);
	LoadOGLProc(PFNGLPIXELZOOMPROC, glPixelZoom);
	LoadOGLProc(PFNGLPOLYGONOFFSETPROC, glPolygonOffset);
	LoadOGLProc(PFNGLPOPATTRIBPROC, glPopAttrib);
	LoadOGLProc(PFNGLPOPCLIENTATTRIBPROC, glPopClientAttrib);
	LoadOGLProc(PFNGLPOPMATRIXPROC, glPopMatrix);
	LoadOGLProc(PFNGLPUSHATTRIBPROC, glPushAttrib);
	LoadOGLProc(PFNGLPUSHCLIENTATTRIBPROC, glPushClientAttrib);
	LoadOGLProc(PFNGLPUSHMATRIXPROC, glPushMatrix);
	LoadOGLProc(PFNGLRASTERPOS2IPROC, glRasterPos2i);
	LoadOGLProc(PFNGLREADPIXELSPROC, glReadPixels);
	LoadOGLProc(PFNGLROTATEFPROC, glRotatef);
	LoadOGLProc(PFNGLSCALEFPROC, glScalef);
	LoadOGLProc(PFNGLSHADEMODELPROC, glShadeModel);
	LoadOGLProc(PFNGLTEXCOORD2FPROC, glTexCoord2f);
	LoadOGLProc(PFNGLTEXCOORDPOINTERPROC, glTexCoordPointer);
	LoadOGLProc(PFNGLTEXENVFPROC, glTexEnvf);
	LoadOGLProc(PFNGLTEXENVFVPROC, glTexEnvfv);
	LoadOGLProc(PFNGLTEXENVIPROC, glTexEnvi);
	LoadOGLProc(PFNGLTEXIMAGE2DPROC, glTexImage2D);
	LoadOGLProc(PFNGLTEXPARAMETERFPROC, glTexParameterf);
	LoadOGLProc(PFNGLTEXPARAMETERIPROC, glTexParameteri);
	LoadOGLProc(PFNGLTEXSUBIMAGE2DPROC, glTexSubImage2D);
	LoadOGLProc(PFNGLTRANSLATEFPROC, glTranslatef);
	LoadOGLProc(PFNGLVERTEX3FPROC, glVertex3f);
	LoadOGLProc(PFNGLVERTEXPOINTERPROC, glVertexPointer);
	LoadOGLProc(PFNGLVIEWPORTPROC, glViewport);

#endif

	if (!mode) {
		ogl_have_paletted_texture = 0;
		ogl_have_secondary_color = 0;
		
		ogl_use_paletted_texture = 0;
		ogl_use_secondary_color = 0;
		
		return;
	}
	
	if (ogl_missing_func) {
		fprintf(stderr, "Unable to load OpenGL Library: missing function %s\n", ogl_missing_func);
		exit(EXIT_FAILURE);
	}
	
	ext = (const char *) pglGetString(GL_EXTENSIONS);

	ogl_have_paletted_texture = check_token(ext, "GL_EXT_paletted_texture");	
	ogl_have_secondary_color = check_token(ext, "GL_EXT_secondary_color");

#ifndef GL_COLOR_TABLE_WIDTH_EXT
#define GL_COLOR_TABLE_WIDTH_EXT	GL_COLOR_TABLE_WIDTH
#endif
	
#if !defined(_PANDORA)
	if (ogl_have_paletted_texture) {
		ogl_missing_func = NULL;
		
		LoadOGLProc(PFNGLCOLORTABLEEXTPROC, glColorTableEXT);
		LoadOGLProc(PFNGLGETCOLORTABLEPARAMETERIVEXTPROC, glGetColorTableParameterivEXT);
		
		if (!ogl_missing_func) {	
			GLint size;
			
			pglColorTableEXT(GL_PROXY_TEXTURE_2D, GL_RGBA, 256, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
			pglGetColorTableParameterivEXT(GL_PROXY_TEXTURE_2D, GL_COLOR_TABLE_WIDTH_EXT, &size);
			
			if (size != 256) {
				ogl_have_paletted_texture = 0;
			}	
		} else {
			ogl_have_paletted_texture = 0;
		}
	}
#endif
	
#if defined(_GL_STATIC)
	ogl_have_secondary_color = 0;
#else
	if (ogl_have_secondary_color) {
		ogl_missing_func = NULL;

		LoadOGLProc(PFNGLSECONDARYCOLOR3FEXTPROC, glSecondaryColor3fEXT);
		LoadOGLProc(PFNGLSECONDARYCOLOR3FVEXTPROC, glSecondaryColor3fvEXT);
		LoadOGLProc(PFNGLSECONDARYCOLOR3UBEXTPROC, glSecondaryColor3ubEXT);
		LoadOGLProc(PFNGLSECONDARYCOLOR3UBVEXTPROC, glSecondaryColor3ubvEXT);
		LoadOGLProc(PFNGLSECONDARYCOLORPOINTEREXTPROC, glSecondaryColorPointerEXT);

		if (ogl_missing_func) {
			ogl_have_secondary_color = 0;
		}
	}

#endif
	
	ogl_use_paletted_texture = ogl_have_paletted_texture;
	ogl_use_secondary_color = ogl_have_secondary_color;

	// fprintf(stderr, "RENDER DEBUG: pal:%d sec:%d\n", ogl_use_paletted_texture, ogl_use_secondary_color);
}

int check_for_errors(const char *file, int line)
{
	GLenum error;
	int diderror = 0;
	
	while ((error = pglGetError()) != GL_NO_ERROR) {
		fprintf(stderr, "OPENGL ERROR: %04X (%s:%d)\n", error, file, line);
		
		diderror = 1;
	}
	
	return diderror;
}
