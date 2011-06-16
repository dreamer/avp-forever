#ifndef _INCLUDED_IFF_ILBM_HPP_
#define _INCLUDED_IFF_ILBM_HPP_

#include "iff.hpp"

namespace IFF
{
	class IlbmBmhdChunk : public Chunk
	{
		public:
			uint16_t width;
			uint16_t height;
			uint16_t xTopLeft;
			uint16_t yTopLeft;
			uint8_t  nBitPlanes;
			enum
			{
				MASK_NONE = 0,
				MASK_HASMASK = 1,
				MASK_TRANSPARENTCOL = 2,
				MASK_LASSO = 3
			};
			uint8_t eMasking;
			enum
			{
				COMPRESS_NONE = 0,
				COMPRESS_RUNLENGTH = 1,
				COMPRESS_S3TC = 2   //will have s3tc chunk instead of body chunk
			};
			uint8_t  eCompression;
			uint8_t  flags;
			uint16_t iTranspCol;
			uint8_t  xAspectRatio;
			uint8_t  yAspectRatio;
			uint16_t xMax;
			uint16_t yMax;
			
			IlbmBmhdChunk() { m_idCk = "BMHD"; }
			
		protected:
			virtual void Serialize(Archive * pArchv);
	};
	
	class IlbmCmapChunk : public Chunk
	{
		public:
			uint32_t nEntries;
			RGBTriple * pTable;
			
			void CreateNew(unsigned nSize);
			
			IlbmCmapChunk() : pTable(NULL) { m_idCk = "CMAP"; }
			virtual ~IlbmCmapChunk();
			
		protected:
			virtual void Serialize(Archive * pArchv);
	};
	
	inline void IlbmCmapChunk::CreateNew(uint32_t nSize)
	{
		delete[] pTable;
		pTable = new RGBTriple [nSize];
		nEntries = nSize;
	}
	
	class IlbmBodyChunk : public Chunk
	{
		public:
			IlbmBodyChunk() : pData(NULL), pDecodeDst(NULL)
				#ifndef IFF_READ_ONLY
					, pEncodeDst(NULL), pEncodeSrc(NULL)
				#endif
				{ m_idCk = "BODY"; }
			virtual ~IlbmBodyChunk();
			
			#ifndef IFF_READ_ONLY
				bool BeginEncode();
				bool EncodeFirstRow(uint32_t const * pRow);
				bool EncodeNextRow(uint32_t const * pRow);
				bool EndEncode();
				
				float GetCompressionRatio() const;
			#endif
			
			bool BeginDecode() const;
			uint32_t const * DecodeFirstRow() const;
			uint32_t const * DecodeNextRow() const;
			bool EndDecode() const;
			
		protected:
			virtual void Serialize(Archive * pArchv);

			virtual bool GetHeaderInfo() const; // fills in these three data members
			mutable uint32_t nWidth;
			mutable uint32_t eCompression;
			mutable uint32_t nBitPlanes;

		private:
			uint32_t nSize;
			uint8_t * pData;
			
			mutable uint8_t const * pDecodeSrc;
			mutable uint32_t nRemaining;
			mutable uint32_t * pDecodeDst;
			
			#ifndef IFF_READ_ONLY
				DataBlock * pEncodeDst;
				uint8_t * pEncodeSrc;
				
				uint32_t nSizeNonCprss;
				uint32_t nSizeCprss;
			#endif
	};
	
	#ifndef IFF_READ_ONLY
		inline bool IlbmBodyChunk::EncodeFirstRow(uint32_t const * pRow)
		{
			if (BeginEncode())
				return EncodeNextRow(pRow);
			else
				return false;
		}
		
		inline float IlbmBodyChunk::GetCompressionRatio() const
		{
			if (!nSizeNonCprss) return 0.0F;
			
			return (static_cast<float>(nSizeNonCprss)-static_cast<float>(nSizeCprss))/static_cast<float>(nSizeNonCprss);
		}
	#endif
	
	inline uint32_t const * IlbmBodyChunk::DecodeFirstRow() const
	{
		if (BeginDecode())
			return DecodeNextRow();
		else
			return NULL;
	}
	
	inline bool IlbmBodyChunk::EndDecode() const
	{
		if (pDecodeDst)
		{
			delete[] pDecodeDst;
			pDecodeDst = NULL;
			return true;
		}
		else return false;
	}
	
	class IlbmGrabChunk : public Chunk
	{
		public:
			uint16_t xHotSpot;
			uint16_t yHotSpot;
			
			IlbmGrabChunk() { m_idCk = "GRAB"; }
			
		protected:
			virtual void Serialize(Archive * pArchv);
	};
}

#endif // ! _INCLUDED_IFF_ILBM_HPP_
