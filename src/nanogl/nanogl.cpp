/*
Copyright (C) 2007-2008 Olli Hinkka

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//#include <e32std.h>

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "nanogl.h"
#include "glesinterface.h"
#include "gl.h"

void * glesLib = NULL;
GlESInterface* glEsImpl = NULL;
static const int KGLEsFunctionCount = 175;

extern void InitGLStructs();

int CreateGlEsInterface()
{
	glEsImpl = (GlESInterface *)calloc(1, sizeof(GlESInterface));
	//glEsImpl = new GlESInterface;
	if (glEsImpl == NULL)
	{
		return -1;
	}

	printf("Looking up GLES symbols...\n");
	glEsImpl->eglChooseConfig = (int (*)(int, const int *, int *, int, int *)) dlsym(glesLib, "eglChooseConfig");
	if(glEsImpl->eglChooseConfig == NULL) { printf("Error on eglChooseConfig: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglCopyBuffers = (int (*)(int, int, void*)) dlsym(glesLib, "eglCopyBuffers");
	if(glEsImpl->eglCopyBuffers == NULL) { printf("Error on eglCopyBuffers: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglCreateContext = (int (*)(int, int, int, const int *)) dlsym(glesLib, "eglCreateContext");
	if(glEsImpl->eglCreateContext == NULL) { printf("Error on eglCreateContext: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglCreatePbufferSurface = (int (*)(int, int, const int *)) dlsym(glesLib, "eglCreatePbufferSurface");
	if(glEsImpl->eglCreatePbufferSurface == NULL) { printf("Error on eglCreatePbufferSurface: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglCreatePixmapSurface = (int (*)(int, int, void*, const int *)) dlsym(glesLib, "eglCreatePixmapSurface");
	if(glEsImpl->eglCreatePixmapSurface == NULL) { printf("Error on eglCreatePixmapSurface: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglCreateWindowSurface = (int (*)(int, int, void*, const int *)) dlsym(glesLib, "eglCreateWindowSurface");
	if(glEsImpl->eglCreateWindowSurface == NULL) { printf("Error on eglCreateWindowSurface: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglDestroyContext = (int (*)(int, int)) dlsym(glesLib, "eglDestroyContext");
	if(glEsImpl->eglDestroyContext == NULL) { printf("Error on eglDestroyContext: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglDestroySurface = (int (*)(int, int)) dlsym(glesLib, "eglDestroySurface");
	if(glEsImpl->eglDestroySurface == NULL) { printf("Error on eglDestroySurface: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglGetConfigAttrib = (int (*)(int, int, int, int *)) dlsym(glesLib, "eglGetConfigAttrib");
	if(glEsImpl->eglGetConfigAttrib == NULL) { printf("Error on eglGetConfigAttrib: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglGetConfigs = (int (*)(int, int *, int, int *)) dlsym(glesLib, "eglGetConfigs");
	if(glEsImpl->eglGetConfigs == NULL) { printf("Error on eglGetConfigs: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglGetCurrentContext = (int (*)(void)) dlsym(glesLib, "eglGetCurrentContext");
	if(glEsImpl->eglGetCurrentContext == NULL) { printf("Error on eglGetCurrentContext: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglGetCurrentDisplay = (int (*)(void)) dlsym(glesLib, "eglGetCurrentDisplay");
	if(glEsImpl->eglGetCurrentDisplay == NULL) { printf("Error on eglGetCurrentDisplay: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglGetCurrentSurface = (int (*)(int)) dlsym(glesLib, "eglGetCurrentSurface");
	if(glEsImpl->eglGetCurrentSurface == NULL) { printf("Error on eglGetCurrentSurface: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglGetDisplay = (int (*)(int)) dlsym(glesLib, "eglGetDisplay");
	if(glEsImpl->eglGetDisplay == NULL) { printf("Error on eglGetDisplay: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglGetError = (int (*)(void)) dlsym(glesLib, "eglGetError");
	if(glEsImpl->eglGetError == NULL) { printf("Error on eglGetError: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglGetProcAddress = (void (*(*) (const char *))(...)) dlsym(glesLib, "eglGetProcAddress");
	if(glEsImpl->eglGetProcAddress == NULL) { printf("Error on eglGetProcAddress: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglInitialize = (int (*)(int, int *, int *)) dlsym(glesLib, "eglInitialize");
	if(glEsImpl->eglInitialize == NULL) { printf("Error on eglInitialize: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglMakeCurrent = (int (*)(int, int, int, int)) dlsym(glesLib, "eglMakeCurrent");
	if(glEsImpl->eglMakeCurrent == NULL) { printf("Error on eglMakeCurrent: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglQueryContext = (int (*)(int, int, int, int *)) dlsym(glesLib, "eglQueryContext");
	if(glEsImpl->eglQueryContext == NULL) { printf("Error on eglQueryContext: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglQueryString = (const char * (*)(int, int)) dlsym(glesLib, "eglQueryString");
	if(glEsImpl->eglQueryString == NULL) { printf("Error on eglQueryString: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglQuerySurface = (int (*)(int, int, int, int *)) dlsym(glesLib, "eglQuerySurface");
	if(glEsImpl->eglQuerySurface == NULL) { printf("Error on eglQuerySurface: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglSwapBuffers = (int (*)(int, int)) dlsym(glesLib, "eglSwapBuffers");
	if(glEsImpl->eglSwapBuffers == NULL) { printf("Error on eglSwapBuffers: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglTerminate = (int (*)(int)) dlsym(glesLib, "eglTerminate");
	if(glEsImpl->eglTerminate == NULL) { printf("Error on eglTerminate: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglWaitGL = (int (*)(void)) dlsym(glesLib, "eglWaitGL");
	if(glEsImpl->eglWaitGL == NULL) { printf("Error on eglWaitGL: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglWaitNative = (int (*)(int)) dlsym(glesLib, "eglWaitNative");
	if(glEsImpl->eglWaitNative == NULL) { printf("Error on eglWaitNative: (%s)\n", dlerror()); return -1; }
	glEsImpl->glActiveTexture = (void (*)(unsigned int)) dlsym(glesLib, "glActiveTexture");
	if(glEsImpl->glActiveTexture == NULL) { printf("Error on glActiveTexture: (%s)\n", dlerror()); return -1; }
	glEsImpl->glAlphaFunc = (void (*)(unsigned int, float)) dlsym(glesLib, "glAlphaFunc");
	if(glEsImpl->glAlphaFunc == NULL) { printf("Error on glAlphaFunc: (%s)\n", dlerror()); return -1; }
	glEsImpl->glAlphaFuncx = (void (*)(unsigned int, int)) dlsym(glesLib, "glAlphaFuncx");
	if(glEsImpl->glAlphaFuncx == NULL) { printf("Error on glAlphaFuncx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glBindTexture = (void (*)(unsigned int, unsigned int)) dlsym(glesLib, "glBindTexture");
	if(glEsImpl->glBindTexture == NULL) { printf("Error on glBindTexture: (%s)\n", dlerror()); return -1; }
	glEsImpl->glBlendFunc = (void (*)(unsigned int, unsigned int)) dlsym(glesLib, "glBlendFunc");
	if(glEsImpl->glBlendFunc == NULL) { printf("Error on glBlendFunc: (%s)\n", dlerror()); return -1; }
	glEsImpl->glClear = (void (*)(unsigned int)) dlsym(glesLib, "glClear");
	if(glEsImpl->glClear == NULL) { printf("Error on glClear: (%s)\n", dlerror()); return -1; }
	glEsImpl->glClearColor = (void (*)(float, float, float, float)) dlsym(glesLib, "glClearColor");
	if(glEsImpl->glClearColor == NULL) { printf("Error on glClearColor: (%s)\n", dlerror()); return -1; }
	glEsImpl->glClearColorx = (void (*)(int, int, int, int)) dlsym(glesLib, "glClearColorx");
	if(glEsImpl->glClearColorx == NULL) { printf("Error on glClearColorx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glClearDepthf = (void (*)(float)) dlsym(glesLib, "glClearDepthf");
	if(glEsImpl->glClearDepthf == NULL) { printf("Error on glClearDepthf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glClearDepthx = (void (*)(int)) dlsym(glesLib, "glClearDepthx");
	if(glEsImpl->glClearDepthx == NULL) { printf("Error on glClearDepthx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glClearStencil = (void (*)(int)) dlsym(glesLib, "glClearStencil");
	if(glEsImpl->glClearStencil == NULL) { printf("Error on glClearStencil: (%s)\n", dlerror()); return -1; }
	glEsImpl->glClientActiveTexture = (void (*)(unsigned int)) dlsym(glesLib, "glClientActiveTexture");
	if(glEsImpl->glClientActiveTexture == NULL) { printf("Error on glClientActiveTexture: (%s)\n", dlerror()); return -1; }
	glEsImpl->glColor4f = (void (*)(float, float, float, float)) dlsym(glesLib, "glColor4f");
	if(glEsImpl->glColor4f == NULL) { printf("Error on glColor4f: (%s)\n", dlerror()); return -1; }
	glEsImpl->glColor4x = (void (*)(int, int, int, int)) dlsym(glesLib, "glColor4x");
	if(glEsImpl->glColor4x == NULL) { printf("Error on glColor4x: (%s)\n", dlerror()); return -1; }
	glEsImpl->glColorMask = (void (*)(unsigned char, unsigned char, unsigned char, unsigned char)) dlsym(glesLib, "glColorMask");
	if(glEsImpl->glColorMask == NULL) { printf("Error on glColorMask: (%s)\n", dlerror()); return -1; }
	glEsImpl->glColorPointer = (void (*)(int, unsigned int, int, const void *)) dlsym(glesLib, "glColorPointer");
	if(glEsImpl->glColorPointer == NULL) { printf("Error on glColorPointer: (%s)\n", dlerror()); return -1; }
	glEsImpl->glCompressedTexImage2D = (void (*)(unsigned int, int, unsigned int, int, int, int, int, const void *)) dlsym(glesLib, "glCompressedTexImage2D");
	if(glEsImpl->glCompressedTexImage2D == NULL) { printf("Error on glCompressedTexImage2D: (%s)\n", dlerror()); return -1; }
	glEsImpl->glCompressedTexSubImage2D = (void (*)(unsigned int, int, int, int, int, int, unsigned int, int, const void *)) dlsym(glesLib, "glCompressedTexSubImage2D");
	if(glEsImpl->glCompressedTexSubImage2D == NULL) { printf("Error on glCompressedTexSubImage2D: (%s)\n", dlerror()); return -1; }
	glEsImpl->glCopyTexImage2D = (void (*)(unsigned int, int, unsigned int, int, int, int, int, int)) dlsym(glesLib, "glCopyTexImage2D");
	if(glEsImpl->glCopyTexImage2D == NULL) { printf("Error on glCopyTexImage2D: (%s)\n", dlerror()); return -1; }
	glEsImpl->glCopyTexSubImage2D = (void (*)(unsigned int, int, int, int, int, int, int, int)) dlsym(glesLib, "glCopyTexSubImage2D");
	if(glEsImpl->glCopyTexSubImage2D == NULL) { printf("Error on glCopyTexSubImage2D: (%s)\n", dlerror()); return -1; }
	glEsImpl->glCullFace = (void (*)(unsigned int)) dlsym(glesLib, "glCullFace");
	if(glEsImpl->glCullFace == NULL) { printf("Error on glCullFace: (%s)\n", dlerror()); return -1; }
	glEsImpl->glDeleteTextures = (void (*)(int, const unsigned int *)) dlsym(glesLib, "glDeleteTextures");
	if(glEsImpl->glDeleteTextures == NULL) { printf("Error on glDeleteTextures: (%s)\n", dlerror()); return -1; }
	glEsImpl->glDepthFunc = (void (*)(unsigned int)) dlsym(glesLib, "glDepthFunc");
	if(glEsImpl->glDepthFunc == NULL) { printf("Error on glDepthFunc: (%s)\n", dlerror()); return -1; }
	glEsImpl->glDepthMask = (void (*)(unsigned char)) dlsym(glesLib, "glDepthMask");
	if(glEsImpl->glDepthMask == NULL) { printf("Error on glDepthMask: (%s)\n", dlerror()); return -1; }
	glEsImpl->glDepthRangef = (void (*)(float, float)) dlsym(glesLib, "glDepthRangef");
	if(glEsImpl->glDepthRangef == NULL) { printf("Error on glDepthRangef: (%s)\n", dlerror()); return -1; }
	glEsImpl->glDepthRangex = (void (*)(int, int)) dlsym(glesLib, "glDepthRangex");
	if(glEsImpl->glDepthRangex == NULL) { printf("Error on glDepthRangex: (%s)\n", dlerror()); return -1; }
	glEsImpl->glDisable = (void (*)(unsigned int)) dlsym(glesLib, "glDisable");
	if(glEsImpl->glDisable == NULL) { printf("Error on glDisable: (%s)\n", dlerror()); return -1; }
	glEsImpl->glDisableClientState = (void (*)(unsigned int)) dlsym(glesLib, "glDisableClientState");
	if(glEsImpl->glDisableClientState == NULL) { printf("Error on glDisableClientState: (%s)\n", dlerror()); return -1; }
	glEsImpl->glDrawArrays = (void (*)(unsigned int, int, int)) dlsym(glesLib, "glDrawArrays");
	if(glEsImpl->glDrawArrays == NULL) { printf("Error on glDrawArrays: (%s)\n", dlerror()); return -1; }
	glEsImpl->glDrawElements = (void (*)(unsigned int, int, unsigned int, const void *)) dlsym(glesLib, "glDrawElements");
	if(glEsImpl->glDrawElements == NULL) { printf("Error on glDrawElements: (%s)\n", dlerror()); return -1; }
	glEsImpl->glEnable = (void (*)(unsigned int)) dlsym(glesLib, "glEnable");
	if(glEsImpl->glEnable == NULL) { printf("Error on glEnable: (%s)\n", dlerror()); return -1; }
	glEsImpl->glEnableClientState = (void (*)(unsigned int)) dlsym(glesLib, "glEnableClientState");
	if(glEsImpl->glEnableClientState == NULL) { printf("Error on glEnableClientState: (%s)\n", dlerror()); return -1; }
	glEsImpl->glFinish = (void (*)(void)) dlsym(glesLib, "glFinish");
	if(glEsImpl->glFinish == NULL) { printf("Error on glFinish: (%s)\n", dlerror()); return -1; }
	glEsImpl->glFlush = (void (*)(void)) dlsym(glesLib, "glFlush");
	if(glEsImpl->glFlush == NULL) { printf("Error on glFlush: (%s)\n", dlerror()); return -1; }
	glEsImpl->glFogf = (void (*)(unsigned int, float)) dlsym(glesLib, "glFogf");
	if(glEsImpl->glFogf == NULL) { printf("Error on glFogf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glFogfv = (void (*)(unsigned int, const float *)) dlsym(glesLib, "glFogfv");
	if(glEsImpl->glFogfv == NULL) { printf("Error on glFogfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glFogx = (void (*)(unsigned int, int)) dlsym(glesLib, "glFogx");
	if(glEsImpl->glFogx == NULL) { printf("Error on glFogx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glFogxv = (void (*)(unsigned int, const int *)) dlsym(glesLib, "glFogxv");
	if(glEsImpl->glFogxv == NULL) { printf("Error on glFogxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glFrontFace = (void (*)(unsigned int)) dlsym(glesLib, "glFrontFace");
	if(glEsImpl->glFrontFace == NULL) { printf("Error on glFrontFace: (%s)\n", dlerror()); return -1; }
	glEsImpl->glFrustumf = (void (*)(float, float, float, float, float, float)) dlsym(glesLib, "glFrustumf");
	if(glEsImpl->glFrustumf == NULL) { printf("Error on glFrustumf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glFrustumx = (void (*)(int, int, int, int, int, int)) dlsym(glesLib, "glFrustumx");
	if(glEsImpl->glFrustumx == NULL) { printf("Error on glFrustumx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGenTextures = (void (*)(int, unsigned int *)) dlsym(glesLib, "glGenTextures");
	if(glEsImpl->glGenTextures == NULL) { printf("Error on glGenTextures: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetError = (unsigned int (*)(void)) dlsym(glesLib, "glGetError");
	if(glEsImpl->glGetError == NULL) { printf("Error on glGetError: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetIntegerv = (void (*)(unsigned int, int *)) dlsym(glesLib, "glGetIntegerv");
	if(glEsImpl->glGetIntegerv == NULL) { printf("Error on glGetIntegerv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetString = (const unsigned char * (*)(unsigned int)) dlsym(glesLib, "glGetString");
	if(glEsImpl->glGetString == NULL) { printf("Error on glGetString: (%s)\n", dlerror()); return -1; }
	glEsImpl->glHint = (void (*)(unsigned int, unsigned int)) dlsym(glesLib, "glHint");
	if(glEsImpl->glHint == NULL) { printf("Error on glHint: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLightModelf = (void (*)(unsigned int, float)) dlsym(glesLib, "glLightModelf");
	if(glEsImpl->glLightModelf == NULL) { printf("Error on glLightModelf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLightModelfv = (void (*)(unsigned int, const float *)) dlsym(glesLib, "glLightModelfv");
	if(glEsImpl->glLightModelfv == NULL) { printf("Error on glLightModelfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLightModelx = (void (*)(unsigned int, int)) dlsym(glesLib, "glLightModelx");
	if(glEsImpl->glLightModelx == NULL) { printf("Error on glLightModelx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLightModelxv = (void (*)(unsigned int, const int *)) dlsym(glesLib, "glLightModelxv");
	if(glEsImpl->glLightModelxv == NULL) { printf("Error on glLightModelxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLightf = (void (*)(unsigned int, unsigned int, float)) dlsym(glesLib, "glLightf");
	if(glEsImpl->glLightf == NULL) { printf("Error on glLightf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLightfv = (void (*)(unsigned int, unsigned int, const float *)) dlsym(glesLib, "glLightfv");
	if(glEsImpl->glLightfv == NULL) { printf("Error on glLightfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLightx = (void (*)(unsigned int, unsigned int, int)) dlsym(glesLib, "glLightx");
	if(glEsImpl->glLightx == NULL) { printf("Error on glLightx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLightxv = (void (*)(unsigned int, unsigned int, const int *)) dlsym(glesLib, "glLightxv");
	if(glEsImpl->glLightxv == NULL) { printf("Error on glLightxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLineWidth = (void (*)(float)) dlsym(glesLib, "glLineWidth");
	if(glEsImpl->glLineWidth == NULL) { printf("Error on glLineWidth: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLineWidthx = (void (*)(int)) dlsym(glesLib, "glLineWidthx");
	if(glEsImpl->glLineWidthx == NULL) { printf("Error on glLineWidthx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLoadIdentity = (void (*)(void)) dlsym(glesLib, "glLoadIdentity");
	if(glEsImpl->glLoadIdentity == NULL) { printf("Error on glLoadIdentity: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLoadMatrixf = (void (*)(const float *)) dlsym(glesLib, "glLoadMatrixf");
	if(glEsImpl->glLoadMatrixf == NULL) { printf("Error on glLoadMatrixf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLoadMatrixx = (void (*)(const int *)) dlsym(glesLib, "glLoadMatrixx");
	if(glEsImpl->glLoadMatrixx == NULL) { printf("Error on glLoadMatrixx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glLogicOp = (void (*)(unsigned int)) dlsym(glesLib, "glLogicOp");
	if(glEsImpl->glLogicOp == NULL) { printf("Error on glLogicOp: (%s)\n", dlerror()); return -1; }
	glEsImpl->glMaterialf = (void (*)(unsigned int, unsigned int, float)) dlsym(glesLib, "glMaterialf");
	if(glEsImpl->glMaterialf == NULL) { printf("Error on glMaterialf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glMaterialfv = (void (*)(unsigned int, unsigned int, const float *)) dlsym(glesLib, "glMaterialfv");
	if(glEsImpl->glMaterialfv == NULL) { printf("Error on glMaterialfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glMaterialx = (void (*)(unsigned int, unsigned int, int)) dlsym(glesLib, "glMaterialx");
	if(glEsImpl->glMaterialx == NULL) { printf("Error on glMaterialx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glMaterialxv = (void (*)(unsigned int, unsigned int, const int *)) dlsym(glesLib, "glMaterialxv");
	if(glEsImpl->glMaterialxv == NULL) { printf("Error on glMaterialxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glMatrixMode = (void (*)(unsigned int)) dlsym(glesLib, "glMatrixMode");
	if(glEsImpl->glMatrixMode == NULL) { printf("Error on glMatrixMode: (%s)\n", dlerror()); return -1; }
	glEsImpl->glMultMatrixf = (void (*)(const float *)) dlsym(glesLib, "glMultMatrixf");
	if(glEsImpl->glMultMatrixf == NULL) { printf("Error on glMultMatrixf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glMultMatrixx = (void (*)(const int *)) dlsym(glesLib, "glMultMatrixx");
	if(glEsImpl->glMultMatrixx == NULL) { printf("Error on glMultMatrixx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glMultiTexCoord4f = (void (*)(unsigned int, float, float, float, float)) dlsym(glesLib, "glMultiTexCoord4f");
	if(glEsImpl->glMultiTexCoord4f == NULL) { printf("Error on glMultiTexCoord4f: (%s)\n", dlerror()); return -1; }
	glEsImpl->glMultiTexCoord4x = (void (*)(unsigned int, int, int, int, int)) dlsym(glesLib, "glMultiTexCoord4x");
	if(glEsImpl->glMultiTexCoord4x == NULL) { printf("Error on glMultiTexCoord4x: (%s)\n", dlerror()); return -1; }
	glEsImpl->glNormal3f = (void (*)(float, float, float)) dlsym(glesLib, "glNormal3f");
	if(glEsImpl->glNormal3f == NULL) { printf("Error on glNormal3f: (%s)\n", dlerror()); return -1; }
	glEsImpl->glNormal3x = (void (*)(int, int, int)) dlsym(glesLib, "glNormal3x");
	if(glEsImpl->glNormal3x == NULL) { printf("Error on glNormal3x: (%s)\n", dlerror()); return -1; }
	glEsImpl->glNormalPointer = (void (*)(unsigned int, int, const void *)) dlsym(glesLib, "glNormalPointer");
	if(glEsImpl->glNormalPointer == NULL) { printf("Error on glNormalPointer: (%s)\n", dlerror()); return -1; }
	glEsImpl->glOrthof = (void (*)(float, float, float, float, float, float)) dlsym(glesLib, "glOrthof");
	if(glEsImpl->glOrthof == NULL) { printf("Error on glOrthof: (%s)\n", dlerror()); return -1; }
	glEsImpl->glOrthox = (void (*)(int, int, int, int, int, int)) dlsym(glesLib, "glOrthox");
	if(glEsImpl->glOrthox == NULL) { printf("Error on glOrthox: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPixelStorei = (void (*)(unsigned int, int)) dlsym(glesLib, "glPixelStorei");
	if(glEsImpl->glPixelStorei == NULL) { printf("Error on glPixelStorei: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPointSize = (void (*)(float)) dlsym(glesLib, "glPointSize");
	if(glEsImpl->glPointSize == NULL) { printf("Error on glPointSize: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPointSizex = (void (*)(int)) dlsym(glesLib, "glPointSizex");
	if(glEsImpl->glPointSizex == NULL) { printf("Error on glPointSizex: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPolygonOffset = (void (*)(float, float)) dlsym(glesLib, "glPolygonOffset");
	if(glEsImpl->glPolygonOffset == NULL) { printf("Error on glPolygonOffset: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPolygonOffsetx = (void (*)(int, int)) dlsym(glesLib, "glPolygonOffsetx");
	if(glEsImpl->glPolygonOffsetx == NULL) { printf("Error on glPolygonOffsetx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPopMatrix = (void (*)(void)) dlsym(glesLib, "glPopMatrix");
	if(glEsImpl->glPopMatrix == NULL) { printf("Error on glPopMatrix: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPushMatrix = (void (*)(void)) dlsym(glesLib, "glPushMatrix");
	if(glEsImpl->glPushMatrix == NULL) { printf("Error on glPushMatrix: (%s)\n", dlerror()); return -1; }
//	glEsImpl->glQueryMatrixxOES = (unsigned int (*)(int *, int *)) dlsym(glesLib, "glQueryMatrixxOES");
//	if(glEsImpl->glQueryMatrixxOES == NULL) { printf("Error on glQueryMatrixxOES: (%s)\n", dlerror()); return -1; }
	glEsImpl->glReadPixels = (void (*)(int, int, int, int, unsigned int, unsigned int, void *)) dlsym(glesLib, "glReadPixels");
	if(glEsImpl->glReadPixels == NULL) { printf("Error on glReadPixels: (%s)\n", dlerror()); return -1; }
	glEsImpl->glRotatef = (void (*)(float, float, float, float)) dlsym(glesLib, "glRotatef");
	if(glEsImpl->glRotatef == NULL) { printf("Error on glRotatef: (%s)\n", dlerror()); return -1; }
	glEsImpl->glRotatex = (void (*)(int, int, int, int)) dlsym(glesLib, "glRotatex");
	if(glEsImpl->glRotatex == NULL) { printf("Error on glRotatex: (%s)\n", dlerror()); return -1; }
	glEsImpl->glSampleCoverage = (void (*)(float, unsigned char)) dlsym(glesLib, "glSampleCoverage");
	if(glEsImpl->glSampleCoverage == NULL) { printf("Error on glSampleCoverage: (%s)\n", dlerror()); return -1; }
	glEsImpl->glSampleCoveragex = (void (*)(int, unsigned char)) dlsym(glesLib, "glSampleCoveragex");
	if(glEsImpl->glSampleCoveragex == NULL) { printf("Error on glSampleCoveragex: (%s)\n", dlerror()); return -1; }
	glEsImpl->glScalef = (void (*)(float, float, float)) dlsym(glesLib, "glScalef");
	if(glEsImpl->glScalef == NULL) { printf("Error on glScalef: (%s)\n", dlerror()); return -1; }
	glEsImpl->glScalex = (void (*)(int, int, int)) dlsym(glesLib, "glScalex");
	if(glEsImpl->glScalex == NULL) { printf("Error on glScalex: (%s)\n", dlerror()); return -1; }
	glEsImpl->glScissor = (void (*)(int, int, int, int)) dlsym(glesLib, "glScissor");
	if(glEsImpl->glScissor == NULL) { printf("Error on glScissor: (%s)\n", dlerror()); return -1; }
	glEsImpl->glShadeModel = (void (*)(unsigned int)) dlsym(glesLib, "glShadeModel");
	if(glEsImpl->glShadeModel == NULL) { printf("Error on glShadeModel: (%s)\n", dlerror()); return -1; }
	glEsImpl->glStencilFunc = (void (*)(unsigned int, int, unsigned int)) dlsym(glesLib, "glStencilFunc");
	if(glEsImpl->glStencilFunc == NULL) { printf("Error on glStencilFunc: (%s)\n", dlerror()); return -1; }
	glEsImpl->glStencilMask = (void (*)(unsigned int)) dlsym(glesLib, "glStencilMask");
	if(glEsImpl->glStencilMask == NULL) { printf("Error on glStencilMask: (%s)\n", dlerror()); return -1; }
	glEsImpl->glStencilOp = (void (*)(unsigned int, unsigned int, unsigned int)) dlsym(glesLib, "glStencilOp");
	if(glEsImpl->glStencilOp == NULL) { printf("Error on glStencilOp: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexCoordPointer = (void (*)(int, unsigned int, int, const void *)) dlsym(glesLib, "glTexCoordPointer");
	if(glEsImpl->glTexCoordPointer == NULL) { printf("Error on glTexCoordPointer: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexEnvf = (void (*)(unsigned int, unsigned int, float)) dlsym(glesLib, "glTexEnvf");
	if(glEsImpl->glTexEnvf == NULL) { printf("Error on glTexEnvf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexEnvfv = (void (*)(unsigned int, unsigned int, const float *)) dlsym(glesLib, "glTexEnvfv");
	if(glEsImpl->glTexEnvfv == NULL) { printf("Error on glTexEnvfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexEnvx = (void (*)(unsigned int, unsigned int, int)) dlsym(glesLib, "glTexEnvx");
	if(glEsImpl->glTexEnvx == NULL) { printf("Error on glTexEnvx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexEnvxv = (void (*)(unsigned int, unsigned int, const int *)) dlsym(glesLib, "glTexEnvxv");
	if(glEsImpl->glTexEnvxv == NULL) { printf("Error on glTexEnvxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexImage2D = (void (*)(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *)) dlsym(glesLib, "glTexImage2D");
	if(glEsImpl->glTexImage2D == NULL) { printf("Error on glTexImage2D: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexParameterf = (void (*)(unsigned int, unsigned int, float)) dlsym(glesLib, "glTexParameterf");
	if(glEsImpl->glTexParameterf == NULL) { printf("Error on glTexParameterf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexParameterx = (void (*)(unsigned int, unsigned int, int)) dlsym(glesLib, "glTexParameterx");
	if(glEsImpl->glTexParameterx == NULL) { printf("Error on glTexParameterx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexSubImage2D = (void (*)(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *)) dlsym(glesLib, "glTexSubImage2D");
	if(glEsImpl->glTexSubImage2D == NULL) { printf("Error on glTexSubImage2D: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTranslatef = (void (*)(float, float, float)) dlsym(glesLib, "glTranslatef");
	if(glEsImpl->glTranslatef == NULL) { printf("Error on glTranslatef: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTranslatex = (void (*)(int, int, int)) dlsym(glesLib, "glTranslatex");
	if(glEsImpl->glTranslatex == NULL) { printf("Error on glTranslatex: (%s)\n", dlerror()); return -1; }
	glEsImpl->glVertexPointer = (void (*)(int, unsigned int, int, const void *)) dlsym(glesLib, "glVertexPointer");
	if(glEsImpl->glVertexPointer == NULL) { printf("Error on glVertexPointer: (%s)\n", dlerror()); return -1; }
	glEsImpl->glViewport = (void (*)(int, int, int, int)) dlsym(glesLib, "glViewport");
	if(glEsImpl->glViewport == NULL) { printf("Error on glViewport: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglSwapInterval = (int (*)(int, int)) dlsym(glesLib, "eglSwapInterval");
	if(glEsImpl->eglSwapInterval == NULL) { printf("Error on eglSwapInterval: (%s)\n", dlerror()); return -1; }
	glEsImpl->glBindBuffer = (void (*)(unsigned int, unsigned int)) dlsym(glesLib, "glBindBuffer");
	if(glEsImpl->glBindBuffer == NULL) { printf("Error on glBindBuffer: (%s)\n", dlerror()); return -1; }
	glEsImpl->glBufferData = (void (*)(unsigned int, int, const void *, unsigned int)) dlsym(glesLib, "glBufferData");
	if(glEsImpl->glBufferData == NULL) { printf("Error on glBufferData: (%s)\n", dlerror()); return -1; }
	glEsImpl->glBufferSubData = (void (*) (unsigned int, int, int, const void *)) dlsym(glesLib, "glBufferSubData");
	if(glEsImpl->glBufferSubData == NULL) { printf("Error on glBufferSubData: (%s)\n", dlerror()); return -1; }
	glEsImpl->glClipPlanef = (void (*)(unsigned int, const float *)) dlsym(glesLib, "glClipPlanef");
	if(glEsImpl->glClipPlanef == NULL) { printf("Error on glClipPlanef: (%s)\n", dlerror()); return -1; }
	glEsImpl->glClipPlanex = (void (*)(unsigned int, const int *)) dlsym(glesLib, "glClipPlanex");
	if(glEsImpl->glClipPlanex == NULL) { printf("Error on glClipPlanex: (%s)\n", dlerror()); return -1; }
	glEsImpl->glColor4ub = (void (*)(unsigned char, unsigned char, unsigned char, unsigned char)) dlsym(glesLib, "glColor4ub");
	if(glEsImpl->glColor4ub == NULL) { printf("Error on glColor4ub: (%s)\n", dlerror()); return -1; }
	glEsImpl->glDeleteBuffers = (void (*)(int, const unsigned int *)) dlsym(glesLib, "glDeleteBuffers");
	if(glEsImpl->glDeleteBuffers == NULL) { printf("Error on glDeleteBuffers: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGenBuffers = (void (*)(int, unsigned int *)) dlsym(glesLib, "glGenBuffers");
	if(glEsImpl->glGenBuffers == NULL) { printf("Error on glGenBuffers: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetBooleanv = (void (*)(unsigned int, unsigned char *)) dlsym(glesLib, "glGetBooleanv");
	if(glEsImpl->glGetBooleanv == NULL) { printf("Error on glGetBooleanv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetBufferParameteriv = (void (*)(unsigned int, unsigned int, int *)) dlsym(glesLib, "glGetBufferParameteriv");
	if(glEsImpl->glGetBufferParameteriv == NULL) { printf("Error on glGetBufferParameteriv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetClipPlanef = (void (*)(unsigned int, float *)) dlsym(glesLib, "glGetClipPlanef");
	if(glEsImpl->glGetClipPlanef == NULL) { printf("Error on glGetClipPlanef: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetClipPlanex = (void (*)(unsigned int, int *)) dlsym(glesLib, "glGetClipPlanex");
	if(glEsImpl->glGetClipPlanex == NULL) { printf("Error on glGetClipPlanex: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetFixedv = (void (*)(unsigned int, int *)) dlsym(glesLib, "glGetFixedv");
	if(glEsImpl->glGetFixedv == NULL) { printf("Error on glGetFixedv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetFloatv = (void (*)(unsigned int, float *)) dlsym(glesLib, "glGetFloatv");
	if(glEsImpl->glGetFloatv == NULL) { printf("Error on glGetFloatv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetLightfv = (void (*)(unsigned int, unsigned int, float *)) dlsym(glesLib, "glGetLightfv");
	if(glEsImpl->glGetLightfv == NULL) { printf("Error on glGetLightfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetLightxv = (void (*)(unsigned int, unsigned int, int *)) dlsym(glesLib, "glGetLightxv");
	if(glEsImpl->glGetLightxv == NULL) { printf("Error on glGetLightxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetMaterialfv = (void (*)(unsigned int, unsigned int, float *)) dlsym(glesLib, "glGetMaterialfv");
	if(glEsImpl->glGetMaterialfv == NULL) { printf("Error on glGetMaterialfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetMaterialxv = (void (*)(unsigned int, unsigned int, int *)) dlsym(glesLib, "glGetMaterialxv");
	if(glEsImpl->glGetMaterialxv == NULL) { printf("Error on glGetMaterialxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetPointerv = (void (*)(unsigned int, void **)) dlsym(glesLib, "glGetPointerv");
	if(glEsImpl->glGetPointerv == NULL) { printf("Error on glGetPointerv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetTexEnvfv = (void (*)(unsigned int, unsigned int, float *)) dlsym(glesLib, "glGetTexEnvfv");
	if(glEsImpl->glGetTexEnvfv == NULL) { printf("Error on glGetTexEnvfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetTexEnviv = (void (*)(unsigned int, unsigned int, int *)) dlsym(glesLib, "glGetTexEnviv");
	if(glEsImpl->glGetTexEnviv == NULL) { printf("Error on glGetTexEnviv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetTexEnvxv = (void (*)(unsigned int, unsigned int, int *)) dlsym(glesLib, "glGetTexEnvxv");
	if(glEsImpl->glGetTexEnvxv == NULL) { printf("Error on glGetTexEnvxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetTexParameterfv = (void (*)(unsigned int, unsigned int, float *)) dlsym(glesLib, "glGetTexParameterfv");
	if(glEsImpl->glGetTexParameterfv == NULL) { printf("Error on glGetTexParameterfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetTexParameteriv = (void (*)(unsigned int, unsigned int, int *)) dlsym(glesLib, "glGetTexParameteriv");
	if(glEsImpl->glGetTexParameteriv == NULL) { printf("Error on glGetTexParameteriv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glGetTexParameterxv = (void (*)(unsigned int, unsigned int, int *)) dlsym(glesLib, "glGetTexParameterxv");
	if(glEsImpl->glGetTexParameterxv == NULL) { printf("Error on glGetTexParameterxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glIsBuffer = (unsigned char (*)(unsigned int)) dlsym(glesLib, "glIsBuffer");
	if(glEsImpl->glIsBuffer == NULL) { printf("Error on glIsBuffer: (%s)\n", dlerror()); return -1; }
	glEsImpl->glIsEnabled = (unsigned char (*)(unsigned int)) dlsym(glesLib, "glIsEnabled");
	if(glEsImpl->glIsEnabled == NULL) { printf("Error on glIsEnabled: (%s)\n", dlerror()); return -1; }
	glEsImpl->glIsTexture = (unsigned char (*)(unsigned int)) dlsym(glesLib, "glIsTexture");
	if(glEsImpl->glIsTexture == NULL) { printf("Error on glIsTexture: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPointParameterf = (void (*)(unsigned int, float)) dlsym(glesLib, "glPointParameterf");
	if(glEsImpl->glPointParameterf == NULL) { printf("Error on glPointParameterf: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPointParameterfv = (void (*)(unsigned int, const float *)) dlsym(glesLib, "glPointParameterfv");
	if(glEsImpl->glPointParameterfv == NULL) { printf("Error on glPointParameterfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPointParameterx = (void (*)(unsigned int, int)) dlsym(glesLib, "glPointParameterx");
	if(glEsImpl->glPointParameterx == NULL) { printf("Error on glPointParameterx: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPointParameterxv = (void (*)(unsigned int, const int *)) dlsym(glesLib, "glPointParameterxv");
	if(glEsImpl->glPointParameterxv == NULL) { printf("Error on glPointParameterxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glPointSizePointerOES = (void (*)(unsigned int, int, const void *)) dlsym(glesLib, "glPointSizePointerOES");
	if(glEsImpl->glPointSizePointerOES == NULL) { printf("Error on glPointSizePointerOES: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexEnvi = (void (*)(unsigned int, unsigned int, int)) dlsym(glesLib, "glTexEnvi");
	if(glEsImpl->glTexEnvi == NULL) { printf("Error on glTexEnvi: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexEnviv = (void (*)(unsigned int, unsigned int, const int *)) dlsym(glesLib, "glTexEnviv");
	if(glEsImpl->glTexEnviv == NULL) { printf("Error on glTexEnviv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexParameterfv = (void (*)(unsigned int, unsigned int, const float *)) dlsym(glesLib, "glTexParameterfv");
	if(glEsImpl->glTexParameterfv == NULL) { printf("Error on glTexParameterfv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexParameteri = (void (*)(unsigned int, unsigned int, int)) dlsym(glesLib, "glTexParameteri");
	if(glEsImpl->glTexParameteri == NULL) { printf("Error on glTexParameteri: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexParameteriv = (void (*)(unsigned int, unsigned int, const int *)) dlsym(glesLib, "glTexParameteriv");
	if(glEsImpl->glTexParameteriv == NULL) { printf("Error on glTexParameteriv: (%s)\n", dlerror()); return -1; }
	glEsImpl->glTexParameterxv = (void (*)(unsigned int, unsigned int, const int *)) dlsym(glesLib, "glTexParameterxv");
	if(glEsImpl->glTexParameterxv == NULL) { printf("Error on glTexParameterxv: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglBindTexImage = (int (*)(int, int, int)) dlsym(glesLib, "eglBindTexImage");
	if(glEsImpl->eglBindTexImage == NULL) { printf("Error on eglBindTexImage: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglReleaseTexImage = (int (*)(int, int, int)) dlsym(glesLib, "eglReleaseTexImage");
	if(glEsImpl->eglReleaseTexImage == NULL) { printf("Error on eglReleaseTexImage: (%s)\n", dlerror()); return -1; }
	glEsImpl->eglSurfaceAttrib = (int (*)(int, int, int, int)) dlsym(glesLib, "eglSurfaceAttrib");
	if(glEsImpl->eglSurfaceAttrib == NULL) { printf("Error on eglSurfaceAttrib: (%s)\n", dlerror()); return -1; }
	printf("Done!\n");
	return 0;
}

int nanoGL_Init()
{
	printf("Opening GLES library...\n");
	glesLib = dlopen("/usr/lib/libGLES_CM.so", RTLD_NOW);
	if (glesLib == NULL)
	{
		printf( "dlopen failed!\n" );
		return -1;
	}
	printf( "CreateGlEsInterface\n" );
	if (CreateGlEsInterface() == -1)
	{
		printf( "CreateGlEsInterface failed\n" );
		if (glesLib)
		{
			dlclose(glesLib);
		}
		glesLib = NULL;

		return -1;
	}
	InitGLStructs();
	return 0;
}

void nanoGL_Destroy()
{
	printf( "nanoGL_Destroy called\n" );
	if (glEsImpl)
	{
		free(glEsImpl);
		glEsImpl = NULL;
	}
	if (glesLib)
	{
		dlclose(glesLib);
	}
	glesLib = NULL;
}
