#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
/*
#ifndef byte
typedef unsigned char byte;
#endif
#ifndef true
#define true 1
#endif
*/
static size_t giSize = 0;
static size_t giRead = 0;
static byte *gpBuf = NULL;
static bool gBadRead = false;

// XDM3038: added support for sizeless messages (size == 0)
void BEGIN_READ(void *buffer, size_t size)
{
// too strict	ASSERT(gpBuf == NULL);
	giRead = 0;
	gBadRead = 0;
	giSize = size;
	gpBuf = (byte *)buffer;
}

size_t READ_REMAINING(void)// XDM3035
{
	if (giSize > 0)// MsgFunc_HudText comes without size
		return giSize - giRead;

	return 0;
}

size_t END_READ(void)// XDM3035
{
	size_t remaining = READ_REMAINING();
#if defined (_DEBUG)
	if (remaining > 0)
	{
		conprintf(1, "WARNING! Message has %u remaining bytes unread!\n", remaining);
	}
#endif
	if (gBadRead)
	{
		conprintf(1, "WARNING! Message has encountered read errors! Remaining bytes: %u\n", remaining);
	}
	giRead = 0;
	gBadRead = 0;
	giSize = 0;
	gpBuf = NULL;
	return remaining;
}

signed char READ_CHAR(void)
{
	signed char c;
	if (giSize > 0 && giRead+sizeof(signed char) > giSize)
	{
		gBadRead = true;
		return -1;
	}
	c = (signed char)gpBuf[giRead];
	giRead += sizeof(signed char);
	return c;
}

byte READ_BYTE(void)
{
	byte c;
	if (giSize > 0 && giRead+sizeof(byte) > giSize)
	{
		gBadRead = true;
		return 0;
	}
	c = (byte)gpBuf[giRead];
	giRead += sizeof(byte);
	return c;
}

short READ_SHORT(void)
{
	short c;
	if (giSize > 0 && giRead+sizeof(short) > giSize)
	{
		gBadRead = true;
		return -1;
	}
	c = (short)(gpBuf[giRead] + (gpBuf[giRead+1] << 8));
	giRead += sizeof(short);
	return c;
}

word READ_WORD(void)
{
	short c;
	if (giSize > 0 && giRead+sizeof(word) > giSize)
	{
		gBadRead = true;
		return 0;
	}
	c = (word)(gpBuf[giRead] + (gpBuf[giRead+1] << 8));
	giRead += sizeof(word);
	return c;
}

uint32 READ_UINT32(void)
{
	uint32 c;
	if (giSize > 0 && giRead+sizeof(uint32) > giSize)
	{
		gBadRead = true;
		return 0;
	}
	memcpy(&c, &gpBuf[giRead], sizeof(c));
//	c = gpBuf[giRead] + (gpBuf[giRead + 1] << 8) + (gpBuf[giRead + 2] << 16) + (gpBuf[giRead + 3] << 24);
	giRead += sizeof(c);
	return c;
}

long READ_LONG(void)
{
	long c;
	if (giSize > 0 && giRead+sizeof(long) > giSize)
	{
		gBadRead = true;
		return -1;
	}
	c = gpBuf[giRead] + (gpBuf[giRead + 1] << 8) + (gpBuf[giRead + 2] << 16) + (gpBuf[giRead + 3] << 24);
	giRead += sizeof(long);
	return c;
}

float READ_FLOAT(void)
{
	if (giSize > 0 && giRead+4 > giSize)
	{
		gBadRead = true;
		return 0.0f;
	}
	static float fData;
	memcpy(&fData, &gpBuf[giRead], sizeof(fData));
	giRead += sizeof(fData);
	return fData;
/*	union
	{
		byte    b[4];
		float   f;
		int     l;
	} dat;
	dat.b[0] = gpBuf[giRead];
	dat.b[1] = gpBuf[giRead+1];
	dat.b[2] = gpBuf[giRead+2];
	dat.b[3] = gpBuf[giRead+3];
	giRead += 4;
//	dat.l = LittleLong(dat.l);
	return dat.f;*/
}

// it has precision of 1/8th of a unit and minmax = max2bytes/8 = +-4095
float READ_COORD(void)
{
	return (float)((float)READ_SHORT() * (1.0f/8));
}

void READ_COORD3(Vector &v)
{
	v.x = READ_COORD();
	ASSERT(READ_REMAINING() > 0);
	v.y = READ_COORD();
	ASSERT(READ_REMAINING() > 0);
	v.z = READ_COORD();
}

float READ_ANGLE(void)
{
	return (float)(((float)READ_BYTE()*360.0f)/256.0f);// XDM3038b: 0...360 version must use unsigned data!!
}

float READ_HIRESANGLE(void)
{
	return (float)(((float)READ_SHORT()*360.0f)/65536.0f);
}

// WARNING: Each call overwrites result of previous!
// WARNING: engine may send size == 0 for a string!
char *READ_STRING(void)
{
	static char string[MSG_STRING_BUFFER];
	size_t l = 0;
	signed char c;
	string[0] = 0;
	do
	{
		if (giSize > 0 && giRead+1 > giSize)// XDM3038: fix
			break;// no more characters

		c = READ_CHAR();
		if (c == 0)// XDM c == -1
			break;

		string[l] = c;
		++l;
	} while (l < /*sizeof(string)*/MSG_STRING_BUFFER-1);

	string[l] = 0;
	return string;
}




// faster and more flexible versions of above

bool READ_CHAR(signed char &c)
{
	if (giSize > 0 && giRead+sizeof(signed char) > giSize)
	{
		gBadRead = true;
		return false;
	}
	c = (signed char)gpBuf[giRead];
	giRead += sizeof(signed char);
	return true;
}

bool READ_BYTE(byte &c)
{
	if (giSize > 0 && giRead+sizeof(byte) > giSize)
	{
		gBadRead = true;
		return false;
	}
	c = (byte)gpBuf[giRead];
	giRead += sizeof(byte);
	return true;
}

bool READ_SHORT(short &c)
{
	if (giSize > 0 && giRead+sizeof(short) > giSize)
	{
		gBadRead = true;
		return false;
	}
	c = (short)(gpBuf[giRead] + (gpBuf[giRead+1] << 8));
	giRead += sizeof(short);
	return true;
}

bool READ_WORD(word &c)
{
	if (giSize > 0 && giRead+sizeof(word) > giSize)
	{
		gBadRead = true;
		return false;
	}
	c = (word)(gpBuf[giRead] + (gpBuf[giRead+1] << 8));
	giRead += sizeof(word);
	return true;
}

bool READ_LONG(long &c)
{
	if (giSize > 0 && giRead+sizeof(long) > giSize)
	{
		gBadRead = true;
		return false;
	}
	c = gpBuf[giRead] + (gpBuf[giRead + 1] << 8) + (gpBuf[giRead + 2] << 16) + (gpBuf[giRead + 3] << 24);
	giRead += sizeof(long);
	return true;
}

bool READ_FLOAT(float &c)
{
	if (giSize > 0 && giRead+4 > giSize)
	{
		gBadRead = true;
		return 0.0f;
	}
	union
	{
		byte    b[4];
		float   f;
		int     l;
	} dat;
	dat.b[0] = gpBuf[giRead];
	dat.b[1] = gpBuf[giRead+1];
	dat.b[2] = gpBuf[giRead+2];
	dat.b[3] = gpBuf[giRead+3];
	giRead += 4;
//	dat.l = LittleLong(dat.l);
	c = dat.f;
	return true;
}

bool READ_COORD(float &c)
{
	c = (float)((float)READ_SHORT() * (1.0f/8));
	return true;
}

bool READ_VECTOR(Vector &v)
{
	if (READ_REMAINING() <= (sizeof(short)*3))
		return false;

	v.x = READ_COORD();
	v.y = READ_COORD();
	v.z = READ_COORD();
	return true;
}

bool READ_ANGLE(float &c)
{
	c = (float)((float)READ_CHAR() * (360.0f/256));
	return true;
}

bool READ_HIRESANGLE(float &c)
{
	c = (float)((float)READ_SHORT() * (360.0f/65536));
	return true;
}

size_t READ_STRING(char *string, const size_t &stringlength)
{
	if (READ_REMAINING() <= 0)
		return 0;

	size_t l = 0;
	signed char c;
	string[0] = 0;
	do
	{
		if (giSize > 0 && giRead+1 > giSize)
			break;// no more characters

		c = READ_CHAR();
		if (c == 0)// XDM c == -1
			break;

		string[l] = c;
		++l;
	} while (l < stringlength-1);

	string[l] = 0;
	return l;
}
