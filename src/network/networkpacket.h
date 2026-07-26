/*
Minetest
Copyright (C) 2015 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#pragma once

#include "util/pointer.h"
#include "util/numeric.h"
#include "networkprotocol.h"
#include <SColor.h>
#include <cstdint>

class NetworkPacket
{

public:
	NetworkPacket(uint16_t command, u32 datasize, session_t peer_id, uint16_t protocol_version, uint8_t SrvID);
	NetworkPacket(uint16_t command, u32 datasize, session_t peer_id, uint16_t protocol_version);
	NetworkPacket(uint16_t command, u32 datasize, session_t peer_id);
	NetworkPacket(uint16_t command, u32 datasize);
	NetworkPacket() = default;

	~NetworkPacket();

	void putRawPacket(const u8 *data, u32 datasize, session_t peer_id);
	void clear();
	void updateData();

	// Increment size
	void increaseReadOffset(u32 sum);
	void incrementSize(u32 dtas);
	// Set raw data
	void resizeData(size_t sz);
	// Set peer id
	void setPeerID(session_t id);
	// Decrease read offset
	void decreaseReadOffset(u32 new_);
	// Decrease some bytes
	void decreaseFront();
	// Sets player ID, only for proxy to slave
	void setPlayerID(uint16_t ID);
	// Used ONLY to send packets to player [Compile data in a certain m_data position <start>]
	void BufferStart(u8 value);
	// Returns if the packet are reliable or not
	bool reliableOption();
	// Sets the packet reliable option
	void setReliableOpt(bool rel);
	// Asks nicely if the option reliable have been set
	bool didSetReliableOpt();
	
	bool didSetChannel();
	void setChannel(u8 chan);
	u8 getChannel();
	
	// Getters
	uint16_t getServerId() { return m_server_id; }
	u32 getSize() const { return m_datasize; }
	session_t getPeerId() const { return m_peer_id; }
	uint16_t getCommand() { return m_command; }
	const u32 getRemainingBytes() const { return m_datasize - m_read_offset; }
	const char *getRemainingString() { return getString(m_read_offset); }
	uint16_t getProtocolVersion() const { return m_protocol_version; }
	bool usingPlayerId() const { return m_using_player_id; }

	// Returns a c-string without copying.
	// A better name for this would be getRawString()
	const char *getString(u32 from_offset);
	// major difference to putCString(): doesn't write len into the buffer
	void putRawString(const char *src, u32 len);
	void putRawString(const std::string &src)
	{
		putRawString(src.c_str(), src.size());
	}

	NetworkPacket &operator>>(std::string &dst);
	NetworkPacket &operator<<(const std::string &src);

	void putLongString(const std::string &src);

	NetworkPacket &operator>>(std::wstring &dst);
	NetworkPacket &operator<<(const std::wstring &src);

	std::string readLongString();

	NetworkPacket &operator>>(char &dst);
	NetworkPacket &operator<<(char src);

	NetworkPacket &operator>>(bool &dst);
	NetworkPacket &operator<<(bool src);

	u8 getU8(u32 offset);

	NetworkPacket &operator>>(u8 &dst);
	NetworkPacket &operator<<(u8 src);

	u8 *getU8Ptr(u32 offset);

	uint16_t getU16(u32 from_offset);
	NetworkPacket &operator>>(uint16_t &dst);
	NetworkPacket &operator<<(uint16_t src);

	NetworkPacket &operator>>(u32 &dst);
	NetworkPacket &operator<<(u32 src);

	NetworkPacket &operator>>(u64 &dst);
	NetworkPacket &operator<<(u64 src);

	NetworkPacket &operator>>(float &dst);
	NetworkPacket &operator<<(float src);

	NetworkPacket &operator>>(v2f &dst);
	NetworkPacket &operator<<(v2f src);

	NetworkPacket &operator>>(v3f &dst);
	NetworkPacket &operator<<(v3f src);

	NetworkPacket &operator>>(s16 &dst);
	NetworkPacket &operator<<(s16 src);

	NetworkPacket &operator>>(s32 &dst);
	NetworkPacket &operator<<(s32 src);

	NetworkPacket &operator>>(v2s32 &dst);
	NetworkPacket &operator<<(v2s32 src);

	NetworkPacket &operator>>(v3s16 &dst);
	NetworkPacket &operator<<(v3s16 src);

	NetworkPacket &operator>>(v3s32 &dst);
	NetworkPacket &operator<<(v3s32 src);

	NetworkPacket &operator>>(video::SColor &dst);
	NetworkPacket &operator<<(video::SColor src);

	// Temp, we remove SharedBuffer when migration finished
	// ^ this comment has been here for 4 years
	Buffer<u8> oldForgePacket();

	inline void setProtocolVersion(const uint16_t protocol_version)
	{
		m_protocol_version = protocol_version;
	}

	void disassemble_pkt(u8 *var[UINT16_MAX]);

private:
	void checkReadOffset(u32 from_offset, u32 field_size);

	inline void checkDataSize(u32 field_size)
	{
		if (m_read_offset + field_size > m_datasize) {
			m_datasize = m_read_offset + field_size;
			m_data.resize(m_datasize);
		}
	}

	std::vector<u8> m_data;
	u32 m_datasize = 0;
	u32 m_read_offset = 0;
	uint16_t m_command = 0;
	session_t m_peer_id = 0;
	uint16_t m_protocol_version = 37;
	bool m_using_player_id = false;
	uint16_t m_player_id = 0;
	int m_to_sum = 0;
	int m_buffer_start = 0;
	uint8_t m_server_id = 0; //0 is default
	bool reliable = false;
	bool r_set = false;
	bool set_channel = false;
	u8 channel = 0;
};
