//-----------------------------------------------------------------------------
// Xawari
// Copyright (c) 2001-2017
// Distributed under the terms of Mozilla Public License (MPL) version 2.0
//
// vector.cpp
// Something to do with class Vector (it's in the header actually)
//
// Vector is a 3D vector class which is as fast as an ordinary float[3] array,
// but supports arithmetic operations and many useful functions in a user-friendly way.
// It is fully backwards-compatible with (float *) and (float[3]) functions.
//
//-----------------------------------------------------------------------------
#include "vector.h"
//#include <xmmintrin.h>
/*
#if defined(_LINUX)
#define USE_STDC_FOR_SIMD 0
#else
#define USE_STDC_FOR_SIMD 0
#endif
*/

// Use this name instead of "vec3_origin" to avoid conflicts with PM code, leave it alone. This also applies to "vec3_t".
const vec3_t g_vecZero(0.0f,0.0f,0.0f);

// TODO: make use of SIMD instructions globally
// TODO: make expressions like Vector c = a + 100*b; use less temporary objects... Not CPPossible.


//-----------------------------------------------------------------------------
// Purpose: EXAMPLE: Use class Vector for arguments like this:
// Input  : &vInput - it you use Vectors as input arguments, write like this (protects it from modifications and allows compiler to optimize the resulting code).
//			&vOutput - "output" argument, which this function can modify in the place where it was called from.
// Output : Vector - a new temporary instance of a vector, not a good thing to do. Just an example.
//-----------------------------------------------------------------------------
/*Vector UseVector(const Vector &vInput, Vector &vOutput)
{
	Vector vOne(0,0,1);// direct initialization
	Vector vTwo(vOne);// copies vOne
	Vector vThree;// default initialization (0,0,0)

	// If you have any of these old-style vectors
	float fOldStyleVector[3];
	fOldStyleVector[0] = 3;
	fOldStyleVector[1] = 6;
	fOldStyleVector[2] = 9;
	vThree = fOldStyleVector;// It is possible to copy contents of a simple array[3]

	// Although it is possible, do not write this: Vector vFour = vThree + vTwo * 10.0f;
	// Instead, write:
	Vector vFour(vTwo);// fast copy
	vFour *= 10.0f;// fast multiplication
	vFour += vThree;// fast addition

	// To CALCULATE length:
	vec_t fLength = vFour.Length();// this is an expensive operation btw

	// To GET A normalized vector:
	Vector vNorm(vFour.Normalize());// vFour is UNMODIFIED
	// To NORMALIZE a vector:
	fLength = vFour.NormalizeSelf();// We can save its former length as a bonus

	// To copy back to an array[3]
	vFour.CopyToArray(fOldStyleVector);

	// Instead of normalizing and multiplying you can write this:
	vFour.SetLength(100);

	// Same as vFour *= -1, but faster
	vFour.Negate();

	// To clear a Vector, use this function:
	vFour.Clear();

	// To check if a Vector is (0,0,0) use this instead of == Vector(0,0,0) to avoid creation of a temporary object
	if (vFour.IsZero())
		vFour.z += 10;// access x, y and z as ordinary members

	uint32 i = 1;
	vFour[i] *= 4;// access x, y and z as array elements

	// Instead of using old VectorMA funciton, use this:
	vFour.MultiplyAdd(256, vTwo);

	return vFour;// Since return type is just "Vector", a copy of vFour will be created and passed back to caller.
}*/
