#include "ServerSession.h"
#include "ChattingServer.h"
#include <iterator>

const auto error = std::placeholders::_1;
const auto bytes_transferred = std::placeholders::_2;

Session::Session(int nSessionID, boost::asio::io_service& io_service, ChatServer* pServer)
	: m_Socket(io_service)
	, m_nSessionID(nSessionID)
	, m_pServer(pServer)
{
	m_bCompletedWrite = true;
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
			error,
			bytes_transferred));
}

void Session::PostSend(
	const bool bImmediately,
	const int nSize,
	const std::vector<byte>& pData)
{
	std::vector<byte> pSendData;

	if (bImmediately == false)
	{
		std::copy_n(pData.begin(), nSize, std::back_inserter(pSendData));
		m_SendDataQueue.push_back(pSendData);
	}

	if (m_bCompletedWrite == false)
	{
		return;
	}

	boost::asio::async_write(
		m_Socket,
		boost::asio::buffer(pSendData, nSize),
		std::bind(&Session::handle_write, this,
			error,
			bytes_transferred));
}

void Session::SetName(const std::u32string& name)
{
	m_Name = name;
}

void Session::SetName(const char32_t* pszName)
{
	m_Name = pszName;
}

const std::u32string& Session::GetName() const
{
	return m_Name;
}

void Session::handle_write(
	const boost::system::error_code& error,
	size_t bytes_transferred)
{
	m_SendDataQueue.pop_front();

	if (m_SendDataQueue.empty() == false)
	{
		m_bCompletedWrite = false;
		auto pData = m_SendDataQueue.front();
		auto pHeader = reinterpret_cast<const PACKET_HEADER*>(&pData[0]);
		PostSend(true, pHeader->nSize, pData);
	}
	else
	{
		m_bCompletedWrite = true;
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

		m_pServer->CloseSession(m_nSessionID);
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
				m_pServer->ProcessPacket(m_nSessionID, m_PacketBuffer.begin() + nReadData);

				nPacketData -= pHeader->nSize;
				nReadData += pHeader->nSize;
			}
			else
			{
				break;
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
}