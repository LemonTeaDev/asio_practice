#pragma once
#include <deque>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "Protocol.h"

class ChatServer;

class Session
{
public:
	Session(int nSessionID, boost::asio::io_service& io_service, ChatServer* pServer);
	~Session();

	const int SessionID() const;
	boost::asio::ip::tcp::socket& Socket();
	void Init();
	void PostReceive();
	void PostSend(const bool bImmediately, const int nSize, const std::vector<byte>& pData);
	void SetName(const char32_t* pszName);
	void SetName(const std::u32string& name);
	const std::u32string& GetName() const;

private:
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);

	int m_nSessionID;
	boost::asio::ip::tcp::socket m_Socket;
	std::vector<byte> m_ReceiveBuffer;
	int m_nPacketBufferMark;
	std::vector<byte> m_PacketBuffer;
	bool m_bCompletedWrite;
	std::deque<const std::vector<byte>> m_SendDataQueue;
	std::u32string m_Name;
	ChatServer* m_pServer;
};