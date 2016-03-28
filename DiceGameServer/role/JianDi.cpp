#include "JianDi.h"
#include "..\GameGrail.h"
#include "..\UserTask.h"

bool JianDi::cmdMsgParse(UserTask* session, uint16_t type, ::google::protobuf::Message *proto)
{
	switch(type)
	{
	case MSG_RESPOND:
		Respond* respond = (Respond*)proto;
		switch(respond->respond_id())
		{
		case JIAN_HUN_SHOU_HU :
			//tryNotify��������Ϸ���̴߳���Ϣ��ֻ��id���ڵ�ǰ�ȴ�id������state���ڵ�ǰstate������step���ڵ�ǰstep����Ϸ���̲߳Ż����
			session->tryNotify(id, STATE_TIMELINE_2_MISS,JIAN_HUN_SHOU_HU, respond);
			return true;
			break;

		case BU_QU_YI_ZHI :
			//tryNotify��������Ϸ���̴߳���Ϣ��ֻ��id���ڵ�ǰ�ȴ�id������state���ڵ�ǰstate������step���ڵ�ǰstep����Ϸ���̲߳Ż����
			session->tryNotify(id,STATE_AFTER_ATTACK,BU_QU_YI_ZHI, respond);
			return true;
			break;

		case JIAN_QI_ZHAN :
			//tryNotify��������Ϸ���̴߳���Ϣ��ֻ��id���ڵ�ǰ�ȴ�id������state���ڵ�ǰstate������step���ڵ�ǰstep����Ϸ���̲߳Ż����
			session->tryNotify(id,STATE_TIMELINE_2_HIT,JIAN_QI_ZHAN, respond);
			return true;
			break;

		case TIAN_SHI_ZHI_HUN:
			//tryNotify��������Ϸ���̴߳���Ϣ��ֻ��id���ڵ�ǰ�ȴ�id������state���ڵ�ǰstate������step���ڵ�ǰstep����Ϸ���̲߳Ż����
			session->tryNotify(id,STATE_TIMELINE_1,TIAN_SHI_ZHI_HUN, respond);
			return true;
			break;

		case E_MO_ZHI_HUN:
			//tryNotify��������Ϸ���̴߳���Ϣ��ֻ��id���ڵ�ǰ�ȴ�id������state���ڵ�ǰstate������step���ڵ�ǰstep����Ϸ���̲߳Ż����
			session->tryNotify(id,STATE_TIMELINE_1,E_MO_ZHI_HUN, respond);
			return true;
			break;
		}
	}
	//ûƥ���򷵻�false
	return false;
}

//ͳһ��p_before_turn_begin ��ʼ�����ֻغϱ���
int JianDi::p_before_turn_begin(int &step, int currentPlayerID) 
{
	used_TIAN_SHI_ZHI_HUN=false;
	used_E_MO_ZHI_HUN=false;
	int ret = TianShiYuEMo();

	return ret; 
}

//����ʹ֮�꡿ �� ����ħ֮�꡿
int JianDi::p_timeline_1(int &step, CONTEXT_TIMELINE_1 *con)
{
	int srcID = con->attack.srcID;
	int dstID = con->attack.dstID;
	TianShiYuEMo();
	int ret = GE_INVALID_STEP;
	// ���Ǳ���    || ������������          || û������  || û�н���
	if(srcID != id || !con->attack.isActive || flag == 0 || coverCards.size() == 0){
		return GE_SUCCESS;
	}	
	if(step == STEP_INIT) {
		//������ʹ֮��
		if(flag==1)
		{
			step =TIAN_SHI_ZHI_HUN;
		}
		//������ħ֮��
		else if(flag==2)
		{
			step =E_MO_ZHI_HUN;
		}
		else 
		{
			step =STEP_DONE;
			ret = GE_SUCCESS;
		}
	}

	if(step==TIAN_SHI_ZHI_HUN)
	{
		ret=TianShiZhiHun();
		if(toNextStep(ret)|| ret == GE_URGENT){
			step = STEP_DONE;
		}
	}

	if(step==E_MO_ZHI_HUN)
	{
		ret=EMoZhiHun();
		if(toNextStep(ret)|| ret == GE_URGENT){
			step = STEP_DONE;
		}
	}
	return ret;
}

//������ն�� ����ħ֮�꡿����ʹ֮�꡿
int JianDi::p_timeline_2_hit(int &step, CONTEXT_TIMELINE_2_HIT *con)
{
	int ret = GE_INVALID_STEP;
	if(con->attack.srcID != id || !con->attack.isActive){
		return GE_SUCCESS;
	}
	//����������Ϊ0
	if(step == STEP_INIT)
	{
		step=JIAN_QI_ZHAN;
	}
	if(step ==JIAN_QI_ZHAN)
	{
		ret = JianQiZhan(con);
		if(toNextStep(ret) || GE_URGENT == ret)
		{
			//ȫ����������step���STEP_DONE
			step = E_MO_ZHI_HUN;
		}
	}
	if(step == E_MO_ZHI_HUN)
	{
		if(used_E_MO_ZHI_HUN)
		{
			network::SkillMsg skill;
			Coder::skillNotice(id, id,E_MO_ZHI_HUN, skill);
			engine->sendMessage(-1, MSG_SKILL, skill);
			//�����У��˺�+1
			con->harm.point+=1;
			used_E_MO_ZHI_HUN=false;
		}
		step = TIAN_SHI_ZHI_HUN;
	}

	if(step == TIAN_SHI_ZHI_HUN)
	{
		if(used_TIAN_SHI_ZHI_HUN)
		{
			//�����У�����+2����
			ret = TianShiZhiHun_EffectHit();
			used_TIAN_SHI_ZHI_HUN=false;
		}
		step = STEP_DONE;
	}
	return ret;
}

//���𹥡�����ħ֮�꡿����ʹ֮�꡿�������ػ���
int JianDi::p_timeline_2_miss(int &step, CONTEXT_TIMELINE_2_MISS *con) { 

	//��֤�Ǳ���ɫ��Ӧս������ʧҲ�����
	if(con->srcID != id || !con->isActive){
		return GE_SUCCESS;
	}
	int ret = GE_INVALID_STEP;
	if(step == STEP_INIT)
	{
		step = YANG_GONG;
	}
	if(step == YANG_GONG)
	{
		ret = YangGon();
		step = JIAN_HUN_SHOU_HU;
	}
	if(step == JIAN_HUN_SHOU_HU)
	{
		ret = JianHunShouHu(con);
		if(toNextStep(ret) || GE_URGENT == ret)
		{
			//ȫ����������step���STEP_DONE
			step = TIAN_SHI_ZHI_HUN;
		}
	}
	if(step == TIAN_SHI_ZHI_HUN)
	{
		if (used_TIAN_SHI_ZHI_HUN )
		{
			ret = TianShiZhiHun_EffectMiss();
			used_TIAN_SHI_ZHI_HUN = false;
		}
		step = E_MO_ZHI_HUN;
	}
	if(step == E_MO_ZHI_HUN)
	{
		if(used_E_MO_ZHI_HUN)
		{
			ret = EMoZhiHun_EffectMiss();
			used_E_MO_ZHI_HUN = false;
		}
		step = STEP_DONE;
	}
	return ret;
}

int JianDi::p_after_attack(int &step, int playerID)
{
	int ret = GE_INVALID_STEP;
	//
	if(playerID != id)
	{
		return GE_SUCCESS;
	}
	if(getEnergy()>0)
		addAction(ACTION_ATTACK, BU_QU_YI_ZHI);
	return GE_SUCCESS;
}

int JianDi::p_additional_action(int chosen)
{
	int ret = GE_INVALID_STEP;
	PlayerEntity::p_additional_action(chosen);
	if(chosen == BU_QU_YI_ZHI)
	{
		ret = BuQuYiZhi();
	}
	return ret;
}

//����ʹ���ħ��
int JianDi::TianShiYuEMo()
{
	int energey = getEnergy();
	if(energey == 0)         flag=0;
	else if(energey % 2 != 0)  flag=1; //����Ϊ��ʹ֮��
	else                       flag=2; //˫��Ϊ��ħ֮��

	return GE_SUCCESS;
}
//���𹥡�
int JianDi::YangGon()
{
	network::SkillMsg skill;
	Coder::skillNotice(id, id,YANG_GONG, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
	setToken(0,token[0]+1);
	network::GameInfo update_info;
	Coder::tokenNotice(id,0,token[0], update_info);
	engine->sendMessage(-1, MSG_GAME, update_info);
	return GE_SUCCESS;
}
//�������ػ���
int JianDi::JianHunShouHu(CONTEXT_TIMELINE_2_MISS* con)
{
	if(getCoverCardNum() >= 3 || used_TIAN_SHI_ZHI_HUN || used_E_MO_ZHI_HUN)
		return GE_SUCCESS;
	network::SkillMsg skill;
	Coder::skillNotice(id, id,JIAN_HUN_SHOU_HU, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
	/*
	cardID=con->cardID;
	vector<int> cards;
	cards.push_back(cardID);
	addCoverCards(1, cards); 
	//���¸���
	GameInfo game_info;
	Coder::coverNotice(id,getCoverCards(), game_info);
	engine->sendMessage(-1, MSG_GAME, game_info);
	*/
	engine->setStateMoveOneCardNotToHand(-1,DECK_DISCARD,id,DECK_COVER,con->cardID,id,JIAN_HUN_SHOU_HU,false);
	return GE_URGENT;
}
//��������־��
int JianDi::BuQuYiZhi()
{   
	vector<int> cards;

	//��������
	if(crystal > 0)
		setCrystal(--crystal);
	else if (gem > 0)
		setGem(--gem);

	GameInfo game_info;
	Coder::energyNotice(id, gem, crystal,game_info);

	//���һ��������
	setToken(0,token[0]+1);
	Coder::tokenNotice(id,0,token[0], game_info);
	engine->sendMessage(-1, MSG_GAME, game_info);

	//��һ������
	HARM  harm;
	harm.srcID = id;
	harm.type = HARM_NONE;
	harm.point =1;  //��������
	harm.cause =BU_QU_YI_ZHI;
	engine->setStateMoveCardsToHand(-1, DECK_PILE, id, DECK_HAND,1,cards, harm, false);

	return GE_URGENT;
}
//������ն��
int JianDi::JianQiZhan(CONTEXT_TIMELINE_2_HIT *con)
{
	if(token[0] == 0)
		return GE_SUCCESS;
	int ret;
	int dstID;
	int JianQiCount;
	CommandRequest cmd_req;
	Coder::askForSkill(id,JIAN_QI_ZHAN, cmd_req);
	Command *cmd = (Command*)(&cmd_req.commands(cmd_req.commands_size()-1));
	cmd->add_args(con->attack.dstID);
	if(engine->waitForOne(id, network::MSG_CMD_REQ, cmd_req))
	{
		void* reply;
		if (GE_SUCCESS == (ret = engine->getReply(id, reply)))
		{
			Respond* respond = (Respond*) reply;
			//���� 
			if(respond->args(0) == 1){
				//��������
				network::SkillMsg skill;
				Coder::skillNotice(id, id,JIAN_QI_ZHAN, skill);
				engine->sendMessage(-1, MSG_SKILL, skill);

				JianQiCount=respond->args(1);
				dstID=respond->dst_ids(0);

				//�Ƴ�����
				setToken(0,token[0]-JianQiCount);
				network::GameInfo update_info;
				Coder::tokenNotice(id,0,token[0], update_info);
				engine->sendMessage(-1, MSG_GAME, update_info);

				HARM  harm;
				harm.srcID = id;
				harm.type = HARM_MAGIC;
				harm.point =JianQiCount;  //��������
				harm.cause =JIAN_QI_ZHAN;
				engine->setStateTimeline3(dstID,harm);
				return GE_URGENT;
			}
			else
			{
				return GE_SUCCESS;
			}
		}
		return ret;
	}
	else
	{
		//��ʱɶ��������
		return GE_TIMEOUT;
	}
}
//����ʹ֮�꡿
int JianDi::TianShiZhiHun()
{    
	int ret;
	int card_id;
	CommandRequest cmd_req;
	Coder::askForSkill(id, TIAN_SHI_ZHI_HUN, cmd_req);
	//���޵ȴ�����UserTask����tryNotify����
	if(engine->waitForOne(id, network::MSG_CMD_REQ, cmd_req))
	{
		void* reply;
		if (GE_SUCCESS == (ret = engine->getReply(id, reply)))
		{
			Respond* respond = (Respond*) reply;
			//����
			if(respond->args(0)==1)         //����
			{
				//���漼��
				network::SkillMsg skill;
				Coder::skillNotice(id, id,TIAN_SHI_ZHI_HUN, skill);
				engine->sendMessage(-1, MSG_SKILL, skill);

				used_TIAN_SHI_ZHI_HUN = true;

				//�Ƴ�һ������ʹ֮�꡿
				card_id = respond->card_ids(0);
				engine->setStateMoveOneCardNotToHand(id,DECK_COVER,-1,DECK_DISCARD,card_id,id,TIAN_SHI_ZHI_HUN,false);
				return GE_URGENT;
			}

			return GE_SUCCESS ;
		}
		return ret;
	}

	else{
		//��ʱɶ��������
		return GE_TIMEOUT;
	}

}

int JianDi::TianShiZhiHun_EffectHit()
{
	network::SkillMsg skill;
	Coder::skillNotice(id, id,TIAN_SHI_ZHI_HUN, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
	//+2�����ơ�
	addCrossNum(2);
	GameInfo update_info;
	Coder::crossNotice(id, getCrossNum(), update_info);
	engine->sendMessage(-1, MSG_GAME, update_info);
	return GE_SUCCESS;
}

int JianDi::TianShiZhiHun_EffectMiss()
{
	//+1��ʿ����
	int color=getColor();
	TeamArea *m_teamArea = engine->getTeamArea();
	int morale = m_teamArea->getMorale(color);
	if(morale<15) 
	{
		network::SkillMsg skill;
		Coder::skillNotice(id, id,TIAN_SHI_ZHI_HUN, skill);
		engine->sendMessage(-1, MSG_SKILL, skill);

		m_teamArea->setMorale(color, morale+1);
	}
	morale = m_teamArea->getMorale(color);
	GameInfo game_info;	
	if (color == RED)
	{
		game_info.set_red_morale(morale);  
	}
	else
	{
		game_info.set_blue_morale(morale);
	}
	engine->sendMessage(-1, MSG_GAME, game_info);
	return GE_SUCCESS;
}

//����ħ֮�꡿
int JianDi::EMoZhiHun()
{
	int ret;
	int card_id;
	CommandRequest cmd_req;
	Coder::askForSkill(id,E_MO_ZHI_HUN, cmd_req);
	//���޵ȴ�����UserTask����tryNotify����
	if(engine->waitForOne(id, network::MSG_CMD_REQ, cmd_req))
	{
		void* reply;
		if (GE_SUCCESS == (ret = engine->getReply(id, reply)))
		{
			Respond* respond = (Respond*) reply;
			//����
			if(respond->args(0)==1)         //����
			{
				//���漼��
				network::SkillMsg skill;
				Coder::skillNotice(id, id,E_MO_ZHI_HUN, skill);
				engine->sendMessage(-1, MSG_SKILL, skill);
				used_E_MO_ZHI_HUN=true;
				//�Ƴ�һ������ħ֮�꡿
				card_id = respond->card_ids(0);
				engine->setStateMoveOneCardNotToHand(id, DECK_COVER,-1,DECK_DISCARD,card_id,id,E_MO_ZHI_HUN,false);
				//GameInfo game_info;
				//Coder::coverNotice(id,getCoverCards(), game_info);
				//engine->sendMessage(-1, MSG_GAME, game_info);
				return GE_URGENT;
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

int JianDi::EMoZhiHun_EffectMiss()
{
	network::SkillMsg skill;
	Coder::skillNotice(id, id,E_MO_ZHI_HUN, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
	//+2 ��������
	setToken(0,token[0]+2);
	network::GameInfo update_info;
	Coder::tokenNotice(id,0,token[0], update_info);
	engine->sendMessage(-1, MSG_GAME, update_info);
	return GE_SUCCESS;
}