#ifndef _tables_h_
#define _tables_h_

const int kTableSize = 4096;

extern int oneoversin[kTableSize];
extern const int sine[kTableSize];
extern const int cosine[kTableSize];
extern const short ArcCosTable[kTableSize];
extern const short ArcSineTable[kTableSize];
extern const short ArcTanTable[256];

#endif