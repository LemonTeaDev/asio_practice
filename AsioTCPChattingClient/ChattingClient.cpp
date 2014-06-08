#include "ChattingClient.h"
#include <boost/thread.hpp>
#include <functional>

ChatClient::ChatClient(boost::asio::io_service& io_service)
	: m_IOService(io_service), m_Socket(io_service)
{
	m_bIsLogin = false;
}

ChatClient::~ChatClient()
{
	m_lock.lock();

	while (m_SendDataQueue.empty() == false)
	{
		m_SendDataQueue.pop_front();
	}

	m_lock.unlock();
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
		std::bind(&ChatClient::handle_connect, this,
		boost::asio::placeholders::error));
}

void ChatClient::Close()
{
	if (m_Socket.is_open())
	{
		m_Socket.close();
	}
}

void ChatClient::PostSend(const bool bImmediately, const int nSize, shared_byte pData)
{
	shared_byte pSendData = nullptr;

	m_lock.lock();		// 락 시작

	if (bImmediately == false)
	{
		m_SendDataQueue.push_back(pSendData);
	}

	pSendData = pData;

	if (bImmediately || m_SendDataQueue.size() < 2)
	{
		boost::asio::async_write(m_Socket, boost::asio::buffer(pSendData.get(), nSize),
			std::bind(&ChatClient::handle_write, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
			);
	}

	m_lock.unlock();	// 락 완료
}

void ChatClient::PostReceive()
{
	std::fill(m_ReceiveBuffer.begin(), m_ReceiveBuffer.end(), 0);

	m_Socket.async_read_some(
		boost::asio::buffer(m_ReceiveBuffer),
		std::bind(&ChatClient::handle_receive, this,
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
	shared_byte pData = nullptr;

	m_lock.lock();
	
	m_SendDataQueue.pop_front();
	if (m_SendDataQueue.empty() == false)
	{
		pData = m_SendDataQueue.front();
	}
	
	m_lock.unlock();

	if (pData != nullptr)
	{
		auto pHeader = reinterpret_cast<PACKET_HEADER*>(pData.get());
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
		if (static_cast<int>(m_PacketBuffer.size()) < bytes_transferred)
		{
			m_PacketBuffer.resize(bytes_transferred * 2);
		}
		std::copy_n(
			m_ReceiveBuffer.data(),
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

			PACKET_HEADER* pHeader = (PACKET_HEADER*)&m_PacketBuffer[nReadData];

			if (pHeader->nSize <= nPacketData)
			{
				ProcessPacket(&m_PacketBuffer[nReadData]);

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
			std::array<byte, MAX_RECEIVE_BUFFER_LEN>  tempBuffer = { 0, };

			std::copy_n(
				m_PacketBuffer.begin() + nReadData,
				nReadData + nPacketData,
				tempBuffer.begin());
			std::copy_n(
				tempBuffer.begin(),
				nPacketData,
				m_PacketBuffer.begin());
		}

		m_nPacketBufferMark = nPacketData;

		PostReceive();
	}
}

void ChatClient::ProcessPacket(byte* pData)
{
	auto pHeader = reinterpret_cast<PACKET_HEADER*>(pData);
	if (pHeader == nullptr) { return; }

	switch (pHeader->nID)
	{
		case RES_IN:
		{
			auto pPacket = reinterpret_cast<PKT_RES_IN*>(pData);
			PostLogin();
			std::cout << "클라이언트 로그인 성공 ?: " << pPacket->bIsSuccess << std::endl;
		}
		break;
	case NOTICE_CHAT:
		{
			auto pPacket = reinterpret_cast<PKT_NOTICE_CHAT*>(pData);
			std::cout << pPacket->szName << ": " << pPacket->szMessage << std::endl;
		}
		break;
	}
}
