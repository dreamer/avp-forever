#ifndef _included_endianio_h_
#define _included_endianio_h_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

#include <stdint.h>

uint8_t GetByte(FILE *fp);
uint16_t GetLittleWord(FILE *fp);
uint32_t GetLittleDword(FILE *fp);

void PutByte(uint8_t v, FILE *fp);
void PutLittleWord(uint16_t v, FILE *fp);
void PutLittleDword(uint32_t v, FILE *fp);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* _included_endianio_h_ */

