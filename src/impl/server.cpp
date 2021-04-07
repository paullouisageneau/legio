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

#include <iostream>

namespace legio::impl {

Server::Server(const Configuration &config, Node *node) : Component(node) {
	try {
		rtc::WebSocketServer::Configuration webSocketServerConfig;
		webSocketServerConfig.port = config.port.value_or(0);
		// webSocketServerConfig.enableTls = true;
		// webSocketServerConfig.certificatePemFile = ...
		// webSocketServerConfig.keyPemFile = ...
		mWebSocketServer = std::make_shared<rtc::WebSocketServer>(std::move(webSocketServerConfig));

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
}

Server::~Server() {}

void Server::update() {}

void Server::notify(const events::variant &event) {}

} // namespace legio::impl

#endif
