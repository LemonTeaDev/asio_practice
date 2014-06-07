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
	
	std::thread thread(boost::bind(&boost::asio::io_service::run, &io_svc));

	std::wstring inputMsg;

	while (std::getline(std::wcin, inputMsg))
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
			PKT_REQ_IN* pReqIn = new PKT_REQ_IN;
			pReqIn->Init();
			pReqIn->name = codeCvt.to_bytes(inputMsg);

			client.PostSend(false, pReqIn->nSize, PacketSPtr(reinterpret_cast<byte*>(pReqIn)));
		}
		else
		{
			PKT_REQ_CHAT* pReqChat = new PKT_REQ_CHAT;
			pReqChat->Init();
			pReqChat->message = codeCvt.to_bytes(inputMsg);

			client.PostSend(false, pReqChat->nSize, PacketSPtr(reinterpret_cast<byte*>(pReqChat)));
		}
	}

	io_svc.stop();

	client.Close();

	thread.join();

	std::cout << "클라이언트를 종료해 주세요" << std::endl;

	return 0;
}