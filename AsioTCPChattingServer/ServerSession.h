#pragma once
#include <deque>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "../Common/Protocol.h"

class ChatServer;

class Session
{
public:
	Session(int nSessionID, boost::asio::io_service& io_service, ChatServer* const pServer);
	~Session();

	const int SessionID() const;
	boost::asio::ip::tcp::socket& Socket();
	void Init();
	void PostReceive();
	void PostSend(const bool bImmediately, const int nSize, PacketSPtr pData);
	void SetName(const std::string& name);
	const std::wstring GetName() const;
	const std::string& GetRawName() const;

private:
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);

	int m_nSessionID;
	boost::asio::ip::tcp::socket m_Socket;
	std::vector<byte> m_ReceiveBuffer;
	std::vector<byte> m_PacketBuffer;
	int m_nPacketBufferMark;
	bool m_bCompletedWrite;
	std::deque<PacketSPtr> m_SendDataQueue;
	std::string m_Name;
	ChatServer* const m_pServer;
};