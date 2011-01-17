
#include "3dc.h"

#include <math.h>

#include "inline.h"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"
#include "pvisible.h"
#include "kzsort.h"
#include "kshape.h"


/*

 Platform Specific Functions

 These functions have been written specifically for a given platform.

 They are not necessarily IO or inline functions; these have their own files.

*/


/*

 externs for commonly used global variables and arrays

*/

	extern VECTORCH RotatedPts[];
	extern unsigned int Outcodes[];
	extern DISPLAYBLOCK *Global_ODB_Ptr;
	extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
	extern SHAPEHEADER *Global_ShapeHeaderPtr;
	extern MATRIXCH LToVMat;
	#if SupportMorphing
	extern VECTORCH MorphedPts[];
	extern MORPHDISPLAY MorphDisplay;
	#endif

	extern DISPLAYBLOCK *OnScreenBlockList[];
	extern int NumOnScreenBlocks;
	extern int NumActiveBlocks;
	extern DISPLAYBLOCK *ActiveBlockList[];
	#if SupportModules
	extern char *ModuleLocalVisArray;
	#endif

/*

 Global Variables

*/

	LONGLONGCH ll_one14 = {one14, 0};
	LONGLONGCH ll_zero = {0, 0};


/*

 Find out which objects are in the View Volume.

 The list of these objects, "OnScreenBlockList", is constructed in a number
 of stages.

 The Range Test is a simple outcode of the View Space Location against the
 Object Radius. The View Volume Test is more involved. The VSL is tested
 against each of the View Volume Planes. If it is more than one Object Radius
 outside any plane, the test fails.

*/

/*

 This is the main view volume test shell

*/


#if StandardShapeLanguage


/*

 Shape Points

 The item array is a table of pointers to item arrays. The points data is
 in the first and only item array.

 Rotate each point and write it out to the Rotated Points Buffer

 NOTE:

 There is a global pointer to the shape header "Global_ShapeHeaderPtr"

*/

#define print_bfcro_stats FALSE

#if SupportMorphing
#define checkmorphpts FALSE
#endif

#endif	/* StandardShapeLanguage */


/*

 WideMul2NarrowDiv

 This function takes two pairs of integers, adds their 64-bit products
 together, divides the summed product with another integer and then returns
 the result of that divide, which is also an integer.

*/

int WideMul2NarrowDiv(int a, int b, int c, int d, int e)
{
	LONGLONGCH f;
	LONGLONGCH g;

	MUL_I_WIDE(a, b, &f);
	MUL_I_WIDE(c, d, &g);
	ADD_LL_PP(&f, &g);

	return NarrowDivide(&f, e);
}


/*

 Calculate Plane Normal from three POP's

 The three input vectors are treated as POP's and used to make two vectors.
 These are then crossed to create the normal.

 Make two vectors; (2-1) & (3-1)
 Cross them
 Normalise the vector
  Find the magnitude of the vector
  Divide each component by the magnitude

*/

void MakeNormal(VECTORCH *v1, VECTORCH *v2, VECTORCH *v3, VECTORCH *v4)
{
	VECTORCHF vect0;
	VECTORCHF vect1;
	VECTORCHF n;

	/* vect0 = v2 - v1 */
	vect0.vx = v2->vx - v1->vx;
	vect0.vy = v2->vy - v1->vy;
	vect0.vz = v2->vz - v1->vz;

	/* vect1 = v3 - v1 */
	vect1.vx = v3->vx - v1->vx;
	vect1.vy = v3->vy - v1->vy;
	vect1.vz = v3->vz - v1->vz;

	/* nx = v0y.v1z - v0z.v1y */
	n.vx = (vect0.vy * vect1.vz) - (vect0.vz * vect1.vy);

	/* ny = v0z.v1x - v0x.v1z */
	n.vy = (vect0.vz * vect1.vx) - (vect0.vx * vect1.vz);

	/* nz = v0x.v1y - v0y.v1x */
	n.vz = (vect0.vx * vect1.vy) - (vect0.vy * vect1.vx);

	FNormalise(&n);

	f2i(v4->vx, n.vx * ONE_FIXED);
	f2i(v4->vy, n.vy * ONE_FIXED);
	f2i(v4->vz, n.vz * ONE_FIXED);

	#if 0
	textprint(" - v4 = %d,%d,%d\n", v4->vx, v4->vy, v4->vz);

	#endif
}


/*

 Normalise a vector.

 The returned vector is a fixed point unit vector.

 WARNING!

 The vector must be no larger than 2<<14 because of the square root.
 Because this is an integer function, small components produce errors.

 e.g.

 (100,100,0)

 m=141 (141.42)

 nx = 100 * ONE_FIXED / m = 46,479
 ny = 100 * ONE_FIXED / m = 46,479
 nz = 0

 New m ought to be 65,536 but in fact is 65,731 i.e. 0.29% too large.

*/

void Normalise(VECTORCH *nvector)
{
	VECTORCHF n;
	float m;

	n.vx = nvector->vx;
	n.vy = nvector->vy;
	n.vz = nvector->vz;

	m = 65536.0f / sqrt((n.vx * n.vx) + (n.vy * n.vy) + (n.vz * n.vz));

	f2i(nvector->vx, (n.vx * m) );
	f2i(nvector->vy, (n.vy * m) );
	f2i(nvector->vz, (n.vz * m) );
}


void Normalise2d(VECTOR2D *nvector)
{
	VECTOR2DF n;
	float m;

	n.vx = nvector->vx;
	n.vy = nvector->vy;

	m = sqrt((n.vx * n.vx) + (n.vy * n.vy));

	nvector->vx = (n.vx * ONE_FIXED) / m;
	nvector->vy = (n.vy * ONE_FIXED) / m;
}


void FNormalise(VECTORCHF *n)
{
	float m;

	m = sqrt((n->vx * n->vx) + (n->vy * n->vy) + (n->vz * n->vz));

	n->vx /= m;
	n->vy /= m;
	n->vz /= m;
}

void FNormalise2d(VECTOR2DF *n)
{
	float m;

	m = sqrt((n->vx * n->vx) + (n->vy * n->vy));

	n->vx /= m;
	n->vy /= m;
}


/*

 Return the magnitude of a vector

*/

int Magnitude(VECTORCH *v)
{
	VECTORCHF n;
	int m;

	n.vx = v->vx;
	n.vy = v->vy;
	n.vz = v->vz;

	f2i(m, sqrt((n.vx * n.vx) + (n.vy * n.vy) + (n.vz * n.vz)));

	return m;
}

/*

 Shift the 64-bit value until is LTE the limit

 Return the shift value

*/

int FindShift64(LONGLONGCH *value, LONGLONGCH *limit)
{
	int shift = 0;
	int s;
	LONGLONGCH value_tmp;

	EQUALS_LL(&value_tmp, value);

	s = CMP_LL(&value_tmp, &ll_zero);
	if (s < 0)
		NEG_LL(&value_tmp);

	while (GT_LL(&value_tmp, limit)) 
	{
		shift++;

		ASR_LL(&value_tmp, 1);
	}

	return shift;
}


/*

 MaxLONGLONGCH

 Return a pointer to the largest value of a long long array

*/

void MaxLONGLONGCH(LONGLONGCH *llarrayptr, int llarraysize, LONGLONGCH *llmax)
{
	int i;

	EQUALS_LL(llmax, &ll_zero);

	for (i = llarraysize; i!=0; i--) 
	{
		if (LT_LL(llmax, llarrayptr))
		{
			EQUALS_LL(llmax, llarrayptr);
		}
		llarrayptr++;
	}
}


/*

 Some operators derived from the 64-bit CMP function.

*/


/*

 GT_LL

 To express if(a > b)

 use

 if(GT_LL(a, b))

*/

int GT_LL(LONGLONGCH *a, LONGLONGCH *b)
{
	int s = CMP_LL(a, b);		/* a-b */

	if (s > 0) 
		return (TRUE);
	else 
		return (FALSE);
}


/*

 LT_LL

 To express if(a < b)

 use

 if(LT_LL(a, b))

*/

int LT_LL(LONGLONGCH *a, LONGLONGCH *b)
{
	int s = CMP_LL(a, b);		/* a-b */

	if (s < 0) 
		return (TRUE);
	else 
		return (FALSE);
}


/*

 Matrix Rotatation of a Vector - Inline Version

 Overwrite the Source Vector with the Rotated Vector

 x' = v.c1
 y' = v.c2
 z' = v.c3

*/

void RotVect(VECTORCH *v, MATRIXCH *m)
{
	int x, y, z;

	x =  MUL_FIXED(m->mat11, v->vx);
	x += MUL_FIXED(m->mat21, v->vy);
	x += MUL_FIXED(m->mat31, v->vz);

	y  = MUL_FIXED(m->mat12, v->vx);
	y += MUL_FIXED(m->mat22, v->vy);
	y += MUL_FIXED(m->mat32, v->vz);

	z  = MUL_FIXED(m->mat13, v->vx);
	z += MUL_FIXED(m->mat23, v->vy);
	z += MUL_FIXED(m->mat33, v->vz);

	v->vx = x;
	v->vy = y;
	v->vz = z;
}



/*

 Dot Product Function - Inline Version

 It accepts two pointers to vectors and returns an int result

*/

int _Dot(VECTORCH *vptr1, VECTORCH *vptr2)
{
	int dp;

	dp  = MUL_FIXED(vptr1->vx, vptr2->vx);
	dp += MUL_FIXED(vptr1->vy, vptr2->vy);
	dp += MUL_FIXED(vptr1->vz, vptr2->vz);

	return(dp);
}


/*

 Make a Vector - Inline Version

 v3 = v1 - v2

*/

void MakeV(VECTORCH *v1, VECTORCH *v2, VECTORCH *v3)
{
	v3->vx = v1->vx - v2->vx;
	v3->vy = v1->vy - v2->vy;
	v3->vz = v1->vz - v2->vz;
}

/*

 Add a Vector.

 v2 = v2 + v1

*/

void AddV(VECTORCH *v1, VECTORCH *v2)
{
	v2->vx += v1->vx;
	v2->vy += v1->vy;
	v2->vz += v1->vz;
}
