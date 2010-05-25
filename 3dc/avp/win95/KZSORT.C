#include "3dc.h"
#include "inline.h"
#include "module.h"

#include "stratdef.h"
#include "gamedef.h"

#include "kzsort.h"
#include "kshape.h"

#include "d3d_render.h"
#define UseLocalAssert TRUE
#include "ourasert.h"
#include "avp_userprofile.h"

extern int *ItemPointers[maxpolyptrs];
extern int ItemCount;

extern int NumVertices;
extern int WireFrameMode;
extern int DrawingAReflection;

struct KItem KItemList[maxpolyptrs];

#if 0
static struct KItem KItemList2[maxpolyptrs];
#endif

static struct KObject VisibleModules[MAX_NUMBER_OF_VISIBLE_MODULES];
static struct KObject VisibleModules2[MAX_NUMBER_OF_VISIBLE_MODULES];
static struct KObject *SortedModules;
static struct KObject VisibleObjects[maxobjects];

void SetFogDistance(int fogDistance);
void ReflectObject(DISPLAYBLOCK *dPtr);
void ClearTranslucentPolyList(void);
void RenderPlayersImageInMirror(void);
void AddHierarchicalShape(DISPLAYBLOCK *dptr, VIEWDESCRIPTORBLOCK *VDB_Ptr);


/*KJL*****************************
* externs for new shape function *
*****************************KJL*/
int *MorphedObjectPointsPtr=0;

static void MergeItems(struct KItem *src1, int n1, struct KItem *src2, int n2, struct KItem *dest)
{
	/* merge the 2 sorted lists: at src1, length n1, and at src2, length n2, into dest */

	while (n1>0 && n2>0) /* until one list is exhausted */
	{
		if (src1->SortKey < src2->SortKey)
		{
			/* src1 is nearer */
			*dest++ = *src1++;
			n1--;
		}
	 	else
		{
			/* src2 is nearer */
			*dest++ = *src2++;
			n2--;
		}
	}

	if (n1==0)
	{
	   /* remainder in srce2 goes into dest */
	   while (n2>0)
	   {
			*dest++ = *src2++;
			n2--;
	   }
	}
	else
	{
	   /* remainder in srce1 goes into dest */
	   while (n1>0)
	   {
			*dest++ = *src1++;
			n1--;
	   }
	}
}

static void MergeObjects(struct KObject *src1, int n1, struct KObject *src2, int n2, struct KObject *dest)
{
	/* merge the 2 sorted lists: at src1, length n1, and at src2, length n2, into dest */

	while (n1>0 && n2>0) /* until one list is exhausted */
	{
		if (src1->SortKey < src2->SortKey)
		{
			/* src1 is nearer */
			*dest++ = *src1++;
			n1--;
		}
	 	else
		{
			/* src2 is nearer */
			*dest++ = *src2++;
			n2--;
		}
	}

	if (n1==0)
	{
	   /* remainder in srce2 goes into dest */
	   while (n2>0)
	   {
			*dest++ = *src2++;
			n2--;
	   }
	}
	else
	{
	   /* remainder in srce1 goes into dest */
	   while (n1>0)
	   {
			*dest++ = *src1++;
			n1--;
	   }
	}
}

void SortModules(unsigned int noOfItems)
{
	unsigned int partitionSize;
	unsigned int noOfPasses = 0;

	struct KObject *mergeFrom = &VisibleModules[0];
	struct KObject *mergeTo = &VisibleModules2[0];
	struct KObject *mergeTemp;

	unsigned int offSet;

	for (partitionSize = 1; partitionSize < noOfItems; partitionSize *= 2)
	{
		/* for each partition size...
		   loop through partition pairs and merge */

		/* initialise partition and destination offsets */
		offSet = 0;

		/* do merges for this partition size,
		omitting the last merge if the second partition is incomplete  */
		while((offSet+(partitionSize*2)) <= noOfItems)
		{
			MergeObjects(
				(mergeFrom+offSet),
				partitionSize,
				(mergeFrom+offSet+partitionSize),
				partitionSize,
				(mergeTo+offSet) );

			offSet += partitionSize*2;
		}

		/* At this stage, there's less than 2 whole partitions
		left in the array.  If there's no data left at all, then
		there's nothing left to do.  However, if there's any data
		left at the end of the array, we need to do something with it:

		If there's more than a full partition, merge it against the remaining
		partial partition.  If there's less than a full partition, just copy
		it across (via the MergeObjects fn): it will be merged in again during a
		later pass.

		*/

		if((offSet+partitionSize) < noOfItems)
		{
			/* merge full partition against a partial partition */
			MergeObjects(
				(mergeFrom+offSet),
				partitionSize,
				(mergeFrom+offSet+partitionSize),
				(noOfItems - (offSet+partitionSize)),
				(mergeTo+offSet) );
		}
		else if(offSet < noOfItems)
		{
			/* pass the incomplete partition thro' the merge fn
			   to copy it across */
			MergeObjects(
				(mergeFrom+offSet),
				(noOfItems-offSet),
				(mergeFrom+offSet),	/* this is a dummy parameter ... */
				0,
				(mergeTo+offSet) );
		}

		/* count number of passes */
		noOfPasses++;
		/* swap source and destination */
		mergeTemp = mergeFrom;
		mergeFrom = mergeTo;
		mergeTo = mergeTemp;
	}

	/* check where the final list is, and move if neccesary */
	if (noOfPasses%2 == 1)
	{
		/* final list is in the auxiliary buffer */
		SortedModules = VisibleModules2;
	}
	else
	{
		SortedModules = VisibleModules;
	}
}



/* KJL 12:21:51 02/11/97 - This routine is too big and ugly. Split & clean up required! */
void KRenderItems(VIEWDESCRIPTORBLOCK *VDBPtr)
{
	extern int NumOnScreenBlocks;
	extern DISPLAYBLOCK *OnScreenBlockList[];
	int numOfObjects = NumOnScreenBlocks;
	int numVisMods=0;
	int numVisObjs=0;

	while (numOfObjects)
	{
		extern DISPLAYBLOCK *Player;

		DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
		MODULE *modulePtr = objectPtr->ObMyModule;

		/* if it's a module, which isn't inside another module */
		if (modulePtr && !(modulePtr->m_flags & m_flag_slipped_inside))
		{
			// these tests should really be done using the camera (VDB) position
			extern MODULE *playerPherModule;
			if (playerPherModule == modulePtr)
			{
				VisibleModules[numVisMods].DispPtr = objectPtr;
				VisibleModules[numVisMods].SortKey = smallint;
			}
			else
			{
				VECTORCH position;
				VECTORCH dist;

				position.vx = modulePtr->m_world.vx - Player->ObWorld.vx;
				position.vy = modulePtr->m_world.vy - Player->ObWorld.vy;
				position.vz = modulePtr->m_world.vz - Player->ObWorld.vz;

			    {
				    int minX = modulePtr->m_minx + position.vx;
				    int maxX = modulePtr->m_maxx + position.vx;

					if (maxX<0) /* outside maxX side */
					{
						dist.vx = maxX;
					}
					else if (minX>0) /* outside minX faces */
					{
						dist.vx = minX;
					}
					else /* between faces */
					{
						dist.vx = 0;
					}
				}
			    {
				    int minY = modulePtr->m_miny + position.vy;
				    int maxY = modulePtr->m_maxy + position.vy;

					if (maxY<0) /* outside maxY side */
					{
						dist.vy = maxY;
					}
					else if (minY>0) /* outside minY faces */
					{
						dist.vy = minY;
					}
					else /* between faces */
					{
						dist.vy = 0;
					}
				}
			    {
				    int minZ = modulePtr->m_minz + position.vz;
				    int maxZ = modulePtr->m_maxz + position.vz;

					if (maxZ<0) /* outside maxZ side */
					{
						dist.vz = maxZ;
					}
					else if (minZ>0) /* outside minZ faces */
					{
						dist.vz = minZ;
					}
					else /* between faces */
					{
						dist.vz = 0;
					}
				}

				VisibleModules[numVisMods].DispPtr = objectPtr;

				VisibleModules[numVisMods].SortKey = Magnitude(&dist);
			}

   			if(numVisMods>MAX_NUMBER_OF_VISIBLE_MODULES)
			{
				/* outside the environment! */
				textprint("MAX_NUMBER_OF_VISIBLE_MODULES (%d) exceeded!\n",MAX_NUMBER_OF_VISIBLE_MODULES);
				textprint("Possibly outside the environment!\n");
//				LOCALASSERT(0);
				return;
			}

			numVisMods++;
   		}
		else /* it's just an object */
		{
			VisibleObjects[numVisObjs].DispPtr = objectPtr;
			/* this sort key defaults to the object not being drawn, ie. a grenade
			behind a closed door (so there is no module behind door) would still be
			in the OnScreenBlockList but need not be drawn. */
			VisibleObjects[numVisObjs].SortKey = 0X7FFFFFFF;
			numVisObjs++;
		}
   	}
	textprint("numvismods %d\n",numVisMods);
	textprint("numvisobjs %d\n",numVisObjs);
	{
		int numMods = numVisMods;

		while(numMods)
		{
			int n = numMods;

			int furthestModule=0;
			int furthestDistance=0;

		   	while(n)
			{
				n--;
				if (furthestDistance < VisibleModules[n].SortKey)
				{
					furthestDistance = VisibleModules[n].SortKey;
					furthestModule = n;
				}
			}

			numMods--;

			VisibleModules2[numMods] = VisibleModules[furthestModule];
			VisibleModules[furthestModule] = VisibleModules[numMods];
			SortedModules = VisibleModules2;
		}
	}
	{
		int fogDistance = 0x7f000000;

		int o = numVisObjs;
		while(o--)
		{
			DISPLAYBLOCK *objectPtr = VisibleObjects[o].DispPtr;
			int maxX = objectPtr->ObWorld.vx + objectPtr->ObRadius;
			int minX = objectPtr->ObWorld.vx - objectPtr->ObRadius;
			int maxZ = objectPtr->ObWorld.vz + objectPtr->ObRadius;
			int minZ = objectPtr->ObWorld.vz - objectPtr->ObRadius;
			int maxY = objectPtr->ObWorld.vy + objectPtr->ObRadius;
			int minY = objectPtr->ObWorld.vy - objectPtr->ObRadius;

			int numMods = 0;
			while(numMods<numVisMods)
			{
				MODULE *modulePtr = SortedModules[numMods].DispPtr->ObMyModule;

				if (maxX >= modulePtr->m_minx+modulePtr->m_world.vx)
				if (minX <= modulePtr->m_maxx+modulePtr->m_world.vx)
			    if (maxZ >= modulePtr->m_minz+modulePtr->m_world.vz)
			    if (minZ <= modulePtr->m_maxz+modulePtr->m_world.vz)
			    if (maxY >= modulePtr->m_miny+modulePtr->m_world.vy)
			    if (minY <= modulePtr->m_maxy+modulePtr->m_world.vy)
				{
					VisibleObjects[o].SortKey=numMods;
					break;
				}
				numMods++;
			}

			if (CurrentVisionMode == VISION_MODE_PRED_SEEALIENS && objectPtr->ObStrategyBlock)
			{
				if (objectPtr->ObStrategyBlock->I_SBtype == I_BehaviourAlien)
					VisibleObjects[o].DrawBeforeEnvironment = 0;//1;
			}
			else
			{
				VisibleObjects[o].DrawBeforeEnvironment = 0;
			}
		}

		if (fogDistance<0) fogDistance=0;
		SetFogDistance(fogDistance);
	}
	DrawingAReflection=0;
	{
		int numMods = numVisMods;
		#if 1
		{
			int o = numVisObjs;
			CheckWireFrameMode(WireFrameMode&2);
			while(o)
			{
				o--;

				if(VisibleObjects[o].DrawBeforeEnvironment)
				{
					DISPLAYBLOCK *dptr = VisibleObjects[o].DispPtr;
					AddShape(VisibleObjects[o].DispPtr,VDBPtr);
					#if MIRRORING_ON
					if (MirroringActive && !dptr->HModelControlBlock)
					{
						ReflectObject(dptr);

						MakeVector(&dptr->ObWorld, &VDBPtr->VDB_World, &dptr->ObView);
						RotateVector(&dptr->ObView, &VDBPtr->VDB_Mat);

				  		DrawingAReflection=1;
				  		AddShape(dptr,VDBPtr);
				  		DrawingAReflection=0;
						ReflectObject(dptr);
					}
					#endif
				}
			}
		}

		ClearTranslucentPolyList();

		if (MOTIONBLUR_CHEATMODE)
		{
			for (numMods=0; numMods<numVisMods; numMods++)
			{
				MODULE *modulePtr = SortedModules[numMods].DispPtr->ObMyModule;

				CheckWireFrameMode(WireFrameMode&1);
		  		AddShape(SortedModules[numMods].DispPtr,VDBPtr);
				#if MIRRORING_ON
				if (MirroringActive)
				{
					DISPLAYBLOCK *dptr = SortedModules[numMods].DispPtr;
					{
						ReflectObject(dptr);

						MakeVector(&dptr->ObWorld, &VDBPtr->VDB_World, &dptr->ObView);
						RotateVector(&dptr->ObView, &VDBPtr->VDB_Mat);

					  	DrawingAReflection=1;
				  		AddShape(dptr,VDBPtr);
			  			DrawingAReflection=0;
		 				ReflectObject(dptr);
					}
				}
				#endif
				CheckWireFrameMode(WireFrameMode&2);
				{
					int o = numVisObjs;
					while(o)
					{
						o--;

						if(VisibleObjects[o].SortKey == numMods && !VisibleObjects[o].DrawBeforeEnvironment)
						{
							DISPLAYBLOCK *dptr = VisibleObjects[o].DispPtr;
							AddShape(VisibleObjects[o].DispPtr,VDBPtr);
							#if MIRRORING_ON
							if (MirroringActive && !dptr->HModelControlBlock)
							{
								ReflectObject(dptr);

								MakeVector(&dptr->ObWorld, &VDBPtr->VDB_World, &dptr->ObView);
								RotateVector(&dptr->ObView, &VDBPtr->VDB_Mat);

					  			DrawingAReflection=1;
						  		AddShape(dptr,VDBPtr);
					  			DrawingAReflection=0;
								ReflectObject(dptr);
							}
							#endif
						}
					}
				}

		 		D3D_DrawWaterTest(modulePtr);
			}
		}
		else
		{
			while(numMods--)
			{
				MODULE *modulePtr = SortedModules[numMods].DispPtr->ObMyModule;

				CheckWireFrameMode(WireFrameMode&1);
		  		AddShape(SortedModules[numMods].DispPtr,VDBPtr);
				#if MIRRORING_ON
				if (MirroringActive)
				{
					DISPLAYBLOCK *dptr = SortedModules[numMods].DispPtr;
					{
						ReflectObject(dptr);

						MakeVector(&dptr->ObWorld, &VDBPtr->VDB_World, &dptr->ObView);
						RotateVector(&dptr->ObView, &VDBPtr->VDB_Mat);

					  	DrawingAReflection=1;
				  		AddShape(dptr,VDBPtr);
			  			DrawingAReflection=0;
		 				ReflectObject(dptr);
					}
				}
				#endif
				CheckWireFrameMode(WireFrameMode&2);
				{
					int o = numVisObjs;
					while(o)
					{
						o--;

						if(VisibleObjects[o].SortKey == numMods && !VisibleObjects[o].DrawBeforeEnvironment)
						{
							DISPLAYBLOCK *dptr = VisibleObjects[o].DispPtr;
							AddShape(VisibleObjects[o].DispPtr,VDBPtr);
							#if MIRRORING_ON
							if (MirroringActive && !dptr->HModelControlBlock)
							{
								ReflectObject(dptr);

								MakeVector(&dptr->ObWorld, &VDBPtr->VDB_World, &dptr->ObView);
								RotateVector(&dptr->ObView, &VDBPtr->VDB_Mat);

					  			DrawingAReflection=1;
						  		AddShape(dptr,VDBPtr);
					  			DrawingAReflection=0;
								ReflectObject(dptr);
							}
							#endif
						}
					}
				}

		 		D3D_DrawWaterTest(modulePtr);
			}
		}
//	 			OutputTranslucentPolyList();
		#endif

		/* KJL 12:51:00 13/08/98 - scan for hierarchical objects which aren't going to be drawn,
		and update their timers */
		{
			int o = numVisObjs;
			while(o)
			{
				o--;

				if(VisibleObjects[o].SortKey == 0x7fffffff)
				{
					if(VisibleObjects[o].DispPtr->HModelControlBlock)
					{
						DoHModelTimer(VisibleObjects[o].DispPtr->HModelControlBlock);
					}
				}
			}
		}

	#if MIRRORING_ON
		if (MirroringActive)
		{
			DrawingAReflection=1;
			RenderPlayersImageInMirror();
			DrawingAReflection=0;
		}
	#endif
	}
}

#if 0
static int ObjectIsInModule(DISPLAYBLOCK *objectPtr,MODULE *modulePtr)
{
	int objectSize = objectPtr->ObRadius;
	VECTORCH position = objectPtr->ObWorld;

	position.vx -= modulePtr->m_world.vx;
	position.vy -= modulePtr->m_world.vy;
	position.vz -= modulePtr->m_world.vz;

	if (position.vx + objectSize >= modulePtr->m_minx)
    	if (position.vx - objectSize <= modulePtr->m_maxx)
		    if (position.vz + objectSize >= modulePtr->m_minz)
			    if (position.vz - objectSize <= modulePtr->m_maxz)
				    if (position.vy + objectSize >= modulePtr->m_miny)
					    if (position.vy - objectSize <= modulePtr->m_maxy)
							return 1;

	return 0;

}
#endif
static int PointIsInModule(VECTORCH *pointPtr,MODULE *modulePtr)
{
	VECTORCH position = *pointPtr;
	position.vx -= modulePtr->m_world.vx;
	position.vy -= modulePtr->m_world.vy;
	position.vz -= modulePtr->m_world.vz;

	if (position.vx >= modulePtr->m_minx)
    	if (position.vx <= modulePtr->m_maxx)
		    if (position.vz >= modulePtr->m_minz)
			    if (position.vz <= modulePtr->m_maxz)
				    if (position.vy >= modulePtr->m_miny)
					    if (position.vy <= modulePtr->m_maxy)
							return 1;
	return 0;

}



void RenderThisDisplayblock(DISPLAYBLOCK *dbPtr)
{
/*
	bjd - returning from this function before calling
	AddShape makes the marines weapons disappear. Nothing else changes
	when playing marine level 1
*/
	extern VIEWDESCRIPTORBLOCK *ActiveVDBList[];
	VIEWDESCRIPTORBLOCK *VDBPtr = ActiveVDBList[0];

  	AddShape(dbPtr, VDBPtr);
}

void RenderThisHierarchicalDisplayblock(DISPLAYBLOCK *dbPtr)
{
/*
	bjd - returning from this function early makes the marines weapons
	and NPCs disappear, nothing else!
*/
	extern VIEWDESCRIPTORBLOCK *ActiveVDBList[];
	VIEWDESCRIPTORBLOCK *VDBPtr = ActiveVDBList[0];

  	AddHierarchicalShape(dbPtr,VDBPtr);
	#if MIRRORING_ON
	if (MirroringActive && dbPtr->ObStrategyBlock)
	{
		ReflectObject(dbPtr);

		MakeVector(&dbPtr->ObWorld, &VDBPtr->VDB_World, &dbPtr->ObView);
		RotateVector(&dbPtr->ObView, &VDBPtr->VDB_Mat);

	  	AddHierarchicalShape(dbPtr,VDBPtr);
		ReflectObject(dbPtr);
	}
	#endif
}
