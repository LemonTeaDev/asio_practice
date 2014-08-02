#pragma once
#include <array>
#include <algorithm>
#include <memory>
#include <functional>
#include <string>
#include <vector>

const unsigned short PORT_NUMBER = 31400;
const int MAX_RECEIVE_BUFFER_LEN = 10;
const int MAX_CHAT_MESSAGE_LEN = 20;
typedef char byte;

typedef std::shared_ptr<byte> shared_byte;

typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef short s16;

struct packet_header
{
	packet_header()
	:	id_(0),
		content_size_(0)
	{
	}
	size_t get_packet_size() const;

	u16 id_;
	u32 content_size_;
};

enum Packets
{
	REQUEST_LOGIN = 1,
	REPLY_LOGIN = 2,
	REQUEST_CHAT = 6,
	NOTICE_CHAT = 7,
};

extern shared_byte build_packet(u32 id, u32 content_size);

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
