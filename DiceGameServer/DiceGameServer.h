#ifndef __DICE_GAME_SERVER_H__
#define __DICE_GAME_SERVER_H__

#include "zMisc.h"
#include "zNetService.h"
#include "GameManager.h"
#include "UserSessionManager.h"
#include "UserTask.h"
#include "boost/thread.hpp"
#include "GameGrailCommon.h"

class DiceGameServer : public zNetService, public SingletonBase<DiceGameServer>
{
public:
	friend class SingletonBase<DiceGameServer>;
	
	zTCPTask* CreateTask(uint16_t usPort) 
	{
		return new UserTask(getIOSerivce());
	}

	zTCPTask* newTCPTask(const uint16_t usPort)
	{
		return new UserTask(getIOSerivce());
	}

	bool serviceCallback()
	{
		zNetService::serviceCallback();
		GameManager::getInstance().Check();					
		UserSessionManager::getInstance().doCmd();	
		return true;
	}

private:
	DiceGameServer():zNetService("DiceGameServer"){ }
	~DiceGameServer();
	bool grailInit();

public:
	bool serverInit();
	void reload();
};

#endif