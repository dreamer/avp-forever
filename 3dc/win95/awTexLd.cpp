#ifndef DB_LEVEL
#define DB_LEVEL 4
#endif
#include "db.h"
#include <assert.h>

#ifndef NDEBUG
	#define HT_FAIL db_log1
	#include "hash_tem.hpp" // for the backup surfaces memory leak checking
#endif

#include "iff.hpp"

#include "list_tem.hpp"

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#include "awTexLd.h"
#include "awTexLd.hpp"

#ifdef _CPPRTTI
	#include <typeinfo.h>
#endif

/* awTexLd.cpp - Author: Jake Hotson */

/*****************************************/
/* Preprocessor switches for experiments */
/*****************************************/

#define MIPMAPTEST 0 // experiment to create mip map surfaces for textures, but doesn't bother putting any data into them

/*****************************/
/* DB_LEVEL dependent macros */
/*****************************/

#if DB_LEVEL >= 5
#define inline // prevent function inlining at level 5 debugging
#endif

/*****************************************************/
/* ZEROFILL and SETDWSIZE macros ensure that I won't */
/* accidentally get the parameters wrong             */
/*****************************************************/

// zero mem
template <class X>
static inline void ZEROFILL(X & x)
{
	memset(&x,0,sizeof(X));
}

// set dwSize
template <class X>
static inline void SETDWSIZE(X & x)
{
	x.dwSize = sizeof(X);
}

template <class X>
static inline void INITDXSTRUCT(X & x)
{
	ZEROFILL(x);
	SETDWSIZE(x);
}

/*****************************************************************/
/* Put everything I can in a namespace to avoid naming conflicts */
/*****************************************************************/

namespace AwTl
{
	/**************************************************/
	/* Allow breakpoints to be potentially hard coded */
	/* into macros and template functions             */
	/**************************************************/

	db_code5(void BrkPt(){})
	#define BREAKPOINT db_code5(::AwTl::BrkPt();)

	#if DB_LEVEL > 4
	static unsigned GetRefCount(IUnknown * pUnknown)
	{
		if (!pUnknown) return 0;
		pUnknown->AddRef();
		return static_cast<unsigned>(pUnknown->Release());
	}
	#endif

	/*********************************/
	/* Pixel format global structure */
	/*********************************/

	PixelFormat pixelFormat;

	PixelFormat pfTextureFormat;
	PixelFormat pfSurfaceFormat;

	static inline void SetBitShifts(unsigned * leftShift,unsigned * rightShift,unsigned mask)
	{
		if (!mask)
			*leftShift = 0;
		else
			for (*leftShift = 0; !(mask & 1); ++*leftShift, mask>>=1)
				;
		for (*rightShift = 8; mask; --*rightShift, mask>>=1)
			;
	}

	/************************************/
	/* D3D Driver info global structure */
	/************************************/

	static
	struct DriverDesc
	{
		DriverDesc() : validB(false) {}

		bool validB : 1;
		bool needSquareB : 1;
		bool needPow2B : 1;

		unsigned minWidth;
		unsigned minHeight;
		unsigned maxWidth;
		unsigned maxHeight;

		DWORD memFlag;
	}
		driverDesc;

	/*************************************************************************/
	/* Class used to hold all the parameters for the CreateTexture functions */
	/*************************************************************************/

	class CreateTextureParms
	{
		public:
			inline CreateTextureParms()
				: fileNameS(NULL)
				, fileH(INVALID_HANDLE_VALUE)
				, dataP(NULL)
//				, restoreH(NULL)
				, maxReadBytes(UINT_MAX)
				, bytesReadP(NULL)
				, flags(AW_TLF_DEFAULT)
				, originalWidthP(NULL)
				, originalHeightP(NULL)
				, widthP(NULL)
				, heightP(NULL)
//				, backupHP(NULL)
				, prevTexP(static_cast<AVPTEXTURE *>(NULL))
				, prevTexB(false)
				, loadTextureB(false)
				, callbackF(NULL)
				, rectA(NULL)
			{
			}

			SurfUnion DoCreate() const;

			bool loadTextureB;

			LPCTSTR fileNameS;
			HANDLE fileH;
			PtrUnionConst dataP;

			unsigned maxReadBytes;
			unsigned * bytesReadP;

			unsigned flags;

			unsigned * widthP;
			unsigned * heightP;

			unsigned * originalWidthP;
			unsigned * originalHeightP;

			SurfUnion prevTexP;
			bool prevTexB; // used when rectA is non-NULL, otherwise prevTexP is used

			AW_TL_PFN_CALLBACK callbackF;
			void * callbackParam;

			unsigned numRects;
			AwCreateGraphicRegion * rectA;
	};


	/****************************************/
	/* Reference Count Object Debug Support */
	/****************************************/

	#ifndef NDEBUG

		static bool g_bAllocListActive = false;

		class AllocList : public ::HashTable<RefCntObj *>
		{
			public:
				AllocList()
				{
					g_bAllocListActive = true;
				}
				~AllocList()
				{
					if (Size())
					{
						db_log1(("AW: Potential Memory Leaks Detected!!!"));
					}
					#ifdef _CPPRTTI
						#pragma message("Run-Time Type Identification (RTTI) is enabled")
						for (Iterator itLeak(*this) ; !itLeak.Done() ; itLeak.Next())
						{
							db_logf1(("\tAW Object not deallocated: Type: %s RefCnt: %u",typeid(*itLeak.Get()).name(),itLeak.Get()->m_nRefCnt));
						}
						if (Size())
						{
							db_log1(("AW: Object dump complete"));
						}
					#else // ! _CPPRTTI
						#pragma message("Run-Time Type Identification (RTTI) is not enabled - memory leak checking will not report types")
						unsigned nRefs(0);
						for (Iterator itLeak(*this) ; !itLeak.Done() ; itLeak.Next())
						{
							nRefs += itLeak.Get()->m_nRefCnt;
						}
						if (Size())
						{
							db_logf1(("AW: Objects not deallocated: Number of Objects: %u Number of References: %u",Size(),nRefs));
						}
					#endif // ! _CPPRTTI
					g_bAllocListActive = false;
				}
		};

		static AllocList g_listAllocated;

		void DbRemember(RefCntObj * pObj)
		{
			g_listAllocated.AddAsserted(pObj);
		}

		void DbForget(RefCntObj * pObj)
		{
			if (g_bAllocListActive)
				g_listAllocated.RemoveAsserted(pObj);
		}

	#endif // ! NDEBUG

	/********************************************/
	/* structure to contain loading information */
	/********************************************/

	struct LoadInfo
	{
		AVPTEXTURE * textureP;

		unsigned surface_width;
		unsigned surface_height;
		LONG surface_pitch;

		unsigned * widthP;
		unsigned * heightP;
		SurfUnion prevTexP;
		SurfUnion resultP;
		unsigned top,left,bottom,right;
		unsigned width,height; // set to right-left and bottom-top

		AwCreateGraphicRegion * rectP;

		bool skipB; // used to indicate that a surface/texture was not lost and .`. does not need restoring

		LoadInfo()
			: textureP(NULL)
			, skipB(false)
		{
		}
	};

	/*******************************/
	/* additiional texture formats */
	/*******************************/

	struct AdditionalPixelFormat : PixelFormat
	{
		bool canDoTranspB;
		unsigned maxColours;

		// for List
		bool operator == (AdditionalPixelFormat const &) const { return false; }
		bool operator != (AdditionalPixelFormat const &) const { return true; }
	};

	static List<AdditionalPixelFormat> listTextureFormats;

} // namespace AwTl

/*******************/
/* Generic Loaders */
/*******************/

#define HANDLE_DXERROR(s) \
	if (DD_OK != awTlLastDxErr)	{ \
		awTlLastErr = AW_TLE_DXERROR; \
		db_logf3(("AwCreateGraphic() failed whilst %s",s)); \
		db_log1("AwCreateGraphic(): ERROR: DirectX SDK call failed"); \
		goto EXIT_WITH_ERROR; \
	} else { \
		db_logf5(("\tsuccessfully completed %s",s)); \
	}

#define ON_ERROR_RETURN_NULL(s) \
	if (awTlLastErr != AW_TLE_OK) { \
		db_logf3(("AwCreateGraphic() failed whilst %s",s)); \
		db_logf1(("AwCreateGraphic(): ERROR: %s",AwTlErrorToString())); \
		return static_cast<AVPTEXTURE *>(NULL); \
	} else { \
		db_logf5(("\tsuccessfully completed %s",s)); \
	}

#define CHECK_MEDIA_ERRORS(s) \
	if (pMedium->m_fError) { \
		db_logf3(("AwCreateGraphic(): The following media errors occurred whilst %s",s)); \
		if (pMedium->m_fError & MediaMedium::MME_VEOFMET) { \
			db_log3("\tA virtual end of file was met"); \
			if (awTlLastErr == AW_TLE_OK) awTlLastErr = AW_TLE_EOFMET; \
		} \
		if (pMedium->m_fError & MediaMedium::MME_EOFMET) { \
			db_log3("\tAn actual end of file was met"); \
			if (awTlLastErr == AW_TLE_OK) awTlLastErr = AW_TLE_EOFMET; \
		} \
		if (pMedium->m_fError & MediaMedium::MME_OPENFAIL) { \
			db_log3("\tThe file could not be opened"); \
			if (awTlLastErr == AW_TLE_OK) { awTlLastErr = AW_TLE_CANTOPENFILE; awTlLastWinErr = GetLastError(); } \
		} \
		if (pMedium->m_fError & MediaMedium::MME_CLOSEFAIL) { \
			db_log3("\tThe file could not be closed"); \
			if (awTlLastErr == AW_TLE_OK) { awTlLastErr = AW_TLE_CANTOPENFILE; awTlLastWinErr = GetLastError(); } \
		} \
		if (pMedium->m_fError & MediaMedium::MME_UNAVAIL) { \
			db_log3("\tA requested operation was not available"); \
			if (awTlLastErr == AW_TLE_OK) { awTlLastErr = AW_TLE_CANTREADFILE; awTlLastWinErr = GetLastError(); } \
		} \
		if (pMedium->m_fError & MediaMedium::MME_IOERROR) { \
			db_log3("\tA read error occurred"); \
			if (awTlLastErr == AW_TLE_OK) { awTlLastErr = AW_TLE_CANTREADFILE; awTlLastWinErr = GetLastError(); } \
		} \
	}

AwTl::SurfUnion AwBackupTexture::Restore(AwTl::CreateTextureParms const & rParams)
{
	using namespace AwTl;

	ChoosePixelFormat(rParams);

	if (!pixelFormat.validB)
		db_log3("AwCreateGraphic(): ERROR: pixel format not valid");
	if (/*!driverDesc.ddP ||*/ !driverDesc.validB && rParams.loadTextureB)
		db_log3("AwCreateGraphic(): ERROR: driver description not valid");

	awTlLastErr = pixelFormat.validB /*&& driverDesc.ddP*/ && (driverDesc.validB || !rParams.loadTextureB) ? AW_TLE_OK : AW_TLE_NOINIT;

	ON_ERROR_RETURN_NULL("initializing restore")

	OnBeginRestoring(pixelFormat.palettizedB ? 1<<pixelFormat.bitsPerPixel : 0);

	ON_ERROR_RETURN_NULL("initializing restore")

	SurfUnion pTex = CreateTexture(rParams);

	OnFinishRestoring(AW_TLE_OK == awTlLastErr ? true : false);

	return pTex;
}

bool AwBackupTexture::HasTransparentMask(bool bDefault)
{
	return bDefault;
}

DWORD AwBackupTexture::GetTransparentColour()
{
	return 0;
}

void AwBackupTexture::ChoosePixelFormat(AwTl::CreateTextureParms const & _parmsR)
{
	using namespace AwTl;

	pixelFormat.validB = false; // set invalid first

	// which flags to use?
/*
	unsigned fMyFlags =
		_parmsR.flags & AW_TLF_PREVSRCALL ? db_assert1(_parmsR.restoreH), m_fFlags
		: _parmsR.flags & AW_TLF_PREVSRC ? db_assert1(_parmsR.restoreH),
			_parmsR.flags & ~AW_TLF_TRANSP | m_fFlags & AW_TLF_TRANSP
		: _parmsR.flags;
*/
	uint32_t fMyFlags = _parmsR.flags & AW_TLF_PREVSRC ? _parmsR.flags & ~AW_TLF_TRANSP | m_fFlags & AW_TLF_TRANSP : _parmsR.flags;

	// transparency?
	m_bTranspMask = HasTransparentMask(fMyFlags & AW_TLF_TRANSP ? true : false);

	if (_parmsR.loadTextureB || fMyFlags & AW_TLF_TEXTURE)
	{
		// Code from linux port..
		/* Just convert the texture to 32bpp */
		pixelFormat.palettizedB = 0;
		
		pixelFormat.alphaB = 1;
		pixelFormat.validB = 1;
		pixelFormat.texB = 1;
		pixelFormat.bitsPerPixel = 32;
		pixelFormat.redLeftShift = 0;
		pixelFormat.greenLeftShift = 8;
		pixelFormat.blueLeftShift = 16;
		pixelFormat.redRightShift = 0;
		pixelFormat.greenRightShift = 0;
		pixelFormat.blueRightShift = 0;
		pixelFormat.dwRGBAlphaBitMask = 0xFF000000;
	}
}

AwTl::SurfUnion AwBackupTexture::CreateTexture(AwTl::CreateTextureParms const & _parmsR)
{
	using namespace AwTl;

	if (_parmsR.originalWidthP) *_parmsR.originalWidthP = m_nWidth;
	if (_parmsR.originalHeightP) *_parmsR.originalHeightP = m_nHeight;

	AVPTEXTURE *d3d_texture = (AVPTEXTURE*)malloc(sizeof(AVPTEXTURE));

	uint8_t *buffer = (uint8_t *)malloc(m_nWidth * m_nHeight * sizeof(uint32_t));

	unsigned int y = 0;

	Colour * paletteP = m_nPaletteSize ? GetPalette() : NULL;

	bool reversed_rowsB = AreRowsReversed();
	
	if (reversed_rowsB)
	{
		y = m_nHeight-1;
	}

	for (int i = 0, rowcount = m_nHeight; rowcount; --rowcount, i++)
	{	
		PtrUnion src_rowP = GetRowPtr(y);
		db_assert1(src_rowP.voidP);
				
		// allow loading of the next row from the file
		LoadNextRow(src_rowP);
			
		// loop for copying data to surfaces
		{	
			{
				// are we in the vertical range of this surface?
				{		
					// convert and copy the section of the row to the direct draw surface
//					ConvertRow(pLoadInfo->surface_dataP,pLoadInfo->surface_width,src_rowP,pLoadInfo->left,pLoadInfo->width,paletteP db_code1(DB_COMMA m_nPaletteSize));
					PtrUnion my_data = &buffer[y*m_nWidth*4];

					ConvertRow(my_data, m_nWidth, src_rowP, 0, m_nWidth, paletteP db_code1(DB_COMMA m_nPaletteSize));					
				}
			}
		}
				
		// next row
		if (reversed_rowsB)
			--y;
		else
			++y;
	}
		
	db_logf4(("\tThe image with is %ux%u with %u %spalette",m_nWidth,m_nHeight,m_nPaletteSize ? m_nPaletteSize : 0,m_nPaletteSize ? "colour " : ""));

	d3d_texture->width = m_nWidth;
	d3d_texture->height = m_nHeight;

	d3d_texture->buffer = buffer;
				
	return static_cast<SurfUnion>(d3d_texture);
}

void AwBackupTexture::OnBeginRestoring(unsigned nMaxPaletteSize)
{
	if (nMaxPaletteSize && (nMaxPaletteSize < m_nPaletteSize || !m_nPaletteSize))
	{
		awTlLastErr = AW_TLE_CANTPALETTIZE;
		db_logf3(("AwCreateGraphic(): [restoring] ERROR: Palette size is %u, require %u",m_nPaletteSize,nMaxPaletteSize));
	}
}

bool AwBackupTexture::AreRowsReversed()
{
	return false;
}

void AwBackupTexture::ConvertRow(AwTl::PtrUnion pDest, unsigned nDestWidth, AwTl::PtrUnionConst pSrc, unsigned nSrcOffset, unsigned nSrcWidth, AwTl::Colour * pPalette db_code1(DB_COMMA unsigned nPaletteSize))
{
	using namespace AwTl;

	if (pPalette)
	{
		if (pixelFormat.palettizedB)
		{
			GenericConvertRow<Colour::ConvNull,BYTE>::Do(pDest,nDestWidth,pSrc.byteP+nSrcOffset,nSrcWidth);
		}
		else
		{
			if (m_bTranspMask)
				GenericConvertRow<Colour::ConvTransp,BYTE>::Do(pDest,nDestWidth,pSrc.byteP+nSrcOffset,nSrcWidth,pPalette db_code1(DB_COMMA nPaletteSize));
			else
				GenericConvertRow<Colour::ConvNonTransp,BYTE>::Do(pDest,nDestWidth,pSrc.byteP+nSrcOffset,nSrcWidth,pPalette db_code1(DB_COMMA nPaletteSize));
		}
	}
	else
	{
		if (m_bTranspMask)
			GenericConvertRow<Colour::ConvTransp,Colour>::Do(pDest,nDestWidth,pSrc.colourP+nSrcOffset,nSrcWidth);
		else
			GenericConvertRow<Colour::ConvNonTransp,Colour>::Do(pDest,nDestWidth,pSrc.colourP+nSrcOffset,nSrcWidth);
	}
}

void AwBackupTexture::OnFinishRestoring(bool)
{
}

namespace AwTl {

	Colour * TypicalBackupTexture::GetPalette()
	{
		return m_pPalette;
	}

	PtrUnion TypicalBackupTexture::GetRowPtr(unsigned nRow)
	{
		return m_ppPixMap[nRow];
	}

	void TypicalBackupTexture::LoadNextRow(PtrUnion)
	{
		// already loaded
	}

	unsigned TypicalBackupTexture::GetNumColours()
	{
		return m_nPaletteSize;
	}

	unsigned TypicalBackupTexture::GetMinPaletteSize()
	{
		return m_nPaletteSize;
	}


	SurfUnion TexFileLoader::Load(MediaMedium * pMedium, CreateTextureParms const & rParams)
	{
		m_fFlags = rParams.flags;

		awTlLastErr = AW_TLE_OK;

		LoadHeaderInfo(pMedium);

//		CHECK_MEDIA_ERRORS("loading file headers")
		ON_ERROR_RETURN_NULL("loading file headers")

		ChoosePixelFormat(rParams);
#if 0 // bjd
		if (!pixelFormat.validB)
			db_log3("AwCreateGraphic(): ERROR: pixel format not valid");
		if (!driverDesc.ddP || !driverDesc.validB && rParams.loadTextureB)
			db_log3("AwCreateGraphic(): ERROR: driver description not valid");

		awTlLastErr = pixelFormat.validB && driverDesc.ddP && (driverDesc.validB || !rParams.loadTextureB) ? AW_TLE_OK : AW_TLE_NOINIT;

		ON_ERROR_RETURN_NULL("initializing load")

		AllocateBuffers(rParams.backupHP ? true : false, pixelFormat.palettizedB ? 1<<pixelFormat.bitsPerPixel : 0);
#endif
		AllocateBuffers(false, 0); // no backup, not paletised

//		CHECK_MEDIA_ERRORS("allocating buffers")
		ON_ERROR_RETURN_NULL("allocating buffers")

//bjd off for now		db_logf4(("\tThe image in the file is %ux%u with %u %spalette",m_nWidth,m_nHeight,m_nPaletteSize ? m_nPaletteSize : 0,m_nPaletteSize ? "colour " : ""));

		SurfUnion pTex = CreateTexture(rParams);

		bool bOK = AW_TLE_OK == awTlLastErr;

		CHECK_MEDIA_ERRORS("loading image data")

		if (bOK && awTlLastErr != AW_TLE_OK)
		{
#if 0 // bjd
			// an error occurred which was not detected in CreateTexture()
			if (pTex.voidP)
			{
				if (rParams.loadTextureB)
					pTex.textureP->Release();
				else
					pTex.surfaceP->Release();
				pTex.voidP = NULL;
			}
			else
			{
				db_assert1(rParams.rectA);

				for (unsigned i=0; i<rParams.numRects; ++i)
				{
					AwCreateGraphicRegion * pRect = &rParams.rectA[i];

					if (!rParams.prevTexB)
					{
						// release what was created
						if (rParams.loadTextureB)
							pRect->pTexture->Release();
						else
							pRect->pSurface->Release();
					}
					pRect->width = 0;
					pRect->height = 0;
				}
			}
			db_logf1(("AwCreateGraphic(): ERROR: %s",AwTlErrorToString()));
#endif
			bOK = false;
		}

		OnFinishLoading(bOK);

		return pTex;
	}

	void TexFileLoader::OnFinishLoading(bool)
	{
	}


	TypicalTexFileLoader::~TypicalTexFileLoader()
	{
		if (m_pPalette)
		{
			delete[] m_pPalette;

			if (m_pRowBuf) delete[] m_pRowBuf.byteP;
			if (m_ppPixMap)
			{
				delete[] m_ppPixMap->byteP;
				delete[] m_ppPixMap;
			}
		}
		else
		{
			if (m_pRowBuf) delete[] m_pRowBuf.colourP;
			if (m_ppPixMap)
			{
				delete[] m_ppPixMap->colourP;
				delete[] m_ppPixMap;
			}
		}
	}

	unsigned TypicalTexFileLoader::GetNumColours()
	{
		return m_nPaletteSize;
	}

	unsigned TypicalTexFileLoader::GetMinPaletteSize()
	{
		return m_nPaletteSize;
	}

	void TypicalTexFileLoader::AllocateBuffers(bool bWantBackup, unsigned /*nMaxPaletteSize*/)
	{
		if (m_nPaletteSize)
		{
			m_pPalette = new Colour [ m_nPaletteSize ];
		}

		if (bWantBackup)
		{
			m_ppPixMap = new PtrUnion [m_nHeight];
			if (m_nPaletteSize)
			{
				m_ppPixMap->byteP = new BYTE [m_nHeight*m_nWidth];
				BYTE * pRow = m_ppPixMap->byteP;
				for (unsigned y=1;y<m_nHeight;++y)
				{
					pRow += m_nWidth;
					m_ppPixMap[y].byteP = pRow;
				}
			}
			else
			{
				m_ppPixMap->colourP = new Colour [m_nHeight*m_nWidth];
				Colour * pRow = m_ppPixMap->colourP;
				for (unsigned y=1;y<m_nHeight;++y)
				{
					pRow += m_nWidth;
					m_ppPixMap[y].colourP = pRow;
				}
			}
		}
		else
		{
			if (m_nPaletteSize)
				m_pRowBuf.byteP = new BYTE [m_nWidth];
			else
				m_pRowBuf.colourP = new Colour [m_nWidth];
		}
	}

	PtrUnion TypicalTexFileLoader::GetRowPtr(unsigned nRow)
	{
		if (m_ppPixMap)
		{
			return m_ppPixMap[nRow];
		}
		else
		{
			return m_pRowBuf;
		}
	}

	AwBackupTexture * TypicalTexFileLoader::CreateBackupTexture()
	{
		AwBackupTexture * pBackup = new TypicalBackupTexture(*this,m_ppPixMap,m_pPalette);
		m_ppPixMap = NULL;
		m_pPalette = NULL;
		return pBackup;
	}

	/****************************************************************************/
	/* For determining which loader should be used for the file format detected */
	/****************************************************************************/

	static
	class MagicFileIdTree
	{
		public:
			MagicFileIdTree()
				: m_pfnCreate(NULL)
				#ifdef _MSC_VER
				, hack(0)
				#endif
			{
				for (unsigned i=0; i<256; ++i)
					m_arrNextLayer[i]=NULL;
			}

			~MagicFileIdTree()
			{
				for (unsigned i=0; i<256; ++i)
					if (m_arrNextLayer[i]) delete m_arrNextLayer[i];
			}

			MagicFileIdTree * m_arrNextLayer [256];

			TexFileLoader * (* m_pfnCreate) ();

		#ifdef _MSC_VER
			unsigned hack;
		#endif
	}
		* g_pMagicFileIdTree = NULL;

	void RegisterLoader(char const * pszMagic, TexFileLoader * (* pfnCreate) () )
	{
		static MagicFileIdTree mfidt;

		g_pMagicFileIdTree = &mfidt;

		MagicFileIdTree * pLayer = g_pMagicFileIdTree;

		while (*pszMagic)
		{
			BYTE c = static_cast<BYTE>(*pszMagic++);

			if (!pLayer->m_arrNextLayer[c])
				pLayer->m_arrNextLayer[c] = new MagicFileIdTree;

			pLayer = pLayer->m_arrNextLayer[c];
		}

		db_assert1(!pLayer->m_pfnCreate);

		pLayer->m_pfnCreate = pfnCreate;
	}

	static
	TexFileLoader * CreateLoaderObject(MediaMedium * pMedium)
	{
		TexFileLoader * (* pfnBest) () = NULL;

		signed nMoveBack = 0;

		BYTE c;

		MagicFileIdTree * pLayer = g_pMagicFileIdTree;

		while (pLayer)
		{
			if (pLayer->m_pfnCreate)
				pfnBest = pLayer->m_pfnCreate;

			MediaRead(pMedium,&c);

			-- nMoveBack;

			pLayer = pLayer->m_arrNextLayer[c];
		}

		pMedium->MovePos(nMoveBack);

		if (pfnBest)
			return pfnBest();
		else
			return NULL;
	}

	/**********************************/
	/* These are the loader functions */
	/**********************************/

	static inline SurfUnion DoLoadTexture(MediaMedium * pMedium, CreateTextureParms const & rParams)
	{
		TexFileLoader * pLoader = CreateLoaderObject(pMedium);

		if (!pLoader)
		{
			awTlLastErr = AW_TLE_BADFILEFORMAT;
			db_log1("AwCreateGraphic(): ERROR: file format not recognized");
			return static_cast<AVPTEXTURE *>(NULL);
		}
		else
		{
			SurfUnion pTex = pLoader->Load(pMedium,rParams);
			pLoader->Release();
			return pTex;
		}
	}

	static inline SurfUnion LoadTexture(MediaMedium * pMedium, CreateTextureParms const & _parmsR)
	{
		if (_parmsR.bytesReadP||_parmsR.maxReadBytes!=UINT_MAX)
		{
			MediaSection * pMedSect = new MediaSection;
			pMedSect->Open(pMedium,_parmsR.maxReadBytes);
			SurfUnion pTex = DoLoadTexture(pMedSect,_parmsR);
			pMedSect->Close();
			if (_parmsR.bytesReadP) *_parmsR.bytesReadP = pMedSect->GetUsedSize();
			delete pMedSect;
			return pTex;
		}
		else
		{
			return DoLoadTexture(pMedium,_parmsR);
		}
	}

	SurfUnion CreateTextureParms::DoCreate() const
	{
		if (INVALID_HANDLE_VALUE!=fileH)
		{
			MediaWinFileMedium * pMedium = new MediaWinFileMedium;
			pMedium->Attach(fileH);
			SurfUnion pTex = LoadTexture(pMedium,*this);
			pMedium->Detach();
			pMedium->Release();
			return pTex;
		}
		else if (dataP)
		{
			// bjd - we're hitting here on menu graphics load
			MediaMemoryReadMedium * pMedium = new MediaMemoryReadMedium;
			pMedium->Open(dataP);
			SurfUnion pTex = LoadTexture(pMedium,*this);
			pMedium->Close();
			pMedium->Release();
			return pTex;
		}
		else
		{
			assert(1 == 0);
			//db_assert1(restoreH);
			//return restoreH->Restore(*this);
			return NULL;
		}
	}

	// Parse the format string and get the parameters

	static bool ParseParams(CreateTextureParms * pParams, char const * _argFormatS, va_list ap)
	{
		bool bad_parmsB = false;
		db_code2(unsigned ch_off = 0;)
		db_code2(char ch = 0;)

		while (*_argFormatS && !bad_parmsB)
		{
			db_code2(++ch_off;)
			db_code2(ch = *_argFormatS;)
			switch (*_argFormatS++)
			{
				case 's':
					if (pParams->fileNameS || INVALID_HANDLE_VALUE!=pParams->fileH || pParams->dataP/* || pParams->restoreH*/)
						bad_parmsB = true;
					else
					{
						pParams->fileNameS = va_arg(ap,LPCTSTR);
						db_logf4(("\tFilename = \"%s\"",pParams->fileNameS));
					}
					break;
				case 'h':
					if (pParams->fileNameS || INVALID_HANDLE_VALUE!=pParams->fileH || pParams->dataP/* || pParams->restoreH*/)
						bad_parmsB = true;
					else
					{
						pParams->fileH = va_arg(ap,HANDLE);
						db_logf4(("\tFile HANDLE = 0x%08x",pParams->fileH));
					}
					break;
				case 'p':
					if (pParams->fileNameS || INVALID_HANDLE_VALUE!=pParams->fileH || pParams->dataP /*|| pParams->restoreH*/)
						bad_parmsB = true;
					else
					{
						pParams->dataP = va_arg(ap,void const *);
						db_logf4(("\tData Pointer = %p",pParams->dataP));
					}
					break;
				case 'r':
/*
					if (pParams->fileNameS || INVALID_HANDLE_VALUE!=pParams->fileH || pParams->dataP || pParams->restoreH || UINT_MAX!=pParams->maxReadBytes || pParams->bytesReadP || pParams->backupHP)
						bad_parmsB = true;
					else
					{
						pParams->restoreH = va_arg(ap,AW_BACKUPTEXTUREHANDLE);
						db_logf4(("\tRestore Handle = 0x%08x",pParams->restoreH));
					}
*/
					break;
				case 'x':
					if (UINT_MAX!=pParams->maxReadBytes/* || pParams->restoreH*/)
						bad_parmsB = true;
					else
					{
						pParams->maxReadBytes = va_arg(ap,unsigned);
						db_logf4(("\tMax bytes to read = %u",pParams->maxReadBytes));
					}
					break;
				case 'N':
					if (pParams->bytesReadP/* || pParams->restoreH*/)
						bad_parmsB = true;
					else
					{
						pParams->bytesReadP = va_arg(ap,unsigned *);
						db_logf4(("\tPtr to bytes read = %p",pParams->bytesReadP));
					}
					break;
				case 'f':
					if (AW_TLF_DEFAULT!=pParams->flags)
						bad_parmsB = true;
					else
					{
						pParams->flags = va_arg(ap,unsigned);
						db_logf4(("\tFlags = 0x%08x",pParams->flags));
					}
					break;
				case 'W':
					if (pParams->widthP || pParams->rectA)
						bad_parmsB = true;
					else
					{
						pParams->widthP = va_arg(ap,unsigned *);
						db_logf4(("\tPtr to width = %p",pParams->widthP));
					}
					break;
				case 'H':
					if (pParams->heightP || pParams->rectA)
						bad_parmsB = true;
					else
					{
						pParams->heightP = va_arg(ap,unsigned *);
						db_logf4(("\tPtr to height = %p",pParams->heightP));
					}
					break;
				case 'X':
					if (pParams->originalWidthP)
						bad_parmsB = true;
					else
					{
						pParams->originalWidthP = va_arg(ap,unsigned *);
						db_logf4(("\tPtr to image width = %p",pParams->originalWidthP));
					}
					break;
				case 'Y':
					if (pParams->originalHeightP)
						bad_parmsB = true;
					else
					{
						pParams->originalHeightP = va_arg(ap,unsigned *);
						db_logf4(("\tPtr to image height = %p",pParams->originalHeightP));
					}
					break;
				case 'B':
/*
					if (pParams->backupHP || pParams->restoreH)
						bad_parmsB = true;
					else
					{
						pParams->backupHP = va_arg(ap,AW_BACKUPTEXTUREHANDLE *);
						db_logf4(("\tPtr to backup handle = %p",pParams->backupHP));
					}
*/
					break;
				case 't':
					if (pParams->prevTexP.voidP)
						bad_parmsB = true;
					else if (pParams->rectA)
					{
						pParams->prevTexB = true;
						db_log4("\tPrevious DDSurface * or D3DTexture * in rectangle array");
					}
					else if (pParams->loadTextureB)
					{
						pParams->prevTexP = va_arg(ap,AVPTEXTURE *);
						db_logf4(("\tPrevious D3DTexture * = %p",pParams->prevTexP.textureP));
					}
					break;
				case 'c':
					if (pParams->callbackF)
						bad_parmsB = true;
					else
					{
						pParams->callbackF = va_arg(ap,AW_TL_PFN_CALLBACK);
						pParams->callbackParam = va_arg(ap,void *);
						db_logf4(("\tCallback function = %p, param = %p",pParams->callbackF,pParams->callbackParam));
					}
					break;
				case 'a':
					if (pParams->prevTexP.voidP || pParams->rectA || pParams->widthP || pParams->heightP)
						bad_parmsB = true;
					else
					{
						pParams->numRects = va_arg(ap,unsigned);
						pParams->rectA = va_arg(ap,AwCreateGraphicRegion *);
						db_logf4(("\tRectangle array = %p, size = %u",pParams->rectA,pParams->numRects));
					}
					break;
				default:
					bad_parmsB = true;
			}
		}

		if (!pParams->fileNameS && INVALID_HANDLE_VALUE==pParams->fileH && !pParams->dataP/* && !pParams->restoreH*/)
		{
			awTlLastErr = AW_TLE_BADPARMS;
			db_log2("AwCreateGraphic(): ERROR: FALSE data medium is specified");
			return false;
		}
		else if (bad_parmsB)
		{
			awTlLastErr = AW_TLE_BADPARMS;
			db_logf2(("AwCreateGraphic(): ERROR: Unexpected '%c' in format string at character %u",ch,ch_off));
			return false;
		}
		else
		{
			db_log5("\tParameters are OK");
			return true;
		}
	}

	// Use the parameters parsed to load the surface or texture

	SurfUnion LoadFromParams(CreateTextureParms * pParams)
	{
		if (pParams->fileNameS)
		{
			// opens a file, not creates one ;)
			pParams->fileH = avp_CreateFile(pParams->fileNameS,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

			if (INVALID_HANDLE_VALUE==pParams->fileH)
			{
				awTlLastErr = AW_TLE_CANTOPENFILE;
				awTlLastWinErr = GetLastError();
				db_logf1(("AwCreateGraphic(): ERROR opening file \"%s\"",pParams->fileNameS));
				db_log2(AwTlErrorToString());
				return static_cast<AVPTEXTURE *>(NULL);
			}

			SurfUnion textureP = pParams->DoCreate();

			CloseHandle(pParams->fileH);

			return textureP;
		}
		else return pParams->DoCreate();
	}

} // namespace AwTl

/******************************/
/* PUBLIC: AwSetTextureFormat */
/******************************/

#define IS_VALID_MEMBER(sP,mem) (reinterpret_cast<unsigned>(&(sP)->mem) - reinterpret_cast<unsigned>(sP) < static_cast<unsigned>((sP)->dwSize))

#if defined(__WATCOMC__) && (__WATCOMC__ <= 1100) // currently Watcom compiler crashes when the macro is expanded with the db_logf code in it
#define GET_VALID_MEMBER(sP,mem,deflt) (IS_VALID_MEMBER(sP,mem) ? (sP)->mem : (deflt))
#else
#define GET_VALID_MEMBER(sP,mem,deflt) (IS_VALID_MEMBER(sP,mem) ? (sP)->mem : (db_logf4((FUNCTION_NAME ": WARNING: %s->%s is not valid",#sP ,#mem )),(deflt)))
#endif

#define HANDLE_INITERROR(test,s) \
	if (!(test)) { \
		db_logf3((FUNCTION_NAME " failed becuse %s",s)); \
		db_log1(FUNCTION_NAME ": ERROR: unexpected parameters"); \
		return AW_TLE_BADPARMS; \
	} else { \
		db_logf5(("\t" FUNCTION_NAME " passed check '%s'",#test )); \
	}

#define FUNCTION_NAME "AwSetPixelFormat()"
static AW_TL_ERC AwSetPixelFormat(AwTl::PixelFormat * _pfP, void * _ddpfP)
{
	using AwTl::SetBitShifts;

//fprintf(stderr, "AwSetPixelFormat(%p, %p)\n", _pfP, _ddpfP);

	_pfP->validB = false;
	_pfP->palettizedB = true;
	_pfP->validB = true;
	_pfP->palettizedB = 0;
	_pfP->alphaB = 0;
	_pfP->validB = 1;
	_pfP->bitsPerPixel = 32;
	_pfP->redLeftShift = 0;
	_pfP->greenLeftShift = 8;
	_pfP->blueLeftShift = 16;
	_pfP->redRightShift = 0;
	_pfP->greenRightShift = 0;
	_pfP->blueRightShift = 0;
	_pfP->dwRGBAlphaBitMask = 0xFF000000;

	return AW_TLE_OK;
}

/******************************/
/* PUBLIC: AwCreate functions */
/******************************/

AVPTEXTURE * _AWTL_VARARG AwCreateTexture(char const * _argFormatS, ...)
{
	db_logf4(("AwCreateTexture(\"%s\") called",_argFormatS));

	using namespace AwTl;

	va_list ap;
	va_start(ap,_argFormatS);
	CreateTextureParms parms;
	parms.loadTextureB = true;
	bool bParmsOK = ParseParams(&parms, _argFormatS, ap);
	va_end(ap);
	return bParmsOK ? LoadFromParams(&parms).textureP : NULL;
}

/*
AW_TL_ERC AwDestroyBackupTexture(AW_BACKUPTEXTUREHANDLE _bH)
{
	db_logf4(("AwDestroyBackupTexture(0x%08x) called",_bH));
	if (_bH)
	{
		_bH->Release();
		return AW_TLE_OK;
	}
	else
	{
		db_log1("AwDestroyBackupTexture(): ERROR: AW_BACKUPTEXTUREHANDLE==NULL");
		return AW_TLE_BADPARMS;
	}
}
*/

/*********************************/
/* PUBLIC DEBUG: LastErr globals */
/*********************************/

AW_TL_ERC awTlLastErr;
HRESULT awTlLastDxErr;
DWORD awTlLastWinErr;

/*******************************************/
/* PUBLIC DEBUG: AwErrorToString functions */
/*******************************************/

#ifndef NDEBUG
char const * AwWinErrorToString(DWORD error)
{
	if (NO_ERROR==error) return "FALSE error";
	static TCHAR buffer[1024];
#ifdef WIN32
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,error,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),buffer,sizeof buffer/sizeof(TCHAR)-1,NULL))
#endif
		wsprintf(buffer,TEXT("FormatMessage() failed; previous Windows error code: 0x%08X"),error);
	for (TCHAR * bufP = buffer; *bufP; ++bufP)
	{
		switch (*bufP)
		{
			case '\n':
			case '\r':
				*bufP=' ';
		}
	}
	return reinterpret_cast<char *>(buffer);
}

char const * AwTlErrorToString(AwTlErc error)
{
	char const * defaultS;
	switch (error)
	{
		case AW_TLE_OK:
			return "FALSE error";
		case AW_TLE_DXERROR:
			return "Unknown DirectX error";
		case AW_TLE_BADPARMS:
			return "Invalid parameters or functionality not supported";
		case AW_TLE_NOINIT:
			return "Initialization failed or not performed";
		case AW_TLE_CANTOPENFILE:
			defaultS = "Unknown error opening file";
			goto WIN_ERR;
		case AW_TLE_CANTREADFILE:
			defaultS = "Unknown error reading file";
		WIN_ERR:
			if (NO_ERROR==awTlLastWinErr)
				return defaultS;
			else
				return AwWinErrorToString();
		case AW_TLE_EOFMET:
			return "Unexpected end of file during texture load";
		case AW_TLE_BADFILEFORMAT:
			return "Texture file format not recognized";
		case AW_TLE_BADFILEDATA:
			return "Texture file data not consistent";
		case AW_TLE_CANTPALETTIZE:
			return "Texture file data not palettized";
		case AW_TLE_IMAGETOOLARGE:
			return "Image is too large for a texture";
		case AW_TLE_CANTRELOAD:
			return "New image is wrong size or format to load into existing texture";
		default:
			return "Unknown texture loading error";
	}
}

char const * AwDxErrorToString(HRESULT error)
{
	return "AwDxErrorToString Error";
}
#endif
