/**
 * Copyright (c) 2021 Paul-Louis Ageneau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef LEGIO_IMPL_SERVER_H
#define LEGIO_IMPL_SERVER_H

#ifndef __EMSCRIPTEN__

#include "common.hpp"
#include "component.hpp"
#include "configuration.hpp"
#include "transport.hpp"

#include <juice/juice.h>
#include <rtc/websocketserver.hpp>

#include <chrono>

namespace legio::impl {

class Server final : public Component {
public:
	Server(const Configuration &config, Node *node);
	~Server();

	void update();
	void notify(const events::variant &event);

private:
	struct TurnCredentials {
		using clock = std::chrono::system_clock;
		using time_point = std::chrono::time_point<clock>;

		static TurnCredentials Generate();

		string username;
		string password;
		time_point creation;
		time_point expiration;

		clock::duration age() const {
			return std::max(clock::now() - creation, clock::duration::zero());
		}
	};

	TurnCredentials generateTurnCredentials();
	void setupTurnCredentials(const TurnCredentials &credentials);
	void provision(Identifier remoteId, binary payload);

	shared_ptr<rtc::WebSocketServer> mWebSocketServer;
	juice_server_t *mTurnServer;
	optional<TurnCredentials> mCachedTurnCredentials;
	unique_ptr<Transport> mTransport;
};

} // namespace legio::impl

#endif

#endif
