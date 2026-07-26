/*
Minetest
Copyright (C) 2015 nerzhul, Loic Blot <loic.blot@unix-experience.fr>
Copyright (C) 2025 Logiki, Donatto J. Viveros P. <donatto555@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "networkpacket.h"
#include <cstdint>
#include <sstream>
#include "networkexceptions.h"
#include "util/serialize.h"
#include "networkprotocol.h"

//Peerid serves like player id in this case
NetworkPacket::NetworkPacket(uint16_t command, u32 datasize, session_t peer_id, uint16_t protocol_version, uint8_t SrvID): m_datasize(datasize), m_command(command), m_peer_id(peer_id), m_protocol_version(protocol_version), m_server_id(SrvID) {
	if (m_protocol_version == 0)
		m_protocol_version = 37;
	m_data.resize(m_datasize);
}

NetworkPacket::NetworkPacket(uint16_t command, u32 datasize, session_t peer_id, uint16_t protocol_version): m_datasize(datasize), m_command(command), m_peer_id(peer_id), m_protocol_version(protocol_version) {
	if (m_protocol_version == 0)
		m_protocol_version = 37;
	m_data.resize(m_datasize);
}

NetworkPacket::NetworkPacket(uint16_t command, u32 datasize, session_t peer_id):
m_datasize(datasize), m_command(command), m_peer_id(peer_id)
{
	m_data.resize(m_datasize);
}

NetworkPacket::NetworkPacket(uint16_t command, u32 datasize):
m_datasize(datasize), m_command(command)
{
	m_data.resize(m_datasize);
}

NetworkPacket::~NetworkPacket()
{
	m_data.clear();
}

static void fishyfishyfish() {
	
}

void NetworkPacket::checkReadOffset(u32 from_offset, u32 field_size)
{
	if (from_offset + field_size > m_datasize) {
		std::stringstream ss;
		ss << "Reading outside packet (offset: " <<
				from_offset << ", packet size: " << getSize() << ")";
		throw PacketError(ss.str());
	}
}

void NetworkPacket::putRawPacket(const u8 *data, u32 datasize, session_t peer_id)
{
	// If a m_command is already set, we are rewriting on same packet
	// This is not permitted
	//assert(m_command == 0);
	if (m_command != 0)
		errorstream << "Error on packet compress/sent = Command already set" << std::endl;

	m_datasize = datasize - 2;
	m_peer_id = peer_id;
	m_data.resize(m_datasize);

	// split command and datas
	m_command = readU16(&data[0]);
	
	memcpy(m_data.data(), &data[2], m_datasize);
	//m_data.insert(m_data.end(), data + 2, data + 2 + m_datasize);
	//m_data.assign(data + 2, data + datasize);
}

void NetworkPacket::clear()
{
	m_data.clear();
	m_datasize = 0;
	m_read_offset = 0;
	m_command = 0;
	m_peer_id = 0;
}

void NetworkPacket::setPeerID(session_t id) {
	m_peer_id = id;
}

void NetworkPacket::resizeData(size_t sz) {
	m_data.resize(sz);
}

void NetworkPacket::decreaseFront() {
	//m_data.pop_front();
	if (!m_data.empty()) {
		m_data.erase(m_data.begin());
	}
}

void NetworkPacket::decreaseReadOffset(u32 new_) {
	m_read_offset -= new_;
}

void NetworkPacket::increaseReadOffset(u32 sum) {
	m_read_offset += sum;
}

void NetworkPacket::updateData() {
	m_datasize = (u32)m_data.size();
}

void NetworkPacket::incrementSize(u32 dtas) {
	m_datasize += dtas;
}

const char* NetworkPacket::getString(u32 from_offset)
{
	checkReadOffset(from_offset, 0);

	return (char*)&m_data[from_offset];
}

void NetworkPacket::putRawString(const char* src, u32 len)
{
	//if (!src || len == 0) {
	//	errorstream << "Null pointer at putRawString" << std::endl;
	//	return;
	//}
	
	if (m_read_offset + len > m_datasize) {
		m_datasize = m_read_offset + len;
		m_data.resize(m_datasize);
	}

	if (len == 0)
		return;
	
	memcpy(&m_data[m_read_offset], src, len);
	
	//pass to queue
	//as we put data & other data into dta, we should wipe the m_data and then set its data again
	
	//m_data.clear();
	//m_data.assign(dta.begin(), dta.end());
	
	//std::copy(src, src + len, m_data.begin() + m_read_offset);
	m_read_offset += len;
}

NetworkPacket& NetworkPacket::operator>>(std::string& dst)
{
	checkReadOffset(m_read_offset, 2);
	uint16_t strLen = readU16(&m_data[m_read_offset]);
	m_read_offset += 2;

	dst.clear();

	if (strLen == 0) {
		return *this;
	}

	checkReadOffset(m_read_offset, strLen);

	dst.reserve(strLen);
	dst.append((char*)&m_data[m_read_offset], strLen);

	m_read_offset += strLen;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(const std::string &src)
{
	if (src.size() > STRING_MAX_LEN) {
		throw PacketError("String too long");
	}

	uint16_t msgsize = src.size();

	*this << msgsize;

	putRawString(src.c_str(), (u32)msgsize);

	return *this;
}

void NetworkPacket::putLongString(const std::string &src)
{
	if (src.size() > LONG_STRING_MAX_LEN) {
		throw PacketError("String too long");
	}

	u32 msgsize = src.size();

	*this << msgsize;

	putRawString(src.c_str(), msgsize);
}

static constexpr bool NEED_SURROGATE_CODING = sizeof(wchar_t) > 2;

NetworkPacket& NetworkPacket::operator>>(std::wstring& dst)
{
	checkReadOffset(m_read_offset, 2);
	uint16_t strLen = readU16(&m_data[m_read_offset]);
	m_read_offset += 2;

	dst.clear();

	if (strLen == 0) {
		return *this;
	}

	checkReadOffset(m_read_offset, strLen * 2);

	dst.reserve(strLen);
	for (uint16_t i = 0; i < strLen; i++) {
		wchar_t c = readU16(&m_data[m_read_offset]);
		if (NEED_SURROGATE_CODING && c >= 0xD800 && c < 0xDC00 && i+1 < strLen) {
			i++;
			m_read_offset += sizeof(uint16_t);

			wchar_t c2 = readU16(&m_data[m_read_offset]);
			c = 0x10000 + ( ((c & 0x3ff) << 10) | (c2 & 0x3ff) );
		}
		dst.push_back(c);
		m_read_offset += sizeof(uint16_t);
	}

	return *this;
}

NetworkPacket& NetworkPacket::operator<<(const std::wstring &src)
{
	if (src.size() > WIDE_STRING_MAX_LEN) {
		throw PacketError("String too long");
	}

	if (!NEED_SURROGATE_CODING || src.size() == 0) {
		*this << static_cast<uint16_t>(src.size());
		for (uint16_t i = 0; i < src.size(); i++)
			*this << static_cast<uint16_t>(src[i]);

		return *this;
	}

	// write dummy value, to be overwritten later
	const u32 len_offset = m_read_offset;
	u32 written = 0;
	*this << static_cast<uint16_t>(0xfff0);

	for (uint16_t i = 0; i < src.size(); i++) {
		wchar_t c = src[i];
		if (c > 0xffff) {
			// Encode high code-points as surrogate pairs
			u32 n = c - 0x10000;
			*this << static_cast<uint16_t>(0xD800 | (n >> 10))
				<< static_cast<uint16_t>(0xDC00 | (n & 0x3ff));
			written += 2;
		} else {
			*this << static_cast<uint16_t>(c);
			written++;
		}
	}

	if (written > WIDE_STRING_MAX_LEN)
		throw PacketError("String too long");
	writeU16(&m_data[len_offset], written);

	return *this;
}

std::string NetworkPacket::readLongString()
{
	checkReadOffset(m_read_offset, 4);
	u32 strLen = readU32(&m_data[m_read_offset]);
	m_read_offset += 4;

	if (strLen == 0) {
		return "";
	}

	if (strLen > LONG_STRING_MAX_LEN) {
		throw PacketError("String too long");
	}

	checkReadOffset(m_read_offset, strLen);

	std::string dst;

	dst.reserve(strLen);
	dst.append((char*)&m_data[m_read_offset], strLen);

	m_read_offset += strLen;

	return dst;
}

NetworkPacket& NetworkPacket::operator>>(char& dst)
{
	checkReadOffset(m_read_offset, 1);

	dst = readU8(&m_data[m_read_offset]);

	m_read_offset += 1;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(char src)
{
	checkDataSize(1);

	writeU8(&m_data[m_read_offset], src);

	m_read_offset += 1;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u8 src)
{
	checkDataSize(1);

	writeU8(&m_data[m_read_offset], src);

	m_read_offset += 1;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(bool src)
{
	checkDataSize(1);

	writeU8(&m_data[m_read_offset], src);

	m_read_offset += 1;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(uint16_t src)
{
	checkDataSize(2);

	writeU16(&m_data[m_read_offset], src);

	m_read_offset += 2;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u32 src)
{
	checkDataSize(4);

	writeU32(&m_data[m_read_offset], src);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u64 src)
{
	checkDataSize(8);

	writeU64(&m_data[m_read_offset], src);

	m_read_offset += 8;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(float src)
{
	checkDataSize(4);

	writeF(&m_data[m_read_offset], src, m_protocol_version);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(bool& dst)
{
	checkReadOffset(m_read_offset, 1);

	dst = readU8(&m_data[m_read_offset]);

	m_read_offset += 1;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(u8& dst)
{
	checkReadOffset(m_read_offset, 1);

	dst = readU8(&m_data[m_read_offset]);

	m_read_offset += 1;
	return *this;
}

u8 NetworkPacket::getU8(u32 offset)
{
	checkReadOffset(offset, 1);

	return readU8(&m_data[offset]);
}

u8* NetworkPacket::getU8Ptr(u32 from_offset)
{
	if (m_datasize == 0) {
		return NULL;
	}

	checkReadOffset(from_offset, 1);

	return (u8*)&m_data[from_offset];
}

NetworkPacket& NetworkPacket::operator>>(uint16_t& dst)
{
	checkReadOffset(m_read_offset, 2);

	dst = readU16(&m_data[m_read_offset]);

	m_read_offset += 2;
	return *this;
}

uint16_t NetworkPacket::getU16(u32 from_offset)
{
	checkReadOffset(from_offset, 2);

	return readU16(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(u32& dst)
{
	checkReadOffset(m_read_offset, 4);

	dst = readU32(&m_data[m_read_offset]);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(u64& dst)
{
	checkReadOffset(m_read_offset, 8);

	dst = readU64(&m_data[m_read_offset]);

	m_read_offset += 8;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(float& dst)
{
	checkReadOffset(m_read_offset, 4);

	dst = readF(&m_data[m_read_offset], m_protocol_version);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v2f& dst)
{
	checkReadOffset(m_read_offset, 8);

	dst = readV2F(&m_data[m_read_offset], m_protocol_version);

	m_read_offset += 8;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v3f& dst)
{
	checkReadOffset(m_read_offset, 12);

	dst = readV3F(&m_data[m_read_offset], m_protocol_version);

	m_read_offset += 12;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(s16& dst)
{
	checkReadOffset(m_read_offset, 2);

	dst = readS16(&m_data[m_read_offset]);

	m_read_offset += 2;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(s16 src)
{
	*this << (uint16_t) src;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(s32& dst)
{
	checkReadOffset(m_read_offset, 4);

	dst = readS32(&m_data[m_read_offset]);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(s32 src)
{
	*this << (u32) src;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v3s16& dst)
{
	checkReadOffset(m_read_offset, 6);

	dst = readV3S16(&m_data[m_read_offset]);

	m_read_offset += 6;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v2s32& dst)
{
	checkReadOffset(m_read_offset, 8);

	dst = readV2S32(&m_data[m_read_offset]);

	m_read_offset += 8;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v3s32& dst)
{
	checkReadOffset(m_read_offset, 12);

	dst = readV3S32(&m_data[m_read_offset]);

	m_read_offset += 12;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(v2f src)
{
	*this << (float) src.X;
	*this << (float) src.Y;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(v3f src)
{
	*this << (float) src.X;
	*this << (float) src.Y;
	*this << (float) src.Z;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(v3s16 src)
{
	*this << (s16) src.X;
	*this << (s16) src.Y;
	*this << (s16) src.Z;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(v2s32 src)
{
	*this << (s32) src.X;
	*this << (s32) src.Y;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(v3s32 src)
{
	*this << (s32) src.X;
	*this << (s32) src.Y;
	*this << (s32) src.Z;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(video::SColor& dst)
{
	checkReadOffset(m_read_offset, 4);

	dst = readARGB8(&m_data[m_read_offset]);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(video::SColor src)
{
	checkDataSize(4);

	writeU32(&m_data[m_read_offset], src.color);

	m_read_offset += 4;
	return *this;
}

//useful ONLY for proxy
void NetworkPacket::setPlayerID(uint16_t ID) {
	m_using_player_id = true;
	m_player_id = ID;
	m_to_sum = 2;
}

void NetworkPacket::BufferStart(u8 value) {
	m_buffer_start = (int)value;
}

void NetworkPacket::setReliableOpt(bool rel) {
	if (rel != reliable)
		return;
	reliable = rel;
	r_set = true;
}

bool NetworkPacket::reliableOption() {
	return reliable;
}

bool NetworkPacket::didSetReliableOpt() {
	return r_set;
}

bool NetworkPacket::didSetChannel() {
	return set_channel;
}

void NetworkPacket::setChannel(u8 chan) {
	set_channel = true;
	channel = chan;
}

u8 NetworkPacket::getChannel() {
	return channel;
}

Buffer<u8> NetworkPacket::oldForgePacket()
{
	Buffer<u8> sb(m_datasize + 2 + m_to_sum);
	writeU16(&sb[0], m_command);
	//verbosestream << "BUFFER: CMD=" << m_command << ", " << "PID=" << m_player_id << std::endl;
	if (m_to_sum > 0)
		writeU16(&sb[2], m_player_id);
	memcpy(&sb[2 + m_to_sum], m_data.data() + m_buffer_start, m_datasize);
	return sb;
}
