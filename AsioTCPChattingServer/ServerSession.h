#pragma once
#include <deque>
#include <boost/asio.hpp>
#include "../Common/Protocol.h"
#include <string>

class ChatServer;

class Session
{
public:
	Session(int nSessionID, boost::asio::io_service& io_service, ChatServer& server);
	~Session();

	const int SessionID() const;
	boost::asio::ip::tcp::socket& Socket();
	void Init();
	void PostReceive();
	void PostSend(const bool bImmediately, const int nSize, shared_byte pData);
	
	void SetName(const char* name);
	const char* GetName() const;

private:
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);

	int m_nSessionID;
	boost::asio::ip::tcp::socket m_Socket;
	std::array<byte, MAX_RECEIVE_BUFFER_LEN> m_ReceiveBuffer;

	int m_nPacketBufferMark;
	std::vector<byte> m_PacketBuffer;

	std::deque<shared_byte> m_SendDataQueue;
	std::string m_Name;
	ChatServer& m_rServer;
};