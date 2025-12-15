/* QuestManager.h
 Manages quests. Derives from Singleton.
*/
#pragma once

#include "object.h"
#include "Quest.h"
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "system/system_utils.h"

class QuestManager 
{
public:
	QuestManager()
	{
		name = "QuestManager";
		SYSTEM_LOG << "QuestManager Initialized\n";
	}
	virtual ~QuestManager()
	{
		SYSTEM_LOG << "QuestManager Shutdown\n";
	}

	static QuestManager& GetInstance()
	{
		static QuestManager instance;
		return instance;
	}
	static QuestManager& Get() { return GetInstance(); }

	void AddQuest(std::unique_ptr<Quest> q)
	{
		quests.push_back(std::move(q));
	}

	const std::vector<std::unique_ptr<Quest>>& GetQuests() const { return quests; }

private:
	string name;
	std::vector<std::unique_ptr<Quest>> quests;
};
