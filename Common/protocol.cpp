#include "Protocol.h"
#include <boost/crc.hpp>

boost::crc_32_type crc;

size_t packet_header::get_packet_size() const
{
	return content_size_ + sizeof(packet_header);
}

boost::crc_32_type::value_type&& packet_header::calculate_crc() const
{
	crc.process_bytes(&id_, sizeof(packet_header) - sizeof(crc_));
	return crc.checksum();
}

shared_byte build_packet(uint32_t id, uint32_t content_size)
{
	shared_byte packet(new byte[content_size + sizeof(packet_header)]);
	
	packet_header* header = reinterpret_cast<packet_header*>(packet.get());
	header->id_ = id;
	header->content_size_ = content_size;
	header->crc_ = header->calculate_crc();

	return packet;
}
