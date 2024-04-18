/*====================================================================

 Purpose: Reinvented wheel: Vector classes
 Copyright (c) 2010+ Xawari, includes code from Valve Software
 Header is C and C++ compliant
 Basic operations only, no SIMD instructions yet

//==================================================================*/
#ifndef VECTOR_H
#define VECTOR_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* !__MINGW32__ */
#endif

#include "platform.h"
/* Misc C-runtime library headers */
//#include <stdio.h>
/*#include <stdlib.h>*/
#include <math.h>

/* GCC doesn't like this
#define ALLOW_ANONYMOUS_STRUCT to use anonymous unions*/

/* Constants to access Vector if it is used to store angles */
#ifndef PITCH
#define	PITCH	0
#define	YAW		1
#define	ROLL	2
#endif

/* Define globally in project setting to enable validity checks */
/* unused #if defined (VALIDATE_VECTORS)
#define VALIDATE_VECTOR(_v)	ASSERT((_v).IsValid())
#else
#define VALIDATE_VECTOR(_v)	0
#endif*/

#define M_PI_F		((float)(M_PI))/* special floating point version */

/* HL2: NJS: Inlined to prevent floats from being autopromoted to doubles, as with the old system */
#ifndef RAD2DEG
	#define RAD2DEG(x) ((float)(x) * (float)(180.f / M_PI_F))
#endif

#ifndef DEG2RAD
	#define DEG2RAD(x) ((float)(x) * (float)(M_PI_F / 180.f))
#endif

/* A 1D part of all vectors */
typedef float vec_t;


/*=========================================================
 2D Vector - same data-layout as vec_t[2]
=========================================================*/
#if defined (__cplusplus)

class Vector2D
{
public:
#if defined (ALLOW_ANONYMOUS_STRUCT)
	union
	{
		vec_t array[2];
		struct
		{
			vec_t x;
			vec_t y;
		};
	};
#else
	vec_t x;
	vec_t y;
#endif

	// construction/destruction
	inline Vector2D(void): x(0.0f), y(0.0f)				{ }
	inline Vector2D(const float &X, const float &Y)		{ x = X; y = Y; }
	inline Vector2D(const double &X, const double &Y)	{ x = (float)X; y = (float)Y; }
	inline Vector2D(const int &X, const int &Y)			{ x = (float)X; y = (float)Y; }
	inline Vector2D(const Vector2D &v)					{ x = v.x; y = v.y; }
	inline Vector2D(float rgfl[2])						{ x = rgfl[0]; y = rgfl[1]; }

	// operators
	inline Vector2D &operator=(const Vector2D &v)		{ x=v.x; y=v.y; return *this; }
	inline bool operator==(const Vector2D &v) const		{ return (x==v.x && y==v.y); }
	inline bool operator!=(const Vector2D &v) const		{ return (x!=v.x || y!=v.y); }
	inline Vector2D operator-(void) const				{ return Vector2D(-x,-y); }

	inline Vector2D operator+(const Vector2D &v) const	{ return Vector2D(x+v.x, y+v.y); }
	inline Vector2D operator-(const Vector2D &v) const	{ return Vector2D(x-v.x, y-v.y); }
	inline Vector2D operator*(const vec_t &fl) const	{ return Vector2D(x*fl, y*fl); }
	inline Vector2D operator/(const vec_t &fl) const	{ return Vector2D(x/fl, y/fl); }

	// arithmetic operations
	FORCEINLINE Vector2D &operator+=(const Vector2D &v);
	FORCEINLINE Vector2D &operator-=(const Vector2D &v);
	FORCEINLINE Vector2D &operator*=(const Vector2D &v);
	FORCEINLINE Vector2D &operator*=(const vec_t &fl);
	FORCEINLINE Vector2D &operator/=(const Vector2D &v);
	FORCEINLINE Vector2D &operator/=(const vec_t &fl);

	// for modifications
	vec_t &operator[](const int &index)
	{
#if defined (_DEBUG)
		if (index < 0 || index >= 2)// ASSERT?
			return x;
#endif
#if defined (ALLOW_ANONYMOUS_STRUCT)
		return array[index];
#else
		return ((vec_t *)this)[index];
#endif
	}
	// for reading/comparing only
	const vec_t &operator[](const int &index) const
	{
#if defined (_DEBUG)
		if (index < 0 || index >= 2)// ASSERT?
			return x;
#endif
#if defined (ALLOW_ANONYMOUS_STRUCT)
		return array[index];
#else
		return ((vec_t *)this)[index];
#endif
	}

	vec_t *As2f(void)								{ return &x; }
	operator vec_t *()								{ return &x; }// automatically convert to (float *) when needed
	operator vec_t *() const						{ return (vec_t *)&x; }// Hack? Some old functions require (float *) as argument, but we need a way to supply (const Vector)s. Overrides const-protection
	operator const vec_t *() const					{ return &x; }

	inline void Set(const vec_t &X, const vec_t &Y)	{ x = X; y = Y; }
	inline void Set(const vec_t rgfl[2])			{ x = rgfl[0]; y = rgfl[1]; }

	inline void CopyToArray(vec_t *rgfl) const		{ rgfl[0] = x, rgfl[1] = y; }
	inline bool IsZero(void) const					{ return (x==0.0f && y==0.0f); }
#if defined (VALIDATE_VECTORS)
	inline bool IsValid(void) const					{ return IsFinite(x) && IsFinite(y); }
#endif
	inline void Negate(void)						{ x = -x; y = -y; }
	inline vec_t Length(void) const					{ return (vec_t)sqrt(x*x + y*y); }
	inline vec_t Square(void) const					{ return x*y; }
	inline Vector2D Normalize(void) const
	{
		vec_t flLen = Length();
		if (flLen == 0.0f) return Vector2D(0.0f,0.0f);
		flLen = 1.0f / flLen;
		return Vector2D(x * flLen, y * flLen);
	}
	inline vec_t NormalizeSelf(void)// XDM3037: this is how it shoud be
	{
		vec_t flLen = Length();
		if (flLen != 0.0f)
		{
			x /= flLen;
			y /= flLen;
		}
		return flLen;
	}
	// Base address
	vec_t *Base()					{ return (vec_t *)this; }
	vec_t const *Base() const		{ return (vec_t const *)this; }
};


FORCEINLINE Vector2D &Vector2D::operator+=(const Vector2D &v)
{
	//VALIDATE_VECTOR(*this);
	//VALIDATE_VECTOR(v);
	x += v.x; y += v.y;
	return *this;
}

FORCEINLINE Vector2D &Vector2D::operator-=(const Vector2D &v)
{
	//VALIDATE_VECTOR(*this);
	//VALIDATE_VECTOR(v);
	x -= v.x; y -= v.y;
	return *this;
}

FORCEINLINE Vector2D &Vector2D::operator*=(const vec_t &fl)
{
	x *= fl; y *= fl;
	//VALIDATE_VECTOR(*this);
	return *this;
}

FORCEINLINE Vector2D &Vector2D::operator*=(const Vector2D &v)
{
	//VALIDATE_VECTOR(v);
	x *= v.x; y *= v.y;
	//VALIDATE_VECTOR(*this);
	return *this;
}

FORCEINLINE Vector2D &Vector2D::operator/=(const vec_t &fl)
{
	ASSERT(fl != 0.0f);
	vec_t oofl = 1.0f / fl;
	x *= oofl; y *= oofl;
	//VALIDATE_VECTOR(*this);
	return *this;
}

FORCEINLINE Vector2D &Vector2D::operator/=(const Vector2D &v)
{
	//VALIDATE_VECTOR(v);
	ASSERT(v.x != 0.0f && v.y != 0.0f);
	x /= v.x; y /= v.y;
	//VALIDATE_VECTOR(*this);
	return *this;
}

// stand-alone
inline Vector2D operator*(vec_t fl, const Vector2D &v)
{
	return v * fl;
}

inline vec_t DotProduct(const Vector2D &a, const Vector2D &b)
{
	return(a.x*b.x + a.y*b.y);
}






/*=========================================================
 3D Vector - same data-layout as vec_t[3]
=========================================================*/
class Vector
{
public:
#if defined (ALLOW_ANONYMOUS_STRUCT)
	union
	{
		vec_t array[3];
		struct
		{
			vec_t x;
			vec_t y;
			vec_t z;
		};
	};
#else
	vec_t x;
	vec_t y;
	vec_t z;
#endif

	// construction/destruction
	inline Vector(void): x(0.0f), y(0.0f), z(0.0f)					{ }
	inline Vector(const float &X, const float &Y, const float &Z)	{ x = X; y = Y; z = Z; }
	inline Vector(const double &X, const double &Y, const double &Z){ x = (vec_t)X; y = (vec_t)Y; z = (vec_t)Z;	}
	inline Vector(const int &X, const int &Y, const int &Z)			{ x = (vec_t)X; y = (vec_t)Y; z = (vec_t)Z;	}
	inline Vector(const Vector &v)									{ x = v.x; y = v.y; z = v.z; }
	inline Vector(const vec_t rgfl[3])								{ x = rgfl[0]; y = rgfl[1]; z = rgfl[2]; }// memcpy?
	inline Vector(const Vector2D &v)								{ x = v.x; y = v.y; z = 0.0f; }

	// operators
	inline Vector &operator=(const Vector &v)		{ x=v.x; y=v.y; z=v.z; return *this; }
	inline Vector &operator=(const vec_t rgfl[3])	{ x = rgfl[0]; y = rgfl[1]; z = rgfl[2]; return *this; }// memcpy?
	inline bool operator==(const Vector &v) const	{ return (x==v.x && y==v.y && z==v.z); }
	inline bool operator!=(const Vector &v) const	{ return (x!=v.x || y!=v.y || z!=v.z); }
	inline Vector operator-(void) const				{ return Vector(-x,-y,-z); }
	inline vec_t operator|(const Vector &v)			{ return x*v.x + y*v.y + z*v.z; }

	inline Vector operator+(const Vector &v) const	{ return Vector(x+v.x, y+v.y, z+v.z); }
	inline Vector operator-(const Vector &v) const	{ return Vector(x-v.x, y-v.y, z-v.z); }
	inline Vector operator*(const vec_t &fl) const	{ return Vector(x*fl, y*fl, z*fl); }
	inline Vector operator/(const vec_t &fl) const	{ return Vector(x/fl, y/fl, z/fl); }
	//inline Vector operator*(const double &fl) const	{ return Vector(x*fl, y*fl, z*fl); }
	//inline Vector operator/(const double &fl) const	{ return Vector(x/fl, y/fl, z/fl); }
	inline Vector operator*(const Vector &v) const	{ return Vector(x*v.x, y*v.y, z*v.z); }
	inline Vector operator/(const Vector &v) const	{ return Vector(x/v.x, y/v.y, z/v.z); }

	// arithmetic operations
	FORCEINLINE Vector &operator+=(const Vector &v);
	FORCEINLINE Vector &operator-=(const Vector &v);
	FORCEINLINE Vector &operator*=(const Vector &v);
	FORCEINLINE Vector &operator*=(const vec_t &fl);
	FORCEINLINE Vector &operator/=(const Vector &v);
	FORCEINLINE Vector &operator/=(const vec_t &fl);

	// for modifications
	vec_t &operator[](const int &index)
	{
#if defined (_DEBUG)
		if (index < 0 || index >= 3)// ASSERT
			return x;
#endif
#if defined (ALLOW_ANONYMOUS_STRUCT)
		return array[index];
#else
		return ((vec_t *)this)[index];
#endif
	}
	// for reading/comparing only
	const vec_t &operator[](const int &index) const
	{
#if defined (_DEBUG)
		if (index < 0 || index >= 3)// ASSERT
			return x;
#endif
#if defined (ALLOW_ANONYMOUS_STRUCT)
		return array[index];
#else
		return ((vec_t *)this)[index];
#endif
	}

#if defined (ALLOW_ANONYMOUS_STRUCT)
	vec_t *As3f(void)								{ return array; }
	const vec_t *As3f(void) const					{ return array; }
	operator vec_t *()								{ return array; }// automatically convert to (float *) when needed
	operator vec_t *() const						{ return (vec_t *)array; }// Hack? Some old functions require (float *) as argument, but we need a way to supply (const Vector)s. Overrides const-protection
	operator const vec_t *() const					{ return array; }
#else
	vec_t *As3f(void)								{ return &x; }
	const vec_t *As3f(void) const					{ return &x; }
	operator vec_t *()								{ return &x; }// automatically convert to (float *) when needed
	operator vec_t *() const						{ return (vec_t *)&x; }// Hack? Some old functions require (float *) as argument, but we need a way to supply (const Vector)s. Overrides const-protection
	operator const vec_t *() const					{ return &x; }
#endif

	inline void Set(const vec_t &X, const vec_t &Y, const vec_t &Z)	{ x = X; y = Y; z = Z; }

	inline void CopyToArray(vec_t *rgfl) const		{ rgfl[0] = x, rgfl[1] = y, rgfl[2] = z; }
	inline bool IsZero(void) const					{ return (x==0.0f && y==0.0f && z==0.0f); }
	inline bool IsEqualTo(const vec_t &X, const vec_t &Y, const vec_t &Z) const	{ return (x==X && y==Y && z==Z); }
#if defined (VALIDATE_VECTORS)
	inline bool IsValid(void) const					{ return IsFinite(x) && IsFinite(y) && IsFinite(z); }
#endif
	inline void Clear(void)							{ x = 0.0f; y = 0.0f; z = 0.0f; }
	inline void Negate(void)						{ x = -x; y = -y; z = -z; }
	inline vec_t Length(void) const					{ return (vec_t)sqrt(x*x + y*y + z*z); }// |v| = fLength
	inline vec_t Volume(void) const					{ return x*y*z; }
	inline Vector Normalize(void) const
	{
		vec_t flLen = Length();
		if (flLen == 0.0f) return Vector(0.0f,0.0f,1.0f);/* ???? */
		flLen = 1.0f / flLen;
		return Vector(x * flLen, y * flLen, z * flLen);
	}
	inline vec_t NormalizeSelf(void)// XDM3037: this is how it shoud be
	{
		vec_t flLen = Length();
		if (flLen != 0.0f)
		{
			x /= flLen;
			y /= flLen;
			z /= flLen;
		}
		return flLen;
	}
	inline void SetLength(const vec_t &fNewLength)// XDM3038b: fast and useful
	{
		NormalizeSelf();
		x *= fNewLength;
		y *= fNewLength;
		z *= fNewLength;
	}
	// requires engine functions	inline void Randomize(const float &min, const float &max)			{ x = RANDOM_FLOAT(min,max); y = RANDOM_FLOAT(min,max); z = RANDOM_FLOAT(min,max); }
	/* TODO? requires utility headers... probably do not want
	inline void ToAngles(Vector &vAngles)
	{
		VectorAngles(As3f(), vAngles);
	}
	inline void ToDirections(Vector *pForward, Vector *pRight, Vector *pUp)
	{
		AngleVectors(As3f(), pForward, pRight, pUp);
	}*/
	inline void MultiplyAdd(const vec_t &fMultiply, const Vector &vAdd)
	{
		x += fMultiply*vAdd.x;
		y += fMultiply*vAdd.y;
		z += fMultiply*vAdd.z;
	}
	/*inline void MultiplyAdd(const double &fMultiply, const Vector &vAdd)// increased precision?
	{
		x += fMultiply*vAdd.x;
		y += fMultiply*vAdd.y;
		z += fMultiply*vAdd.z;
	}*/
	inline void MirrorByVector(const Vector &vNormal)
	{
		*this -= vNormal * (2.0f * (*this | vNormal));
	}
	// 2D operations
	inline Vector2D Make2D(void) const
	{
		return Vector2D(x,y);
	}
	inline vec_t Length2D(void) const
	{
		return (vec_t)sqrt(x*x + y*y);
	}

	// Base address
	vec_t *Base()					{ return (vec_t *)this; }
	vec_t const *Base() const		{ return (vec_t const *)this; }
};

/*#ifndef DID_VEC3_T_DEFINE
#define DID_VEC3_T_DEFINE
#define vec3_t Vector
#endif*/


FORCEINLINE Vector &Vector::operator+=(const Vector &v)
{
	//VALIDATE_VECTOR(*this);
	//VALIDATE_VECTOR(v);
	x+=v.x; y+=v.y; z += v.z;
	return *this;
}

FORCEINLINE Vector &Vector::operator-=(const Vector &v)
{
	//VALIDATE_VECTOR(*this);
	//VALIDATE_VECTOR(v);
	x-=v.x; y-=v.y; z -= v.z;
	return *this;
}

FORCEINLINE Vector &Vector::operator*=(const vec_t &fl)
{
	x *= fl; y *= fl; z *= fl;
	//VALIDATE_VECTOR(*this);
	return *this;
}

FORCEINLINE Vector &Vector::operator*=(const Vector &v)
{
	//VALIDATE_VECTOR(v);
	x *= v.x; y *= v.y; z *= v.z;
	//VALIDATE_VECTOR(*this);
	return *this;
}

FORCEINLINE Vector &Vector::operator/=(const vec_t &fl)
{
	ASSERT(fl != 0.0f);
	vec_t oofl = 1.0f / fl;
	x *= oofl; y *= oofl; z *= oofl;
	//VALIDATE_VECTOR(*this);
	return *this;
}

FORCEINLINE Vector &Vector::operator/=(const Vector &v)
{
	//VALIDATE_VECTOR(v);
	ASSERT(v.x != 0.0f && v.y != 0.0f && v.z != 0.0f);
	x /= v.x; y /= v.y; z /= v.z;
	//VALIDATE_VECTOR(*this);
	return *this;
}

// stand-alone
inline Vector operator*(vec_t fl, const Vector &v)
{
	return v * fl;
}

// vA*vB = |vA|*|vB|*cos(angle)
inline vec_t DotProduct(const Vector &a, const Vector &b)
{
	return (a.x*b.x+a.y*b.y+a.z*b.z);
}

// |vC| = |vA|*|vB|*sin(angle)
// vA x vB = vC - normal to the plane formed by vectors vA and vB
inline Vector CrossProduct(const Vector &a, const Vector &b)
{
	return Vector(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}



/*=========================================================
 4D Vector - for matrix operations
=========================================================*/
class Vector4D
{
public:
#if defined (ALLOW_ANONYMOUS_STRUCT)
	union
	{
		vec_t array[4];
		struct
		{
			vec_t x;
			vec_t y;
			vec_t z;
			vec_t w;
		};
	};
#else
	vec_t x;
	vec_t y;
	vec_t z;
	vec_t w;
#endif

	// construction/destruction
	inline Vector4D(void) {}
	//inline Vector4D(void): x(0.0f), y(0.0f), z(0.0f), w(0.0f)						{ }
	inline Vector4D(const float &X, const float &Y, const float &Z, const float &W)	{ x = X; y = Y; z = Z; w = W; }
	inline Vector4D(const double &X, const double &Y, const double &Z, const double &W){ x = (float)X; y = (float)Y; z = (float)Z; w = (float)W; }
	inline Vector4D(const int &X, const int &Y, const int &Z, const int &W)			{ x = (float)X; y = (float)Y; z = (float)Z; w = (float)W; }
	inline Vector4D(const Vector4D &v)												{ x = v.x; y = v.y; z = v.z; w = v.w; }
	inline Vector4D(float rgfl[4])													{ x = rgfl[0]; y = rgfl[1]; z = rgfl[2]; w = rgfl[3]; }
	inline Vector4D(const Vector &v)												{ x = v.x; y = v.y; z = v.z; w = 0.0f; }

	// operators
	inline Vector4D &operator=(const Vector4D &v)		{ x=v.x; y=v.y; z=v.z; w=v.w; return *this; }
	inline bool operator==(const Vector4D &v) const		{ return (x==v.x && y==v.y && z==v.z && w==v.w); }
	inline bool operator!=(const Vector4D &v) const		{ return (x!=v.x || y!=v.y || z!=v.z || w!=v.w); }
	inline Vector4D operator-(void) const				{ return Vector4D(-x,-y,-z,-w); }

	/*inline Vector4D operator+(const Vector4D &v) const	{ return Vector4D(x+v.x, y+v.y, z+v.z, w+v.w); }
	inline Vector4D operator-(const Vector4D &v) const	{ return Vector4D(x-v.x, y-v.y, z-v.z, w-v.w); }
	inline Vector4D operator*(const vec_t &fl) const	{ return Vector4D(x*fl, y*fl, z*fl, w*fl); }
	inline Vector4D operator/(const vec_t &fl) const	{ return Vector4D(x/fl, y/fl, z/fl, w/fl); }
	//inline Vector4D operator*(const double &fl) const	{ return Vector4D(x*fl, y*fl, z*fl, w*fl); }
	//inline Vector4D operator/(const double &fl) const	{ return Vector4D(x/fl, y/fl, z/fl, w/fl); }*/

	// for modifications
	vec_t &operator[](const int &index)
	{
//#if defined (_DEBUG)
		ASSERTD(index >= 0 && index < 4);
		//if (index < 0 || index >= 4)// ASSERT
		//	index = 0;
//#endif
#if defined (ALLOW_ANONYMOUS_STRUCT)
		return array[index];
#else
		return ((vec_t *)this)[index];
#endif
	}
	// for reading/comparing only
	const vec_t &operator[](const int &index) const
	{
//#if defined (_DEBUG)
		ASSERTD(index >= 0 && index < 4);
		//if (index < 0 || index >= 4)// ASSERT
		//	index = 0;
//#endif
#if defined (ALLOW_ANONYMOUS_STRUCT)
		return array[index];
#else
		return ((vec_t *)this)[index];
#endif
	}

	vec_t *As4f(void)								{ return &x; }
	operator vec_t *()								{ return &x; }// automatically convert to (float *) when needed
	operator vec_t *() const						{ return (vec_t *)&x; }// Hack? Some old functions require (float *) as argument, but we need a way to supply (const Vector)s. Overrides const-protection
	operator const vec_t *() const					{ return &x; }
	operator Vector()								{ return Vector(x,y,z); }
	operator const Vector() const					{ return Vector(x,y,z); } 

	// initialization
	void Init(vec_t ix = 0.0f, vec_t iy = 0.0f, vec_t iz = 0.0f, vec_t iw = 0.0f)
	{
		x = ix; y = iy; z = iz; w = iw;
	}

#if defined (VALIDATE_VECTORS)
	inline bool IsValid(void) const					{ return IsFinite(x) && IsFinite(y) && IsFinite(z) && IsFinite(w); }
#endif
};


typedef Vector2D vec2_t;
typedef Vector vec3_t;
// UNDONE: NOT NOW! The code is not ready for this. #define vec4_t Vector4D
typedef vec_t vec4_t[4];


#else /* __cplusplus */


typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];


#endif /* __cplusplus */


extern const vec3_t g_vecZero;


#endif /* VECTOR_H */
