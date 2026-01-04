/* GameRules.h
 Defines rules for a videogame (placeholder).
*/
#pragma once

#include <string>
#include <iostream>
#include "system/system_utils.h"

class GameRules 
{
public:
 std::string name;
 GameRules(const std::string& n = "DefaultRules") : name(n) { SYSTEM_LOG << "GameRules '" << name << "' created\n"; }
 ~GameRules() { SYSTEM_LOG << "GameRules '" << name << "' destroyed\n"; }

 static GameRules& GetInstance()
 {
	 static GameRules instance;
	 return instance;
 }
 static GameRules& Get() { return GetInstance(); }


 // Placeholder for rule checks
 bool Validate() const { return true; }
};
