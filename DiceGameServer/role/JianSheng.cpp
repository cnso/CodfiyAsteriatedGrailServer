#include "JianSheng.h"
#include "..\GameGrail.h"
#include "..\UserTask.h"

bool JianSheng::cmdMsgParse(UserTask *session, uint16_t type, ::google::protobuf::Message *proto)
{
	switch(type)
	{
	case MSG_RESPOND:
		Respond* respond = (Respond*)proto;
		switch(respond->respond_id())
		{
		case LIE_FENG_JI:
			//tryNotify��������Ϸ���̴߳���Ϣ��ֻ��id���ڵ�ǰ�ȴ�id������state���ڵ�ǰstate������step���ڵ�ǰstep����Ϸ���̲߳Ż����
			session->tryNotify(id, STATE_TIMELINE_1, LIE_FENG_JI, respond);
			return true;
		}
	}
	//ûƥ���򷵻�false
	return false;
}

//ͳһ��p_before_turn_begin ��ʼ�����ֻغϱ���
int JianSheng::p_before_turn_begin(int &step, int currentPlayerID) 
{
	used_LianXuJi = false;
	used_JianYing = false;
	using_LianXuJi = false;
	attackCount = 0;
	return GE_SUCCESS; 
}

//�ڳ����ʱ��p_xxxx�п���ִ�в�ֹһ�Σ���ÿ�ζ���ͷ�����Ļ�������������Ҫstep��¼ִ�е�����
int JianSheng::p_timeline_1(int &step, CONTEXT_TIMELINE_1 *con)
{
	int ret = GE_INVALID_STEP;
	//��ʥ�ļ��ܶ���Ҫ��Ϊ������
	if(con->attack.srcID != id){
		return GE_SUCCESS;
	}
	//���ɹ�����������ߣ�ʧ���򷵻أ�step�ᱣ�����´��ٽ����Ͳ�������
	//һ�㳬ʱҲ�������һ��
	if(step == STEP_INIT){
		step = LIE_FENG_JI;
	}
	if(step == LIE_FENG_JI){
		ret = LieFengJi(con);
		if(toNextStep(ret)){
			step = JI_FENG_JI;
		}			
	}
	if(step == JI_FENG_JI){
		ret = JiFengJi(con);
		if(toNextStep(ret)){
			step = SHENG_JIAN;
		}			
	}
	if(step == SHENG_JIAN){
		ret = ShengJian(con);
		if(toNextStep(ret)){
			//ȫ����������step���STEP_DONE
			step = STEP_DONE;
		}			
	}

	return ret;
}

//���ж����ж������Ǽ��е�һ���ط�ѯ�ʣ�������ÿ������һ��
int JianSheng::p_after_attack(int &step, int playerID)
{
	int ret = GE_INVALID_STEP;
	//���ǽ�ʥ�Ͳ�������
	if(playerID != id){
		return GE_SUCCESS;
	}
	//���ɹ�����������ߣ�ʧ���򷵻أ�step�ᱣ�����´��ٽ����Ͳ�������
	//һ�㳬ʱҲ�������һ��
	if(step == STEP_INIT){
		ret = LianXuJi(playerID);
		if(toNextStep(ret)){
			step = JIAN_YING;
		}			
	}
	if(step == JIAN_YING){
		ret = JianYing(playerID);
		if(toNextStep(ret)){
			//ȫ����������step���STEP_DONE
			step = STEP_DONE;
		}
	}
	return ret;
}

int JianSheng::v_additional_action(int chosen)
{
	switch(chosen)
	{
	case LIAN_XU_JI:
		//�غ��޶�
		if(used_LianXuJi){
			return GE_INVALID_ACTION;
		}
		break;
	case JIAN_YING:
		//�غ��޶�       || ����
		if(used_JianYing || getEnergy() <= 0){
			return GE_INVALID_ACTION;
		}
		break;
	}
	//ͨ����ɫ��صļ�⣬������⽻���ײ�
	return PlayerEntity::v_additional_action(chosen);
}

int JianSheng::p_additional_action(int chosen)
{
	GameInfo update_info;
	switch(chosen)
	{
	case LIAN_XU_JI:
		used_LianXuJi = true;
		using_LianXuJi = true;
		break;
	case JIAN_YING:
		used_JianYing = true;
		using_LianXuJi = false;
		if(crystal>0){
			setCrystal(--crystal);
		}
		else{
			setGem(--gem);
		}
		Coder::energyNotice(id, gem, crystal, update_info);
		engine->sendMessage(-1, MSG_GAME, update_info);
		break;
	default:
		using_LianXuJi = false;
	}
	//�����ɫ��صĲ������۳������ж������ײ�
	return PlayerEntity::p_additional_action(chosen);
}

int JianSheng::v_attack(int cardID, int dstID, bool realCard)
{
	if(using_LianXuJi){
		CardEntity* card = getCardByID(cardID);
		if(card->getElement() != ELEMENT_WIND){
			return GE_INVALID_ACTION;
		}
	}
	//ͨ����ɫ��صļ�⣬����������⽻���ײ�
	return PlayerEntity::v_attack(cardID, dstID, realCard);
}

int JianSheng::LieFengJi(CONTEXT_TIMELINE_1 *con)
{
	int ret;
	int srcID = con->attack.srcID;
	int dstID = con->attack.dstID;
	int cardID = con->attack.cardID;
	CardEntity* card = getCardByID(cardID);
	//�ǲ��ǽ�ʥ   || �ǲ����ҷ缼
	if(srcID != id || !card->checkSpeciality(LIE_FENG_JI)){
		return GE_SUCCESS;
	}
	PlayerEntity* dst = engine->getPlayerEntity(dstID);
	//Ŀ����û��
	if(GE_SUCCESS != dst->checkBasicEffectByName(NAME_SHIELD) && GE_SUCCESS != dst->checkBasicEffectByName(TIAN_SHI_ZHI_QIANG)){
		return GE_SUCCESS;
	}
	//���㷢��������ѯ�ʿͻ����Ƿ񷢶�
	CommandRequest cmd_req;
	Coder::askForSkill(id, LIE_FENG_JI, cmd_req);
	//���޵ȴ�����UserTask����tryNotify����
	if(engine->waitForOne(id, network::MSG_CMD_REQ, cmd_req))
	{
		void* reply;
		if (GE_SUCCESS == (ret = engine->getReply(srcID, reply)))
		{
			Respond* respond = (Respond*) reply;
			//����
			if(respond->args(0) == 1){
				network::SkillMsg skill;
				Coder::skillNotice(id, dstID, LIE_FENG_JI, skill);
				engine->sendMessage(-1, MSG_SKILL, skill);
				con->hitRate = RATE_NOREATTACK;
				con->checkShield = false;
			}
		}
		return ret;
	}
	else{
		//��ʱɶ��������
		return GE_TIMEOUT;
	}
}

int JianSheng::JiFengJi(CONTEXT_TIMELINE_1 *con)
{
	int srcID = con->attack.srcID;
	int cardID = con->attack.cardID;
	CardEntity* card = getCardByID(cardID);
	//�ǲ��ǽ�ʥ   || �ǲ��Ǽ��缼                       || �ǲ�����������
	if(srcID != id || !card->checkSpeciality(JI_FENG_JI) || !con->attack.isActive){
		return GE_SUCCESS;
	}
	SkillMsg skill;
	Coder::skillNotice(id, con->attack.dstID, JI_FENG_JI, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
	addAction(ACTION_ATTACK, JI_FENG_JI);
	return GE_SUCCESS;
}

int JianSheng::ShengJian(CONTEXT_TIMELINE_1 *con)
{
	int srcID = con->attack.srcID;
	//�ǲ��ǽ�ʥ   || �ǲ�����������
	if(srcID != id || !con->attack.isActive){
		return GE_SUCCESS;
	}
	attackCount++;
	if(3 == attackCount){
		SkillMsg skill;
		Coder::skillNotice(id, con->attack.dstID, SHENG_JIAN, skill);
		engine->sendMessage(-1, MSG_SKILL, skill);
		con->hitRate = RATE_NOMISS;
	}	
	return GE_SUCCESS;
}

//�Թ����ӽǣ�ֻҪ���ƾ��п��ܷ������ȼ��϶����ж�
int JianSheng::LianXuJi(int playerID)
{
	//�ǲ��ǽ�ʥ      || ��û������        || �غ��޶�      || �Ѿ�������������
	if(playerID != id || handCards.empty() || used_LianXuJi || containsAction(LIAN_XU_JI)){
		return GE_SUCCESS;
	}
	addAction(ACTION_ATTACK, LIAN_XU_JI);
	return GE_SUCCESS;
}

int JianSheng::JianYing(int playerID)
{
	//�ǲ��ǽ�ʥ      || ��û������        || ��û������       || �غ��޶�      || �Ѿ����Ͻ�Ӱ��
	if(playerID != id || handCards.empty() || getEnergy() <= 0 || used_JianYing || containsAction(JIAN_YING)){
		return GE_SUCCESS;
	}
	addAction(ACTION_ATTACK, JIAN_YING);
	return GE_SUCCESS;
}