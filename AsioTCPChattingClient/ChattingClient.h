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
	void PostSend(const bool bImmediately, const int nSize, PacketSPtr pData);

private:
	void PostReceive();
	void handle_connect(const boost::system::error_code& error);
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);

	void ProcessPacket(std::vector<byte>::const_iterator itrData);

private:
	boost::asio::io_service& m_IOService;
	boost::asio::ip::tcp::socket m_Socket;

	std::vector<byte> m_ReceiveBuffer;
	std::vector<byte> m_PacketBuffer;
	int m_nPacketBufferMark;

	CRITICAL_SECTION m_lock;
	std::deque<PacketSPtr> m_SendDataQueue;

	bool m_bIsLogin;
};