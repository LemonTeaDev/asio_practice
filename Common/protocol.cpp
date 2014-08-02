#include "Protocol.h"

size_t packet_header::get_packet_size() const
{
	return content_size_ + sizeof(packet_header);
}

shared_byte build_packet(u32 id, u32 content_size)
{
	shared_byte packet(new byte[content_size + sizeof(packet_header)]);
	
	packet_header* header = reinterpret_cast<packet_header*>(packet.get());
	header->id_ = id;
	header->content_size_ = content_size;

	return packet;
}
