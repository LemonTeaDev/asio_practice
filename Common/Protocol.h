#pragma once
#include <array>
#include <algorithm>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <cstdint>
#include <boost/crc.hpp>

const unsigned short PORT_NUMBER = 31400;
const int MAX_RECEIVE_BUFFER_LEN = 10;
const int MAX_CHAT_MESSAGE_LEN = 20;
typedef char byte;

typedef std::shared_ptr<byte> shared_byte;

struct packet_header
{
	packet_header()
	:	id_(0),
		content_size_(0),
		crc_(0)
	{
	}

	size_t get_packet_size() const;
	boost::crc_32_type::value_type&& calculate_crc() const;

	uint16_t id_;
	uint32_t content_size_;
	boost::crc_32_type::value_type crc_;
};

enum Packets
{
	REQUEST_LOGIN = 1,
	REPLY_LOGIN = 2,
	REQUEST_CHAT = 6,
	NOTICE_CHAT = 7,
};

extern shared_byte build_packet(uint32_t id, uint32_t content_size);

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
