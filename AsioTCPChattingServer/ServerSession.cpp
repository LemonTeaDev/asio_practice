#include "ServerSession.h"
#include "ChattingServer.h"
#include <iterator>
#include <locale>
#include <codecvt>

extern std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> codeCvt;

Session::Session(int nSessionID, boost::asio::io_service& io_service, ChatServer& server)
	: m_Socket(io_service)
	, m_nSessionID(nSessionID)
	, m_rServer(server)
{
}

Session::~Session()
{
	while (m_SendDataQueue.empty() == false)
	{
		m_SendDataQueue.pop_front();
	}
}

const int Session::SessionID() const
{
	return m_nSessionID;
}

boost::asio::ip::tcp::socket& Session::Socket()
{
	return m_Socket;
}

void Session::Init()
{
	m_nPacketBufferMark = 0;
}

void Session::PostReceive()
{
	m_Socket.async_read_some(
		boost::asio::buffer(m_ReceiveBuffer),
		std::bind(
			&Session::handle_receive, 
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void Session::PostSend(
	const bool bImmediately,
	const int nSize,
	shared_byte pData)
{
	shared_byte pSendData = nullptr;

	if (bImmediately == false)
	{
		m_SendDataQueue.push_back(pSendData);
	}

	pSendData = pData;

	if (bImmediately == false && m_SendDataQueue.size() > 1)
	{
		return;
	}

	boost::asio::async_write(m_Socket, boost::asio::buffer(pSendData.get(), nSize),
		std::bind(&Session::handle_write, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void Session::SetName(const char* _name)
{
	m_Name = _name;
}

const char* Session::GetName() const
{
	return m_Name.c_str();
}

void Session::handle_write(
	const boost::system::error_code& error,
	size_t bytes_transferred)
{
	m_SendDataQueue.pop_front();

	if (m_SendDataQueue.empty() == false)
	{
		shared_byte pData = m_SendDataQueue.front();
		auto pHeader = reinterpret_cast<PACKET_HEADER*>(pData.get());
		PostSend(true, pHeader->nSize, pData);
	}
}

void Session::handle_receive(
	const boost::system::error_code& error, 
	size_t bytes_transferred)
{
	if (error)
	{
		if (error == boost::asio::error::eof)
		{
			std::cout << "클라이언트와 연결이 끊어졌습니다" << std::endl;
		}
		else
		{
			std::cout << "error No: " << error.value() << " error Message: "
				<< error.message() << std::endl;
		}

		m_rServer.CloseSession(m_nSessionID);
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

			auto pHeader = reinterpret_cast<PACKET_HEADER*>(&m_PacketBuffer[nReadData]);

			if (pHeader->nSize <= nPacketData)
			{
				m_rServer.ProcessPacket(m_nSessionID, &m_PacketBuffer[nReadData]);

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
			std::array<byte, MAX_RECEIVE_BUFFER_LEN>  tempBuffer = {0, };
			
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