#include "../Common/Protocol.h"
#include <boost/asio.hpp>
#include <deque>
#include <vector>
#include <mutex>

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
	void PostSend(const bool bImmediately, const int nSize, shared_byte pData);

private:
	void PostReceive();
	void handle_connect(const boost::system::error_code& error);
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);

	void ProcessPacket(byte* pData);

private:
	boost::asio::io_service& m_IOService;
	boost::asio::ip::tcp::socket m_Socket;

	std::array<byte, MAX_RECEIVE_BUFFER_LEN> m_ReceiveBuffer;

	int m_nPacketBufferMark;
	std::vector<byte> m_PacketBuffer;

	std::mutex m_lock;
	std::deque<shared_byte> m_SendDataQueue;

	bool m_bIsLogin;
};