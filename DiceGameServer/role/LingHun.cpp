#include "LingHun.h"
#include "..\GameGrail.h"
#include "..\UserTask.h"

LingHun::LingHun(GameGrail *engine, int id, int color): PlayerEntity(engine, id, color)
{
	tokenMax[0] = 6;
	tokenMax[1] = 6;
	used_LING_HUN_ZENG_FU = false;
	used_LING_HUN_LIAN_JIE = false;
	using_LING_HUN_LIAN_JIE = false;
	connectID = -1;
}


bool LingHun::cmdMsgParse(UserTask *session, uint16_t type, ::google::protobuf::Message *proto)
{
	switch(type)
	{
	case MSG_RESPOND:
		Respond* respond = (Respond*)proto;
		switch(respond->respond_id())
		{
		case LING_HUN_ZHUAN_HUAN:
			//tryNotify��������Ϸ���̴߳���Ϣ��ֻ��id���ڵ�ǰ�ȴ�id������state���ڵ�ǰstate������step���ڵ�ǰstep����Ϸ���̲߳Ż����
			session->tryNotify(id,STATE_TIMELINE_1,LING_HUN_ZHUAN_HUAN, respond);
			return true;

		case LING_HUN_LIAN_JIE:
			//tryNotify��������Ϸ���̴߳���Ϣ��ֻ��id���ڵ�ǰ�ȴ�id������state���ڵ�ǰstate������step���ڵ�ǰstep����Ϸ���̲߳Ż����
			session->tryNotify(id,STATE_BOOT,LING_HUN_LIAN_JIE, respond);  //��ʲô״̬������
			return true;

		case LING_HUN_ZENG_FU:
			//tryNotify��������Ϸ���̴߳���Ϣ��ֻ��id���ڵ�ǰ�ȴ�id������state���ڵ�ǰstate������step���ڵ�ǰstep����Ϸ���̲߳Ż����
			session->tryNotify(id, STATE_BOOT, LING_HUN_ZENG_FU, respond);  //��ʲô״̬������
			return true;
		case LING_HUN_LIAN_JIE_REACT:
			session->tryNotify(id, STATE_TIMELINE_6, STEP_INIT, respond);
			return true;
		}
	}
	//ûƥ���򷵻�false
	return false;
}

//ͳһ��p_before_turn_begin ��ʼ�����ֻغϱ���
int LingHun::p_before_turn_begin(int &step, int currentPlayerID) 
{
	used_LING_HUN_ZENG_FU = false;
	return GE_SUCCESS; 
}

int LingHun::p_boot(int &step, int currentPlayerID)
{
	int ret = GE_INVALID_STEP;
	if (currentPlayerID != id ){
		return GE_SUCCESS;
	}
	if(step == STEP_INIT)
	{
		step = LING_HUN_ZENG_FU;
	}
	if(step == LING_HUN_ZENG_FU)
	{
		ret = LingHunZengFu();
		if(toNextStep(ret)){
			if(token[0] > 0 && token[1] > 0 && !used_LING_HUN_LIAN_JIE && !used_LING_HUN_ZENG_FU)
				step = LING_HUN_LIAN_JIE;
			else
				step = STEP_DONE;
		}
	}
	if(step == LING_HUN_LIAN_JIE)
	{
		ret = LingHunLianJie();
		if(toNextStep(ret)){
			step = STEP_DONE;
		}
	}
	return ret;
}

int LingHun::p_timeline_1(int &step, CONTEXT_TIMELINE_1 *con)
{
	int ret = GE_INVALID_STEP;
	if(con->attack.srcID != id){
		return GE_SUCCESS;
	}
	//���ɹ�����������ߣ�ʧ���򷵻أ�step�ᱣ�����´��ٽ����Ͳ�������
	//һ�㳬ʱҲ�������һ��
	if(step == STEP_INIT)
	{
		step = LING_HUN_ZHUAN_HUAN;
	}
	if(step == LING_HUN_ZHUAN_HUAN)
	{
		ret = LingHunZhuanHuan(con);
		if(toNextStep(ret) || ret == GE_URGENT){
			step = STEP_DONE;
		}
	}
	return ret;
}


int LingHun::p_timeline_6(int &step, CONTEXT_TIMELINE_6 *con) 
{    
	//��������ӡ���ʹ��          ����������Ϊ�������Ӷ���                                                         
	if(used_LING_HUN_LIAN_JIE && (con->dstID == id || con->dstID == connectID))
	{
		int ret = GE_INVALID_STEP;
		//��ʼ��step
		if(step == STEP_INIT){
			ret = LingHunLianJieReact(con);
			if(ret == GE_URGENT){
				step = LING_HUN_LIAN_JIE_REACT;
			}
		}
		else if(step == LING_HUN_LIAN_JIE_REACT){
			using_LING_HUN_LIAN_JIE = false;
			step = STEP_DONE;
			ret = GE_SUCCESS;
		}
		return ret;
	}
	else{
		return GE_SUCCESS;
	}
}

//�������ʿ������ͨ�������� ��������ɡ������ҷ�ÿ��1��ʿ���½�����+1����ɫ��꡿

int LingHun::p_true_lose_morale(int &step, CONTEXT_LOSE_MORALE *con) 
{ 
	//id-->new_color-->if(new_color=color) 
	int dst_color;
	int current_color;
	int harmPoint=con->howMany;
	dst_color=engine->getPlayerEntity(con->dstID)->getColor();
	current_color=this->getColor();

	if(dst_color==current_color)
	{
		setToken(0,token[0]+harmPoint);  //+1���ƻ꡿
		network::GameInfo update;
		Coder::tokenNotice(id,0,token[0],update);
		engine->sendMessage(-1, MSG_GAME, update);
	}   
	return  GE_SUCCESS;
}

int LingHun::v_magic_skill(Action *action)
{
	int actionID = action->action_id();
	int cardID;
	int playerID = action->src_id();
	CardEntity* card;

	if(playerID != id){
		return GE_INVALID_PLAYERID;
	}
	switch(actionID)
	{
	case  LING_HUN_CI_YU:
		cardID = action->card_ids(0);
		card = getCardByID(cardID);
		//�����Լ�������                          || ���Ƕ�Ӧ�ķ�����                  ||��������С��3  
		if(GE_SUCCESS != checkOneHandCard(cardID) || !card->checkSpeciality(actionID)||getToken(1)<3){
			return GE_INVALID_ACTION;
		}
		break;
	case LING_HUN_ZHEN_BAO:
		cardID = action->card_ids(0);
		card = getCardByID(cardID);
		//�����Լ�������                          || ���Ƕ�Ӧ�ķ�����                  ||�ƻ�����С��3  
		if(GE_SUCCESS != checkOneHandCard(cardID) || !card->checkSpeciality(actionID)||getToken(0)<3){
			return GE_INVALID_ACTION;
		}
		break;

	case LING_HUN_JING_XIANG:
		//��ɫ���---��������
		if(getToken(0)<2){
			return GE_INVALID_ACTION;
		}
		break;

	case LING_HUN_ZHAO_HUAN:
		return GE_SUCCESS;
		break;

	default:
		return GE_INVALID_ACTION;
	}
	return GE_SUCCESS;
}

int LingHun::p_magic_skill(int &step, Action* action)
{
	//p_magic_skill��ͬ�ڱ�Ĵ����㣬����ֻ��һ��ƥ�䣬���ÿһ���������ʱ����ذ�step��ΪSTEP_DONE
	int ret;
	switch(action->action_id())
	{
	case  LING_HUN_CI_YU:
		ret = LingHunCiYu(action);
		if(GE_URGENT == ret){
			step = STEP_DONE;
		}
		break;
	case LING_HUN_ZHEN_BAO:
		ret = LingHunZhenBao(action);
		if(GE_URGENT == ret){
			step = STEP_DONE;
		}	
		break;
	case LING_HUN_JING_XIANG:
		ret = LingHunJingXiang(action);
		if(GE_URGENT == ret){
			step = STEP_DONE;
		}	
		break;
	case LING_HUN_ZHAO_HUAN:
		//����ٻ�
		ret = LingHunZhaoHuan(action);
		if(GE_URGENT == ret){
			step = STEP_DONE;
		}	
		break;
	default:
		return GE_INVALID_ACTION;
	}
	return ret;
}

//--------------�����������-------------------------
int LingHun::LingHunZengFu()
{
	if(getGem() <=0)
		return GE_SUCCESS;

	CommandRequest cmd_req;
	Coder::askForSkill(id, LING_HUN_ZENG_FU, cmd_req);
	//���޵ȴ�����UserTask����tryNotify����
	if(engine->waitForOne(id, network::MSG_CMD_REQ, cmd_req))
	{
		void* reply;
		int ret;
		if (GE_SUCCESS == (ret = engine->getReply(id, reply)))
		{			
			Respond* respond = (Respond*) reply;
			//����
			if (respond->args(0) == 1)  //�Լ�����
			{
				if (gem > 0)
				{
					used_LING_HUN_ZENG_FU = true;
					network::SkillMsg skill_msg;
					Coder::skillNotice(id, id, LING_HUN_ZENG_FU, skill_msg);
					engine->sendMessage(-1, MSG_SKILL, skill_msg);
					setGem(--gem);
					setToken(0,token[0]+2); //��ɫ���+2
					setToken(1,token[1]+2); //��ɫ���+2
					GameInfo game_info;
					Coder::tokenNotice(id,0,token[0],game_info);
					Coder::tokenNotice(id,1,token[1], game_info);
					Coder::energyNotice(id, gem, crystal, game_info);
					engine->sendMessage(-1, MSG_GAME, game_info);
					return GE_SUCCESS;
				}
				else{
					return GE_INVALID_ACTION;
				}
			}
			return GE_SUCCESS;
		}
		return ret;
	}
	else{
		//��ʱɶ��������
		return GE_TIMEOUT;
	}
}
//-----------�������衿--------------
int LingHun::LingHunCiYu(Action* action)
{
	int dstID = action->dst_ids(0);
	int cardID = action->card_ids(0);
	PlayerEntity * dstPlayer = engine->getPlayerEntity(dstID);
	CardEntity* card = getCardByID(cardID);
	//������Ƿ�Ϊ��Ӧ������
	if( !card->checkSpeciality(LING_HUN_CI_YU)){
		return GE_SUCCESS;
	}


	//���漼��
	SkillMsg skill_msg;
	Coder::skillNotice(id, dstID, LING_HUN_CI_YU, skill_msg);
	engine->sendMessage(-1, MSG_SKILL, skill_msg);	

	setToken(1,token[1]-3);              //��ɫ���-3	
	dstPlayer->setGem(dstPlayer->getGem()+2);
	network::GameInfo update;
	Coder::tokenNotice(id,1,token[1],update);
	Coder::energyNotice(dstID, dstPlayer->getGem(), dstPlayer->getCrystal(), update);
	engine->sendMessage(-1, MSG_GAME, update);

	//չʾ����
	CardMsg show_card;
	Coder::showCardNotice(id, 1, cardID, show_card);
	engine->sendMessage(-1, MSG_CARD, show_card);
	engine->setStateMoveOneCardNotToHand(id, DECK_HAND, -1, DECK_DISCARD, cardID, id,LING_HUN_CI_YU, true);
	return GE_URGENT;
}
//-------------������𱬡�--------------
int LingHun::LingHunZhenBao(Action *action)
{
	int dstID = action->dst_ids(0);
	int magic_id = action->action_id();
	int cardID = action->card_ids(0);
	PlayerEntity * dstPlayer = engine->getPlayerEntity(dstID);

	CardEntity* card = getCardByID(cardID);
	//������Ƿ�Ϊ��Ӧ������
	if( !card->checkSpeciality(LING_HUN_ZHEN_BAO)){
		return GE_SUCCESS;
	}

	//���漼��
	SkillMsg skill_msg;
	Coder::skillNotice(id, dstID, magic_id, skill_msg);
	engine->sendMessage(-1, MSG_SKILL, skill_msg);

	//չʾ����
	CardMsg show_card;
	Coder::showCardNotice(id, 1, cardID, show_card);
	engine->sendMessage(-1, MSG_CARD, show_card);

	setToken(0,token[0]-3); //��ɫ���-3	
	network::GameInfo update;
	Coder::tokenNotice(id,0,token[0],update);
	engine->sendMessage(-1, MSG_GAME, update);

	// �����˺�
	HARM harm;
	harm.cause = magic_id;
	if(dstPlayer->getHandCardNum()<3&&dstPlayer->getHandCardMax()>5)
		harm.point = 5;
	else 
		harm.point=3;
	harm.srcID = id;
	harm.type = HARM_MAGIC;
	engine->setStateTimeline3(dstID, harm);

	// ��������
	engine->setStateMoveOneCardNotToHand(id, DECK_HAND, -1, DECK_DISCARD, cardID, id, LING_HUN_ZHEN_BAO, true);

	return GE_URGENT;
}

int LingHun::LingHunJingXiang(Action* action)
{
	vector<int> cardIDs;
	vector<int> cards;
	int cardNum= action->card_ids_size();
	int  dstID = action->dst_ids(0);
	PlayerEntity *dstPlayer = engine->getPlayerEntity(dstID);
	int max=dstPlayer->getHandCardMax();
	int currentHandCards=dstPlayer->getHandCardNum();

	setToken(0,token[0]-2); //��ɫ���-2	
	network::GameInfo update;
	Coder::tokenNotice(id,0,token[0],update);

	engine->sendMessage(-1, MSG_GAME, update);
	
	if(cardNum>3)      cardNum=3;

	for(int i = 0; i < cardNum;i ++)
	{
		cardIDs.push_back(action->card_ids(i));
	}
	//���漼��
	SkillMsg skill_msg;
	Coder::skillNotice(id, dstID, LING_HUN_JING_XIANG, skill_msg);
	engine->sendMessage(-1, MSG_SKILL, skill_msg);

    int drawNum = (currentHandCards+3)<max?3:(max-currentHandCards);

	//�����꾵���Ƕ��Լ��õĻ������Ȱ��������ƣ�������Ӧ�������ƺ����������
	if(dstID == this->id){
		 drawNum = (currentHandCards-cardNum+3)<max?3:(max-(currentHandCards-cardNum));
	}
	if(drawNum>0){
		HARM  jingXiang;
		jingXiang.type = HARM_NONE;
		jingXiang.point = drawNum; 
		jingXiang.srcID = id;
		jingXiang.cause = LING_HUN_JING_XIANG;
		engine->setStateMoveCardsToHand(-1, DECK_PILE, dstID, DECK_HAND, jingXiang.point, vector< int >(), jingXiang, false);
	}
	if(cardNum>0){
		engine->setStateMoveCardsNotToHand(id, DECK_HAND, -1, DECK_DISCARD, cardNum, cardIDs, id, LING_HUN_JING_XIANG, false);//���ƣ��˺�
	}

	//��������״̬����return GE_URGENT
	return GE_URGENT;
}

int LingHun::LingHunZhaoHuan(Action* action)
{
	vector<int> cardIDs;
	vector<int> cards;

	int cardNum= action->card_ids_size();
	setToken(1,token[1]+cardNum+1);  //��ɫ���+��X+1)	
	network::GameInfo update;
	Coder::tokenNotice(id,1,token[1], update);
	engine->sendMessage(-1, MSG_GAME, update);

	for(int i = 0; i < cardNum;i ++)
	{
		cardIDs.push_back(action->card_ids(i));
	}
	//���漼��
	SkillMsg skill_msg;
	Coder::skillNotice(id, id,LING_HUN_ZHAO_HUAN, skill_msg);
	engine->sendMessage(-1, MSG_SKILL, skill_msg);

	//��X����
	CardMsg show_card;
	Coder::showCardNotice(id, cardNum, cardIDs, show_card);
	engine->sendMessage(-1, MSG_CARD, show_card);
	engine->setStateMoveCardsNotToHand(id, DECK_HAND, -1, DECK_DISCARD, cardNum, cardIDs, id, LING_HUN_ZHAO_HUAN, true);//���ƣ��˺�

	//��������״̬����return GE_URGENT
	return GE_URGENT;

}

int LingHun::LingHunZhuanHuan(CONTEXT_TIMELINE_1 *con)
{
	//�ƻ�ת��Ϊ����
	//����ת��Ϊ�ƻ�
	if(!con->attack.isActive || (token[0]<=0 && token[1]<=0))
		return GE_SUCCESS;
	int ret;
	//���㷢��������ѯ�ʿͻ����Ƿ񷢶�
	CommandRequest cmd_req;
	Coder::askForSkill(id, LING_HUN_ZHUAN_HUAN, cmd_req);
	//���޵ȴ�����UserTask����tryNotify����
	if(engine->waitForOne(id, network::MSG_CMD_REQ, cmd_req))
	{
		void* reply;
		if (GE_SUCCESS == (ret = engine->getReply(id, reply)))
		{
			Respond* respond = (Respond*) reply;
			//���� 
			if(respond->args(0) == 0||respond->args(0) == 1){
				if(respond->args(0) == 0)  //�ƻ� ת��Ϊ ����(��������������Ƿ�Ҫ���жϣ����д�������)
				{
					setToken(0,token[0]-1);
					setToken(1,token[1]+1);
				}
				else
				{
					setToken(0,token[0]+1);
					setToken(1,token[1]-1);	   
				}
				network::GameInfo update_info;
				Coder::tokenNotice(id,0,token[0], update_info);
				Coder::tokenNotice(id,1,token[1], update_info);
				engine->sendMessage(-1, MSG_GAME, update_info);

				network::SkillMsg skill;
				Coder::skillNotice(id, id, LING_HUN_ZHUAN_HUAN, skill);
				engine->sendMessage(-1, MSG_SKILL, skill);
			}
		}
		return ret;
	}
	else{
		//��ʱɶ��������
		return GE_TIMEOUT;
	}
}


//��������ӡ�
int LingHun::LingHunLianJie(){

	int ret;
	int dstID;

	//�����ơ���ɫ���С��3��ʱ����ֹ��������Ϊ�������޷��ж�
	int cardNum = this->getHandCardNum();
	if(cardNum == 0 && token[0] < 3 || engine->getGameMaxPlayers() == 4){
		return GE_SUCCESS;
	}

	//���㷢��������ѯ�ʿͻ����Ƿ񷢶�
	CommandRequest cmd_req;
	Coder::askForSkill(id, LING_HUN_LIAN_JIE, cmd_req);
	//���޵ȴ�����UserTask����tryNotify����
	if(engine->waitForOne(id, network::MSG_CMD_REQ, cmd_req))
	{
		void* reply;
		if (GE_SUCCESS == (ret = engine->getReply(id, reply)))
		{
			Respond* respond = (Respond*) reply;
			//���� 
			if(respond->args(0) == 1){
				//����
				dstID=respond->dst_ids(0);
				PlayerEntity* dst = engine->getPlayerEntity(dstID);
				dst->addExclusiveEffect(EX_LING_HUN_LIAN_JIE);
				addExclusiveEffect(EX_LING_HUN_LIAN_JIE);

				setToken(0,token[0]-1);  //��ɫ���-1	
				setToken(1,token[1]-1);  //��ɫ���-1	

				network::GameInfo update;
				Coder::exclusiveNotice(dstID, dst->getExclusiveEffect(), update);
				Coder::exclusiveNotice(id, this->getExclusiveEffect(), update);
				Coder::tokenNotice(id,0,token[0], update);
				Coder::tokenNotice(id,1,token[1], update);
				engine->sendMessage(-1, MSG_GAME, update);

				connectID=dstID;
				used_LING_HUN_LIAN_JIE=true;
			}
		}
		return ret;
	}

	else{
		//��ʱɶ��������
		return GE_TIMEOUT;
	}
}

int LingHun::LingHunLianJieReact(CONTEXT_TIMELINE_6 *con)
{
	if(token[1] < 1 || using_LING_HUN_LIAN_JIE  || con->harm.point < 1){
		return GE_SUCCESS;
	}
	int  ret;
	//���㷢��������ѯ�ʿͻ����Ƿ񷢶�
	CommandRequest cmd_req;
	Coder::askForSkill(id,LING_HUN_LIAN_JIE_REACT, cmd_req);
	Command *cmd = (Command*)(&cmd_req.commands(cmd_req.commands_size()-1));
	cmd->add_args(con->harm.point);
	//���޵ȴ�����UserTask����tryNotify����
	if(engine->waitForOne(id, network::MSG_CMD_REQ, cmd_req))
	{
		void* reply;
		if (GE_SUCCESS == (ret = engine->getReply(id, reply)))
		{
			Respond* respond = (Respond*) reply;
			//���� 
			int howMany = respond->args(0);
			if(howMany > 0){
				if(howMany > token[1] || howMany < 0){
					return GE_INVALID_ARGUMENT;
				}
				//��������
				network::SkillMsg skill;
				Coder::skillNotice(id, connectID, LING_HUN_LIAN_JIE_REACT, skill);
				engine->sendMessage(-1, MSG_SKILL, skill);

				// �����˺�
				HARM harm;
				harm.cause = LING_HUN_LIAN_JIE;
				harm.point = howMany;
				harm.srcID = id;
				harm.type = HARM_MAGIC;
				con->harm.point -= howMany;

				setToken(1, token[1] - howMany);
				network::GameInfo update_info;
				Coder::tokenNotice(id,1,token[1], update_info);
				engine->sendMessage(-1, MSG_GAME, update_info);
				using_LING_HUN_LIAN_JIE = true;

				int dstID = con->dstID;
				if(dstID == id){   
					engine->setStateTimeline6(connectID, harm);
				}
				else{
					engine->setStateTimeline6(id, harm);   
				}	
				return GE_URGENT;
			}
		}
		return ret;
	}
	else{
		//��ʱɶ��������
		return GE_TIMEOUT;
	}
}
