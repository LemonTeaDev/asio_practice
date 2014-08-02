#include "server_session.h"
#include "chatting_server.h"
#include <iterator>
#include <locale>
#include <codecvt>

extern std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> code_cvt;

session::session(int session_id, boost::asio::io_service& io_service, chat_server& server)
	: socket_(io_service)
	, session_id_(session_id)
	, server_(server)
{
}

session::~session()
{
	while (send_data_queue_.empty() == false)
	{
		send_data_queue_.pop_front();
	}
}

const int session::session_id() const
{
	return session_id_;
}

boost::asio::ip::tcp::socket& session::Socket()
{
	return socket_;
}

void session::init()
{
	packet_buffer_mark_ = 0;
}

void session::post_receive()
{
	socket_.async_read_some(
		boost::asio::buffer(receive_buffer_),
		std::bind(
			&session::handle_receive, 
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void session::post_send(
	const bool immediately,
	const int size,
	shared_byte data)
{
	shared_byte send_data = nullptr;

	if (immediately == false)
	{
		send_data_queue_.push_back(send_data);
	}

	send_data = data;

	if (immediately == false && send_data_queue_.size() > 1)
	{
		return;
	}

	boost::asio::async_write(socket_, boost::asio::buffer(send_data.get(), size),
		std::bind(&session::handle_write, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void session::set_name(const char* name)
{
	name_ = name;
}

void session::set_name(const std::string& name)
{
	name_ = name;
}

const char* session::get_name() const
{
	return name_.c_str();
}

void session::handle_write(
	const boost::system::error_code& error,
	size_t bytes_transferred)
{
	send_data_queue_.pop_front();

	if (send_data_queue_.empty() == false)
	{
		shared_byte data = send_data_queue_.front();
		auto header = reinterpret_cast<packet_header*>(data.get());
		post_send(true, header->content_size_, data);
	}
}

void session::handle_receive(
	const boost::system::error_code& error, 
	size_t bytes_transferred)
{
	if (error)
	{
		if (error == boost::asio::error::eof)
		{
			std::cout << "Client Disconnected" << std::endl;
		}
		else
		{
			std::cout << "error No: " << error.value() << " error Message: "
				<< error.message() << std::endl;
		}

		server_.close_session(session_id_);
	}
	else
	{
		//	TODO 
		//	change design:
		//	add crc to header
		//	read stream for sizeof(header) bytes
		//	if !content read mode:
		//		if crc is valid
		//		read content size from header
		//		filter out if size is too big(supp. fake packet)
		//		begin content read mode
		// if content read mode:
		//		[fill packet buffer(vector) with contents until content size]
		//		end content read mode

		if (packet_buffer_.size() - packet_buffer_mark_ <= bytes_transferred)
		{
			packet_buffer_.resize(bytes_transferred * 2);
		}
		std::copy_n(
			receive_buffer_.data(),
			bytes_transferred,
			packet_buffer_.begin() + packet_buffer_mark_);

		u32 packet_data_size = packet_buffer_mark_ + bytes_transferred;
		u32 read_data_size = 0;

		while (packet_data_size > 0)
		{
			if (packet_data_size < sizeof(packet_header))
			{
				break;
			}

			auto header = reinterpret_cast<packet_header*>(&packet_buffer_[read_data_size]);

			// TODO: Check the packet id. 
			// if the packet is a chat request but its length is over the limit,
			// then it should not be processed


			auto packet_size = header->get_packet_size();
			if (packet_size <= packet_data_size)
			{
				server_.process_packet(session_id_, &packet_buffer_[read_data_size]);

				packet_data_size -= packet_size;
				read_data_size += packet_size;

				packet_buffer_mark_ = 0;
			}
			else
			{
				break;
			}
		}

		if (packet_data_size > 0)
		{
			std::vector<byte> temp_buffer;
			if (packet_buffer_.size() > read_data_size + packet_data_size)
			{
				std::vector<byte> temp_buffer(
					packet_buffer_.begin() + read_data_size,
					packet_buffer_.begin() + read_data_size + packet_data_size + 1);

				std::copy_n(
					temp_buffer.begin(),
					packet_data_size,
					packet_buffer_.begin());

				packet_buffer_mark_ = packet_data_size;
			}
			else
			{
				packet_buffer_mark_ += read_data_size;
			}
		}

		post_receive();
	}
}