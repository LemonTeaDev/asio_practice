#pragma once
#include <array>
#include <algorithm>
#include <memory>
#include <functional>
#include <string>
#include <vector>

const unsigned short PORT_NUMBER = 31400;
const int MAX_RECEIVE_BUFFER_LEN = 1024;
const int MAX_NAME_LEN = 17;
const int MAX_MESSAGE_LEN = 129;
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

struct PKT_REQ_IN : public PACKET_HEADER
{
	typedef std::string name_type;
	typedef std::vector<name_type::value_type> name_vector_type;

	void Init()
	{
		nID = REQ_IN;
		nSize = sizeof(PKT_REQ_IN);
	}

	void SetName(std::string& _name)
	{
		name.clear();
		std::copy(_name.begin(), _name.end(), std::back_inserter(name));
		nSize = sizeof(PKT_REQ_IN) + name.capacity() * sizeof(name_type::value_type);
	}

	name_type&& GetName()
	{
		name_type str_name(name.begin(), name.end());
		return std::move(str_name);
	}

private:
	name_vector_type name;
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
		memset(szMessage, 0, MAX_MESSAGE_LEN);
	}

	char szMessage[MAX_MESSAGE_LEN];
};

struct PKT_NOTICE_CHAT : public PACKET_HEADER
{
	void Init()
	{
		nID = NOTICE_CHAT;
		nSize = sizeof(PKT_NOTICE_CHAT);
		memset(szName, 0, MAX_NAME_LEN);
		memset(szMessage, 0, MAX_MESSAGE_LEN);
	}

	char szName[MAX_NAME_LEN];
	char szMessage[MAX_MESSAGE_LEN];
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
