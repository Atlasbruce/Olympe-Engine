/*
	Vector.h
	Nicolas Chereau 2006
	Olympe Engine ::2D Game Editor ::olympe.editor@gmail.com
*/

#include "vector.h"
#include "math.h"
#include "gameengine.h" //used as reference
#include "engine_utils.h"
///////////////////////////////////////////////////////////
float Vector::Dist(Vector& v)
{
	float fresult = 0.0f;
	Vector vResult;
	vResult.x = (float)pow((v.x - x), 2);
	vResult.y = (float)pow((v.y - y), 2);
	vResult.z = (float)pow((v.z - z), 2);
	fresult = (float)sqrt(vResult.x + vResult.y +  vResult.z);
	
	return fresult;
}
///////////////////////////////////////////////////////////
float Vector::sqrDist(Vector& v)
{
	float fresult = 0.0f;
	Vector vResult;
	vResult.x = (float)pow((v.x - x), 2);
	vResult.y = (float)pow((v.y - y), 2);
	vResult.z = (float)pow((v.z - z), 2);
	fresult = (vResult.x + vResult.y +  vResult.z);
	
	return fresult;
}
///////////////////////////////////////////////////////////
Vector vBlend ( Vector& _v1, Vector& _v2, float _fc)
{
	float ff = _fc * (10.0f * GameEngine::fDt );
	return  (_v1 * ff) + (_v2 * (1.0f - ff));

	//return (_v1 * _fc) + (_v2 * (1.0f - _fc));

	// deprecated because it takes more time
	//Vector v =  (_v1 * _fc) + (_v2 * (1.0f - _fc));
	//return v;
}
///////////////////////////////////////////////////////////
ostream& operator<<(ostream &os, const Vector &v)
{
	return os << "(" << v.x << ", " << v.y << ", " << v.z << ")" ; 
}
///////////////////////////////////////////////////////////
bool Vector::operator < (const Vector &v)
{
	return ((x < v.x) && (y < v.y) && (z < v.z)); 
}
///////////////////////////////////////////////////////////
bool Vector::operator <= (const Vector &v)
{
	return ((x <= v.x) && (y <= v.y) && (z <= v.z)); 
}
///////////////////////////////////////////////////////////
bool Vector::operator > (const Vector &v) 
{
	return ((x > v.x) && (y > v.y) && (z > v.z)); 
}
///////////////////////////////////////////////////////////
bool Vector::operator >= (const Vector &v) 
{
	return ((x >= v.x) && (y >= v.y) && (z >= v.z)); 
}
///////////////////////////////////////////////////////////
// ImGui conversion implementations
// Note: These are only usable when imgui.h is included in the translation unit
#ifdef IMGUI_VERSION
#include "third_party/imgui/imgui.h"

ImVec2 Vector::ToImVec2() const
{
	return ImVec2(x, y);
}

Vector Vector::FromImVec2(const ImVec2& v)
{
	return Vector(v.x, v.y, 0.0f);
}
#endif