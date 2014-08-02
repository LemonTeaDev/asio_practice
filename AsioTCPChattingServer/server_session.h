#pragma once
#include <deque>
#include <boost/asio.hpp>
#include "../Common/Protocol.h"
#include <string>

class chat_server;

class session
{
public:
	session(int session_id, boost::asio::io_service& io_service, chat_server& server);
	~session();

	const int session_id() const;
	boost::asio::ip::tcp::socket& Socket();
	void init();
	void post_receive();
	void post_send(const bool immediately, const int size, shared_byte data);
	
	void set_name(const std::string& name);
	void set_name(const char* name);
	const char* get_name() const;

private:
	void handle_write(
		const boost::system::error_code& error, 
		size_t bytes_transferred);
	void handle_receive(
		const boost::system::error_code& error, size_t bytes_transferred);

	u32 session_id_;
	boost::asio::ip::tcp::socket socket_;
	std::array<byte, MAX_RECEIVE_BUFFER_LEN> receive_buffer_;

	u32 packet_buffer_mark_;
	std::vector<byte> packet_buffer_;

	std::deque<shared_byte> send_data_queue_;
	std::string name_;
	chat_server& server_;
};