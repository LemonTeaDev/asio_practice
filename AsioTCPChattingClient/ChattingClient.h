#include "../Common/Protocol.h"
#include <boost/asio.hpp>
#include <deque>
#include <vector>

class ChatClient
{
public:
	ChatClient(boost::asio::io_service& io_service);
	~ChatClient();

	bool IsConnecting() const;
	void PostLogin();
	bool IsLoggedIn() const;
	void Connect(boost::asio::ip::tcp::endpoint endpoint);
	void Close();
	void PostSend(const bool bImmediately, const int nSize, byte* pData);

private:
	void PostReceive();
	void handle_connect(const boost::system::error_code& error);
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);

	void ProcessPacket(const byte* pData);

private:
	boost::asio::io_service& m_IOService;
	boost::asio::ip::tcp::socket m_Socket;

	std::array<byte, 512> m_ReceiveBuffer;

	int m_nPacketBufferMark;
	byte m_PacketBuffer[MAX_RECEIVE_BUFFER_LEN * 2];

	CRITICAL_SECTION m_lock;
	std::deque< byte* > m_SendDataQueue;

	bool m_bIsLogin;
};