#ifndef __NETMESSAGE_H
#define __NETMESSAGE_H

#include "inetmessage.h"
#include "inetchannelinfo.h"

class CNetMessage : public INetMessage
{
public:
	virtual void SetNetChannel(INetChannel* pChan){m_pChan = pChan;}
	virtual void SetReliable(bool m_bR){m_bReliable=m_bR;}
	virtual bool Process(){return true;}
	virtual bool ReadFromBuffer(bf_read&){return true;}
	virtual bool WriteToBuffer(bf_write&){return true;}
	virtual bool IsReliable() const {return m_bReliable;}

	virtual int GetType() const {return 0;}
	virtual int GetGroup() const {return 0;}
	virtual const char* GetName() const {return "";}
	virtual INetChannel* GetNetChannel() const {return m_pChan;}
	virtual const char* ToString() const {return "";}

private:
	bool m_bReliable;
	INetChannel* m_pChan;
};

#endif