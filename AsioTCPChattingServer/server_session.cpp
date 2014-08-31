#include "server_session.h"
#include "chatting_server.h"
#include <iterator>
#include <locale>
#include <codecvt>
#include <cassert>

extern std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> code_cvt;

session::session(int session_id, boost::asio::io_service& io_service, chat_server& server)
	: socket_(io_service)
	, session_id_(session_id)
	, server_(server)
	, content_read_mode_(false)
{
	packet_buffer_.reserve(sizeof(packet_header));
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

void session::clear_packet_buffer()
{
	content_read_mode_ = false;
	packet_buffer_mark_ = 0;
	packet_buffer_.clear();
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
		//	read stream for sizeof(header) bytes
		//	if !content read mode:
		//		if crc is valid
		//		read content size from header
		//		filter out if size is too big(supp. fake packet)
		//		begin content read mode
		// if content read mode:
		//		[fill packet buffer(vector) with contents until content size]
		//		end content read mode

		std::copy_n(
			receive_buffer_.data(),
			bytes_transferred,
			std::inserter(packet_buffer_, packet_buffer_.begin() + packet_buffer_mark_));

		uint32_t packet_data_size = packet_buffer_mark_ + bytes_transferred;

		while (packet_data_size > 0)
		{
			if (packet_data_size < sizeof(packet_header))
			{
				packet_buffer_mark_ += bytes_transferred;
				break;
			}

			auto header = reinterpret_cast<packet_header*>(&packet_buffer_[0]);
			if (!content_read_mode_)
			{
				boost::crc_32_type::value_type crc = header->calculate_crc();
				if (crc != header->crc_)
				{
					clear_packet_buffer();
					break;
				}

				// Check the packet id. 
				// if the packet is a chat request but its length is over the limit,
				// then it should not be processed
				bool is_header_size_valid = false;
				switch (header->id_)
				{
				case REQUEST_CHAT:
					if (header->content_size_ <= MAX_CHAT_MESSAGE_LEN)
					{
						is_header_size_valid = true;
					}
					break;
				default:
					is_header_size_valid = true;
				}
			
				if (!is_header_size_valid)
				{
					clear_packet_buffer();
					break;
				}

				content_read_mode_ = true;
			}

			auto packet_size = header->get_packet_size();
			if (packet_size <= packet_data_size)
			{
				server_.process_packet(session_id_, &packet_buffer_[0]);
				clear_packet_buffer();
				break;
			}
			else
			{
				packet_buffer_mark_ += bytes_transferred;
				break;
			}
		}

		post_receive();
	}
}