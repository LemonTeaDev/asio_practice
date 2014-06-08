#include "ChattingClient.h"
#include <thread>
#include <functional>
#include <boost/asio.hpp>
#include <iostream>
#include <locale>
#include <codecvt>
#include <boost/bind.hpp>

int main()
{
	boost::asio::io_service io_svc;
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> codeCvt;

	auto endpoint = boost::asio::ip::tcp::endpoint(
		boost::asio::ip::address::from_string("127.0.0.1"),
		PORT_NUMBER);

	ChatClient client(io_svc);
	client.Connect(endpoint);
	//std::thread thread(boost::bind(&boost::asio::io_service::run, &io_svc));
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
			std::cout << "������ ������� �ʾҽ��ϴ�" << std::endl;
			continue;
		}

		if (client.IsLoggedIn() == false)
		{
			auto pSendPkt = new PKT_REQ_IN;
			auto pRawSendPkt = reinterpret_cast<byte*>(pSendPkt);

			pSendPkt->Init();
			pSendPkt->SetName(inputMsg);

			client.PostSend(false, pSendPkt->nSize, shared_byte(pRawSendPkt));
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

	std::cout << "Ŭ���̾�Ʈ�� ������ �ּ���" << std::endl;

	return 0;
}