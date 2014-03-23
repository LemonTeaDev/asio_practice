#pragma once
#include <array>
#include <algorithm>

const unsigned short PORT_NUMBER = 31400;
const int MAX_RECEIVE_BUFFER_LEN = 512;
const int MAX_NAME_LEN = 17;
const int MAX_MESSAGE_LEN = 129;
typedef char byte;

struct PACKET_HEADER
{
	short nID;
	short nSize;
};

enum Packets
{
	REQ_IN = 1,
	RES_IN = 2,
	REQ_CHAT = 6,
	NOTICE_CHAT = 7,
};

struct PKT_REQ_IN : public PACKET_HEADER
{
	void Init()
	{
		nID = REQ_IN;
		nSize = sizeof(PKT_REQ_IN);
		name.reserve(MAX_NAME_LEN);
	}

	const size_t GetSize()
	{
		return sizeof(PACKET_HEADER) + name.capacity() * sizeof(std::u32string::value_type);
	}
	
	std::u32string name;
};

struct PKT_RES_IN : public PACKET_HEADER
{
	void Init()
	{
		nID = RES_IN;
		nSize = sizeof(PKT_RES_IN);
		bIsSuccess = false;
	}
	
	bool bIsSuccess;
};

struct PKT_REQ_CHAT : public PACKET_HEADER
{
	void Init()
	{
		nID = REQ_CHAT;
		nSize = sizeof(PKT_REQ_CHAT);
		message.reserve(MAX_MESSAGE_LEN);
	}
	std::u32string message;
};

struct PKT_NOTICE_CHAT : public PACKET_HEADER
{
	void Init()
	{
		nID = NOTICE_CHAT;
		nSize = sizeof(PKT_NOTICE_CHAT);
		name.reserve(MAX_NAME_LEN);
		message.reserve(MAX_MESSAGE_LEN);
	}

	std::u32string name;
	std::u32string message;
};