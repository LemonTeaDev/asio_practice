#pragma  once

#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <string>
#include <vector>
#include "ServerSession.h"
#include "Protocol.h"

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
		std::vector<byte>::const_iterator pData);

private:
	bool PostAccept();
	void handle_accept(
		Session* pSession, 
		const boost::system::error_code& error);

private:
	int m_nSeqNumber;
	bool m_bIsAccepting;
	boost::asio::ip::tcp::acceptor m_acceptor;

	std::vector<Session*> m_SessionList;
	std::deque<int> m_SessionQueue;
};