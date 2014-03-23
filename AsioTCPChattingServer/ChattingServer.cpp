#include "ChattingServer.h"

#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <string>
#include <vector>

#include "ServerSession.h"
#include "Protocol.h"

ChatServer::ChatServer(boost::asio::io_service& io_service)
	: m_acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), PORT_NUMBER))
{
}

ChatServer::~ChatServer()
{
}

void ChatServer::Init(const int nMaxSessionCount)
{
	for (int i=0; i<nMaxSessionCount; ++i)
	{
		Session* pSession = new Session(i, m_acceptor.get_io_service(), this);
		m_SessionList.push_back(pSession);
		m_SessionQueue.push_back(i);
	}
}

void ChatServer::Start()
{
	std::cout << "서버 시작......" << std::endl;
	PostAccept();
}

void ChatServer::CloseSession(const int nSessionID)
{
	std::cout << "클라이언트 접속 종료: 세션 ID: " << nSessionID << std::endl;
	m_SessionQueue.push_back(nSessionID);
	if (m_bIsAccepting == false)
	{
		PostAccept();
	}
}

void ChatServer::ProcessPacket(
	const int nSessionID, 
	std::vector<byte>::const_iterator itrData)
{
	auto pHeader = reinterpret_cast<const PACKET_HEADER*>(&*itrData);

	switch (pHeader->nID)
	{
	case REQ_IN:
		{
			auto pPacket = reinterpret_cast<const PKT_REQ_IN*>(&*itrData);
			m_SessionList[nSessionID]->SetName(pPacket->name);

			std::cout 
				<< "클라이언트 로그인 성공 Name: "
				<< m_SessionList[nSessionID]->GetName().c_str()
				<< std::endl;

			PKT_RES_IN SendPkt;
			SendPkt.Init();
			SendPkt.bIsSuccess = true;
			
			std::vector<byte> sendPkt;
			std::copy_n(reinterpret_cast<byte*>(&sendPkt), sizeof(PKT_RES_IN), back_inserter(sendPkt));

			m_SessionList[nSessionID]->PostSend(
				false,
				SendPkt.nSize,
				sendPkt);
		}
		break;
	case REQ_CHAT:
		{
			auto pPacket = reinterpret_cast<const PKT_REQ_CHAT*>(&*itrData);
			PKT_NOTICE_CHAT SendPkt;
			SendPkt.Init();
			SendPkt.name = m_SessionList[nSessionID]->GetName();
			SendPkt.message = pPacket->message;

			size_t nTotalSessionCount = m_SessionList.size();
			for (size_t i=0; i<nTotalSessionCount; ++i)
			{
				if (m_SessionList[i]->Socket().is_open())
				{
					// TODO@jinuxc
					//m_SessionList[i]->PostSend(false, SendPkt.nSize, )
				}
			}
		}
		break;
	}
}

bool ChatServer::PostAccept()
{
	if (m_SessionQueue.empty())
	{
		m_bIsAccepting = true;
		int nSessionID = m_SessionQueue.front();
		m_SessionQueue.pop_front();
		//m_acceptor.async_accept(m_SessionList[nSessionID]->Socket(),
	}

	return true;
}

void ChatServer::handle_accept(
	Session* pSession, 
	const boost::system::error_code& error)
{
	if (!error)
	{
		std::cout << "클라이언트 접속 성공. SessionID: " << pSession->SessionID() << std::endl;
		
		// pSession->Init();
		//pSession->PostReceive();
		PostAccept();
	}
	else
	{
		std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
	}
}

