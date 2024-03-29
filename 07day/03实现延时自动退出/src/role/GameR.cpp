#include "../../inc/role/GameR.h"
#include "../../inc/protocol/GameMsgList.h"
#include "../../inc/protocol/GameMsgF.h"
#include "../../inc/role/TimeoutTaskR.h"
#include "../../inc/protocol/TimeoutP.h"

#include <iostream>
#include <random>
#include <iconv.h>

using namespace std;

static default_random_engine s_randEngne(time(NULL));
static AOI s_AOIworld;
static list<SyncPlyrIdData> s_pidList =
{
	{1, "海风"},
	{2, "华夏"},
	{3, "如戏"},
	{4, "凌子风"},
	{5, "糖宝"},
	{6, "凡凡"},
	{7, "TSY369"},
	{8, "崔琪贤"},
	{9, "VI8"},
	{10, "火麒麟"},
	{11, "晨曦"},
	{12, "铁链"}
	//{1, "haifeng"},
	//{2, "huaxia"},
	//{3, "ruxi"},
	//{4, "lingzifeng"},
	//{5, "didiowhx"},
	//{6, "fanfan"},
	//{7, "TSY369"},
	//{8, "cuiqixian"},
	//{9, "VI8"},
	//{10, "huoqiling"},
	//{11, "laowang"},
	//{12, "tieliang"}
};
static list<SyncPlyrIdData>::iterator s_pidListIt = s_pidList.begin();


class AutoExitR : public TimeoutTaskR
{
public:
	AutoExitR() : TimeoutTaskR("autoExit", 5) {
		cout << "AutoExitR()" << endl;
	}
	virtual ~AutoExitR() {}

	virtual UserData * ProcMsg(UserData & _poUserData) override
	{
		cout << "procMsg" << endl;
		ZinxKernel::Zinx_Exit();

		return nullptr;
	}
private:

};
static Irole* ps_timeout = NULL;


GameR::GameR()
{
	plyrPosData.X = 140 + s_randEngne() % 30;
	plyrPosData.Y = 0;
	plyrPosData.Z = 140 + s_randEngne() % 30;
	plyrPosData.V = s_randEngne() % 360;
	plyrPosData.bloodValue = 5;

	if (s_pidList.end() != s_pidListIt)
	{
		plyrData.plyrId = s_pidListIt->plyrId;
		plyrData.usrName = s_pidListIt->usrName;
		convG2U(plyrData.usrName);

		++s_pidListIt;
	}
}


GameR::~GameR()
{
}

const int & GameR::getPlyrId() const
{
	return plyrData.plyrId;
}

const string & GameR::getUsrName() const
{
	return plyrData.usrName;
}

const PlyrPosData & GameR::getPlyrPos() const
{
	return plyrPosData;
}

Iprotocol * const & GameR::getProtocol() const
{
	return p_gameP;
}

bool GameR::Init()
{
	s_AOIworld.addPlyr(*this);
	GameMsg* p_sendGm = nullptr;

	//向其发送ID和名称
	SyncPlyrIdData* p_spid = new SyncPlyrIdData;

	p_spid->plyrId = getPlyrId();
	p_spid->usrName = getUsrName();

	p_sendGm = new GameMsg(MSG_TYPE_LOGIN, p_spid);
	ZinxKernel::Zinx_SendOut(*p_sendGm, *p_gameP);

	list<AOIGrid*> gridList = s_AOIworld.getSrdPlyrs(*this);
	//向其发送周围玩家的位置
	SyncPlyrsData* p_spd = new SyncPlyrsData;
	for (AOIGrid * const& p_grid : gridList)
		for (AOIObj* const& p_plyr: *p_grid)
		{
			//if (((GameR*)p_plyr)->getPlyrId() == this->getPlyrId()) continue;
			PlyrData pd;

			pd.plyrId = ((GameR*)p_plyr)->getPlyrId();
			pd.usrName = ((GameR*)p_plyr)->getUsrName();		
			pd.plyrPos.X = ((GameR*)p_plyr)->getPlyrPos().X;
			pd.plyrPos.Y = ((GameR*)p_plyr)->getPlyrPos().Y;
			pd.plyrPos.Z = ((GameR*)p_plyr)->getPlyrPos().Z;
			pd.plyrPos.V = ((GameR*)p_plyr)->getPlyrPos().V;
			pd.plyrPos.bloodValue = ((GameR*)p_plyr)->getPlyrPos().bloodValue;
			p_spd->plyrs.push_back(pd);

			//向周围玩家广播自己位置
			BroadCastData* p_bcd = new BroadCastData;
			p_bcd->plyrId = getPlyrId();
			p_bcd->usrName = getUsrName();
			p_bcd->bcType = 2;
			p_bcd->data.plyrPos.X = getPlyrPos().X;
			p_bcd->data.plyrPos.Y = getPlyrPos().Y;
			p_bcd->data.plyrPos.Z = getPlyrPos().Z;
			p_bcd->data.plyrPos.V = getPlyrPos().V;
			p_bcd->data.plyrPos.bloodValue = getPlyrPos().bloodValue;
			p_sendGm = new GameMsg(MSG_TYPE_BROADCAST, p_bcd);
			ZinxKernel::Zinx_SendOut(*p_sendGm, *((GameR*)p_plyr)->getProtocol());
		}

	p_sendGm = new GameMsg(MSG_TYPE_SRDPLYRS_POS, p_spd);
	ZinxKernel::Zinx_SendOut(*p_sendGm, *p_gameP);
	
	if (0 == ZinxKernel::Zinx_GetAllRole().size())
	{
		if (nullptr != ps_timeout)
		{
			ZinxKernel::Zinx_Del_Role(*ps_timeout);
			delete ps_timeout;
			ps_timeout = NULL;
		}
	}
				
	return true;
}

UserData * GameR::ProcMsg(UserData & _poUserData)
{
	GET_REF2DATA(GameMsgList, gameMsgList, _poUserData);
	for (GameMsg* p_gm : gameMsgList)
	{
		GameMsgData* p_gmd = nullptr;
		GameMsg* p_sendGm = nullptr;
		switch (p_gm->getId())
		{
		case MSG_TYPE_LOGIN:
			{
				//if (nullptr == (p_gmd = p_gm->getMsgData()))
				//{
				//	cout << "nullptr == p_gm->getMsgData()" << endl;
				//	continue;
				//}
				//cout <<
				//	"login message :\n\t"
				//	"lenth: " << p_gm->getSize() << "\n\t" <<
				//	"ID: " << p_gm->getId() << "\n\t" <<
				//	"plyrId: " << ((SyncPlyrIdData*)p_gmd)->plyrId << "\n\t" <<
				//	"usrName: " << ((SyncPlyrIdData*)p_gmd)->usrName << endl;
				//SyncPlyrIdData* p_spid = new SyncPlyrIdData;
				//p_spid->plyrId = 30;
				//p_spid->usrName = "lulu";
				//p_sendGm = new GameMsg(MSG_TYPE_LOGIN, p_spid);
			}
			break;
		case MSG_TYPE_CHAT:
			{
				if (nullptr == (p_gmd = p_gm->getMsgData()))
				{
					cout << "nullptr == p_gm->getMsgData()" << endl;
					continue;
				}
				//cout <<
				//	"chat message :\n\t"
				//	"lenth: " << p_gm->getSize() << "\n\t" <<
				//	"ID: " << p_gm->getId() << "\n\t" <<
				//	"content: " << ((ChatData*)p_gmd)->content << endl;

				for (const AOIGrid& grid : s_AOIworld)
				{
					for (AOIObj* & plyr : const_cast<AOIGrid&>(grid))
					{
						BroadCastData* p_bcd = new BroadCastData;
						p_bcd->plyrId = getPlyrId();
						p_bcd->usrName = getUsrName();
						p_bcd->bcType = 1;
						p_bcd->data.plyrChat.content = 
							((ChatData*)p_gmd)->content;
						p_sendGm = new GameMsg(MSG_TYPE_BROADCAST, p_bcd);
						ZinxKernel::Zinx_SendOut(*p_sendGm, *((GameR*)plyr)->getProtocol());
					}
				}

			}
			break;
		case MSG_TYPE_NEW_POS:
			{
				if (nullptr == (p_gmd = p_gm->getMsgData()))
				{
					cout << "nullptr == p_gm->getMsgData()" << endl;
					continue;
				}
				//给旧的周围玩家发送下线消息
				list<AOIGrid*> oldGrid, unaltGrid, newGrid;

				s_AOIworld.upDateSrdPlyrs(*this, 
					((PlyrPosData*)p_gmd)->X, ((PlyrPosData*)p_gmd)->Z,
					oldGrid, unaltGrid, newGrid);

				plyrPosData.Y = ((PlyrPosData*)p_gmd)->Y;
				plyrPosData.V = ((PlyrPosData*)p_gmd)->V;
				plyrPosData.bloodValue = 
					((PlyrPosData*)p_gmd)->bloodValue;

				
				for (AOIGrid * const &p_grid : oldGrid)
				{
					for (AOIObj* const& p_plyr : *p_grid)
					{
						SyncPlyrIdData* p_spid = new SyncPlyrIdData;
						p_spid->plyrId = getPlyrId();
						p_spid->usrName = getUsrName();
						p_sendGm = new GameMsg(MSG_TYPE_LOGOUT, p_spid);
						ZinxKernel::Zinx_SendOut(*p_sendGm, *((GameR*)p_plyr)->getProtocol());
						p_spid = new SyncPlyrIdData;
						p_spid->plyrId = ((GameR*)p_plyr)->getPlyrId();
						p_spid->usrName = ((GameR*)p_plyr)->getUsrName();
						p_sendGm = new GameMsg(MSG_TYPE_LOGOUT, p_spid);
						ZinxKernel::Zinx_SendOut(*p_sendGm, *p_gameP);
					}
				}

				
				for (AOIGrid * const &p_grid : newGrid)
				{
					for (AOIObj* const& p_plyr : *p_grid)
					{
						BroadCastData* p_bcd = new BroadCastData;
						p_bcd->plyrId = getPlyrId();
						p_bcd->usrName = getUsrName();
						p_bcd->bcType = 2;
						p_bcd->data.plyrPos.X = getPlyrPos().X;
						p_bcd->data.plyrPos.Y = getPlyrPos().Y;
						p_bcd->data.plyrPos.Z = getPlyrPos().Z;
						p_bcd->data.plyrPos.V = getPlyrPos().V;
						p_bcd->data.plyrPos.bloodValue = getPlyrPos().bloodValue;

						p_sendGm = new GameMsg(MSG_TYPE_BROADCAST, p_bcd);
						ZinxKernel::Zinx_SendOut(*p_sendGm, *((GameR*)p_plyr)->getProtocol());
		
						p_bcd = new BroadCastData;
						p_bcd->plyrId = ((GameR*)p_plyr)->getPlyrId();
						p_bcd->usrName = ((GameR*)p_plyr)->getUsrName();
						p_bcd->bcType = 2;
						p_bcd->data.plyrPos.X = ((GameR*)p_plyr)->getPlyrPos().X;
						p_bcd->data.plyrPos.Y = ((GameR*)p_plyr)->getPlyrPos().Y;
						p_bcd->data.plyrPos.Z = ((GameR*)p_plyr)->getPlyrPos().Z;
						p_bcd->data.plyrPos.V = ((GameR*)p_plyr)->getPlyrPos().V;
						p_bcd->data.plyrPos.bloodValue = ((GameR*)p_plyr)->getPlyrPos().bloodValue;

						p_sendGm = new GameMsg(MSG_TYPE_BROADCAST, p_bcd);
						ZinxKernel::Zinx_SendOut(*p_sendGm, *p_gameP);
					}
				}

				for (AOIGrid * const &p_grid : unaltGrid)
				{
					for (AOIObj* const& p_plyr : *p_grid)
					{
						//cout << "unatlGrid:\n" <<
						//	((GameR*)p_plyr)->getPlyrId() << " : " <<
						//	((GameR*)p_plyr)->getUsrName() << endl;

						BroadCastData* p_bcd = new BroadCastData;
						p_bcd->plyrId = getPlyrId();
						p_bcd->usrName = getUsrName();
						p_bcd->bcType = 3;
						p_bcd->data.plyrPos.X = getPlyrPos().X;
						p_bcd->data.plyrPos.Y = getPlyrPos().Y;
						p_bcd->data.plyrPos.Z = getPlyrPos().Z;
						p_bcd->data.plyrPos.V = getPlyrPos().V;
						p_bcd->data.plyrPos.bloodValue = getPlyrPos().bloodValue;

						//cout << "move position: \n\tPos: (" <<
						//	p_bcd->data.plyrPos.X << ", " <<
						//	p_bcd->data.plyrPos.Y << ", " <<
						//	p_bcd->data.plyrPos.Z << ", " <<
						//	p_bcd->data.plyrPos.V << ")\n\t\t" <<
						//	"bloodValue: " << p_bcd->data.plyrPos.bloodValue << endl;
						p_sendGm = new GameMsg(MSG_TYPE_BROADCAST, p_bcd);
						ZinxKernel::Zinx_SendOut(*p_sendGm, *((GameR*)p_plyr)->getProtocol());
					}
				}
			}
			break;
		case MSG_TYPE_BROADCAST:
			{
		//		if (nullptr == (p_gmd = p_gm->getMsgData()))
		//		{
		//			cout << "nullptr == p_gm->getMsgData()" << endl;
		//			continue;
		//		}
		//		cout <<
		//			"broadcast message :\n\t"
		//			"lenth: " << p_gm->getSize() << "\n\t" <<
		//			"ID: " << p_gm->getId() << "\n\t" <<
		//			"plyrId: " << ((BroadCastData*)p_gmd)->plyrId << "\n\t" <<
		//			"bcType: " << ((BroadCastData*)p_gmd)->bcType << "\n\t" <<
		//			"usrName: " << ((BroadCastData*)p_gmd)->usrName << endl;
		//		BroadCastData* p_bcd = new BroadCastData;
		//		switch (((BroadCastData*)p_gmd)->bcType)
		//		{
		//		case 1:
		//			{
		//				cout << "\tchat content: \n\t\t" <<
		//					"content: " << ((BroadCastData*)p_gmd)->data.plyrChat.content << endl;
		//				p_bcd->plyrId = 300;
		//				p_bcd->usrName = "lulu";
		//				p_bcd->bcType = 1;
		//				p_bcd->data.plyrChat.content = "hello";
		//			}
	
		//			break;
		//		case 2:
		//			cout << "\tbirth position: \n\t\t" <<
		//				"Pos: (" <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.X << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.Y << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.Z << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.V << ")\n\t\t" <<
		//				"bloodValue: " << ((BroadCastData*)p_gmd)->data.plyrPos.bloodValue << endl;
		//			p_bcd->plyrId = 300;
		//			p_bcd->usrName = "lulu";
		//			p_bcd->bcType = 2;
		//			p_bcd->data.plyrPos.X = 1.0;
		//			p_bcd->data.plyrPos.Y = 1.0;
		//			p_bcd->data.plyrPos.Z = 1.0;
		//			p_bcd->data.plyrPos.V = 1.0;
		//			p_bcd->data.plyrPos.bloodValue = 5;
		//			break;
		//		case 3:	   
		//			cout << "\tmove position\n\t\t" <<
		//				"Pos: (" <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.X << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.Y << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.Z << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.V << ")\n\t\t" <<
		//				"bloodValue: " << 
		//				((BroadCastData*)p_gmd)->data.plyrPos.bloodValue << endl;
		//			p_bcd->plyrId = 300;
		//			p_bcd->usrName = "lulu";
		//			p_bcd->bcType = 3;
		//			p_bcd->data.plyrPos.X = 2.0;
		//			p_bcd->data.plyrPos.Y = 2.0;
		//			p_bcd->data.plyrPos.Z = 2.0;
		//			p_bcd->data.plyrPos.V = 2.0;
		//			p_bcd->data.plyrPos.bloodValue = 5;
		//			break;
		//		case 4:
		//			cout << "\taction data\n\t\t" <<
		//				"actionData: " << ((BroadCastData*)p_gmd)->data.actionData << endl;
		//			p_bcd->plyrId = 300;
		//			p_bcd->usrName = "lulu";
		//			p_bcd->bcType = 4;
		//			p_bcd->data.actionData = 20;
		//			break;
		//		default:
		//			break;
		//		}
		//		p_sendGm = new GameMsg(MSG_TYPE_BROADCAST, p_bcd);if (nullptr == (p_gmd = p_gm->getMsgData()))
		//		{
		//			cout << "nullptr == p_gm->getMsgData()" << endl;
		//			continue;
		//		}
		//		cout <<
		//			"broadcast message :\n\t"
		//			"lenth: " << p_gm->getSize() << "\n\t" <<
		//			"ID: " << p_gm->getId() << "\n\t" <<
		//			"plyrId: " << ((BroadCastData*)p_gmd)->plyrId << "\n\t" <<
		//			"bcType: " << ((BroadCastData*)p_gmd)->bcType << "\n\t" <<
		//			"usrName: " << ((BroadCastData*)p_gmd)->usrName << endl;
		//		BroadCastData* p_bcd = new BroadCastData;
		//		switch (((BroadCastData*)p_gmd)->bcType)
		//		{
		//		case 1:
		//			{
		//				cout << "\tchat content: \n\t\t" <<
		//					"content: " << ((BroadCastData*)p_gmd)->data.plyrChat.content << endl;
		//				p_bcd->plyrId = 300;
		//				p_bcd->usrName = "lulu";
		//				p_bcd->bcType = 1;
		//				p_bcd->data.plyrChat.content = "hello";
		//			}
	
		//			break;
		//		case 2:
		//			cout << "\tbirth position: \n\t\t" <<
		//				"Pos: (" <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.X << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.Y << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.Z << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.V << ")\n\t\t" <<
		//				"bloodValue: " << ((BroadCastData*)p_gmd)->data.plyrPos.bloodValue << endl;
		//			p_bcd->plyrId = 300;
		//			p_bcd->usrName = "lulu";
		//			p_bcd->bcType = 2;
		//			p_bcd->data.plyrPos.X = 1.0;
		//			p_bcd->data.plyrPos.Y = 1.0;
		//			p_bcd->data.plyrPos.Z = 1.0;
		//			p_bcd->data.plyrPos.V = 1.0;
		//			p_bcd->data.plyrPos.bloodValue = 5;
		//			break;
		//		case 3:	   
		//			cout << "\tmove position\n\t\t" <<
		//				"Pos: (" <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.X << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.Y << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.Z << ", " <<
		//				((BroadCastData*)p_gmd)->data.plyrPos.V << ")\n\t\t" <<
		//				"bloodValue: " << 
		//				((BroadCastData*)p_gmd)->data.plyrPos.bloodValue << endl;
		//			p_bcd->plyrId = 300;
		//			p_bcd->usrName = "lulu";
		//			p_bcd->bcType = 3;
		//			p_bcd->data.plyrPos.X = 2.0;
		//			p_bcd->data.plyrPos.Y = 2.0;
		//			p_bcd->data.plyrPos.Z = 2.0;
		//			p_bcd->data.plyrPos.V = 2.0;
		//			p_bcd->data.plyrPos.bloodValue = 5;
		//			break;
		//		case 4:
		//			cout << "\taction data\n\t\t" <<
		//				"actionData: " << ((BroadCastData*)p_gmd)->data.actionData << endl;
		//			p_bcd->plyrId = 300;
		//			p_bcd->usrName = "lulu";
		//			p_bcd->bcType = 4;
		//			p_bcd->data.actionData = 20;
		//			break;
		//		default:
		//			break;
		//		}
		//		p_sendGm = new GameMsg(MSG_TYPE_BROADCAST, p_bcd);
			}
			break;
		case MSG_TYPE_LOGOUT:
			{
				//if (nullptr == (p_gmd = p_gm->getMsgData()))
				//{
				//	cout << "nullptr == p_gm->getMsgData()" << endl;
				//	continue;
				//}
				//cout <<
				//	"loginout message :\n\t"
				//	"lenth: " << p_gm->getSize() << "\n\t" <<
				//	"ID: " << p_gm->getId() << "\n\t" <<
				//	"plyrId: " << ((SyncPlyrIdData*)p_gmd)->plyrId << "\n\t" <<
				//	"usrName: " << ((SyncPlyrIdData*)p_gmd)->usrName << endl;
				//SyncPlyrIdData* p_spid = new SyncPlyrIdData;
				//p_spid->plyrId = 30;
				//p_spid->usrName = "lulu";
				//p_sendGm = new GameMsg(MSG_TYPE_LOGOUT, p_spid);
			}
			break;
		case MSG_TYPE_SRDPLYRS_POS:
			{
				//if (nullptr == (p_gmd = p_gm->getMsgData()))
				//{
				//	cout << "nullptr == p_gm->getMsgData()" << endl;
				//	continue;
				//}
				//cout <<
				//	"suround plays message :\n\t"
				//	"lenth: " << p_gm->getSize() << "\n\t" <<
				//	"ID: " << p_gm->getId() << endl;
				//for (PlyrData& pd : ((SyncPlyrsData*)p_gmd)->plyrs)
				//{
				//	cout << "\tplyrId: " << pd.plyrId << ";  " <<
				//		"usrName: " << pd.usrName << "\n\t\t" <<
				//		"Pos: (" <<
				//		pd.plyrPos.X << ", " <<
				//		pd.plyrPos.Y << ", " <<
				//		pd.plyrPos.Z << ", " <<
				//		pd.plyrPos.V << ")\n\t\t" <<
				//		"bloodValue: " << pd.plyrPos.bloodValue << endl;
				//}
				//SyncPlyrsData* p_spd = new SyncPlyrsData;

				//for (int i = 0; i < 2; i++)
				//{
				//	PlyrData pd;
				//	pd.plyrId = 300;
				//	pd.plyrPos.X = 1.0;
				//	pd.plyrPos.Y = 1.0;
				//	pd.plyrPos.Z = 1.0;
				//	pd.plyrPos.V = 1.0;
				//	pd.plyrPos.bloodValue = 5;
				//	p_spd->plyrs.push_back(pd);
				//}

				//p_sendGm = new GameMsg(MSG_TYPE_SRDPLYRS_POS, p_spd);
			}
			break;
		default:
			break;
		}

		//ZinxKernel::Zinx_SendOut(*p_sendGm, *p_gameP);
		//cout << "Zinx_SendOut OK" << endl;
	}

	return nullptr;
}

void GameR::Fini()
{
	for (AOIGrid* const& p_grid : s_AOIworld.getSrdPlyrs(*this))
	{
		for (AOIObj* const &p_plyr : *p_grid)
		{
			SyncPlyrIdData* p_spid = new SyncPlyrIdData;
			p_spid->plyrId = getPlyrId();
			p_spid->usrName = getUsrName();
			GameMsg* p_sendGm = new GameMsg(MSG_TYPE_LOGOUT, p_spid);
			ZinxKernel::Zinx_SendOut(*p_sendGm, *((GameR*)p_plyr)->getProtocol());
		}
	}

	s_AOIworld.delPlyr(*this);

	if (0 == ZinxKernel::Zinx_GetAllRole().size())
	{
		ps_timeout = new AutoExitR;
		ZinxKernel::Zinx_Add_Role(*ps_timeout);
	}
}

float & GameR::getX() const
{
	return plyrPosData.X;
}

float & GameR::getY() const
{
	return plyrPosData.Z;
}

void GameR::setXY(const float & x, const float & y)
{
	plyrPosData.X = x;
	plyrPosData.Z = y;
}


char* const& GameR::convG2U(string & strBuf)
{
	codeConvert("gb2312", "utf-8", 
		strBuf.data(), strBuf.size(), convertBuf, sizeof(convertBuf));
	strBuf = convertBuf;
	return convertBuf;
}

char * const & GameR::convG2U(const char * const & c_str)
{
	codeConvert("gb2312", "utf-8",
		c_str, strlen(c_str), convertBuf, sizeof(convertBuf));
	return convertBuf;
}

int GameR::codeConvert(
	const char *const &from_charset,
	const char *const &to_charset,
	const char *const &inbuf,
	const size_t &inlen,
	char *const &outbuf,
	const size_t &outlen)
{
	iconv_t cd;
	char **pin = (char**)&inbuf;
	char **pout = (char**)&outbuf;

	cd = iconv_open(to_charset, from_charset);
	if (cd == 0)
		return -1;
	memset(outbuf, 0, outlen);
	if (iconv(cd, pin, (size_t*)&inlen, pout, (size_t*)&outlen) == -1)
		return -1;
	iconv_close(cd);
	(*pout)[outlen - 1] = '\0';

	return 0;
}

