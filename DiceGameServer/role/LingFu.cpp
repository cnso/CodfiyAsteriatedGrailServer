#include "LingFu.h"
#include "..\GameGrail.h"
#include "..\UserTask.h"

bool LingFu::cmdMsgParse(UserTask* session, uint16_t type, ::google::protobuf::Message *proto)
{
	switch(type)
	{
	case MSG_RESPOND:
		Respond* respond = (Respond*)proto;
		switch(respond->respond_id())
		{
		case LING_LI_BENG_JIE:
			//tryNotify��������Ϸ���̴߳���Ϣ��ֻ��id���ڵ�ǰ�ȴ�id������state���ڵ�ǰstate������step���ڵ�ǰstep����Ϸ���̲߳Ż����
			if(using_LeiMing){
				session->tryNotify(id, STATE_MAGIC_SKILL, LING_LI_BENG_JIE, respond);
			}
			else{
				session->tryNotify(id, STATE_COVER_CHANGE, LING_LI_BENG_JIE, respond);
			}
			return true;
		case BAI_GUI_YE_XING:
			session->tryNotify(id, STATE_COVER_CHANGE, STEP_INIT, respond);
			return true;
		}
	}
	//ûƥ���򷵻�false
	return false;
}

int LingFu::v_magic_skill(Action *action)
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
	case FENG_XING:
		cardID = action->card_ids(0);
		card = getCardByID(cardID);
		//�����Լ�������                          || ���Ƿ�ϵ               
		if(GE_SUCCESS != checkOneHandCard(cardID) || ELEMENT_WIND != card->getElement()){
			return GE_INVALID_CARDID;
		}
		if(2 != action->dst_ids_size()){
			return GE_INVALID_PLAYERID;
		}
		break;
	case LEI_MING:
		cardID = action->card_ids(0);
		card = getCardByID(cardID);
		//�����Լ�������                          || ������ϵ               
		if(GE_SUCCESS != checkOneHandCard(cardID) || ELEMENT_THUNDER != card->getElement()){
			return GE_INVALID_CARDID;
		}
		if(2 != action->dst_ids_size()){
			return GE_INVALID_PLAYERID;
		}
		break;

	default:
		return GE_INVALID_ACTION;
	}
	return GE_SUCCESS;
}

int LingFu::p_magic_skill(int &step, Action* action)
{
	//p_magic_skill��ͬ�ڱ�Ĵ����㣬����ֻ��һ��ƥ�䣬���ÿһ���������ʱ����ذ�step��ΪSTEP_DONE
	int ret = GE_INVALID_ACTION;
	switch(action->action_id())
	{
	case FENG_XING:
		ret = FengXing(action);
		if(GE_URGENT == ret){
			step = STEP_DONE;
		}
		break;
	case LEI_MING:
		if(step == STEP_INIT){
			ret = LeiMing_Cast(action);
			if(GE_URGENT == ret){
				step = LING_LI_BENG_JIE;
				using_LeiMing = true;
			}
		}
		else{
			if(step == LING_LI_BENG_JIE){
				ret = LingLiBengJie();
				if(toNextStep(ret)){
					step = LEI_MING;
				}
			}
			if(step == LEI_MING){
				ret = LeiMing_Effect(action);
				if(GE_URGENT == ret){
					step = STEP_DONE;
				}
			}
		}
		break;
	default:
		return GE_INVALID_ACTION;
	}
	return ret;
}

int LingFu::FengXing(Action *action)
{
	int cardID = action->card_ids(0);
	int dst1ID = action->dst_ids(0);
	int dst2ID = action->dst_ids(1);
	list<int> dstIDs;
	dstIDs.push_back(dst1ID);
	dstIDs.push_back(dst2ID);
	//���漼��
	network::SkillMsg skill;
	Coder::skillNotice(id, dstIDs, FENG_XING, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
    //�������Ʋ�����Ҫ��setStateMoveXXXX��ToHand�Ļ�Ҫ���HARM�����㲻���˺�
	
	HARM fengXing;
	fengXing.cause = FENG_XING;
	fengXing.point = 1;
	fengXing.srcID = id;
	fengXing.type = HARM_NONE;
	//�Ƚ���������������˳��ѹ�����Լ���ʼ������
	PlayerEntity* start = this->getPre();
	PlayerEntity* it = start;
	do{
		//û�����ƾͲ�����
		if(it->getHandCardNum() > 0 && (it->getID() == dst1ID || it->getID() == dst2ID)){
			engine->pushGameState(new StateRequestHand(it->getID(), fengXing, -1, DECK_DISCARD, false, false));
		}
		it = it->getPre();
	}while(it != start);
	NianZhou();
	CardMsg show_card;
	Coder::showCardNotice(id, 1, cardID, show_card);
	engine->sendMessage(-1, MSG_CARD, show_card);
	engine->setStateMoveOneCardNotToHand(id, DECK_HAND, -1, DECK_DISCARD, cardID, id, FENG_XING, true);
	//��������״̬����return GE_URGENT
	return GE_URGENT;
}

int LingFu::LeiMing_Cast(Action *action)
{
	int cardID = action->card_ids(0);
	int dst1ID = action->dst_ids(0);
	int dst2ID = action->dst_ids(1);
	list<int> dstIDs;
	dstIDs.push_back(dst1ID);
	dstIDs.push_back(dst2ID);
	//���漼��
	network::SkillMsg skill;
	Coder::skillNotice(id, dstIDs, LEI_MING, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
    
	NianZhou();
	CardMsg show_card;
	Coder::showCardNotice(id, 1, cardID, show_card);
	engine->sendMessage(-1, MSG_CARD, show_card);
	engine->setStateMoveOneCardNotToHand(id, DECK_HAND, -1, DECK_DISCARD, cardID, id, FENG_XING, true);
	//��������״̬����return GE_URGENT
	return GE_URGENT;
}

int LingFu::LeiMing_Effect(Action *action)
{
	int dst1ID = action->dst_ids(0);
	int dst2ID = action->dst_ids(1);
	//��д�˺��ṹ
	HARM leiMing;
	leiMing.cause = LEI_MING;
	leiMing.point = using_LingLiBengJie ? 2 : 1;
	leiMing.srcID = id;
	leiMing.type = HARM_MAGIC;
	//�Ƚ���������������˳��ѹ
	PlayerEntity* start = this->getPre();
	PlayerEntity* it = start;
	do{
		if(it->getID() == dst1ID || it->getID() == dst2ID){
			engine->setStateTimeline3(it->getID(), leiMing);
		}
		it = it->getPre();
	}while(it != start);
	
	//��������״̬����return GE_URGENT
	return GE_URGENT;
}

int LingFu::LingLiBengJie()
{
	int ret;
	using_LingLiBengJie = false;
	if(getEnergy() < 1){
		return GE_SUCCESS;
	}
	//���㷢��������ѯ�ʿͻ����Ƿ񷢶�
	CommandRequest cmd_req;
	Coder::askForSkill(id, LING_LI_BENG_JIE, cmd_req);
	//���޵ȴ�����UserTask����tryNotify����
	if(engine->waitForOne(id, network::MSG_CMD_REQ, cmd_req))
	{
		void* reply;
		if (GE_SUCCESS == (ret = engine->getReply(id, reply)))
		{
			Respond* respond = (Respond*) reply;
			//����
			if(respond->args(0) == 1){
				network::SkillMsg skill;
				Coder::skillNotice(id, -1, LING_LI_BENG_JIE, skill);
				engine->sendMessage(-1, MSG_SKILL, skill);
				GameInfo update_info;
				if(crystal>0){
					setCrystal(--crystal);
				}
				else{
					setGem(--gem);
				}
				Coder::energyNotice(id, gem, crystal, update_info);
				engine->sendMessage(-1, MSG_GAME, update_info);
				using_LingLiBengJie = true;
			}
		}
		return ret;
	}
	else{
		//��ʱɶ��������
		return GE_TIMEOUT;
	}
}

int LingFu::NianZhou()
{
	//��������
	if(coverCards.size() > 1){
		return GE_SUCCESS;
	}
	HARM nianZhou;
	nianZhou.cause = NIAN_ZHOU;
	nianZhou.point = 1;
	nianZhou.srcID = id;
	nianZhou.type = HARM_NONE;
	engine->pushGameState(new StateRequestHand(id, nianZhou, id, DECK_COVER, false, true));
	return GE_URGENT;
}

int LingFu::p_timeline_2_hit(int &step, CONTEXT_TIMELINE_2_HIT *con)
{
	int ret;
	if(con->attack.srcID != id || !con->attack.isActive){
		return GE_SUCCESS;
	}
	ret = BaiGuiYeXing_Cast();
	if(toNextStep(ret) || GE_URGENT == ret){
		step = STEP_DONE;
	}
	return ret;
}

int LingFu::BaiGuiYeXing_Cast()
{
	if(coverCards.size() < 1){
		return GE_SUCCESS;
	}
	HARM yaoli;
	yaoli.cause = BAI_GUI_YE_XING;
	yaoli.point = 1;
	yaoli.srcID = id;
	yaoli.type = HARM_NONE;
	engine->pushGameState(new StateRequestCover(id, yaoli, -1, DECK_DISCARD, false, true));
	return GE_URGENT;
}

int LingFu::p_cover_change(int &step, int dstID, int direction, int howMany, vector<int> cards, int doerID, int cause)
{
	int ret = GE_INVALID_STEP;
	if(doerID != id || direction != CHANGE_REMOVE){
		return GE_SUCCESS;
	}
	if(step == STEP_INIT){
		ret = BaiGuiYeXing_Expose(cards[0]);
		if(toNextStep(ret)){
			step = LING_LI_BENG_JIE;
			using_LeiMing = false;
		}
	}
	if(step == LING_LI_BENG_JIE){
		ret = LingLiBengJie();
		if(toNextStep(ret)){
			step = BAI_GUI_YE_XING;
		}
	}
	if(step == BAI_GUI_YE_XING){
		ret = BaiGuiYeXing_Effect();
		if(ret == GE_URGENT){
			step = STEP_DONE;
		}			
	}
	return ret;

}

int LingFu::BaiGuiYeXing_Expose(int cardID)
{
	int ret;
	CommandRequest cmd_req;
	Coder::askForSkill(id, BAI_GUI_YE_XING, cmd_req);
	bool isFire = getCardByID(cardID)->getElement() == ELEMENT_FIRE;
	Command *cmd = (Command*)(&cmd_req.commands(cmd_req.commands_size()-1));
	cmd->add_args(isFire? 1 : 0);
	//���޵ȴ�����UserTask����tryNotify����
	if(engine->waitForOne(id, network::MSG_CMD_REQ, cmd_req))
	{
		void* reply;
		if (GE_SUCCESS == (ret = engine->getReply(id, reply)))
		{
			Respond* respond = (Respond*) reply;
			ret = GE_INVALID_ACTION;
			//չʾ
			if(respond->args(0) == 1){
				if(!isFire){
					return GE_INVALID_CARDID;
				}
				if(respond->dst_ids_size() != 2){
					return GE_INVALID_PLAYERID;
				}
				isExposed = true;
				dstIDs[0] = respond->dst_ids(0);
				dstIDs[1] = respond->dst_ids(1);
				list<int> dsts;
				dsts.push_back(dstIDs[0]);
				dsts.push_back(dstIDs[1]);
				network::SkillMsg skill;
				Coder::skillNotice(id, dsts, BAI_GUI_YE_XING, skill);
				engine->sendMessage(-1, MSG_SKILL, skill);
				CardMsg show_card;
				Coder::showCardNotice(id, 1, cardID, show_card);
				engine->sendMessage(-1, MSG_CARD, show_card);
				return GE_SUCCESS;				
			}
			else if(respond->args(0) == 0){
				if(respond->dst_ids_size() != 1){
					return GE_INVALID_PLAYERID;
				}
				isExposed = false;
				dstIDs[0] = respond->dst_ids(0);
				network::SkillMsg skill;
				Coder::skillNotice(id, dstIDs[0], BAI_GUI_YE_XING, skill);
				engine->sendMessage(-1, MSG_SKILL, skill);
				return GE_SUCCESS;
			}
		}
		return ret;
	}
	else{
		isExposed = false;
		dstIDs[0] = 0;
		network::SkillMsg skill;
		Coder::skillNotice(id, dstIDs[0], BAI_GUI_YE_XING, skill);
		engine->sendMessage(-1, MSG_SKILL, skill);
		return GE_TIMEOUT;
	}
}

int LingFu::BaiGuiYeXing_Effect()
{
	HARM baiGui;
	baiGui.cause = BAI_GUI_YE_XING;
	baiGui.point = using_LingLiBengJie ? 2 : 1;
	baiGui.srcID = id;
	baiGui.type = HARM_MAGIC;
	if(isExposed){
		PlayerEntity* it = this->getPre();		
		//�Ƚ���������������˳��ѹ
		do{
			if(it->getID() != dstIDs[0] && it->getID() != dstIDs[1]){
				engine->setStateTimeline3(it->getID(), baiGui);
			}
			it = it->getPre();
		}while(it != this->getPre());
	}
	else{
		engine->setStateTimeline3(dstIDs[0], baiGui);
	}
	return GE_URGENT;
}