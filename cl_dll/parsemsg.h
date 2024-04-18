#ifndef PARSEMSG_H
#define PARSEMSG_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "protocol.h"

#define MSG_STRING_BUFFER	MAX_MESSAGE_STRING//2048

void BEGIN_READ(void *buffer, size_t size);
size_t READ_REMAINING(void);// XDM3035
size_t END_READ(void);// XDM3035

signed char READ_CHAR(void);
byte READ_BYTE(void);
short READ_SHORT(void);
word READ_WORD(void);
uint32 READ_UINT32(void);
long READ_LONG(void);
float READ_FLOAT(void);
float READ_COORD(void);
void READ_COORD3(Vector &v);
float READ_ANGLE(void);
float READ_HIRESANGLE(void);
char *READ_STRING(void);

// XDM3037: these are better
bool READ_CHAR(signed char &c);
bool READ_BYTE(byte &c);
bool READ_SHORT(short &c);
bool READ_WORD(word &c);
bool READ_LONG(long &c);
bool READ_FLOAT(float &c);
bool READ_COORD(float &c);
bool READ_VECTOR(Vector &v);
bool READ_ANGLE(float &c);
bool READ_HIRESANGLE(float &c);
size_t READ_STRING(char *string, const size_t &stringlength);

#endif // PARSEMSG_H
