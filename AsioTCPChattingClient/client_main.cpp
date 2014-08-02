#include "chatting_client.h"
#include <thread>
#include <functional>
#include <boost/asio.hpp>
#include <iostream>
#include <locale>
#include <codecvt>
#include <boost/bind.hpp>
#include <packet.pb.h>

int main()
{
	boost::asio::io_service io_svc;
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> codeCvt;

	auto endpoint = boost::asio::ip::tcp::endpoint(
		boost::asio::ip::address::from_string("127.0.0.1"),
		PORT_NUMBER);

	ChatClient client(io_svc);
	client.connect(endpoint);
	std::thread worker_thread(
		std::bind(static_cast<size_t (boost::asio::io_service::*)()>(&boost::asio::io_service::run), &io_svc));

	std::string input_msg;
	auto header_size = sizeof(packet_header);
	while (std::getline(std::cin, input_msg))
	{
		if (input_msg.length() < 1)
		{
			break;
		}

		if (client.is_connecting() == false)
		{
			std::cout << "Not connected with server" << std::endl;
			continue;
		}

		if (client.is_logged_in() == false)
		{
			LoginRequest login_request;
			login_request.set_name(input_msg);
			auto content_size = login_request.ByteSize();

			shared_byte pkt = build_packet(REQUEST_LOGIN, content_size);
			login_request.SerializeToArray(pkt.get() + header_size, content_size);
			client.post_send(false, header_size + content_size, pkt);
		}
		else
		{
			ChatRequest chat_request;

			/*
			if (input_msg.size() > MAX_CHAT_MESSAGE_LEN)
			{
				std::cout << "* Your chat message is too long. Some parts will be trimmed." << std::endl;
				input_msg = input_msg.substr(0, MAX_CHAT_MESSAGE_LEN);
			}
			*/

			chat_request.set_message(input_msg);
			auto content_size = chat_request.ByteSize();

			shared_byte pkt = build_packet(REQUEST_CHAT, content_size);
			chat_request.SerializeToArray(pkt.get() + header_size, content_size);
			client.post_send(false, header_size + content_size, pkt);
		}
	}

	io_svc.stop();
	client.close();
	worker_thread.join();

	std::cout << "Terminated" << std::endl;

	return 0;
}