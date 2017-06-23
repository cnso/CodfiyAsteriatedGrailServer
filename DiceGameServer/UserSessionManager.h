#ifndef __USER_SESSION_MANAGER_H__
#define __USER_SESSION_MANAGER_H__

#include "UserTask.h"
#include "zMutex.h"
#include "zMisc.h"
#include <iostream>
using namespace std;

typedef UserTask UserSession;
typedef UserSession* UserSession_Ptr;

class UserSessionManager : public SingletonBase<UserSessionManager>
{
public:
	typedef std::map<string, UserSession_Ptr> UserMap;
	typedef UserMap::iterator UserMap_Iter;

	typedef std::map<uint32_t, UserSession_Ptr> UserIdMap;
	typedef UserIdMap::iterator UserIdMap_Iter;

private:
	UserMap m_userMap;
	UserIdMap m_userIdMap;
	zMutex m_userMap_lock;
	int32_t m_userIdSeq;

public:
	int32_t GetUserIdSeq();

	void AddUserById(uint32_t userTempId, UserSession_Ptr session); 
	void AddUser(const string userId, UserSession_Ptr session);
	void RemoveUserById(const uint32_t userTempId);
	void RemoveUser(const string userId);
	void trySendMessage(const string userId, uint16_t proto_type, google::protobuf::Message& proto);
	void doCmd();

public:
	UserSessionManager() { }
	~UserSessionManager() {}
};

#endif
