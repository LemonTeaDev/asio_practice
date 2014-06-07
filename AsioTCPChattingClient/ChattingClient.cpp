#include "ChattingClient.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>

ChatClient::ChatClient(boost::asio::io_service& io_service)
	: m_IOService(io_service), m_Socket(io_service)
{
	m_bIsLogin = false;
	InitializeCriticalSectionAndSpinCount(&m_lock, 4000);
}

ChatClient::~ChatClient()
{
	EnterCriticalSection(&m_lock);

	while (m_SendDataQueue.empty() == false)
	{
		m_SendDataQueue.pop_front();
	}

	LeaveCriticalSection(&m_lock);

	DeleteCriticalSection(&m_lock);
}

bool ChatClient::IsConnecting() const
{
	return m_Socket.is_open();
}

void ChatClient::PostLogin()
{
	m_bIsLogin = true;
}

bool ChatClient::IsLoggedIn() const
{
	return m_bIsLogin;
}

void ChatClient::Connect(boost::asio::ip::tcp::endpoint endpoint)
{
	m_nPacketBufferMark = 0;

	m_Socket.async_connect(endpoint,
		boost::bind(&ChatClient::handle_connect, this,
		boost::asio::placeholders::error));
}

void ChatClient::Close()
{
	if (m_Socket.is_open())
	{
		m_Socket.close();
	}
}

void ChatClient::PostSend(const bool bImmediately, const int nSize, PacketSPtr pData)
{
	PacketSPtr pSendData(nullptr);

	EnterCriticalSection(&m_lock);		// 락 시작

	if (bImmediately == false)
	{
		m_SendDataQueue.push_back(pData);
	}

	pSendData = pData;

	if (bImmediately || m_SendDataQueue.size() < 2)
	{
		boost::asio::async_write(m_Socket, boost::asio::buffer(pSendData.get(), nSize),
			boost::bind(&ChatClient::handle_write, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}

	LeaveCriticalSection(&m_lock);		// 락 완료
}

void ChatClient::PostReceive()
{
	std::fill(m_ReceiveBuffer.begin(), m_ReceiveBuffer.end(), 0);

	m_Socket.async_read_some(
		boost::asio::buffer(m_ReceiveBuffer),
		boost::bind(&ChatClient::handle_receive, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void ChatClient::handle_connect(const boost::system::error_code& error)
{
	if (!error)
	{
		std::cout << "서버 접속 성공" << std::endl;
		std::cout << "이름을 입력하세요!!" << std::endl;

		PostReceive();
	}
	else
	{
		std::cout << "서버 접속 실패. error No: " << error.value() << " error Message: " << error.message() << std::endl;
	}
}

void ChatClient::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
{
	PacketSPtr pData = nullptr;

	EnterCriticalSection(&m_lock);			// 락 시작

	m_SendDataQueue.pop_front();

	if (m_SendDataQueue.empty() == false)
	{
		pData = m_SendDataQueue.front();
	}

	LeaveCriticalSection(&m_lock);			// 락 완료


	if (pData != nullptr)
	{
		auto pHeader = reinterpret_cast<const PACKET_HEADER*>(pData.get());
		PostSend(true, pHeader->nSize, pData);
	}
}

void ChatClient::handle_receive(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (error)
	{
		if (error == boost::asio::error::eof)
		{
			std::cout << "클라이언트와 연결이 끊어졌습니다" << std::endl;
		}
		else
		{
			std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
		}

		Close();
	}
	else
	{
		std::copy_n(
			m_ReceiveBuffer.begin(),
			bytes_transferred,
			m_PacketBuffer.begin() + m_nPacketBufferMark);

		int nPacketData = m_nPacketBufferMark + bytes_transferred;
		int nReadData = 0;

		while (nPacketData > 0)
		{
			if (nPacketData < sizeof(PACKET_HEADER))
			{
				break;
			}

			PACKET_HEADER* pHeader = reinterpret_cast<PACKET_HEADER*>(
				&m_PacketBuffer[nReadData]);

			if (pHeader->nSize <= nPacketData)
			{
				ProcessPacket(m_PacketBuffer.begin() + nReadData);

				nPacketData -= pHeader->nSize;
				nReadData += pHeader->nSize;
			}
			else
			{
				break;
			}
		}

		if (nPacketData > 0)
		{
			std::array<byte, MAX_RECEIVE_BUFFER_LEN> tempBuffer;
			std::copy_n(m_PacketBuffer.begin() + nReadData, nPacketData, tempBuffer.begin());
			std::copy_n(tempBuffer.begin(), nPacketData, m_PacketBuffer.begin() + nReadData);
		}

		m_nPacketBufferMark = nPacketData;

		PostReceive();
	}
}

void ChatClient::ProcessPacket(std::vector<byte>::const_iterator itrData)
{
	auto pHeader = reinterpret_cast<const PACKET_HEADER*>(&*itrData);

	switch (pHeader->nID)
	{
		case RES_IN:
		{
			auto pPacket = reinterpret_cast<const PKT_RES_IN*>(&*itrData);
			PostLogin();
			std::cout << "클라이언트 로그인 성공 ?: " << pPacket->bIsSuccess << std::endl;
		}
		break;
	case NOTICE_CHAT:
		{
			auto pPacket = reinterpret_cast<const PKT_NOTICE_CHAT*>(&*itrData);
			std::cout << pPacket->name.c_str() << ": " << pPacket->message.c_str() << std::endl;
		}
		break;
	}
}