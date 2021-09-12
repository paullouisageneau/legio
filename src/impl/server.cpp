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

#ifndef __EMSCRIPTEN__

#include "server.hpp"
#include "node.hpp"

#include <rtc/websocketserver.hpp>

#include <chrono>
#include <iostream>
#include <random>
#include <sstream>

namespace {

using std::string;

string random_string(size_t length) {
	static const string characters =
	    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::default_random_engine rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, characters.size() - 1);
	string ret;
	ret.reserve(length);
	for (size_t i = 0; i < length; ++i) {
		int r = dist(rng);
		ret += characters[r];
	}
	return ret;
}

} // namespace

namespace legio::impl {

using namespace std::placeholders;
using namespace std::chrono_literals;
using std::chrono::duration_cast;

Server::Server(const Configuration &config, Node *node)
    : Component(node),
      mTransport(std::make_unique<Transport>(node, Message::Provisioning,
                                             std::bind(&Server::provision, this, _1, _2))),
      mTurnServer(nullptr) {
	try {
		if (!config.port)
			throw std::logic_error("Server requires the port to be set");

		juice_server_config_t jconfig = {};
		jconfig.port = *config.port;
		// TODO: allocations
		mTurnServer = juice_server_create(&jconfig);
		if (!mTurnServer)
			throw std::runtime_error("TURN server creation failed");

		try {
			rtc::WebSocketServer::Configuration webSocketServerConfig;
			webSocketServerConfig.port = *config.port;
			// webSocketServerConfig.enableTls = true;
			// webSocketServerConfig.certificatePemFile = ...
			// webSocketServerConfig.keyPemFile = ...
			mWebSocketServer =
			    std::make_shared<rtc::WebSocketServer>(std::move(webSocketServerConfig));

			mWebSocketServer->onClient([this, node](shared_ptr<rtc::WebSocket> webSocket) {
				std::cout << "Incoming WebSocket from "
				          << webSocket->remoteAddress().value_or("[unknown]") << std::endl;
				webSocket->onOpen([this, node, webSocket]() {
					std::cout << "Incoming WebSocket open" << std::endl;
					node->routing->addChannel(webSocket);
				});
			});

		} catch (const std::exception &e) {
			throw std::runtime_error(string("WebSocket server creation failed: ") + e.what());
		}

		if (config.host) {
			node->graph->addLocalProvision(State::HasWebSocket);
			node->graph->addLocalProvision(State::HasTurn);
		}

	} catch (...) {
		if (mTurnServer)
			juice_server_destroy(mTurnServer);

		throw;
	}
}

Server::~Server() { juice_server_destroy(mTurnServer); }

void Server::update() {}

void Server::notify(const events::variant &event) {}

void Server::setupTurnCredentials(const TurnCredentials &credentials) {
	juice_server_credentials_t creds = {};
	creds.username = credentials.username.c_str();
	creds.password = credentials.password.c_str();
	// TODO: allocations

	const auto lifetime = TurnCredentials::clock::now() - credentials.expiration;
	const auto milliseconds = duration_cast<std::chrono::milliseconds>(lifetime);
	if (juice_server_add_credentials(mTurnServer, &creds, milliseconds.count()) < 0)
		throw std::runtime_error("Failed to add TURN credentials");
}

Server::TurnCredentials Server::generateTurnCredentials() {
	if (mCachedTurnCredentials && mCachedTurnCredentials->age() < 10min)
		return *mCachedTurnCredentials;

	auto credentials = TurnCredentials::Generate();
	setupTurnCredentials(credentials);
	mCachedTurnCredentials.emplace(credentials);
	return credentials;
}

void Server::provision(Identifier remoteId, binary payload) {
	auto [type] = unpack_strings<1>(payload);
	if (type != "turn")
		return;

	if (!mConfig.host)
		return;

	std::ostringstream host;
	host << *mConfig.host << ':' << *mConfig.port;

	auto credentials = generateTurnCredentials();

	mTransport->send(std::move(remoteId),
	                 pack_strings<4>({"turn", host.str(), std::move(credentials.username),
	                                  std::move(credentials.password)}));
}

Server::TurnCredentials Server::TurnCredentials::Generate() {
	const auto now = clock::now();
	const auto lifetime = 6h;
	return TurnCredentials{random_string(8), random_string(8), now, now + lifetime};
}

} // namespace legio::impl

#endif
