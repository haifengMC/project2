#pragma once



#include <zinx.h>
#include "../protocol/GameMsgData.h"
#include "../AOI.h"



class GameR :
	public Irole, public AOIObj
{
	Iprotocol* p_gameP = NULL;
	mutable SyncPlyrIdData plyrData;
	mutable PlyrPosData plyrPosData;
public:
	GameR();
	virtual ~GameR();
	
	void bindProtocol(Iprotocol * &&p_gameP) { this->p_gameP = p_gameP; }

	const int& getPlyrId() const;
	const string& getUsrName() const;
	const PlyrPosData& getPlyrPos() const;
	Iprotocol* const& getProtocol() const;

	// ͨ�� Irole �̳�
	virtual bool Init() override;
	virtual UserData * ProcMsg(UserData & _poUserData) override;
	virtual void Fini() override;

	// ͨ�� AOIObj �̳�
	virtual float & getX() const override;
	virtual float & getY() const override;
private:
};



