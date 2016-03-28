#include "MaoXian.h"
#include "..\GameGrail.h"

//ͳһ��p_before_turn_begin ��ʼ�����ֻغϱ���
int MaoXian::p_before_turn_begin(int &step, int currentPlayerID) 
{
	used_TouTianHuanRi = false;
	return GE_SUCCESS; 
}

int MaoXian::v_buy(Action *action)
{
	if(handCards.size()+3 > handCardsMax){
		return GE_INVALID_ACTION;
	}
	int gem = action->args(0);
	int crystal = action->args(1);
	if((gem==1 || gem==2) && crystal==0){
		return GE_SUCCESS;
	}
	return GE_INVALID_ACTION;
}

//�������v_attack�������û����Ǳ�С�����֮��ļ��
int MaoXian::v_attack_skill(Action *action)
{
	int actionID = action->action_id();
	int playerID = action->src_id();
	int cardNum = action->card_ids_size();
	int ret;

	if(playerID != id){
		return GE_INVALID_PLAYERID;
	}
	if(actionID != QI_ZHA){
		return GE_INVALID_ACTION;
	}
	if(cardNum != 2 && cardNum != 3){
		return GE_INVALID_CARDID;
	}
	vector<int> cardIDs(cardNum);
	for(int i = 0; i < cardNum; i++){
		cardIDs[i] = action->card_ids(i);
	}
	if(GE_SUCCESS != (ret = checkHandCards(cardNum, cardIDs))){
		return ret;
	}
	if(GE_SUCCESS != (ret = v_attack(action->args(0), action->dst_ids(0), false))){
		return ret;
	}
	return GE_SUCCESS;
}

int MaoXian::p_attack_skill(int &step, Action* action)
{
	if(action->action_id() != QI_ZHA){
		return GE_INVALID_ACTION;
	}
	int ret = QiZha(step, action);
	if(ret == GE_URGENT){
		step = STEP_DONE;
	}
	return ret;
}

int MaoXian::QiZha(int step, Action *action)
{
	int actionID = action->action_id();
	int cardNum = action->card_ids_size();
	int virtualCardID = action->args(0);
	int dstID = action->dst_ids(0);

	//���漼��
	network::SkillMsg skill;
	Coder::skillNotice(id, dstID, actionID, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
	vector<int> cardIDs(cardNum);
	for(int i = 0; i < cardNum; i++){
		cardIDs[i] = action->card_ids(i);
	}
	//��������
	GameInfo update_info;
	setCrystal(++crystal);
	Coder::energyNotice(id, gem, crystal, update_info);
	engine->sendMessage(-1, MSG_GAME, update_info);
	engine->setStateTimeline1(virtualCardID, dstID, id, true);
	engine->setStateUseCard(virtualCardID, dstID, id, false, false);
	//�������Ʋ�����Ҫ��setStateMoveXXXX��ToHand�Ļ�Ҫ���HARM�����㲻���˺�
	CardMsg show_card;
	Coder::showCardNotice(id, cardNum, cardIDs, show_card);
	engine->sendMessage(-1, MSG_CARD, show_card);
	engine->setStateMoveCardsNotToHand(id, DECK_HAND, -1, DECK_DISCARD, cardNum, cardIDs, id, actionID, true);
	//��������״̬����return GE_URGENT
	return GE_URGENT;
}

//��Ӧ������٣�͵�컻�ո�����ӹ�����һ��flag
int MaoXian::v_magic_skill(Action *action)
{
	int actionID = action->action_id();
	int playerID = action->src_id();
	if(playerID != id){
		return GE_INVALID_PLAYERID;
	}
	if(used_TouTianHuanRi){
		return GE_INVALID_ACTION;
	}
	switch(actionID)
	{
	case TOU_TIAN_HUAN_RI:
		//����              || ����û��ʯ
		if(getEnergy() <= 0 || engine->getTeamArea()->getGem(1-color) < 1){
			return GE_INVALID_ACTION;
		}
		break;
	case TE_SHU_JIA_GONG:
		//����              
		if(getEnergy() <= 0){
			return GE_INVALID_ACTION;
		}
		break;
	default:
		return GE_INVALID_ACTION;
	}
	return GE_SUCCESS;
}

int MaoXian::p_magic_skill(int &step, Action* action)
{
	int ret = GE_INVALID_ACTION;
	switch(action->action_id())
	{
	case TOU_TIAN_HUAN_RI:
		ret = TouTianHuanRi(action);
		break;
	case TE_SHU_JIA_GONG:
		ret = TeShuJiaGong(action);
		break;
	}
	if (GE_SUCCESS == ret){
		step = STEP_DONE;
	}
	return ret;
}

int MaoXian::TouTianHuanRi(Action *action)
{
	//���漼��
	network::SkillMsg skill;
	Coder::skillNotice(id, -1, TOU_TIAN_HUAN_RI, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
	//��������
	GameInfo update_info;
	if(crystal>0){
		setCrystal(--crystal);
	}
	else{
		setGem(--gem);
	}
	Coder::energyNotice(id, gem, crystal, update_info);
	TeamArea* team = engine->getTeamArea();
	team->setGem(1-color, team->getGem(1-color)-1);
	team->setGem(color, team->getGem(color)+1);
	Coder::stoneNotice(1-color, team->getGem(1-color), team->getCrystal(1-color), update_info);
	Coder::stoneNotice(color, team->getGem(color), team->getCrystal(color), update_info);
	engine->sendMessage(-1, MSG_GAME, update_info);
	addAction(ACTION_ATTACK_MAGIC, TOU_TIAN_HUAN_RI);
	used_TouTianHuanRi = true;
	return GE_SUCCESS;
}

int MaoXian::TeShuJiaGong(Action *action)
{
	//���漼��
	network::SkillMsg skill;
	Coder::skillNotice(id, -1, TE_SHU_JIA_GONG, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
	//��������
	GameInfo update_info;
	if(crystal>0){
		setCrystal(--crystal);
	}
	else{
		setGem(--gem);
	}
	Coder::energyNotice(id, gem, crystal, update_info);
	TeamArea* team = engine->getTeamArea();
	int crystal_t = team->getCrystal(color);
	team->setCrystal(color, 0);
	team->setGem(color, team->getGem(color) + crystal_t);
	
	Coder::stoneNotice(color, team->getGem(color), team->getCrystal(color), update_info);
	engine->sendMessage(-1, MSG_GAME, update_info);
	addAction(ACTION_ATTACK_MAGIC, TE_SHU_JIA_GONG);
	used_TouTianHuanRi = true;
	return GE_SUCCESS;
}

int MaoXian::v_special_skill(Action *action)
{
	int actionID = action->action_id();
	int playerID = action->src_id();
	int gem = action->args(0);
	int crystal = action->args(1);
	PlayerEntity* dst = engine->getPlayerEntity(action->dst_ids(0));
	TeamArea* team = engine->getTeamArea();

	if(playerID != id || dst->getColor() != color){
		return GE_INVALID_PLAYERID;
	}
	if(actionID != MAO_XIAN_ZHE_TIAN_TANG || gem > team->getGem(color) || crystal > team->getCrystal(color) || gem + crystal + dst->getEnergy() > dst->getEnergyMax()){
		return GE_INVALID_ACTION;
	}
	return GE_SUCCESS;
}

int MaoXian::p_special_skill(int &step, Action* action)
{
	return MaoXianZheTianTang(action);
}

int MaoXian::MaoXianZheTianTang(Action *action)
{
	int dstID = action->dst_ids(0);
	int gem = action->args(0);
	int crystal = action->args(1);
	//���漼��
	network::SkillMsg skill;
	Coder::skillNotice(id, dstID, MAO_XIAN_ZHE_TIAN_TANG, skill);
	engine->sendMessage(-1, MSG_SKILL, skill);
	//��������	
	PlayerEntity *dst = engine->getPlayerEntity(dstID);
	GameInfo update_info;
	if(crystal>0){
		dst->setCrystal(dst->getCrystal() + crystal);
	}
	if(gem>0){
		dst->setGem(dst->getGem() + gem);
	}
	Coder::energyNotice(dstID, dst->getGem(), dst->getCrystal(), update_info);
	TeamArea* team = engine->getTeamArea();
	team->setGem(color, team->getGem(color) - gem);
	team->setCrystal(color, team->getCrystal(color) - crystal);
	Coder::stoneNotice(color, team->getGem(color), team->getCrystal(color), update_info);
	engine->sendMessage(-1, MSG_GAME, update_info);
	return GE_SUCCESS;
}