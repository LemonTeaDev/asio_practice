#pragma  once

#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <string>
#include <vector>
#include "ServerSession.h"
#include "../Common/Protocol.h"

#include "ServerCommon.h"

class ChatServer
{
public:
	ChatServer(boost::asio::io_service& io_service);
	~ChatServer();
	void Init(const int nMaxSessionCount);
	void Start();
	void CloseSession(const int nSessionID);
	void ProcessPacket(
		const int nSessionID, 
		byte* pData);

private:
	typedef std::shared_ptr<Session> shared_Session;
	bool PostAccept();
	void handle_accept(
		shared_Session pSession, 
		const boost::system::error_code& error);

private:
	int m_nSeqNumber;
	bool m_bIsAccepting;
	boost::asio::ip::tcp::acceptor m_acceptor;

	std::vector<shared_Session> m_SessionList;
	std::deque<int> m_SessionQueue;
};