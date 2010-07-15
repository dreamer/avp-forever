#include "endianio.h"

uint8_t GetByte(FILE *fp)
{
  uint8_t c = fgetc(fp);
  return c;
}
 
uint16_t GetLittleWord(FILE *fp)
{
  unsigned char c1 = fgetc(fp);
  unsigned char c2 = fgetc(fp);
  return c1 + (c2 << 8);
}

uint32_t GetLittleDword(FILE *fp)
{
  unsigned char c1 = fgetc(fp);
  unsigned char c2 = fgetc(fp);
  unsigned char c3 = fgetc(fp);
  unsigned char c4 = fgetc(fp);

  return c1 + ((c2 + ((c3 + (c4 << 8)) << 8)) << 8);
}

void PutByte(uint8_t v, FILE *fp)
{
  fputc(v, fp);
}

void PutLittleWord(uint16_t v, FILE *fp)
{
  unsigned char c1 = v & 0xff;
  unsigned char c2 = (v >> 8) & 0xff;
  fputc(c1, fp);
  fputc(c2, fp);
}

void PutLittleDword(uint32_t v, FILE *fp)
{
  unsigned char c1 = v & 0xff;
  unsigned char c2 = (v >> 8) & 0xff;
  unsigned char c3 = (v >> 16) & 0xff;
  unsigned char c4 = (v >> 24) & 0xff;

  fputc(c1, fp);
  fputc(c2, fp);
  fputc(c3, fp);
  fputc(c4, fp);
}
