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

	bool is_connecting() const;
	void post_login();
	bool is_logged_in() const;
	void connect(boost::asio::ip::tcp::endpoint endpoint);
	void close();
	void post_send(const bool bImmediately, const int nSize, shared_byte data);

private:
	void post_receive();
	void handle_connect(const boost::system::error_code& error);
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);

	void process_packet(byte* data);

private:
	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::socket socket_;

	std::array<byte, MAX_RECEIVE_BUFFER_LEN> receive_buffer_;

	uint32_t packet_buffer_mark_;
	std::vector<byte> packet_buffer_;

	std::mutex lock_;
	std::deque<shared_byte> send_data_queue_;

	bool is_logged_in_;
};