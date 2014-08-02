#pragma  once

#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <string>
#include <vector>
#include "server_session.h"
#include "../Common/Protocol.h"

#include "ServerCommon.h"

class chat_server
{
public:
	chat_server(boost::asio::io_service& io_service);
	~chat_server();
	void init(const int max_session_count);
	void start();
	void close_session(const int session_id);
	void process_packet(const int session_id, byte* data);

private:
	typedef std::shared_ptr<session> shared_session;
	bool post_accept();
	void handle_accept(
		shared_session pSession, 
		const boost::system::error_code& error);

private:
	int seq_number;
	bool is_accepting_;
	boost::asio::ip::tcp::acceptor acceptor_;

	std::vector<shared_session> session_list_;
	std::deque<int> session_queue_;
};