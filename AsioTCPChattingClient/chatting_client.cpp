#include <boost/thread.hpp>
#include <functional>
#include <packet.pb.h>
#include "chatting_client.h"
#include "scoped_lock.h"

ChatClient::ChatClient(boost::asio::io_service& io_service)
	: io_service_(io_service), socket_(io_service)
{
	is_logged_in_ = false;
}

ChatClient::~ChatClient()
{
	scoped_lock lock(lock_);

	while (send_data_queue_.empty() == false)
	{
		send_data_queue_.pop_front();
	}
}

bool ChatClient::is_connecting() const
{
	return socket_.is_open();
}

void ChatClient::post_login()
{
	is_logged_in_ = true;
}

bool ChatClient::is_logged_in() const
{
	return is_logged_in_;
}

void ChatClient::connect(boost::asio::ip::tcp::endpoint endpoint)
{
	packet_buffer_mark_ = 0;

	socket_.async_connect(endpoint,
		std::bind(&ChatClient::handle_connect, this,
		boost::asio::placeholders::error));
}

void ChatClient::close()
{
	if (socket_.is_open())
	{
		socket_.close();
	}
}

void ChatClient::post_send(const bool immediately, const int size, shared_byte data)
{
	shared_byte send_data = nullptr;

	scoped_lock lock(lock_);

	if (immediately == false)
	{
		send_data_queue_.push_back(send_data);
	}

	send_data = data;

	if (immediately || send_data_queue_.size() < 2)
	{
		boost::asio::async_write(socket_, boost::asio::buffer(send_data.get(), size),
			std::bind(&ChatClient::handle_write, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}
}

void ChatClient::post_receive()
{
	std::fill(receive_buffer_.begin(), receive_buffer_.end(), 0);

	socket_.async_read_some(
		boost::asio::buffer(receive_buffer_),
		std::bind(&ChatClient::handle_receive, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void ChatClient::handle_connect(const boost::system::error_code& error)
{
	if (!error)
	{
		std::cout << "Server Connected" << std::endl;
		std::cout << "Enter Your Name:" << std::endl;

		post_receive();
	}
	else
	{
		std::cout 
			<< "Server Connect Failed. error No: " 
			<< error.value() 
			<< " error Message: " 
			<< error.message() 
			<< std::endl;
	}
}

void ChatClient::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
{
	shared_byte data = nullptr;

	{	// a critical section
		scoped_lock lock(lock_);

		send_data_queue_.pop_front();
		if (send_data_queue_.empty() == false)
		{
			data = send_data_queue_.front();
		}
	}

	if (data != nullptr)
	{
		auto header = reinterpret_cast<packet_header*>(data.get());
		post_send(true, header->content_size_, data);
	}
}

void ChatClient::handle_receive(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (error)
	{
		if (error == boost::asio::error::eof)
		{
			std::cout << "Client Disconnected" << std::endl;
		}
		else
		{
			std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
		}

		close();
	}
	else
	{
		if (packet_buffer_.size() < bytes_transferred)
		{
			packet_buffer_.resize(bytes_transferred * 2);
		}
		std::copy_n(
			receive_buffer_.data(),
			bytes_transferred,
			packet_buffer_.begin() + packet_buffer_mark_);

		uint32_t packet_data_size = packet_buffer_mark_ + bytes_transferred;
		uint32_t read_data_size = 0;

		while (packet_data_size > 0)
		{
			if (packet_data_size < sizeof(packet_header))
			{
				break;
			}

			auto header = 
				reinterpret_cast<packet_header*>(&packet_buffer_[read_data_size]);

			auto packet_size = header->get_packet_size();
			if (packet_size <= packet_data_size)
			{
				process_packet(&packet_buffer_[read_data_size]);

				packet_data_size -= packet_size;
				read_data_size += packet_size;
			}
			else
			{
				break;
			}
		}

		if (packet_data_size > 0)
		{
			std::vector<byte> temp_buffer(
				packet_buffer_.begin() + read_data_size, 
				packet_buffer_.begin() + read_data_size + packet_data_size + 1);

			std::copy_n(
				temp_buffer.begin(),
				packet_data_size,
				packet_buffer_.begin());
		}

		packet_buffer_mark_ = packet_data_size;

		post_receive();
	}
}

void ChatClient::process_packet(byte* data)
{
	auto header = reinterpret_cast<packet_header*>(data);
	if (header == nullptr) { return; }

	auto header_size = sizeof(packet_header);
	switch (header->id_)
	{
		case REPLY_LOGIN:
		{
			LoginResponse login_response;
			login_response.ParseFromArray(data + header_size, header->content_size_);

			post_login();
			if (login_response.success())
			{
				std::cout << "Login Success" << std::endl;
			}
			else
			{
				std::cout << "Login Fail" << std::endl;
			}
		}
		break;
	case NOTICE_CHAT:
		{
			ChatNotify chat_notify;
			chat_notify.ParseFromArray(data + header_size, header->content_size_);

			std::cout << chat_notify.name() << ": " << chat_notify.message() << std::endl;
		}
		break;
	}
}
