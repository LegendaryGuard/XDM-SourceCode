#ifndef UTIL_COMMON_H
#define UTIL_COMMON_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#define GAME_BASE_DIR		"valve"

#define DOT_1DEGREE			0.9998476951564
#define DOT_2DEGREE			0.9993908270191
#define DOT_3DEGREE			0.9986295347546
#define DOT_4DEGREE			0.9975640502598
#define DOT_5DEGREE			0.9961946980917
#define DOT_6DEGREE			0.9945218953683
#define DOT_7DEGREE			0.9925461516413
#define DOT_8DEGREE			0.9902680687416
#define DOT_9DEGREE			0.9876883405951
#define DOT_10DEGREE		0.9848077530122
#define DOT_15DEGREE		0.9659258262891
#define DOT_20DEGREE		0.9396926207859
#define DOT_25DEGREE		0.9063077870367

#define AUTOAIM_2DEGREES	0.0348994967025
#define AUTOAIM_5DEGREES	0.08715574274766
#define AUTOAIM_8DEGREES	0.1391731009601
#define AUTOAIM_10DEGREES	0.1736481776669


// Player user rights on the server
enum user_rights_e
{
	USER_RIGHTS_NONE = 0,
	USER_RIGHTS_PLAYER,
	USER_RIGHTS_MODERATOR,
	USER_RIGHTS_ADMINISTRATOR,
	USER_RIGHTS_DEVELOPER,
	USER_RIGHTS_COUNT// must be last
};// user_rights_t;

extern const char *g_UserRightsNames[];


// platform.h void DBG_PrintF(char *szFmt, ...);

// 32-bit versions!
// Keeps clutter down a bit, when using a float as a bit-vector
//#define SetBits(flBitVector, bits)							((flBitVector) = (uint32)(flBitVector) | (bits))
//#define ClearBits(flBitVector, bits)						((flBitVector) = (uint32)(flBitVector) & ~(bits))
#define Bit(bit)											(1u << bit)
#define SetBits(flBitVector, bits)							(flBitVector |= (bits))
#define ClearBits(flBitVector, bits)						(flBitVector &= ~(bits))
//#define FBitSet(flBitVector, bit)							((int)(flBitVector) & (bit))
inline bool FBitSet(uint32 flBitVector, uint32 bit)			{ return (flBitVector & bit) != 0; }// true if ANY of bits are set
inline bool FBitSetAll(uint32 flBitVector, uint32 bit)		{ return (flBitVector & bit) == bit; }// true if ALL of bits are set
inline bool FBitExclude(uint32 flBitVector, uint32 bit)		{ return (flBitVector & ~bit) != 0; }// true when there are bits other than 'bit'

// BitOperationNxM operates on N of M-byte arguments
inline uint32 BitMerge2x2(uint16 short1, uint16 short2)						{ return (uint32)((short1 << 0) | (short2 << CHAR_BIT*2)); }
inline uint32 BitMerge4x1(byte byte1, byte byte2, byte byte3, byte byte4)	{ return (uint32)((byte1 << 0) | (byte2 << CHAR_BIT) | (byte3 << CHAR_BIT*2) | (byte4 << CHAR_BIT*3)); }
inline uint16 BitMerge2x1(byte byte1, byte byte2)							{ return (uint16)((byte1 << 0) | (byte2 << CHAR_BIT)); }
inline byte BitMerge2x4bit(byte hbyte1, byte hbyte2)						{ return (byte)(((hbyte1 << 0) & 0x0F) | (hbyte2 << CHAR_BIT/2)); }

inline void BitSplit2x2(uint32 in, uint16 &out1, uint16 &out2)				{ out1 = (uint16)(in & 0xFFFF); out2 = (uint16)((in & 0xFFFF0000) >> (CHAR_BIT*2)); }
inline void BitSplit4x1(uint32 in, byte &o1, byte &o2, byte &o3, byte &o4)	{ o1 = (byte)(in & 0xFF); o2 = (byte)((in & 0xFF00) >> CHAR_BIT); o3 = (byte)((in & 0xFF0000) >> (CHAR_BIT*2)); o4 = (byte)((in & 0xFF000000) >> (CHAR_BIT*3)); }
inline void BitSplit2x1(uint16 in, byte &o1, byte &o2)						{ o1 = (byte)(in & 0xFF); o2 = (byte)((in & 0xFF00) >> CHAR_BIT); }
inline void BitSplit2x4bit(byte in, byte &o1, byte &o2)						{ o1 = (byte)(in & 0x0F); o2 = (byte)((in & 0xF0) >> CHAR_BIT/2); }

void conprintf(int devlevel, const char *format, ...);
char *UTIL_VarArgs(const char *format, ...);

float UTIL_WeaponTimeBase(void);

int UTIL_SharedRandomLong(const unsigned int &seed, const int &low, const int &high);
float UTIL_SharedRandomFloat(const unsigned int &seed, const float &low, const float &high);

bool StringTo4F(const char *str, float *array4f);
bool ParseArray4f(const char *name, const char *value, float *array4f);

int TrainSpeed(const int &iSpeed, const int &iMax);

int UTIL_PointContents(const Vector &vPoint);
bool UTIL_LiquidContents(const Vector &vPoint);
float UTIL_WaterLevel(const Vector &position, vec_t fMinZ, vec_t fMaxZ);

int UTIL_BloodToStreak(int color);
int UTIL_BloodDecalIndex(const int &bloodColor);

const char *ParseRandomString(const char *szSourceString);
void Pathname_Convert(char *pathname);
const char *UTIL_StripFileNameExt(const char *path);
void UTIL_StripFileName(const char *path, char *buffer, size_t bufferlength);
bool UTIL_FileExtensionIs(const char *path, const char *checkext);
bool UTIL_ExpandPathSecure(const char *relpath, char *fullpath, size_t fullpathmax, bool bToGameBaseDir);
bool UTIL_FileExists(const char *relpath);
bool UTIL_GetTrueFileName(char *relpath, size_t length);
bool UTIL_FixFileName(char *relpath, size_t length);
size_t UTIL_ListFiles(const char *search);
FILE *LoadFile(const char *name, const char *mode);
bool StringInList(const char *szString, const char *szFileName);
bool UTIL_LoadRawPalette(const char *filename);

const Vector &RandomVector(void);
const Vector &RandomVector(const float &fHalfRange);
const Vector &RandomVector(const float &fHalfX, const float &fHalfY, const float &fHalfZ);
const Vector &RandomVector(const Vector &vHalfRange);
const Vector &RandomVector(const float &fMin, const float &fMax);
const Vector &RandomVector(const Vector &vMin, const Vector &vMax);

const int &RANDOM_INT2(const int &a, const int &b);
int Array_FindInt(const int *pArray, const int value);
bool RestrictInt(int &iValue, const int iDefault, const size_t iOptionsCount, ...);

Vector UTIL_ClampVectorToBox(const Vector &input, const Vector &clampSize);

Vector GetNearestPointOfABoxMinsMaxs(const Vector &vecPoint, const Vector &vecMins, const Vector &vecMaxs);
Vector GetNearestPointOfABox(const Vector &vecPoint, const Vector &vecCenter, const Vector &vecVertex);
bool PointInCone(const Vector &vPoint, const Vector &vPeak, const Vector &vBase, const float &baseradius);

size_t strbegin(const char *string, const char *substring);

// HL20130901: Unicode routines
bool Q_IsValidUChar32(uchar32 uVal);
uint16 Q_UTF8ToUChar32(const char *pUTF8_, uchar32 &uValueOut, bool &bErrorOut);
bool Q_UnicodeValidate(const char *pUTF8);

char *memfgets(byte *pMemFile, int fileSize, int &filePos, char *pBuffer, int bufferSize);

char *COM_Parse(char *pData, char *pToken, bool bParseComments = false, const char *pSingleCharacterTokens = NULL);
char *COM_Parse(char *pData);
char *COM_Token(void);
int COM_TokenWaiting(char *pBuffer);

uint32 CONFIG_GenerateFromList(const char *listfilename, const char *configfilename);
uint32 CONFIG_GenerateFromTemplate(const char *templatefilename, const char *configfilename, bool usecurrentvalues);
//void ParseFileKV(const char *name, void (*kvcallback) (char *key, char *value, unsigned short structurestate));

uint32 UserRightByName(const char *pName);

void UTIL_DebugBeam(const Vector &vecSrc, const Vector &vecEnd, const float &life, byte r, byte g, byte b);
void UTIL_DebugBox(const Vector &vecMins, const Vector &vecMaxs, const float &life, byte r, byte g, byte b);
void UTIL_DebugPoint(const Vector &vecPos, const float &life, byte r, byte g, byte b);
void UTIL_DebugAngles(const Vector &origin, const Vector &angles, const float &life, const float &radius);

#endif /* UTIL_COMMON_H */
