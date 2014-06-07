#include "ChattingServer.h"

#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <functional>

#include "ServerSession.h"
#include "../Common/Protocol.h"

std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> codeCvt;

ChatServer::ChatServer(boost::asio::io_service& io_service)
	: m_acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), PORT_NUMBER))
{
	m_bIsAccepting = false;
}

ChatServer::~ChatServer()
{
	for (size_t i = 0; i < m_SessionList.size(); ++i)
	{
		if (m_SessionList[i]->Socket().is_open())
		{
			m_SessionList[i]->Socket().close();
		}

		delete m_SessionList[i];
	}
}

void ChatServer::Init(const int nMaxSessionCount)
{
	for (int i=0; i < nMaxSessionCount; ++i)
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
	m_SessionList[nSessionID]->Socket().close();
	m_SessionQueue.push_back(nSessionID);
	if (m_bIsAccepting == false)
	{
		PostAccept();
	}
}

void ChatServer::ProcessPacket(
	const int nSessionID, 
	const byte* pData)
{
	PACKET_HEADER* pheader = (PACKET_HEADER*)pData;

	switch (pheader->nID)
	{
		case REQ_IN:
		{
			PKT_REQ_IN* pPacket = (PKT_REQ_IN*)pData;
			m_SessionList[nSessionID]->SetName(pPacket->szName);

			std::cout << "클라이언트 로그인 성공 Name: " << m_SessionList[nSessionID]->GetName() << std::endl;

			PKT_RES_IN SendPkt;
			SendPkt.Init();
			SendPkt.bIsSuccess = true;

			m_SessionList[nSessionID]->PostSend(false, SendPkt.nSize, (byte*)&SendPkt);
		}
		break;

		case REQ_CHAT:
		{
			PKT_REQ_CHAT* pPacket = (PKT_REQ_CHAT*)pData;

			PKT_NOTICE_CHAT SendPkt;
			SendPkt.Init();
			strncpy_s(SendPkt.szName, MAX_NAME_LEN, m_SessionList[nSessionID]->GetName(), MAX_NAME_LEN - 1);
			strncpy_s(SendPkt.szMessage, MAX_MESSAGE_LEN, pPacket->szMessage, MAX_MESSAGE_LEN - 1);

			size_t nTotalSessionCount = m_SessionList.size();

			for (size_t i = 0; i < nTotalSessionCount; ++i)
			{
				if (m_SessionList[i]->Socket().is_open())
				{
					m_SessionList[i]->PostSend(false, SendPkt.nSize, (byte*)&SendPkt);
				}
			}
		}
		break;
	}

	return;
}

bool ChatServer::PostAccept()
{
	if (m_SessionQueue.empty())
	{
		m_bIsAccepting = false;
		return false;
	}

	m_bIsAccepting = true;
	int nSessionID = m_SessionQueue.front();
	m_SessionQueue.pop_front();
	m_acceptor.async_accept(
		m_SessionList[nSessionID]->Socket(),
		std::bind(&ChatServer::handle_accept,
			this,
			m_SessionList[nSessionID],
			boost::asio::placeholders::error));

	return true;
}

void ChatServer::handle_accept(
	Session* pSession, 
	const boost::system::error_code& error)
{
	if (!error)
	{
		std::cout << "클라이언트 접속 성공. SessionID: " << pSession->SessionID() << std::endl;
		
		pSession->Init();
		pSession->PostReceive();
		PostAccept();
	}
	else
	{
		std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
	}
}

