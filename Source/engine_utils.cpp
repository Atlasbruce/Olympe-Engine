/*
	engine_utils.cpp
	Nicolas Chereau 2006
	Olympe Engine ::2D Game Editor ::olympe.editor@gmail.com
*/

#pragma once

#include "engine_utils.h"
//#include <windows.h>
#include "gameengine.h" //used as reference
#include <iostream>
#include "world.h"
#include <fstream>


//////////////////////////////////////////////////////////
void MsgBox(string& stitle, string& smsg, bool _berror)
{
	/*if (_berror)
		MessageBoxA(NULL, smsg.c_str(), stitle.c_str(), MB_OK | MB_ICONERROR | MB_APPLMODAL);
	else
		MessageBoxA(NULL, smsg.c_str(), stitle.c_str(), MB_OK | MB_ICONWARNING | MB_APPLMODAL);/**/
}
//////////////////////////////////////////////////////////
// Clamp a float into a range
float FloatClamp (float _f, float fMin, float fMax)
{
	if (_f <= fMin) 
		return fMin;

	if (_f >= fMax)
		return fMax;

	return _f;
}
int IntClamp (int _i, int iMin, int iMax)
{
	if (_i <= iMin) 
		return iMin;

	if (_i >= iMax)
		return iMax;

	return _i;
}
Vector VectorClamp(Vector _v, Vector _vMin, Vector _vMax)
{
	if (_v <= _vMin)
		return _vMin;

	if (_v >= _vMax)
		return _vMax;
	return _v;
}
//////////////////////////////////////////////////////////
// float blending
float fBlend ( float _f1, float _f2, float _fc)
{
	float f =  (_f1 * _fc ) + (_f2 * (1.0f - _fc ));
	return f;
}

