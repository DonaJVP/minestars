/*
 * MineStars - MultiCraft - Minetest/Luanti
 * Copyright (C) 2025 Logiki, Donatto J. Viveros. P. <donatto555@gmail.com>
 * Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "../server.h"
#include "../network/networkpacket.h"
#include "../log.h"
#include "../remoteplayer.h"
#include "../player.h"
#include "../content/mods.h"
#include "../server/mods.h"
#include "../ServerNetworkEngine.h"
#include "../filesys.h"
#include "../util/sha1.h"
#include "../util/base64.h"
#include "../addons/addons.hpp"
#include <cstdint>
#include <string>

struct SendableMedia
{
    std::string name;
    std::string path;
    std::string data;

    SendableMedia(const std::string &name_="", const std::string &path_="",
                  const std::string &data_=""):
                  name(name_),
                  path(path_),
                  data(data_)
                  {}
};

//BEGIN MODEL_MANIPULATION

// Hacks because I don't want to make duplicate read/write functions for little
// endian numbers.
u32 readU32_le(std::istream &is) {
    char buf[4] = {0};
    is.read(buf, sizeof(buf));
    std::reverse(buf, buf + sizeof(buf));
    return readU32((u8 *)buf);
}

v3f readV3F32_le(std::istream &is) {
    char buf[12] = {0};
    is.read(buf, sizeof(buf));
    std::reverse(buf, buf + 4);
    std::reverse(buf + 4, buf + 8);
    std::reverse(buf + 8, buf + 12);
    return readV3F32((u8 *)buf);
}

void writeV3F32_le(std::ostream &os, v3f pos) {
    char buf[12];
    writeV3F32((u8 *)buf, pos);
    std::reverse(buf, buf + 4);
    std::reverse(buf + 4, buf + 8);
    std::reverse(buf + 8, buf + 12);
    os.write(buf, sizeof(buf));
}

// Converts MT 5+ player models into MT 0.4 compatible models
std::string makeCompatPlayerModel(std::string b3d) {
    std::stringstream ss(b3d);

    // ss.read(4) != "BB3D"
    const u32 header = readU32_le(ss);
    if (header != 0x44334242) {
        warningstream << "Invalid B3D header in player model: " << header << std::endl;
        return "";
    }

    readU32(ss); // Length
    readU32(ss); // Version

    // Look for the node
    while (ss.good()) {
        const u32 name = readU32_le(ss);
        const u32 length = readU32_le(ss);

        // name != "NODE"
        if (name != 0x45444f4e) {
            ss.ignore(length);
            continue;
        }

        // Node name
        ss.ignore(length, '\x00');

        // Node position
        std::streampos p = ss.tellg();
        const v3f offset_pos = readV3F32_le(ss) - v3f(0, BS, 0);

        // Write the new position back to the stringstream
        ss.seekp(p);
        writeV3F32_le(ss, offset_pos);

        return ss.str();
    }

    warningstream << "Could not find base position in B3D file" << std::endl;
    return "";
}

//END MODEL_MANIPULATION

bool Server::dynamicAddMedia(const std::string &filepath, std::vector<RemotePlayer*> &sent_to)
{
    if (ServersNetworkObject->AreSlave) {
        errorstream << "Illegal use of 'dynamicAddMedia' on a Slave server, returning." << std::endl;
        return false;
    }
    std::string filename = fs::GetFilenameFromPath(filepath.c_str());
    if (m_media.find(filename) != m_media.end()) {
        errorstream << "Server::dynamicAddMedia(): file \"" << filename
        << "\" already exists in media cache" << std::endl;
        return false;
    }
    // Load the file and add it to our media cache
    std::string filedata, raw_hash;
    bool ok = addMediaFile(filename, filepath, &filedata, &raw_hash);
    if (!ok)
        return false;
    // Push file to existing clients
    NetworkPacket pkt(TOCLIENT_MEDIA_PUSH, 0);
    pkt << raw_hash << filename << (bool) true;
    pkt.putLongString(filedata);

    m_clients.lock();
    for (auto &pair : m_clients.getClientList()) {
        if (pair.second->getState() < CS_DefinitionsSent)
            continue;
        if (pair.second->net_proto_version < 39)
            continue;

        if (auto player = m_env->getPlayer(pair.second->peer_id))
            sent_to.emplace_back(player);
        /*
         *		FIXME: this is a very awful hack
         *		The network layer only guarantees ordered delivery inside a channel.
         *		Since the very next packet could be one that uses the media, we have
         *		to push the media over ALL channels to ensure it is processed before
         *		it is used.
         *		In practice this means we have to send it twice:
         *		- channel 1 (HUD)
         *		- channel 0 (everything else: e.g. play_sound, object messages)
         */
        m_clients.send(pair.second->peer_id, 1, &pkt, true);
        m_clients.send(pair.second->peer_id, 0, &pkt, true);
    }
    m_clients.unlock();
    return true;
}

//Only used on proxy
void Server::sendMediaAnnouncement(uint16_t peer_id, const std::string &lang_code)
{
    const u16 protocol_version = m_clients.getProtocolVersion(peer_id);


    // Make packet
    NetworkPacket pkt(TOCLIENT_ANNOUNCE_MEDIA, 0, peer_id);

    u16 media_sent = 0;
    std::string lang_suffix;
    lang_suffix.append(".").append(lang_code).append(".tr");
    for (const auto &i : m_media) {
        if (str_ends_with(i.first, ".tr") && !str_ends_with(i.first, lang_suffix))
            continue;
        if (str_ends_with(i.first, ".tr.e") && !str_ends_with(i.first, lang_suffix + ".e"))
            continue;
        // Skip dummy entries on 5.0+ clients
        if (protocol_version >= 37 && i.second.sha1_digest.empty())
            continue;
        media_sent++;
    }

    std::unordered_map<std::string, MediaInfo> external_media = ServersNetworkObject->getMedia();
    for (const auto &i: external_media) {
        if (str_ends_with(i.first, ".tr") && !str_ends_with(i.first, lang_suffix))
            continue;
        if (str_ends_with(i.first, ".tr.e") && !str_ends_with(i.first, lang_suffix + ".e"))
            continue;
        if (m_media.find(i.first) != m_media.end()) //Somehow some media continues appearing here
            continue;
        if ((u32)media_sent + 1 >= UINT16_MAX)
            FATAL_ERROR("Media Overflow");
        media_sent++;
    }

    pkt << media_sent;

    for (const auto &i : m_media) {
        if (str_ends_with(i.first, ".tr") && !str_ends_with(i.first, lang_suffix))
            continue;
        if (str_ends_with(i.first, ".tr.e") && !str_ends_with(i.first, lang_suffix + ".e"))
            continue;
        if (protocol_version >= 37 && i.second.sha1_digest.empty())
            continue;

        pkt << i.first;

        if (protocol_version < 37 &&
            m_compat_media.find(i.first) != m_compat_media.end()) {
            pkt << m_compat_media[i.first].sha1_digest;
            } else {
                FATAL_ERROR_IF(i.second.sha1_digest.empty(), "Attempt to send dummy media");
                pkt << i.second.sha1_digest;
            }
    }


    for (const auto &i: external_media) {
        if (str_ends_with(i.first, ".tr") && !str_ends_with(i.first, lang_suffix))
            continue;
        if (str_ends_with(i.first, ".tr.e") && !str_ends_with(i.first, lang_suffix + ".e"))
            continue;
        verbosestream << "MediaVerifSend: " << i.first << ", sha=" << i.second.sha1_digest << std::endl;
        pkt << i.first;
        pkt << i.second.sha1_digest;
    }

    pkt << g_settings->get("remote_media");
    if (g_settings->getBool("disable_texture_packs"))
        pkt << true;

    Send(&pkt);

    verbosestream << "Server: Announcing files to id(" << peer_id
    << "): count=" << media_sent << " size=" << pkt.getSize() << std::endl;
    //warningstream << "Took " << timer.getTimerTime() << "ms" << std::endl;
}

void Server::sendRequestedMedia(session_t peer_id, const std::vector<std::string> &tosend) {
    verbosestream<<"Server::sendRequestedMedia(): " <<"Sending files to client"<<std::endl;

    /* Read files */

    // Put 5kB in one bunch (this is not accurate)
    u32 bytes_per_bunch = 5000;

    std::vector< std::vector<SendableMedia> > file_bunches;
    file_bunches.emplace_back();

    u32 file_size_bunch_total = 0;

    std::unordered_map<std::string, MediaInfo> external_media = ServersNetworkObject->getMedia();

    const u16 protocol_version = m_clients.getProtocolVersion(peer_id);
    for (const std::string &name : tosend) {
        if ((m_media.find(name) == m_media.end()) && !ServersNetworkObject->HasMedia(name)) {
            errorstream<<"Server::sendRequestedMedia(): Client asked for "
            <<"unknown file \""<<(name)<<"\""<<std::endl;
            continue;
        }

        //TODO get path + name
        std::string tpath = m_media[name].path;
        if (tpath.empty() && ServersNetworkObject->HasMedia(name)) {
            tpath = external_media[name].path;
        }

        // Use compatibility media on older clients
        if (protocol_version < 37 &&
            m_compat_media.find(name) != m_compat_media.end()) {
            file_bunches[file_bunches.size()-1].emplace_back(name, tpath,
                                                             m_compat_media[name].data);
            continue;
            }

            if (tpath.empty()) {
                errorstream<<"Server::sendRequestedMedia(): New client asked for "
                <<"compatibility media file \""<<(name)<<"\""<<std::endl;
                continue;
            }

            // Read data
            std::ifstream fis(tpath.c_str(), std::ios_base::binary);
            if(!fis.good()){
                errorstream<<"Server::sendRequestedMedia(): Could not open \""
                <<tpath<<"\" for reading"<<std::endl;
                continue;
            }
            std::ostringstream tmp_os(std::ios_base::binary);
            bool bad = false;
            for(;;) {
                char buf[1024];
                fis.read(buf, 1024);
                std::streamsize len = fis.gcount();
                tmp_os.write(buf, len);
                file_size_bunch_total += len;
                if(fis.eof())
                    break;
                if(!fis.good()) {
                    bad = true;
                    break;
                }
            }
            if (bad) {
                errorstream<<"Server::sendRequestedMedia(): Failed to read \""
                <<name<<"\""<<std::endl;
                continue;
            }
            infostream<<"Server::sendRequestedMedia(): Loaded \""<<name<<"\""<<std::endl; //debug
            // Put in list
            file_bunches[file_bunches.size()-1].emplace_back(name, tpath, tmp_os.str());
            // Start next bunch if got enough data
            if(file_size_bunch_total >= bytes_per_bunch) {
                file_bunches.emplace_back();
                file_size_bunch_total = 0;
            }

    }

    /* Create and send packets */
    u16 num_bunches = file_bunches.size();
    for (u16 i = 0; i < num_bunches; i++) {
        /*
            *		u16 command
            *		u16 total number of texture bunches
            *		u16 index of this bunch
            *		u32 number of files in this bunch
            *		for each file {
            *			u16 length of name
            *			string name
            *			u32 length of data
            *			data
            *      }
        */
        NetworkPacket pkt(TOCLIENT_MEDIA, 4 + 0, peer_id);
        pkt << num_bunches << i << (u32) file_bunches[i].size();

        for (const SendableMedia &j : file_bunches[i]) {
            pkt << j.name;
            pkt.putLongString(j.data);
        }

        verbosestream << "Server::sendRequestedMedia(): bunch "
        << i << "/" << num_bunches
        << " files=" << file_bunches[i].size()
        << " size="  << pkt.getSize() << std::endl;
        Send(&pkt);
    }
}

void Server::fillMediaCache()
{
    infostream << "Server: Calculating media file checksums" << std::endl;

    // Collect all media file paths
    std::vector<std::string> paths;
    // The paths are ordered in descending priority
    fs::GetRecursiveDirs(paths, porting::path_user + DIR_DELIM + "textures" + DIR_DELIM + "server");
    fs::GetRecursiveDirs(paths, m_gamespec.path + DIR_DELIM + "textures");
    //m_modmgr->getModsMediaPaths(paths); // OBSOLETE.
    // Insert each.
    for (const std::string &i: sAddons->getMediaPaths())
        paths.push_back(i);

    // Collect media file information from paths into cache
    for (const std::string &mediapath : paths) {
        std::vector<fs::DirListNode> dirlist = fs::GetDirListing(mediapath);
        for (const fs::DirListNode &dln : dirlist) {
            if (dln.dir) // Ignore dirs (already in paths)
                continue;

            const std::string &filename = dln.name;
            if (m_media.find(filename) != m_media.end()) // Do not override
                continue;

            std::string filepath = mediapath;
            filepath.append(DIR_DELIM).append(filename);
            addMediaFile(filename, filepath);
        }
    }

    infostream << "Server: " << m_media.size() << " media files collected" << std::endl;
}

bool Server::addMediaFile(const std::string &filename, const std::string &filepath, std::string *filedata_to, std::string *digest_to) {
    // If name contains illegal characters, ignore the file
    if (!string_allowed(filename, TEXTURENAME_ALLOWED_CHARS)) {
        infostream << "Server: ignoring illegal file name: \""
        << filename << "\"" << std::endl;
        return false;
    }
    // If name is not in a supported format, ignore it
    const char *supported_ext[] = {
        ".png", ".jpg", ".bmp", ".tga",
        ".pcx", ".ppm", ".psd", ".wal", ".rgb",
        ".ogg",
        ".x", ".b3d", ".md2", ".obj",
        // Custom translation file format
        ".tr",
        ".e",
        NULL
    };
    if (removeStringEnd(filename, supported_ext).empty()) {
        infostream << "Server: ignoring unsupported file extension: \""
        << filename << "\"" << std::endl;
        return false;
    }
    // Ok, attempt to load the file and add to cache

    // Read data
    std::string filedata;
    if (!fs::ReadFile(filepath, filedata)) {
        errorstream << "Server::addMediaFile(): Failed to open \""
        << filepath << "\" for reading" << std::endl;
        return false;
    }

    if (filedata.empty()) {
        errorstream << "Server::addMediaFile(): Empty file \""
        << filepath << "\"" << std::endl;
        return false;
    }

    SHA1 sha1;
    sha1.addBytes(filedata.c_str(), filedata.length());

    unsigned char *digest = sha1.getDigest();
    std::string sha1_base64 = base64_encode(digest, 20);
    if (digest_to)
        *digest_to = std::string((char*) digest, 20);
    free(digest);

    // Put in list
    m_media[filename] = MediaInfo(filepath, sha1_base64);

    // Add a compatibility model if required
    if (isCompatPlayerModel(filename)) {
        // Offset the mesh
        const std::string filedata_compat = makeCompatPlayerModel(filedata);
        if (filedata_compat != "") {
            SHA1 sha1;
            sha1.addBytes(filedata_compat.c_str(), filedata_compat.length());
            unsigned char *digest = sha1.getDigest();
            std::string sha1_base64 = base64_encode(digest, 20);
            free(digest);

            // If the original model is being sent then rename the
            // compatibility one so it doesn't conflict. The renamed model is
            // used in player_sao.cpp if the setting is enabled.
            std::string fn_compat = filename;
            if (g_settings->getBool("compat_send_original_model")) {
                fn_compat = "_mc_compat_" + fn_compat;

                // Add a dummy m_media entry
                m_media[fn_compat] = MediaInfo("", "");
            }

            m_compat_media[fn_compat] = InMemoryMediaInfo(filedata_compat, sha1_base64);
        }
    }
    if (filedata_to)
        *filedata_to = std::move(filedata);
    return true;
}
