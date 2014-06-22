#pragma once
#include <array>
#include <algorithm>
#include <memory>
#include <functional>
#include <string>
#include <vector>

const unsigned short PORT_NUMBER = 31400;
const int MAX_RECEIVE_BUFFER_LEN = 1024;
typedef char byte;

typedef std::shared_ptr<byte> shared_byte;

struct PACKET_HEADER
{
	int nID;
	int nSize;
};

enum Packets
{
	REQ_IN = 1,
	RES_IN = 2,
	REQ_CHAT = 6,
	NOTICE_CHAT = 7,
};

namespace boost
{
	namespace asio
	{
		namespace placeholders
		{
			const auto error = std::placeholders::_1;
			const auto bytes_transferred = std::placeholders::_2;
		}
	}
}
