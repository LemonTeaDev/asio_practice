#include "ChattingClient.h"
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
	client.Connect(endpoint);
	std::thread thread(std::bind(static_cast<size_t (boost::asio::io_service::*)()>(&boost::asio::io_service::run), &io_svc));

	std::string inputMsg;

	while (std::getline(std::cin, inputMsg))
	{
		if (inputMsg.length() < 1)
		{
			break;
		}

		if (client.IsConnecting() == false)
		{
			std::cout << "서버와 연결되지 않았습니다" << std::endl;
			continue;
		}

		if (client.IsLoggedIn() == false)
		{
			LoginRequest sendPkt;
			sendPkt.set_name(inputMsg);
			
			auto pktSize = sendPkt.ByteSize();

			shared_byte pkt(new byte[pktSize]);
			sendPkt.SerializeToArray(pkt.get(), pktSize);
			client.PostSend(false, pktSize, pkt);

			/*
			auto pSendPkt = new PKT_REQ_IN;
			auto pRawSendPkt = reinterpret_cast<byte*>(pSendPkt);

			pSendPkt->Init();
			strncpy_s(pSendPkt->szName, MAX_NAME_LEN, inputMsg.c_str(), MAX_NAME_LEN - 1);

			client.PostSend(false, pSendPkt->nSize, shared_byte(pRawSendPkt));
			*/
		}
		else
		{
			auto pSendPkt = new PKT_REQ_CHAT;
			auto pRawSendPkt = reinterpret_cast<byte*>(pSendPkt);

			pSendPkt->Init();
			strncpy_s(pSendPkt->szMessage, MAX_MESSAGE_LEN, inputMsg.c_str(), MAX_MESSAGE_LEN - 1);

			client.PostSend(false, pSendPkt->nSize, shared_byte(pRawSendPkt));
		}
	}

	io_svc.stop();

	client.Close();

	thread.join();

	std::cout << "클라이언트를 종료해 주세요" << std::endl;

	return 0;
}