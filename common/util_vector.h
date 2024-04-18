/*

  Purpose: vector manipulation functions 

  This header must be C compliant!
*/
/*
  WARNING!
 All arguments must be passed as (float *) and not vec3_t!
 Old shitty code depends on it!
*/

#ifndef UTIL_VECTOR_H
#define UTIL_VECTOR_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif


#ifndef M_PI
#define M_PI		3.14159265358979323846	/* matches value in gcc v2 math.h */
#endif

//const float d180pi = 180.0f / (float)M_PI;
//const float pi180 = (float)M_PI/180.0f;/* XDM3035 */

// should this really be inline?
FORCEINLINE void SinCos(float rad, float *sinf, float *cosf)
{
#if defined (_WIN32)
	_asm
	{
		fld DWORD PTR [rad]
		fsincos
		mov edx, DWORD PTR [cosf]
		mov eax, DWORD PTR [sinf]
		fstp DWORD PTR [edx]
		fstp DWORD PTR [eax]
	}
//#elif __linux))
#else
	register double __cosr, __sinr;
 	__asm __volatile__ ("fsincos" : "=t" (__cosr), "=u" (__sinr) : "0" (rad));
  	*sinf = __sinr;
  	*cosf = __cosr;
#endif
}

//#define FDotProduct(a, b) (fabs((a[0])*(b[0])) + fabs((a[1])*(b[1])) + fabs((a[2])*(b[2])))
float anglemod(const float &a);
float AngleDiff(const float &destAngle, const float &srcAngle);
float UTIL_AngleMod(const float &angle);
float UTIL_Approach(const float &target, float value, const float &speed);
float UTIL_ApproachAngle(float target, float value, float speed);
float UTIL_AngleDistance(const float &next, const float &cur);
// Use for ease-in, ease-out style interpolation (accel/decel)
float UTIL_SplineFraction(float value, const float &scale);

/* vector routines */
//int VectorCompare(const float *v1, const float *v2);
void VectorCopy(const float *src, float *dst);
void VectorAdd(const float *a, const float *b, float *dst);
void VectorSubtract(const float *a, const float *b, float *dst);
//void VectorMultiply(const float *a, const float *b, float *dst);
void VectorScale(const float *in, const float &scale, float *out);
void VectorMA(const float *veca, const float &scale, const float *vecb, float *out);

vec_t Length(const float *v);
//float Distance(const float *v1, const float *v2);
//float DotProduct(const float *x, const float *y);
void CrossProduct(const float *v1, const float *v2, float *out);

#define FDotProduct(a, b) (fabs((a[0])*(b[0])) + fabs((a[1])*(b[1])) + fabs((a[2])*(b[2])))

float VectorNormalize(float *v);
void VectorInverse(float *v);

#if defined (__cplusplus)
void VectorRandom(float *v, float min = -1.0f, float max = 1.0f);
inline void VectorClear(float *a) { a[0]=0.0f; a[1]=0.0f; a[2]=0.0f; }
#else
//#error UTIL_VECTOR_H included in C file!
void VectorRandom(float *v, float min, float max);
void VectorClear(float *a) { a[0]=0.0f; a[1]=0.0f; a[2]=0.0f; }
#endif

void VectorRandom(float *v, const float *halfvolume);
void VectorRandom(float *v, const float *mins, const float *maxs);
vec3_t VectorRandom(void);
/*void VectorMatrix(const float *forward, float *right, float *up);*/

// XDM3037a: normalization routines
// Base universal functions
void NormalizeValueF(float *value, const float &min, const float &max);
float NormalizeValueFr(const float &srcvalue, const float &min, const float &max);
// The following are just presets
void NormalizeAngle180(float *angle);
float NormalizeAngle180r(const float &angle);
void NormalizeAngle360(float *angle);
float NormalizeAngle360r(const float &angle);
void NormalizeAngles(float *angles);// +-180 version
void NormalizeAngles360(float *angles);// 0...360 version

void NormalizeValueI(int *value, const int &min, const int &max);

bool StringToVec(const char *str, vec_t *vec);

vec_t VecToYaw(const vec_t *src);

void VectorAngles(const vec_t *forward, vec_t *angles);
void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
void AngleVectorsTranspose(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);

void InterpolateAngles(float *start, float *end, float *output, float frac);
float AngleBetweenVectors(const vec3_t &v1, const vec3_t &v2);
float AngleCosBetweenVectors(const vec3_t &v1, const vec3_t &v2);

//void AngleMatrix(const float *angles, float (*matrix)[4]);
void AngleMatrix(const float *origin, const float *angles, float (*matrix)[4]);
void AngleIMatrix(const float *origin, const float *angles, float (*matrix)[4]);

void VectorTransform(const vec3_t &in1, float in2[3][4], vec3_t &out);
void ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);
void MatrixCopy(float in[3][4], float out[3][4]);

void AngleQuaternion(float *angles, vec4_t quaternion);
void QuaternionSlerp(const vec4_t &p, vec4_t q, const float &t, vec4_t qt);
void QuaternionMatrix(vec4_t quaternion, float (*matrix)[4]);

void V_SmoothInterpolateAngles(float *startAngle, float *endAngle, float *finalAngle, float degreesPerSec, float &frametime);
bool PointInBounds(const vec3_t &point, const vec3_t &mins2, const vec3_t &maxs2);
bool BoundsIntersect(const vec3_t &mins1, const vec3_t &maxs1, const vec3_t &mins2, const vec3_t &maxs2);

bool IsFacing(const vec3_t &origin, const vec3_t &v_angle, const vec3_t &reference);

//#define BOX_ON_PLANE_SIDE(emins, emaxs, p) (((p)->type < 3)?(((p)->dist <= (emins)[(p)->type])? 1 : (((p)->dist >= (emaxs)[(p)->type])? 2 : 3)):BoxOnPlaneSide((emins), (emaxs), (p)))

#define PlaneDist(point,plane) (plane->type < 3 ? point[(int)plane->type] : DotProduct(point, plane->normal))
#define PlaneDiff(point,plane) ((plane->type < 3 ? point[(int)plane->type] : DotProduct(point, plane->normal)) - plane->dist)

#endif /* UTIL_VECTOR_H */
