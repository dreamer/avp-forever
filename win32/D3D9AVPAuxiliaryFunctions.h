#pragma once
#include <d3dx9.h>
#include "prototyp.h"

/**
 * Makes a D3DXMATRIX from a fixedPoint rotation/orientation matrix and optionally an integer 
 * position, position is expected to be non-fixed point. Matrix is column major
 */
__forceinline D3DXMATRIX ConvertMatrixFixedToFloat(MATRIXCH * fixedMatrix, VECTORCH * pos = 0)
{
	D3DXMATRIX result;
	ZeroMemory(result,sizeof(D3DXMATRIX));

	result._11 = float(fixedMatrix->mat11 / 65536.0f);
	result._12 = float(fixedMatrix->mat12 / 65536.0f);
	result._13 = float(fixedMatrix->mat13 / 65536.0f);
	

	result._21 = float(fixedMatrix->mat21 / 65536.0f);
	result._22 = float(fixedMatrix->mat22 / 65536.0f);
	result._23 = float(fixedMatrix->mat23 / 65536.0f);

	result._31 = float(fixedMatrix->mat31 / 65536.0f);
	result._32 = float(fixedMatrix->mat32 / 65536.0f);
	result._33 = float(fixedMatrix->mat33 / 65536.0f);

	result._44 = 1.0f; // Keep homegenous element to 1

	if(pos != 0)
	{
		result._41 = (float)pos->vx;
		result._42 = (float)pos->vy;
		result._43 = (float)pos->vz;
	}
	return result;
}

__forceinline D3DXVECTOR3 ConvertVectorFixedToFloat(VECTORCH * fixedVector)
{
	return D3DXVECTOR3((fixedVector->vx / 65536.0f),(fixedVector->vy / 65536.0f),(fixedVector->vz / 65536.0f));
}

__forceinline D3DXVECTOR3 ConvertVectorIntegerToFloat(VECTORCH * vector)
{
	return D3DXVECTOR3(vector->vx,vector->vy,vector->vz);
}