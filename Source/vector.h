/*
	Vector.h
	Nicolas Chereau 2006
	Olympe Engine ::2D Game Editor ::olympe.editor@gmail.com
	nchereau@gmail.com
*/

#pragma once

#include "math.h"
#include <ostream>
#include <SDL3/SDL.h>
using namespace std;

class Vector;
	Vector vBlend ( Vector& , Vector& , float );
	float fDist ( Vector& , Vector&);

class Vector
{
public:
	
	float x,y,z;
	Vector(void) { x = 0.0f; y = 0.0f; z = 0.0f;} // default constructor
	Vector(const Vector &v) { x = v.x; y = v.y; z = v.z;}// constructor by copy
	Vector(float _x, float _y, float _z) { x = _x; y = _y; z = _z;}
	virtual ~Vector () {}
	
	friend ostream &operator<<(ostream &os, const Vector &v);

	inline Vector&	operator = (const Vector &v) { x = v.x; y = v.y; z = v.z; return *this;}
	inline Vector&	operator = (const float &f) { x = f; y = f; z = f; return *this;}
	inline Vector	operator - (const Vector &v) { return Vector(x-v.x, y-v.y, z-v.z); }
	inline Vector	operator + (const Vector &v) { return Vector(x+v.x, y+v.y, z+v.z); }
	inline Vector	operator * (float scalar) { return Vector(x*scalar, y*scalar, z*scalar); }
	inline Vector&	operator / (float scalar) { x/=scalar; y/=scalar; z/=scalar; return *this; }
	
	bool operator < (const Vector &v);
	bool operator <= (const Vector &v);
	bool operator > (const Vector &v);
	bool operator >= (const Vector &v);
	
	inline Vector&	operator -= (const Vector &v) { x-=v.x; y-=v.y; z-=v.z; return *this; }
	inline Vector&	operator -= (float scalar) { x-=scalar; y-=scalar; z-=scalar; return *this; }
	inline Vector&	operator += (const Vector &v) { x+=v.x; y+=v.y; z+=v.z; return *this; }
	inline Vector&	operator += (float scalar) { x+=scalar; y+=scalar; z+=scalar; return *this; }
	inline Vector&	operator *= (float scalar) { x*=scalar; y*=scalar; z*=scalar; return *this; }
	inline Vector&	operator /= (float scalar) { x/=scalar; y/=scalar; z/=scalar; return *this; }
	
	
	inline Vector	operator- () { return Vector(-x, -y, -z); }
	inline bool		operator== (const Vector &v) { return (x==v.x && y==v.y && z==v.z); }
	inline bool		operator!= (const Vector &v) { return (x!=v.x || y!=v.y || z!=v.z); }

	inline SDL_FPoint ToFPoint() const { SDL_FPoint p = { x, y }; return p; }
	inline SDL_Point ToPoint() const { SDL_Point p = { (int)x, (int)y }; return p; }

	inline float Norm() const { return sqrtf(x*x + y*y + z*z); }
	inline float operator ^ (const Vector &V)	const// cross product
		{	return (x * V.y) - (y * V.x) - (z * V.z); } 
	inline float operator * (const Vector &V)	const// dot product
		{	return (x*V.x) + (y*V.y) + (z*V.z); } 

	inline Vector&	Normalize() { float rc=1/Norm(); x*=rc; y*=rc; z*=rc; return *this; }
	inline float		Length() const { return Norm(); }
	inline float Magnitude() const { return Norm(); }

	inline bool bIsNull (void) { return ((x+y+z) == 0.0f);}
	inline float Dist (Vector&);
	inline float sqrDist(Vector&);

	inline Vector& Set (float _x, float _y, float _z) { x = _x; y = _y; z = _z; return *this;}

	inline float Angle (const Vector& xE)
	{
		float dot = (*this) * xE;
		float cross = (*this) ^ xE;
		
		// angle between segments
		float angle = (float) -atan2(cross, dot);

		return angle;
	}

	inline Vector& Rotate(float angle)
	{
		float tx = x;
		x =  x * cos(angle) - y * sin(angle);
		y = tx * sin(angle) + y * cos(angle);

		return *this;
	}

	inline Vector& Scale(const Vector& _v)
	{
		this->x *= _v.x;
		this->y *= _v.y;
		this->z *= _v.z;
		return *this;
	}

	inline Vector& Scale(const float _x, float _y, float _z)
	{
		this->x *= _x;
		this->y *= _y;
		this->z *= _z;
		return *this;
	}


	inline void Clamp(const Vector& min, const Vector& max)
	{
		x = (x < min.x)? min.x : (x > max.x)? max.x : x;
		y = (y < min.y)? min.y : (y > max.y)? max.y : y;
		z = (z < min.z)? min.z : (z > max.z)? max.z : z;
	}

	inline Vector Blend(Vector& _vDdest, float _ffactor)
	{
		*this = vBlend( *this, _vDdest, _ffactor);
		return *this;
	}
};

//static Vector vector_null = Vector(0.0f, 0.0f, 0.0f);
#define vector_null		Vector(0.0f, 0.0f, 0.0f)
#define vector_up		Vector(0.0f, -1.0f, 0.0f)
#define vector_down		Vector(0.0f, 1.0f, 0.0f)
#define vector_left		Vector(-1.0f, 0.0f, 0.0f)
#define vector_right	Vector(1.0f, 0.0f, 0.0f)
#define	vector_identity	Vector(1.0f, 1.0f, 1.0f)

Vector vBlend ( Vector& _v1, Vector& _v2, float _fc);

inline float fDist ( Vector& _v1, Vector& _v2)
{
	float fd;

	fd =  (float)pow((_v2.x - _v1.x), 2);
	fd += (float)pow((_v2.y - _v1.y), 2);
	fd += (float)pow((_v2.z - _v1.z), 2);
	
	return (float)sqrt(fd);
}

inline float fDist2 ( Vector& _v1, Vector& _v2)
{
	float fd;

	fd =  (float)pow((_v2.x - _v1.x), 2);
	fd += (float)pow((_v2.y - _v1.y), 2);
	fd += (float)pow((_v2.z - _v1.z), 2);
	
	return fd;
}

inline Vector vNormal ( Vector& _v1, bool bLeftSide = true)
{
	Vector v;
	if (bLeftSide)
	{
		v.x =  _v1.y;
		v.y =  -_v1.x;
	}
	else
	{
		v.x =  -_v1.y;
		v.y =  _v1.x;
	}
	v = v.Normalize();
	return v;
}

inline float fAngle ( Vector& _v1, Vector& _v2)
{
	float dot = _v1 * _v2;
	float cross = _v1 ^ _v2;
		
	// angle between segments
	float angle = (float) -atan2(cross, dot);

	return angle;
}
