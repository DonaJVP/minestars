/*
 M i*netest
 Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "../server.h"
#include "../ServerNetworkEngine.h"

void Server::ShutdownState::reset()
{
    m_timer = 0.0f;
    message.clear();
    should_reconnect = false;
    is_requested = false;
}

void Server::ShutdownState::trigger(float delay, const std::string &msg, bool reconnect)
{
    m_timer = delay;
    message = msg;
    should_reconnect = reconnect;
}

void Server::ShutdownState::tick(float dtime, Server *server)
{
    if (m_timer <= 0.0f)
        return;

    // Timed shutdown
    static const float shutdown_msg_times[] =
    {
        1, 2, 3, 4, 5, 10, 20, 40, 60, 120, 180, 300, 600, 1200, 1800, 3600
    };

    // Automated messages
    if (m_timer < shutdown_msg_times[ARRLEN(shutdown_msg_times) - 1]) {
        for (float t : shutdown_msg_times) {
            // If shutdown timer matches an automessage, shot it
            if (m_timer > t && m_timer - dtime < t) {
                std::wstring periodicMsg = getShutdownTimerMessage();

                infostream << wide_to_utf8(periodicMsg).c_str() << std::endl;
                server->SendChatMessage(PEER_ID_INEXISTENT, periodicMsg);
                break;
            }
        }
    }

    m_timer -= dtime;
    if (m_timer < 0.0f) {
        m_timer = 0.0f;
        is_requested = true;
    }
}

std::wstring Server::ShutdownState::getShutdownTimerMessage() const
{
    std::wstringstream ws;
    ws << L"*** Server shutting down in "
    << duration_to_string(myround(m_timer)).c_str() << ".";
    return ws.str();
}

void Server::requestShutdown(const std::string &msg, bool reconnect, float delay)
{
    if (delay == 0.0f) {
        // No delay, shutdown immediately
        m_shutdown_state.is_requested = true;
        // only print to the infostream, a chat message saying
        // "Server Shutting Down" is sent when the server destructs.
        infostream << "*** Immediate Server shutdown requested." << std::endl;
    } else if (delay < 0.0f && m_shutdown_state.isTimerRunning()) {
        // Negative delay, cancel shutdown if requested
        m_shutdown_state.reset();
        std::wstringstream ws;

        ws << L"*** Server shutdown canceled.";

        infostream << wide_to_utf8(ws.str()).c_str() << std::endl;
        SendChatMessage(PEER_ID_INEXISTENT, ws.str());
        // m_shutdown_* are already handled, skip.
        return;
    } else if (delay > 0.0f) {
        // Positive delay, tell the clients when the server will shut down
        std::wstringstream ws;

        ws << L"*** Server shutting down in "
        << duration_to_string(myround(delay)).c_str()
        << ".";

        infostream << wide_to_utf8(ws.str()).c_str() << std::endl;
        SendChatMessage(PEER_ID_INEXISTENT, ws.str());
    }

    m_shutdown_state.trigger(delay, msg, reconnect);
}
