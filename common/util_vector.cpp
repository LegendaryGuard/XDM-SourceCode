//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
// NOTE: most methods are useful only for float[] arrays,
// don't use these with class vector.
//-----------------------------------------------------------------------------
#include <math.h>
#include "vector.h"
#include "util_vector.h"
#if defined(CLIENT_DLL)
#include "hud.h"
#include "cl_util.h"// RANDOM_FLOAT
#else
#include "extdll.h"
#include "util.h"// RANDOM_FLOAT
#endif

#define MAX_ITERATIONS	32

// XDM3035: from mathlib
float anglemod(const float &a)
{
	return (360.0f/65536.0f) * ((int)(a*(65536.0f/360.0f)) & 65535);
}

float AngleDiff(const float &destAngle, const float &srcAngle)
{
	float delta = destAngle - srcAngle;
	NormalizeAngle180(&delta);
	return delta;
}

// made compatible with const functions
float UTIL_AngleMod(const float &angle)
{
	float a = angle;
	if (a < 0.0f)
		a += 360.0f * ((int)(a / 360.0f) + 1);
	else if (a >= 360.0f)
		a -= 360.0f * ((int)(a / 360.0f));

	return a;
}

float UTIL_Approach(const float &target, float value, const float &speed)
{
	float delta = target - value;
	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else 
		value = target;

	return value;
}

float UTIL_ApproachAngle(float target, float value, float speed)
{
	target = UTIL_AngleMod(target);
	value = UTIL_AngleMod(target);

	float delta = target - value;
	// Speed is assumed to be positive
	if (speed < 0)
		speed = -speed;

	NormalizeAngle180(&delta);// XDM3038

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else 
		value = target;

	return value;
}

float UTIL_AngleDistance(const float &next, const float &cur)
{
	float delta = next - cur;
	NormalizeAngle180(&delta);// XDM3038
	return delta;
}

float UTIL_SplineFraction(float value, const float &scale)
{
	value = scale * value;
	float valueSquared = value * value;
	// Nice little ease-in, ease-out spline-like curve
	return 3 * valueSquared - 2 * valueSquared * value;
}

/*int VectorCompare(const float *v1, const float *v2)
{
	unsigned int i;
	for (i=0 ; i<3 ; ++i)
		if (v1[i] != v2[i])
			return 0;

	return 1;
}*/

void VectorCopy(const float *src, float *dst)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}

void VectorAdd(const float *a, const float *b, float *dst)
{
	dst[0]=a[0]+b[0];
	dst[1]=a[1]+b[1];
	dst[2]=a[2]+b[2];
}

void VectorSubtract(const float *a, const float *b, float *dst)
{
	dst[0]=a[0]-b[0];
	dst[1]=a[1]-b[1];
	dst[2]=a[2]-b[2];
}

/*void VectorMultiply(const float *a, const float *b, float *dst)
{
	dst[0]=a[0]*b[0];
	dst[1]=a[1]*b[1];
	dst[2]=a[2]*b[2];
}*/

void VectorScale(const float *in, const float &scale, float *out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void VectorMA(const float *veca, const float &scale, const float *vecb, float *out)
{
	out[0] = veca[0] + scale*vecb[0];
	out[1] = veca[1] + scale*vecb[1];
	out[2] = veca[2] + scale*vecb[2];
}

vec_t Length(const float *v)
{
	vec_t length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	return sqrt(length);// FIXME
}

/*float Distance(const float *v1, const float *v2)
{
	vec3_t d;
	VectorSubtract(v2,v1,d);
	return Length(d);
}

float DotProduct(const float *x, const float *y)
{
	return (x[0]*y[0] + x[1]*y[1] + x[2]*y[2]);
}*/

void CrossProduct(const float *v1, const float *v2, float *out)
{
	out[0] = v1[1]*v2[2] - v1[2]*v2[1];
	out[1] = v1[2]*v2[0] - v1[0]*v2[2];
	out[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

float VectorNormalize(float *v)
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt(length);		// FIXME

	if (length)
	{
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
	return length;
}

void VectorInverse(float *v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

// -1 ... 1 by default
void VectorRandom(float *v, float min, float max)
{
	//vec3_t v;
	//ASSERT(v != NULL);
	//if (v)
	{
		v[0] = RANDOM_FLOAT(min, max);
		v[1] = RANDOM_FLOAT(min, max);
		v[2] = RANDOM_FLOAT(min, max);
	}
	//return v;
}

void VectorRandom(float *v, const float *halfvolume)
{
	v[0] = RANDOM_FLOAT(-halfvolume[0], halfvolume[0]);
	v[1] = RANDOM_FLOAT(-halfvolume[1], halfvolume[1]);
	v[2] = RANDOM_FLOAT(-halfvolume[2], halfvolume[2]);
}

void VectorRandom(float *v, const float *mins, const float *maxs)
{
	v[0] = RANDOM_FLOAT(mins[0], maxs[0]);
	v[1] = RANDOM_FLOAT(mins[1], maxs[1]);
	v[2] = RANDOM_FLOAT(mins[2], maxs[2]);
}

//-----------------------------------------------------------------------------
// Purpose: with return value
// Output : vec3_t
//-----------------------------------------------------------------------------
vec3_t VectorRandom(void)
{
	static vec3_t v;// faster?
	VectorRandom(v);
	return v;
}

//-----------------------------------------------------------------------------
// Purpose: Normalize: wrap value to specified range.
//			Totally customizable version (modifies source value)
// Input  : *value - float (output)
//			&min - minimum allowed value
//			&max - maximum (not) allowed value (value can be equal to min only)
//-----------------------------------------------------------------------------
void NormalizeValueF(float *value, const float &min, const float &max)
{
	float range = max-min;
#if defined (_DEBUG)
	static size_t c;// thread-unsafe!
	c = 0;
	while (*value >= max && c < MAX_ITERATIONS)// XDM3037a: >= WARNING! TODO: TESTME!
	{
		*value -= range;
		++c;
	}
	if (c == 0)// "max" check did not work
	{
		while (*value < min && c < MAX_ITERATIONS)
		{
			*value += range;
			++c;
		}
	}
	ASSERT(c < MAX_ITERATIONS);// XDM: this will reveal bogus numbers!
	if (c >= MAX_ITERATIONS)

		*value = min;// XDM: SUPRALEGAL MEASURES!! (tm)
#else// Fast and dangerous version!
	if (*value > max)
		*value -= range;
	else if (*value < min)
		*value += range;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Normalize: Totally customizable version (with return value)
// Input  : &srcvalue - 1D float
//			&min - minimum allowed value
//			&max - maximum allowed value
// Output : float - normalized value
//-----------------------------------------------------------------------------
float NormalizeValueFr(const float &srcvalue, const float &min, const float &max)
{
	static float fRet;
	fRet = srcvalue;
	NormalizeValueF(&fRet, min, max);
	return fRet;
}

// 1D, modifies original
// -180...180 version
void NormalizeAngle180(float *angle)
{
	NormalizeValueF(angle, -180, 180);
}

// returns new value
// -180...180 version
float NormalizeAngle180r(const float &angle)
{
	return NormalizeValueFr(angle, -180, 180);
}

// 1D, modifies original
// 0...360 version
void NormalizeAngle360(float *angle)
{
	NormalizeValueF(angle, 0, 360);
}

// returns new value
// 0...360 version
float NormalizeAngle360r(const float &angle)
{
	return NormalizeValueFr(angle, 0, 360);
}

// 3D, modifies original
void NormalizeAngles(float *angles)
{
	NormalizeAngle180(&angles[0]);
	NormalizeAngle180(&angles[1]);
	NormalizeAngle180(&angles[2]);
}

void NormalizeAngles360(float *angles)
{
	NormalizeAngle360(&angles[0]);
	NormalizeAngle360(&angles[1]);
	NormalizeAngle360(&angles[2]);
}

//-----------------------------------------------------------------------------
// Purpose: Normalize: wrap value to specified range.
//			Totally customizable version (modifies source value)
// Input  : *value - integer (output)
//			&min - minimum allowed value
//			&max - maximum (not) allowed value (value can be equal to min only)
//-----------------------------------------------------------------------------
void NormalizeValueI(int *value, const int &min, const int &max)
{
	static size_t c;
	c = 0;
	int range = max-min;
	while (*value >= max && c < MAX_ITERATIONS)
	{
		*value -= range;
		++c;
	}
	if (c == 0)// "max" check did not work
	{
		while (*value < min && c < MAX_ITERATIONS)
		{
			*value += range;
			++c;
		}
	}
	ASSERT(c < MAX_ITERATIONS);// XDM: this will reveal bogus numbers!
	if (c >= MAX_ITERATIONS)
		*value = min;// XDM: SUPRALEGAL MEASURES!! (tm)
}

//-----------------------------------------------------------------------------
// Purpose: Parse XYZ from string
// Note   : Resulting output values are undefined on failure.
// Input  : *str - "0.0 0.0 0.0"
//			*vec - output - either Vector or float[3]
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool StringToVec(const char *str, vec_t *vec)
{
	if (str && *str)
	{
		if (sscanf(str, "%f %f %f", &vec[0], &vec[1], &vec[2]) == 3)
			return true;
	}
	return false;
}

// XDM3035: optimized versions

vec_t VecToYaw(const vec_t *src)
{
	vec_t yaw;
	if (src[1] == 0.0f && src[0] == 0.0f)
	{
		yaw = 0.0f;
	}
	else
	{
		yaw = RAD2DEG(atan2(src[1], src[0]));
		if (yaw < 0)
			yaw += 360.0f;
	}
	return yaw;
}

// XDM3038: Stupid Quake Bug (SQB) is fixed here, but it's not fixable in the engine... what should I do?
void VectorAngles(const vec_t *forward, vec_t *angles)
{
	vec_t tmp, yaw, pitch;
	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0.0f;
#if defined (NOSQB)
		if (forward[2] > 0.0f)
			pitch = 270.0f;// FIXED: SQB
		else
			pitch = 90.0f;
#else
		if (forward[2] > 0.0f)
			pitch = 90.0f;
		else
			pitch = 270.0f;
#endif
	}
	else
	{
		yaw = RAD2DEG(atan2(forward[1], forward[0]));
		if (yaw < 0.0f)
			yaw += 360.0f;

		tmp = sqrt(forward[0]*forward[0] + forward[1]*forward[1]);
#if defined (NOSQB)
		pitch = RAD2DEG(-atan2(forward[2], tmp));// FIXED: SQB
#else
		pitch = RAD2DEG(atan2(forward[2], tmp));
#endif
		if (pitch < 0.0f)
			pitch += 360.0f;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0.0f;
}

void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up)
{
	vec_t sp, cp, sy, cy, sr, cr;
	// XDM3035c: X_DEG2RAD
	SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
	SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
	SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1.0f*sr*sp*cy + -1.0f*cr*-sy);
		right[1] = (-1.0f*sr*sp*sy + -1.0f*cr*cy);
		right[2] = -1.0f*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}

void AngleVectorsTranspose(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up)
{
	vec_t sp, cp, sy, cy, sr, cr;
	SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
	SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
	SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = (sr*sp*cy+cr*-sy);
		forward[2] = (cr*sp*cy+-sr*-sy);
	}
	if (right)
	{
		right[0] = cp*sy;
		right[1] = (sr*sp*sy+cr*cy);
		right[2] = (cr*sp*sy+-sr*cy);
	}
	if (up)
	{
		up[0] = -sp;
		up[1] = sr*cp;
		up[2] = cr*cp;
	}
}

// old and buggy
void InterpolateAngles(float *start, float *end, float *output, float frac)
{
	unsigned int i;
	float ang1, ang2;
	float d;

	NormalizeAngles(start);
	NormalizeAngles(end);

	for (i = 0; i < 3; ++i)
	{
		ang1 = start[i];
		ang2 = end[i];

		d = ang2 - ang1;
		// why not?	NormalizeAngle180(&d);
		if (d > 180.0f)
			d -= 360.0f;
		else if (d < -180.0f)
			d += 360.0f;

		output[i] = ang1 + d * frac;
	}
	NormalizeAngles(output);
}

// You'd better provide normalized vectors
float AngleBetweenVectors(const vec3_t &v1, const vec3_t &v2)
{
	vec_t l1 = Length(v1);
	if (l1 == 0.0f)// XDM3038a
		return 0.0f;

	vec_t l2 = Length(v2);
	if (l2 == 0.0f)
		return 0.0f;

	return RAD2DEG((float)acos(DotProduct(v1, v2)) / (l1*l2));
}

// Use when you don't really need the angle, but its cosinus
float AngleCosBetweenVectors(const vec3_t &v1, const vec3_t &v2)
{
	float l1 = Length(v1);
	if (l1 == 0.0f)// XDM3038a
		return 0.0f;

	float l2 = Length(v2);
	if (l2 == 0.0f)
		return 0.0f;

	return DotProduct(v1, v2) / (l1*l2);
}

void AngleMatrix(const float *origin, const float *angles, float (*matrix)[4])
{
	float sr, sp, sy, cp, cy, cr;
	SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
	SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
	SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);
	matrix[0][0] = cp*cy;
	matrix[1][0] = cp*sy;
	matrix[2][0] = -sp;
	matrix[0][1] = sr*sp*cy+cr*-sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[2][1] = sr*cp;
	matrix[0][2] = (cr*sp*cy+-sr*-sy);
	matrix[1][2] = (cr*sp*sy+-sr*cy);
	matrix[2][2] = cr*cp;
	matrix[0][3] = origin[0];
	matrix[1][3] = origin[1];
	matrix[2][3] = origin[2];
}

void AngleIMatrix(const float *origin, const float *angles, float matrix[3][4])
{
	float sr, sp, sy, cp, cy, cr;// sin/cos pitch/yaw/roll
	SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
	SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
	SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);
	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp*cy;
	matrix[0][1] = cp*sy;
	matrix[0][2] = -sp;
	matrix[1][0] = sr*sp*cy+cr*-sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[1][2] = sr*cp;
	matrix[2][0] = (cr*sp*cy+-sr*-sy);
	matrix[2][1] = (cr*sp*sy+-sr*cy);
	matrix[2][2] = cr*cp;
	matrix[0][3] = origin[0];
	matrix[1][3] = origin[1];
	matrix[2][3] = origin[2];
}

void VectorTransform(const vec3_t &in1, float in2[3][4], vec3_t &out)
{
	out[0] = DotProduct(in1, (vec3_t)in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, (vec3_t)in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, (vec3_t)in2[2]) + in2[2][3];
}

void ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];
}

// angles index are not the same as ROLL, PITCH, YAW

void MatrixCopy(float in[3][4], float out[3][4])
{
	memcpy(out, in, sizeof(float) * 12);//3.0 * 4.0 );
}

void AngleQuaternion(float *angles, vec4_t quaternion)
{
	float sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	SinCos(angles[0] * 0.5f, &sr, &cr);
	SinCos(angles[1] * 0.5f, &sp, &cp);
	SinCos(angles[2] * 0.5f, &sy, &cy);

	quaternion[0] = sr*cp*cy-cr*sp*sy;// X
	quaternion[1] = cr*sp*cy+sr*cp*sy;// Y
	quaternion[2] = cr*cp*sy-sr*sp*cy;// Z
	quaternion[3] = cr*cp*cy+sr*sp*sy;// W
}

void QuaternionSlerp(const vec4_t &p, vec4_t q, const float &t, vec4_t qt)
{
	//unsigned int i;
	float	omega, cosom, sinom, sclp, sclq;
	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;

	/* AMD Optimization Guide 40546	for (i = 0; i < 4; ++i)
	for (i = 0; i < 4; ++i)
	{
		a += (p[i]-q[i])*(p[i]-q[i]);
		b += (p[i]+q[i])*(p[i]+q[i]);
	}*/
	a += (p[0]-q[0])*(p[0]-q[0]);
	b += (p[0]+q[0])*(p[0]+q[0]);
	a += (p[1]-q[1])*(p[1]-q[1]);
	b += (p[1]+q[1])*(p[1]+q[1]);
	a += (p[2]-q[2])*(p[2]-q[2]);
	b += (p[2]+q[2])*(p[2]+q[2]);
	a += (p[3]-q[3])*(p[3]-q[3]);
	b += (p[3]+q[3])*(p[3]+q[3]);

	if (a > b)
	{
	//	for (i = 0; i < 4; ++i)
	//		q[i] = -q[i];
		q[0] = -q[0];
		q[1] = -q[1];
		q[2] = -q[2];
		q[3] = -q[3];
	}
	cosom = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

	if ((1.0f + cosom) > 0.000001)
	{
		if ((1.0f - cosom) > 0.000001)
		{
			omega = (float)acos(cosom);
			sinom = (float)sin(omega);
			sclp = sin((1.0f - t)*omega) / sinom;
			sclq = sin(t*omega) / sinom;
		}
		else
		{
			sclp = 1.0f - t;
			sclq = t;
		}
		//for (i = 0; i < 4; i++)
		//	qt[i] = sclp * p[i] + sclq * q[i];
		qt[0] = sclp * p[0] + sclq * q[0];
		qt[1] = sclp * p[1] + sclq * q[1];
		qt[2] = sclp * p[2] + sclq * q[2];
		qt[3] = sclp * p[3] + sclq * q[3];
	}
	else
	{
		qt[0] =-q[1];
		qt[1] = q[0];
		qt[2] =-q[3];
		qt[3] = q[2];

		sclp = (float)sin((1.0f - t) * (0.5f * M_PI_F));
		sclq = (float)sin(t * (0.5 * M_PI));

		//for (i = 0; i < 3; ++i)
		//	qt[i] = sclp * p[i] + sclq * qt[i];
		qt[0] = sclp * p[0] + sclq * qt[0];
		qt[1] = sclp * p[1] + sclq * qt[1];
		qt[2] = sclp * p[2] + sclq * qt[2];
		//qt[3] ???
	}
}

void QuaternionMatrix(vec4_t quaternion, float (*matrix)[4])
{
	matrix[0][0] = 1.0f -	2.0f * quaternion[1] * quaternion[1] - 2.0f * quaternion[2] * quaternion[2];
	matrix[1][0] =			2.0f * quaternion[0] * quaternion[1] + 2.0f * quaternion[3] * quaternion[2];
	matrix[2][0] =			2.0f * quaternion[0] * quaternion[2] - 2.0f * quaternion[3] * quaternion[1];
	matrix[0][1] =			2.0f * quaternion[0] * quaternion[1] - 2.0f * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0f -	2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[2] * quaternion[2];
	matrix[2][1] =			2.0f * quaternion[1] * quaternion[2] + 2.0f * quaternion[3] * quaternion[0];
	matrix[0][2] =			2.0f * quaternion[0] * quaternion[2] + 2.0f * quaternion[3] * quaternion[1];
	matrix[1][2] =			2.0f * quaternion[1] * quaternion[2] - 2.0f * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0f -	2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[1] * quaternion[1];
}

void V_SmoothInterpolateAngles(float *startAngle, float *endAngle, float *finalAngle, float degreesPerSec, float &frametime)
{
	float absd,frac,d,threshhold;

	NormalizeAngles(startAngle);
	NormalizeAngles(endAngle);

	for (unsigned int i = 0; i < 3; ++i)
	{
		d = endAngle[i] - startAngle[i];

		if (d > 180.0f)
			d -= 360.0f;
		else if (d < -180.0f)
			d += 360.0f;

		//TODO	NormalizeAngle(&d);

		absd = (float)fabs(d);

		if (absd > 0.01f)
		{
			frac = degreesPerSec * frametime;
			threshhold = degreesPerSec / 4.0f;

			if (absd < threshhold)
			{
				float h = absd / threshhold;
				h *= h;
				frac *= h;  // slow down last degrees
			}

			if (frac > absd)
			{
				finalAngle[i] = endAngle[i];
			}
			else
			{
				if (d > 0)
					finalAngle[i] = startAngle[i] + frac;
				else
					finalAngle[i] = startAngle[i] - frac;
			}
		}
		else
			finalAngle[i] = endAngle[i];
	}
	NormalizeAngles(finalAngle);
}

bool PointInBounds(const vec3_t &point, const vec3_t &mins, const vec3_t &maxs)
{
	if (point[0] < mins[0] || point[1] < mins[1] || point[2] < mins[2])
		return false;
	if (point[0] > maxs[0] || point[1] > maxs[1] || point[2] > maxs[2])
		return false;

	return true;
}

bool BoundsIntersect(const vec3_t &mins1, const vec3_t &maxs1, const vec3_t &mins2, const vec3_t &maxs2)
{
	if (mins1[0] > maxs2[0] || mins1[1] > maxs2[1] || mins1[2] > maxs2[2])
		return false;
	if (maxs1[0] < mins2[0] || maxs1[1] < mins2[1] || maxs1[2] < mins2[2])
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&v_angle - 
//			&reference - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsFacing(const vec3_t &origin, const vec3_t &v_angle, const vec3_t &reference)
{
	Vector forward, angle(v_angle);
	Vector vecDir(reference);
	vecDir -= origin;
	vecDir.z = 0;
	vecDir.NormalizeSelf();
	//angle = v_angle;
	angle.x = 0;
	AngleVectors(angle, forward, NULL, NULL);// TODO: use AngleVectors()
	// He's facing me, he meant it
	// UNDONE: TODO: use FOV!
	if (DotProduct(forward, vecDir) > 0.96)	// +/- 15 degrees or so
		return true;

	return false;
}
