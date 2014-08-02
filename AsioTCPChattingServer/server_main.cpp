#include "chatting_server.h"

const int MAX_SESSION_COUNT = 100;
int main()
{
	boost::asio::io_service io_service;
	chat_server server(io_service);
	server.init(MAX_SESSION_COUNT);
	server.start();

	io_service.run();

	std::cout << "Network Connection Ended" << std::endl;

	getchar();
	return 0;
}