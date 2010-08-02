#ifndef _frustum_h_ /* Is this your first time? */
#define _frustum_h_ 1

#include "kshape.h"
/*
 * KJL 15:13:43 7/17/97 - frustum.h
 *
 * function prototypes & pointers for things connected
 * to the view frustum and clipping
 *
 */

enum FrustumType
{
	FRUSTUM_TYPE_NORMAL,
	FRUSTUM_TYPE_WIDE
};

extern void SetFrustumType(enum FrustumType frustumType);

/* GOURAUD POLYGON CLIPPING */
extern void GouraudPolygon_ClipWithZ(void);
extern void (*GouraudPolygon_ClipWithNegativeX)(void);
extern void (*GouraudPolygon_ClipWithPositiveY)(void);
extern void (*GouraudPolygon_ClipWithNegativeY)(void);
extern void (*GouraudPolygon_ClipWithPositiveX)(void);

/* TEXTURED POLYGON CLIPPING */
extern void TexturedPolygon_ClipWithZ(void);
extern void (*TexturedPolygon_ClipWithNegativeX)(void);
extern void (*TexturedPolygon_ClipWithPositiveY)(void);
extern void (*TexturedPolygon_ClipWithNegativeY)(void);
extern void (*TexturedPolygon_ClipWithPositiveX)(void);

/* GOURAUD TEXTURED POLYGON CLIPPING */
extern void GouraudTexturedPolygon_ClipWithZ(void);
extern void (*GouraudTexturedPolygon_ClipWithNegativeX)(void);
extern void (*GouraudTexturedPolygon_ClipWithPositiveY)(void);
extern void (*GouraudTexturedPolygon_ClipWithNegativeY)(void);
extern void (*GouraudTexturedPolygon_ClipWithPositiveX)(void);

/* FRUSTUM TESTS */
extern int PolygonWithinFrustum(POLYHEADER *polyPtr);
extern int PolygonShouldBeDrawn(POLYHEADER *polyPtr);
extern int (*ObjectWithinFrustum)(DISPLAYBLOCK *dbPtr);
extern int (*ObjectCompletelyWithinFrustum)(DISPLAYBLOCK *dbPtr);
extern int (*VertexWithinFrustum)(RENDERVERTEX *vertexPtr);
extern void (*TestVerticesWithFrustum)(void);

extern int DecalWithinFrustum(DECAL *decalPtr);
extern int QuadWithinFrustum(void);
extern int TriangleWithinFrustum(void);


/* pass a pointer to a vertex to be tested; results are returned in an int,
using the following defines */
#define INSIDE_FRUSTUM_Z_PLANE		1
#define INSIDE_FRUSTUM_PX_PLANE		2
#define INSIDE_FRUSTUM_NX_PLANE		4
#define INSIDE_FRUSTUM_PY_PLANE		8
#define INSIDE_FRUSTUM_NY_PLANE		16
#define INSIDE_FRUSTUM				31

extern char FrustumFlagForVertex[maxrotpts];

#endif
