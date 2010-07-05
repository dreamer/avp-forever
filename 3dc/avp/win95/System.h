#ifndef _included_system_h_
#define _included_system_h_

/*   AVP

 Project Specific System Equates etc.

*/


#ifdef __cplusplus

extern "C" {

#endif



/********************* SYSTEM, PLATFORM AND GAME************/

/* redefined as visual studio 2008 uses Yes and No for sourceannotations.h */
#define TRUE  1
#define FALSE 0

#ifdef _DEBUG /* standard compiler command line debugging-ON switch */
	#define debug TRUE
#elif defined(NDEBUG) /* standard compiler command line debugging-OFF switch */
	#define debug FALSE
#else /* default switch */
	#define debug TRUE
#endif

#define Term -1



/********************  General *****************************/

#define SupportFPMathsFunctions				TRUE
#define SupportFPSquareRoot					TRUE

#define GlobalScale 1



#define one14 16383

typedef int fixed_int;
#define ONE_FIXED 65536
#define ONE_FIXED_SHIFT 16

#define Digital	FALSE


/* Offsets from *** int pointer *** for vectors and vertices */

typedef enum {

	ix,
	iy,
	iz

} PTARRAYINDICES;

#define StopCompilationOnMultipleInclusions FALSE
#define UseProjPlatAssert                   TRUE /* assert fired functions are in dxlog.c */


/***************  CAMERA AND VIEW VOL********************/
#define NearZ 	1024
#define FarZ 	ONE_FIXED

#define SupportMultiCamModules	TRUE



/************* Timer and Frame Rate Independence *************/

#define TimerFrame		1000	
#define TimerFrame10	10000
#define NormalFrame ONE_FIXED
#define NormalFrameShift ONE_FIXED_SHIFT


/***************** Angles  and VALUES ******************/

#define deg10 114
#define deg22pt5 256
#define deg45 512
#define deg90 1024
#define deg180 2048
#define deg270 3072
#define deg315 3584
#define deg337pt5 3840
#define deg350 3980
#define deg360 4096
#define wrap360 4095

#define Cosine45 		46341		/* 46340.95001 cosine(45deg)*/
#define DefaultSlope 	46341

#define bigint 1<<30		/*  max int size*/
#define smallint -(bigint)	/* smallest int size*/


/****************** BUFFER SIZES **********************/

#define maxvdbs 1
#define maxobjects 750
extern int maxshapes;
#define maxstblocks 1000

#define maxrotpts 10000

//tuning for morph sizes
#define maxmorphPts 1024

#define maxpolys 4000
#define maxpolyptrs maxpolys
#define maxpolypts 9			/* Translates as number of vectors */

#define vsize 3					/* Scale for polygon vertex indices */

#define MaxImages 400
#define MaxImageGroups 1

#define oversample_8bitg_threshold 256



/************** Some Shell and Loading Platform Compiler Options ******************/

#undef RIFF_SYSTEM
#define RIFF_SYSTEM
#define TestRiffLoaders TRUE
#define LoadingMapsShapesAndTexturesEtc		FALSE

#define pc_backdrops						FALSE

#define flic_player							TRUE


/***************** DRAW SORT *******************/
#define SupportTrackOptimisation			FALSE


/***************** SHAPE DATA DEFINES************/

#define StandardShapeLanguage					TRUE

#define SupportModules 							TRUE
#define IncludeModuleFunctionPrototypes			TRUE


#define SupportMorphing							TRUE
#define LazyEvaluationForMorphing				FALSE


/***************** COLLISION DEFINES*************/
#define StandardStrategyAndCollisions		FALSE
#define IntermediateSSACM	FALSE		/* User preference */



/************** TEXTURE DEFINES*******************/

#define maxTxAnimblocks	100

/* Texture usage of the colour int */

#define TxDefn 16				/* Shift up for texture definition index */
#define TxLocal 0x8000			/* Set bit 15 to signify a local index */
#define ClrTxIndex 0xffff0000	/* AND with this to clear the low 16-bits */
#define ClrTxDefn 0x0000ffff	/* AND with this to clear the high 16-bits */

#ifdef __cplusplus
	
	};

#endif

//#define SYSTEM_INCLUDED

#endif

