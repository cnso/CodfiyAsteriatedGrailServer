#include "GMCommand.h"
#include "map"

typedef void (*PTRFUN)(GameGrail*, PlayerEntity*, vector<string>&); 
map<string, PTRFUN> cmd_mapping;

/*
energy: ���ý�ɫ������������2������ʯ��ˮ��
*/
void setEnergy(GameGrail* engine, PlayerEntity* player, vector<string>& ss)
{
	player->setGem(atoi(ss[1].c_str()));
	player->setCrystal(stoi(ss[2].c_str()));

	GameInfo game_info;
	Coder::energyNotice(player->getID(), player->getGem(), player->getCrystal(), game_info);
	engine->sendMessage(-1, MSG_GAME, game_info);
}

/*
card: ����������ƣ���������������Ϊ��ӵ���id
*/
void addCard(GameGrail* engine, PlayerEntity* player, vector<string>& ss)
{
	int howmany;
	vector<int> cards;
	for (int i = 1; i < ss.size(); ++i)
		cards.push_back(atoi(ss[i].c_str()));
	howmany = ss.size() - 1;
	player->addHandCards(howmany, cards);

	GameInfo game_info;
	Coder::handNotice(player->getID(), player->getHandCards(), game_info);
	engine->sendMessage(-1, MSG_GAME, game_info);
}

/*
card: ����������Ӹ��ƣ���������������Ϊ��ӵ���id
*/
void addCoverCard(GameGrail* engine, PlayerEntity* player, vector<string>& ss)
{
	int howmany;
	vector<int> cards;
	for (int i = 1; i < ss.size(); ++i)
		cards.push_back(atoi(ss[i].c_str()));
	howmany = ss.size() - 1;
	player->addCoverCards(howmany, cards);  //

	GameInfo game_info;
	Coder::coverNotice(player->getID(), player->getCoverCards(), game_info);
	engine->sendMessage(-1, MSG_GAME, game_info);
}

/*
cross: ��������
*/
void setCross(GameGrail* engine, PlayerEntity* player, vector<string>& ss)
{
	int howmany = atoi(ss[1].c_str());
	player->addCrossNum(howmany-player->getCrossNum(), -2);   // 1000��������

	GameInfo game_info;
	Coder::crossNotice(player->getID(), player->getCrossNum(), game_info);
	engine->sendMessage(-1, MSG_GAME, game_info);
}

void setTeamArea(GameGrail* engine, PlayerEntity* player, vector<string>& ss)
{
	if(ss.size()<5) 
		return;
	int color = atoi(ss[1].c_str());
	int grail = atoi(ss[2].c_str());
	int gem = atoi(ss[3].c_str());
	int crystal = atoi(ss[4].c_str());
	TeamArea* m_teamArea = engine->getTeamArea();
	m_teamArea->setGem(color, gem);
	m_teamArea->setCrystal(color, crystal);
	m_teamArea->setCup(color, grail);

	GameInfo update_info;
	if (color == RED){
		update_info.set_red_gem(m_teamArea->getGem(color));
		update_info.set_red_crystal(m_teamArea->getCrystal(color));
		update_info.set_red_grail(m_teamArea->getCup(color));
	}
	else{
		update_info.set_blue_gem(m_teamArea->getGem(color));
		update_info.set_blue_crystal(m_teamArea->getCrystal(color));
		update_info.set_blue_grail(m_teamArea->getCup(color));
	}
	engine->sendMessage(-1, MSG_GAME, update_info);
}

/*
¼��gmָ���gmָ�����cmd_mapping�У�keyΪgmָ���ʽ���ַ�����value�Ǵ�������ָ��
*/
void initialize_gm_command()
{
	cmd_mapping["!`energy"] = setEnergy;            // ��������
	cmd_mapping["!`card"] = addCard;                // �������
	cmd_mapping["!`cross"] = setCross;              // �������
	cmd_mapping["!`covercard"] = addCoverCard;      // ��Ӹ���
	cmd_mapping["!`team"] = setTeamArea;            // �ı�ս����
}

/*
����ָ���ַ����и�
*/
void split(const string& src, const string& separator, vector<string>& dest)
{
    string str = src;
    string substring;
    string::size_type start = 0, index;

    do
    {
        index = str.find_first_of(separator,start);
        if (index != string::npos)
        {    
            substring = str.substr(start,index-start);
            dest.push_back(substring);
            start = str.find_first_not_of(separator,index);
            if (start == string::npos) return;
        }
    }while(index != string::npos);
    
    //the last token
    substring = str.substr(start);
    dest.push_back(substring);
}

void gm_cmd(GameGrail* engine, PlayerEntity* player, string cmd)
{
	vector<string> ss;
	split(cmd, " ", ss);
	// ����ָ����ö�Ӧ��gm������û�ж�Ӧָ���ֱ������
	if (cmd_mapping.find(ss[0]) == cmd_mapping.end())
	{
		return;
	}
	PTRFUN func = cmd_mapping[ss[0]];
	func(engine, player, ss);
}
