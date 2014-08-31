#pragma once
#include <deque>
#include <boost/asio.hpp>
#include <boost/crc.hpp>
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

	void clear_packet_buffer();

	uint32_t session_id_;
	boost::asio::ip::tcp::socket socket_;
	std::array<byte, sizeof(packet_header)> receive_buffer_;

	uint32_t packet_buffer_mark_;
	std::vector<byte> packet_buffer_;

	std::deque<shared_byte> send_data_queue_;
	std::string name_;

	bool content_read_mode_;
	chat_server& server_;
};