#include "chatting_server.h"

#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <functional>

#include "server_session.h"
#include "../Common/Protocol.h"
#include <packet.pb.h>

std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> code_cvt;

chat_server::chat_server(boost::asio::io_service& io_service)
	: acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), PORT_NUMBER))
{
	is_accepting_ = false;
}

chat_server::~chat_server()
{
	for (size_t i = 0; i < session_list_.size(); ++i)
	{
		if (session_list_[i]->Socket().is_open())
		{
			session_list_[i]->Socket().close();
		}
	}
}

void chat_server::init(const int nMaxSessionCount)
{
	for (int i=0; i < nMaxSessionCount; ++i)
	{
		shared_session pSession(new session(i, acceptor_.get_io_service(), *this));
		session_list_.push_back(pSession);
		session_queue_.push_back(i);
	}
}

void chat_server::start()
{
	std::cout << "Initializing Server......" << std::endl;
	post_accept();
}

void chat_server::close_session(const int session_id)
{
	std::cout << "Client Connection Closed: Session ID: " << session_id << std::endl;
	session_list_[session_id]->Socket().close();
	session_queue_.push_back(session_id);
	if (is_accepting_ == false)
	{
		post_accept();
	}
}

void chat_server::process_packet(
	const int session_id, 
	byte* data)
{
	auto recv_header = reinterpret_cast<packet_header*>(data);
	if (recv_header == nullptr) { return; }

	auto header_size = sizeof(packet_header);
	switch (recv_header->id_)
	{
		case REQUEST_LOGIN:
		{
			// Read CL Packet
			LoginRequest login_request;
			login_request.ParseFromArray(data + header_size, recv_header->content_size_);

			session_list_[session_id]->set_name(login_request.name());
			std::cout << "Client Logged In, Name: " << session_list_[session_id]->get_name() << std::endl;

			// Reply to CL
			LoginResponse login_response;
			login_response.set_success(true);
			auto content_size = login_response.ByteSize();

			shared_byte packet = build_packet(REPLY_LOGIN, content_size);
			login_response.SerializeToArray(packet.get() + header_size, content_size);
			session_list_[session_id]->post_send(
				false, 
				header_size + content_size, 
				packet);
		}
		break;

		case REQUEST_CHAT:
		{
			// Read CL Packet
			ChatRequest chat_request;
			chat_request.ParseFromArray(data + header_size, recv_header->content_size_);

			// Broadcast to CLs
			ChatNotify notification;
			notification.set_name(session_list_[session_id]->get_name());
			notification.set_message(chat_request.message());
			
			auto content_size = notification.ByteSize();
			shared_byte pkt = build_packet(NOTICE_CHAT, content_size);
			notification.SerializeToArray(pkt.get() + header_size, content_size);

			size_t session_count = session_list_.size();
			for (size_t i = 0; i < session_count; ++i)
			{
				if (session_list_[i]->Socket().is_open())
				{
					session_list_[i]->post_send(
						false, 
						header_size + content_size,
						pkt);
				}
			}
		}
		break;
	}

	return;
}

bool chat_server::post_accept()
{
	if (session_queue_.empty())
	{
		is_accepting_ = false;
		return false;
	}

	is_accepting_ = true;
	int session_id = session_queue_.front();
	session_queue_.pop_front();
	acceptor_.async_accept(
		session_list_[session_id]->Socket(),
		std::bind(&chat_server::handle_accept,
			this,
			session_list_[session_id],
			boost::asio::placeholders::error));

	return true;
}

void chat_server::handle_accept(
	shared_session session, 
	const boost::system::error_code& error)
{
	if (!error)
	{
		std::cout << "Client Connection Accepted. SessionID: " << session->session_id() << std::endl;
		
		session->init();
		session->post_receive();
		post_accept();
	}
	else
	{
		std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
	}
}

