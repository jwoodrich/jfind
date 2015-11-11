#ifndef _BINFILE_H
#define _BINFILE_H

#define READU1(F,D) (fread(&D,1,sizeof(unsigned char),F))
#define READU2(F,D) (fread(&D,1,sizeof(unsigned short),F) && (D=htons(D))||1)
#define READU4(F,D) (fread(& D,1,sizeof(unsigned int),F) && (D=htonl(D))||1)

#endif
