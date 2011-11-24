/*******************************************************************
 *
 *    DESCRIPTION: 	tallfont.cpp
 *
 *    AUTHOR: David Malcolm
 *
 *    HISTORY:  Created 18/3/98
 *
 *******************************************************************/

/* Includes ********************************************************/
#include "3dc.h"

	#include "db.h"
	#include "dxlog.h"
	#include "tallfont.hpp"
	#include "awTexLd.h"
	#include "ffstdio.h"
	#define UseLocalAssert TRUE
	#include "ourasert.h"
	#include "inline.h"
	#include "RimLoader.h"

/* Constants *******************************************************/

/* Macros **********************************************************/

/* Imported function prototypes ************************************/

/* Imported data ***************************************************/

/* Exported globals ************************************************/

/* Internal type definitions ***************************************/

/* Internal function prototypes ************************************/

/* Internal globals ************************************************/

/* Exported function definitions ***********************************/
// class IndexedFont_Proportional_Column : public IndexedFont_Proportional
// public:
void
IndexedFont_Proportional_Column :: RenderChar_Clipped
(
	struct r2pos& R2Pos_Cursor,
	const struct r2rect& R2Rect_Clip,
	int FixP_Alpha,
	ProjChar ProjCh
) const
{
	// Easy first attempt: pass on to unclipped routine, but only if no clipping
	// required.  Otherwise ignore...
	if (r2rect(R2Pos_Cursor, GetWidth(ProjCh), GetHeight()).bFitsIn(R2Rect_Clip))
	{
		RenderChar_Unclipped
		(
			R2Pos_Cursor,
			FixP_Alpha,
			ProjCh
		);
	}
	else
	{
		R2Pos_Cursor.x += GetWidth(ProjCh);
	}
}

void
IndexedFont_Proportional_Column :: RenderChar_Unclipped
(
	struct r2pos& R2Pos_Cursor,
	int FixP_Alpha,
	ProjChar ProjCh
) const
{
	#if 1
	unsigned int theOffset;

	if (ProjCh == ' ')
	{
		// Space is a special case:
		R2Pos_Cursor.x += SpaceWidth();
		return;
	}

	if (GetOffset(theOffset, ProjCh))
	{
		if (GetWidth(ProjCh) > 0)
		{
			RECT destRect;

			destRect.left = R2Pos_Cursor.x;
			destRect.top = R2Pos_Cursor.y;

			R2Pos_Cursor.x += GetWidth(ProjCh);

			destRect.right = R2Pos_Cursor.x++;
			destRect.bottom = R2Pos_Cursor.y + GetHeight();

			RECT tempnonConstRECTSoThatItCanWorkWithMicrosoft = WindowsRectForOffset[ theOffset ];
		}
	}
	#else
	textprintXY
	(
		R2Pos_Cursor . x,
		R2Pos_Cursor . y,
		"%c",
		ProjCh
	);
	#endif
}

int
IndexedFont_Proportional_Column :: GetMaxWidth(void) const
{
	return R2Size_OverallImage.w;
}

int
IndexedFont_Proportional_Column :: GetWidth
(
	ProjChar ProjCh_In
) const
{
	unsigned int offsetTemp;

	if (ProjCh_In == ' ')
	{
		return SpaceWidth();
	}

	if (GetOffset(offsetTemp, ProjCh_In))
	{
		return WidthForOffset[ offsetTemp ];
	}
	else
	{
		return 0;
	}
}

// static
IndexedFont_Proportional_Column* IndexedFont_Proportional_Column :: Create
(
	FontIndex I_Font_New,
	char* Filename,
	int HeightPerChar_New,
	int SpaceWidth_New,
	int ASCIICodeForInitialCharacter
)
{
	IndexedFont_Proportional_Column* pFont = new IndexedFont_Proportional_Column
	(
		I_Font_New,
		Filename,
		HeightPerChar_New,
		SpaceWidth_New,
		ASCIICodeForInitialCharacter
	);

	SCString::UpdateAfterFontChange( I_Font_New );

	return pFont;
}


IndexedFont_Proportional_Column :: ~IndexedFont_Proportional_Column()
{
	GLOBALASSERT(image_ptr);
	ReleaseAvPTexture(image_ptr);
	image_ptr = NULL;
}

IndexedFont_Proportional_Column :: IndexedFont_Proportional_Column
(
	FontIndex I_Font_New,
	char* Filename,
	int HeightPerChar_New,
	int SpaceWidth_New,
	int ASCIICodeForInitialCharacter
) : IndexedFont_Proportional
	(
		I_Font_New
	),
	ASCIICodeForOffset0
	(
		ASCIICodeForInitialCharacter
	),
	HeightPerChar_Val(HeightPerChar_New),
	SpaceWidth_Val(SpaceWidth_New),
	NumChars(0)
{
	{
		uint32_t nWidth  = 0;
		uint32_t nHeight = 0;

		RimLoader indexFontRIM;
		std::string filePath = Filename;
		if (!indexFontRIM.Open("graphics\\" + filePath))
		{
			return;
		}

		indexFontRIM.GetDimensions(nWidth, nHeight);
#if 0
		unsigned nWidth, nHeight;
	
		//see if graphic can be found in fast file
		size_t fastFileLength;
		void const * pFastFileData = ffreadbuf(Filename,&fastFileLength);
		
		if (pFastFileData)
		{
			//load from fast file
			image_ptr = AwCreateTexture
			(
				"pxfXYB",
				pFastFileData,
				fastFileLength,
				(
					0
				),
				&nWidth,
				&nHeight
			);
		}
		else
		{
			//load graphic from rim file
			image_ptr = AwCreateTexture
			(
				"sfXYB",
				Filename,
				(
					0
				),
				&nWidth,
				&nHeight
			);
		}
#endif

		R2Size_OverallImage.w = nWidth;
		R2Size_OverallImage.h = nHeight;
	}

	GLOBALASSERT(image_ptr);

	GLOBALASSERT(R2Size_OverallImage.w > 0);
	GLOBALASSERT(R2Size_OverallImage.h > 0);

	NumChars = (R2Size_OverallImage.h)/HeightPerChar_Val;

	GLOBALASSERT( NumChars < MAX_CHARS_IN_TALLFONT );

	for (int i=0;i < NumChars;i++)
	{
		WindowsRectForOffset[ i ].top = (i*HeightPerChar_Val);
		WindowsRectForOffset[ i ].bottom = ((i+1)*HeightPerChar_Val);
		WindowsRectForOffset[ i ].left = 0;
		WindowsRectForOffset[ i ].right = R2Size_OverallImage.w;

		WidthForOffset[ i ] = R2Size_OverallImage.w;
	}

	UpdateWidths();
}

void
IndexedFont_Proportional_Column :: UpdateWidths(void)
{
	// called by constructor

	// Test: read the surface:
	{
		// Read the data...
		{
			for (int iOffset=0;iOffset<NumChars;iOffset++)
			{
				int y = iOffset * HeightPerChar_Val;
				int x = ( R2Size_OverallImage.w - 1);

				#if 0
				db_logf1(("Character offset %i",iOffset));
				#endif

				while (x>0)
				{
					if (bAnyNonTransparentPixelsInColumn(r2pos(x,y), HeightPerChar_Val))
					{
						break; 
							
					}
					// and the current value of (x+1) is the width to use
					x--;
				}

				SetWidth(iOffset, x+1);
			}
		}
	}
}

// static
bool
IndexedFont_Proportional_Column :: bAnyNonTransparentPixelsInColumn
(
	r2pos R2Pos_TopOfColumn,
	int HeightOfColumn
)
{
	return TRUE;
}


/////////////////////////////////////////////////////////////////
// 3/4/98 DHM: A new implementation, supporting character kerning
/////////////////////////////////////////////////////////////////

// class IndexedFont_Kerned_Column : public IndexedFont_Kerned
// public:
void
IndexedFont_Kerned_Column :: RenderChar_Clipped
(
	struct r2pos& R2Pos_Cursor,
	const struct r2rect& R2Rect_Clip,
	int FixP_Alpha,
	ProjChar ProjCh
) const
{
	unsigned int theOffset;

	if (ProjCh == ' ')
	{
		// Space is a special case:
		return;
	}

	if (GetOffset(theOffset, ProjCh))
	{
		if
		(
			GetWidth(ProjCh) > 0
		)
		{
			{
#if 0 // bjd - leaving here for cloud table reference..

				// This code adapted from DrawGraphicWithAlphaChannel();
				// it assumes you're in a 16-bit mode...
				DDSURFACEDESC ddsdimage;
				
				memset(&ddsdimage, 0, sizeof(ddsdimage));
				ddsdimage.dwSize = sizeof(ddsdimage);

				/* lock the image */
				while (image_ptr->Lock(NULL, &ddsdimage, DDLOCK_WAIT, NULL) == DDERR_WASSTILLDRAWING);

				// okay, now we have the surfaces, we can copy from one to the other,
				// darkening pixels as we go
				{
					long fontimagePitchInShorts = (ddsdimage.lPitch/2); 
					long backbufferPitchInShorts = (BackBufferPitch/2); 

					unsigned short* fontimageRowStartPtr =
					(
						((unsigned short *)ddsdimage.lpSurface)
						+
						(GetHeight()*theOffset*fontimagePitchInShorts)
					);

					unsigned short* backbufferRowStartPtr =
					(
						((unsigned short *)ScreenBuffer)
						+
						(R2Pos_Cursor.y*backbufferPitchInShorts)
						+
						(R2Pos_Cursor.x)
					);
					int screenY = R2Pos_Cursor.y;

					for (int yCount=GetHeight(); yCount>0; yCount--)
					{
						unsigned short* fontimagePtr = fontimageRowStartPtr;
						unsigned short* backbufferPtr = backbufferRowStartPtr;

						if (screenY >= R2Rect_Clip.y0 && screenY <= R2Rect_Clip.y1)
						for (int xCount=FullWidthForOffset[theOffset]; xCount>0;xCount--)
						{
							int r = CloudTable[(xCount+R2Pos_Cursor.x+CloakingPhase/64)&127][(screenY+CloakingPhase/128)&127];
//							b += CloudTable[((xCount+R2Pos_Cursor.x)/2-CloakingPhase/96)&127][((yCount+R2Pos_Cursor.y)/4+CloakingPhase/64)&127]/4;
//							b += CloudTable[((xCount+R2Pos_Cursor.x+10)/4-CloakingPhase/64)&127][((yCount+R2Pos_Cursor.y-50)/8+CloakingPhase/32)&127]/8;
//							if (b>ONE_FIXED) b = ONE_FIXED;
							r = MUL_FIXED(FixP_Alpha,r);
							if (*fontimagePtr)
							{
								unsigned int backR = (int)(*backbufferPtr) & DisplayPixelFormat.dwRBitMask;
								unsigned int backG = (int)(*backbufferPtr) & DisplayPixelFormat.dwGBitMask;
								unsigned int backB = (int)(*backbufferPtr) & DisplayPixelFormat.dwBBitMask;

								unsigned int fontR = (int)(*fontimagePtr) & DisplayPixelFormat.dwRBitMask;
								unsigned int fontG = (int)(*fontimagePtr) & DisplayPixelFormat.dwGBitMask;
								unsigned int fontB = (int)(*fontimagePtr) & DisplayPixelFormat.dwBBitMask;

								backR += MUL_FIXED(r,fontR);
								if (backR>DisplayPixelFormat.dwRBitMask) backR = DisplayPixelFormat.dwRBitMask;
								else backR &= DisplayPixelFormat.dwRBitMask;

								backG += MUL_FIXED(r,fontG);
								if (backG>DisplayPixelFormat.dwGBitMask) backG = DisplayPixelFormat.dwGBitMask;
								else backG &= DisplayPixelFormat.dwGBitMask;

								backB += MUL_FIXED(r,fontB);
								if (backB>DisplayPixelFormat.dwBBitMask) backB = DisplayPixelFormat.dwBBitMask;
								else backB &= DisplayPixelFormat.dwBBitMask;

								*backbufferPtr = (short)(backR|backG|backB);
							}
							fontimagePtr++;
							backbufferPtr++;
						}
						screenY++;
						fontimageRowStartPtr += fontimagePitchInShorts;
						backbufferRowStartPtr += backbufferPitchInShorts;
					}
				}
#endif
				
//				image_ptr->Unlock((LPVOID)ddsdimage.lpSurface);
			}
		}
	}
}

void
IndexedFont_Kerned_Column :: RenderChar_Unclipped
(
	struct r2pos& R2Pos_Cursor,
	int FixP_Alpha,
	ProjChar ProjCh
) const
{
	unsigned int theOffset;

	if (ProjCh == ' ')
	{
		// Space is a special case:
		return;
	}

	if (GetOffset(theOffset, ProjCh))
	{
		if (GetWidth(ProjCh) > 0)
		{
			{
#if 0 // bjd
				// This code adapted from DrawGraphicWithAlphaChannel();
				// it assumes you're in a 16-bit mode...
				DDSURFACEDESC ddsdback, ddsdimage;
				
				memset(&ddsdback, 0, sizeof(ddsdback));
				memset(&ddsdimage, 0, sizeof(ddsdimage));
				ddsdback.dwSize = sizeof(ddsdback);
				ddsdimage.dwSize = sizeof(ddsdimage);


				/* lock the image */
				while (image_ptr->Lock(NULL, &ddsdimage, DDLOCK_WAIT, NULL) == DDERR_WASSTILLDRAWING);

				/* lock the backbuffer */
				while (lpDDSBack->Lock(NULL, &ddsdback, DDLOCK_WAIT, NULL) == DDERR_WASSTILLDRAWING);

				// okay, now we have the surfaces, we can copy from one to the other,
				// darkening pixels as we go
				{
					long fontimagePitchInShorts = (ddsdimage.lPitch/2); 
					long backbufferPitchInShorts = (ddsdback.lPitch/2); 

					unsigned short* fontimageRowStartPtr =
					(
						((unsigned short *)ddsdimage.lpSurface)
						+
						(GetHeight()*theOffset*fontimagePitchInShorts)
					);

					unsigned short* backbufferRowStartPtr =
					(
						((unsigned short *)ddsdback.lpSurface)
						+
						(R2Pos_Cursor.y*backbufferPitchInShorts)
						+
						(R2Pos_Cursor.x)
					);

					for (int yCount=GetHeight(); yCount>0; yCount--)
					{
						unsigned short* fontimagePtr = fontimageRowStartPtr;
						unsigned short* backbufferPtr = backbufferRowStartPtr;
						int yIndex = (yCount+R2Pos_Cursor.y+CloakingPhase/128)&127;
						int xIndex = R2Pos_Cursor.x+CloakingPhase/64;

						for (int xCount=FullWidthForOffset[theOffset]; xCount>0;xCount--)
						{
							int r = CloudTable[(xCount+xIndex)&127][yIndex];
//							b += CloudTable[((xCount+R2Pos_Cursor.x)/2-CloakingPhase/96)&127][((yCount+R2Pos_Cursor.y)/4+CloakingPhase/64)&127]/4;
//							b += CloudTable[((xCount+R2Pos_Cursor.x+10)/4-CloakingPhase/64)&127][((yCount+R2Pos_Cursor.y-50)/8+CloakingPhase/32)&127]/8;
//							if (b>ONE_FIXED) b = ONE_FIXED;
							r = MUL_FIXED(FixP_Alpha,r);
							if (*fontimagePtr)
							{
								unsigned int backR = (int)(*backbufferPtr) & DisplayPixelFormat.dwRBitMask;
								unsigned int backG = (int)(*backbufferPtr) & DisplayPixelFormat.dwGBitMask;
								unsigned int backB = (int)(*backbufferPtr) & DisplayPixelFormat.dwBBitMask;

								unsigned int fontR = (int)(*fontimagePtr) & DisplayPixelFormat.dwRBitMask;
								unsigned int fontG = (int)(*fontimagePtr) & DisplayPixelFormat.dwGBitMask;
								unsigned int fontB = (int)(*fontimagePtr) & DisplayPixelFormat.dwBBitMask;

								backR += MUL_FIXED(r,fontR);
								if (backR>DisplayPixelFormat.dwRBitMask) backR = DisplayPixelFormat.dwRBitMask;
								else backR &= DisplayPixelFormat.dwRBitMask;

								backG += MUL_FIXED(r,fontG);
								if (backG>DisplayPixelFormat.dwGBitMask) backG = DisplayPixelFormat.dwGBitMask;
								else backG &= DisplayPixelFormat.dwGBitMask;

								backB += MUL_FIXED(r,fontB);
								if (backB>DisplayPixelFormat.dwBBitMask) backB = DisplayPixelFormat.dwBBitMask;
								else backB &= DisplayPixelFormat.dwBBitMask;

								*backbufferPtr = (short)(backR|backG|backB);
							}
							fontimagePtr++;
							backbufferPtr++;
						}

						fontimageRowStartPtr += fontimagePitchInShorts;
						backbufferRowStartPtr += backbufferPitchInShorts;
					}
				}
#endif				
//				lpDDSBack->Unlock((LPVOID)ddsdback.lpSurface);
//				image_ptr->Unlock((LPVOID)ddsdimage.lpSurface);
			}
		}
	}
}

int
IndexedFont_Kerned_Column :: GetMaxWidth(void) const
{
	return R2Size_OverallImage.w;
}

int
IndexedFont_Kerned_Column :: GetWidth
(
	ProjChar ProjCh_In
) const
{
	unsigned int offsetTemp;

	if (ProjCh_In == ' ')
	{
		return SpaceWidth();
	}

	if (GetOffset(offsetTemp, ProjCh_In))
	{
		return FullWidthForOffset[ offsetTemp ];
	}
	else
	{
		return 0;
	}
}

int
IndexedFont_Kerned_Column :: GetXInc
(
	ProjChar currentProjCh,
	ProjChar nextProjCh
) const
{
	GLOBALASSERT(currentProjCh!='\0');
		// LOCALISEME

	if (currentProjCh == ' ')
	{
		return SpaceWidth();
	}

	if (!((nextProjCh>='0' && nextProjCh<=']') || (nextProjCh>='a' && nextProjCh<='}')))
	{
		return GetWidth(currentProjCh);
	}

	unsigned int currentOffset;

	if (GetOffset(currentOffset, currentProjCh))
	{
		unsigned int nextOffset;

		if (GetOffset(nextOffset, nextProjCh))
		{
			return XIncForOffset[currentOffset][nextOffset];
		}
		else
		{
			return FullWidthForOffset[currentOffset];
		}
	}
	else
	{
		return 0;
	}		
}

// static
IndexedFont_Kerned_Column* IndexedFont_Kerned_Column :: Create
(
	FontIndex I_Font_New,
	char* Filename,
	int HeightPerChar_New,
	int SpaceWidth_New,
	int ASCIICodeForInitialCharacter
)
{
/*
	IndexedFont_Kerned_Column* pFont = new IndexedFont_Kerned_Column
	(
		I_Font_New,
		Filename,
		HeightPerChar_New,
		SpaceWidth_New,
		ASCIICodeForInitialCharacter
	);
*/
	SCString::UpdateAfterFontChange( I_Font_New );

//	return pFont;
	return 0;
}


IndexedFont_Kerned_Column :: ~IndexedFont_Kerned_Column()
{
	GLOBALASSERT(image_ptr);
	ReleaseAvPTexture(image_ptr);
	image_ptr = NULL;
}

IndexedFont_Kerned_Column::IndexedFont_Kerned_Column(FontIndex I_Font_New, char* Filename, int HeightPerChar_New, int SpaceWidth_New, int ASCIICodeForInitialCharacter
) : IndexedFont_Kerned
	(
		I_Font_New
	),
	ASCIICodeForOffset0
	(
		ASCIICodeForInitialCharacter
	),
	HeightPerChar_Val(HeightPerChar_New),
	SpaceWidth_Val(SpaceWidth_New),
	NumChars(0)
{
	{
		uint32_t nWidth  = 0;
		uint32_t nHeight = 0;

		RimLoader indexFontRIM;
		std::string filePath = Filename;
		if (!indexFontRIM.Open("graphics\\" + filePath))
		{
			return;
		}

		indexFontRIM.GetDimensions(nWidth, nHeight);

#if 0
		//see if graphic can be found in fast file
		size_t fastFileLength;
		void const * pFastFileData = ffreadbuf(Filename,&fastFileLength);
		
		if (pFastFileData)
		{
			//load from fast file
			image_ptr = AwCreateTexture
			(
				"pxfXYB",
				pFastFileData,
				fastFileLength,
				(
					0
				),
				&nWidth,
				&nHeight
			);
		}
		else
		{
			//load graphic from rim file
			image_ptr = AwCreateTexture
			(
				"sfXYB",
				Filename,
				(
					0
				),
				&nWidth,
				&nHeight
			);
		}
#endif	
		R2Size_OverallImage.w = nWidth;
		R2Size_OverallImage.h = nHeight;

	}

//	GLOBALASSERT(image_ptr);

	GLOBALASSERT(R2Size_OverallImage.w > 0);
	GLOBALASSERT(R2Size_OverallImage.h > 0); 
	
	NumChars = (R2Size_OverallImage.h) / HeightPerChar_Val;

	GLOBALASSERT( NumChars < MAX_CHARS_IN_TALLFONT );

	for (int i = 0; i < NumChars; i++)
	{
		WindowsRectForOffset[i].top = (i*HeightPerChar_Val);
		WindowsRectForOffset[i].bottom = ((i+1)*HeightPerChar_Val);
		WindowsRectForOffset[i].left = 0;
		WindowsRectForOffset[i].right = R2Size_OverallImage.w;

		FullWidthForOffset[i] = R2Size_OverallImage.w;
	}

	UpdateWidths();
	UpdateXIncs();
}

void
IndexedFont_Kerned_Column :: UpdateWidths(void)
{
	// called by constructor

	// Test: read the surface:
	{
		// Read the data...
		{
			for (int iOffset=0;iOffset<NumChars;iOffset++)
			{
				int y = iOffset * HeightPerChar_Val;
				int x = ( R2Size_OverallImage.w - 1);

				#if 0
				db_logf1(("Character offset %i",iOffset));
				#endif

				while (x > 0)
				{
					if
					(
						bAnyNonTransparentPixelsInColumn
						(
							r2pos(x, y), // r2pos R2Pos_TopOfColumn,
							HeightPerChar_Val // int HeightOfColumn
							//&tempDDSurfaceDesc // LPDDSURFACEDESC lpDDSurfaceDesc
						)
					)
					{
						break;
							// and the current value of (x+1) is the width to use
					}

					x--;
				}

				SetWidth(iOffset, x+1);
			}
		}
	}
}

void
IndexedFont_Kerned_Column :: UpdateXIncs(void)
{
	size_t RowsToProcess = NumChars*GetHeight();
	int* minOpaqueX = new int[RowsToProcess];
	int* maxOpaqueX = new int[RowsToProcess];

	// Build tables of min/max opaque pixels in each row of the image:
	{
		for (int Row=0;Row<RowsToProcess;Row++)
		{
			// Find right-most pixel in row
			{
				int rightmostX=GetMaxWidth()-1;
				while (rightmostX>0)
				{
/*					if
					(
						bOpaque
						(
							&tempDDSurfaceDesc,
							rightmostX, // int x,
							Row
						)

					)

					{
						break;
					}
					else
					{
						rightmostX--;
					}
*/
				}
				
				maxOpaqueX[Row] = rightmostX;
			}
			
			// Find left-most pixel in row of second character
			{
				int leftmostX = 0;
				while (leftmostX < GetMaxWidth())
				{
#if 0 //bjd
					if
					(
						bOpaque
						(
							&tempDDSurfaceDesc,
							leftmostX, // int x,
							Row // int y
						)
					)

					{
						break;
					}
					else
					{
						leftmostX++;
					}
#endif
				}
				minOpaqueX[Row] = leftmostX;
			}
		}
	}
		// Use the table of opaque extents:

		for (int i = 0; i < NumChars; i++)
		{
			for (int j = 0; j < NumChars; j++)
			{
				int XInc = CalcXInc
				(
					i, // unsigned int currentOffset,
					j, // unsigned int nextOffset,
					minOpaqueX,
					maxOpaqueX
				);
				
				GLOBALASSERT(XInc>=0);
				#if 0
				GLOBALASSERT(XInc<=GetMaxWidth());
				#endif

				XIncForOffset[i][j] =XInc;
			}
		}
//	}

	// Destroy the table of opaque extents:
	{
		delete[] maxOpaqueX;
		delete[] minOpaqueX;
	}
}


// static
bool
IndexedFont_Kerned_Column :: bAnyNonTransparentPixelsInColumn
(
	r2pos R2Pos_TopOfColumn,
	int HeightOfColumn
)
{
	return FALSE;
}

int
IndexedFont_Kerned_Column :: CalcXInc
(
	unsigned int currentOffset,
	unsigned int nextOffset,
	int* minOpaqueX,
	int* maxOpaqueX
)
{
	GLOBALASSERT(currentOffset<NumChars);
	GLOBALASSERT(nextOffset<NumChars);
	GLOBALASSERT( minOpaqueX );
	GLOBALASSERT( maxOpaqueX );

	// Compare rows in the pair of images (they have the same height)
	// First idea:
	// Iterate through X-inc values, finding the smallest
	// one which is "valid" for all rows.
	// "Valid" means there's no overlapping of non-transparent pixels
	// when the second image is displaced by the X-inc.
	// Tried this, but it took too long.

	// Second attempt:
	// Iterate through all rows, finding smallest X-inc which is valid
	// for each row, and maintaining what is the biggest "smallest X-inc"
	// you have so far.  This will be the return value.

	{
		int Biggest_MinXInc = 0;
		for (int Row=0;Row<GetHeight();Row++)
		{
			int MinXIncForRow = GetSmallestXIncForRow
			(
				currentOffset,
				nextOffset,
				minOpaqueX,
				maxOpaqueX,
				Row
			);

			if (MinXIncForRow > Biggest_MinXInc)
			{
				Biggest_MinXInc = MinXIncForRow;
			}
		}

		return Biggest_MinXInc;
	}
}

bool
IndexedFont_Kerned_Column :: OverlapOnRow
(
	unsigned int currentOffset,
	unsigned int nextOffset,
	int Row,
	int ProposedXInc
)
{
	GLOBALASSERT(currentOffset<NumChars);
	GLOBALASSERT(nextOffset<NumChars);
	
	#if 1
	{
		// Find right-most pixel in row of first character
		int rightmostX = 0;
		int firstoffsetY = Row+(currentOffset*GetHeight());

#if 0 // bjd
		for (rightmostX=GetMaxWidth()-1;rightmostX>0;rightmostX--)
		{
			if
			(
				bOpaque
				(
					&tempDDSurfaceDesc, // LPDDSURFACEDESC lpDDSurfaceDesc
					rightmostX, // int x,
					firstoffsetY // int y
				)
			)
			else
			{
				break;
			}
		}
#endif
		
		// Find left-most pixel in row of second character
		int leftmostX = 0;
		int nextoffsetY = Row+(nextOffset*GetHeight());

#if 0 // bjd
		for (leftmostX=0;leftmostX<GetMaxWidth();leftmostX++)
		{
			if
			(
				bOpaque
				(
					&tempDDSurfaceDesc, // LPDDSURFACEDESC lpDDSurfaceDesc
					leftmostX, // int x,
					nextoffsetY // int y
				)
			)
			{
				break;
			}
		}
#endif

		// Is there an overlap when displaced by the proposed XInc?
		{
			return
			(
				(rightmostX)
				>=
				(leftmostX+ProposedXInc)
			);
		}
	}
	#else
	return TRUE;
		// for now
	#endif
}

int
IndexedFont_Kerned_Column :: GetSmallestXIncForRow
(
	unsigned int currentOffset,
	unsigned int nextOffset,
	int* minOpaqueX,
	int* maxOpaqueX,
	unsigned int Row
)
{
	GLOBALASSERT( currentOffset < NumChars );
	GLOBALASSERT( nextOffset < NumChars );
	GLOBALASSERT( minOpaqueX );
	GLOBALASSERT( maxOpaqueX );
	GLOBALASSERT( Row < GetHeight() );

	{
		int Difference =
		(
			maxOpaqueX[Row+(currentOffset*GetHeight())] - minOpaqueX[Row+(nextOffset*GetHeight())]
			+1
		);

		if (Difference>0)
		{
			return Difference;
		}
		else
		{
			return 0;
		}
	}
}


bool
IndexedFont_Kerned_Column :: bOpaque
(
	int x,
	int y
		// must be in range
)
{
	return true;
}
















/* Internal function definitions ***********************************/
