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

#include "networking.hpp"
#include "node.hpp"

#include <rtc/websocket.hpp>

#include <algorithm>
#include <iostream>
#include <random>

namespace legio::impl {

using namespace std::placeholders;

Networking::Networking(Node *node)
    : Component(node),
      mTransport(std::make_shared<Transport>(node, Message::Signaling,
                                             std::bind(&Networking::receive, this, _1, _2))) {}

Networking::~Networking() {}

bool Networking::isConnected() const {
	std::unique_lock lock(mMutex);
	for (const auto &[id, peering] : mPeerings)
		if (peering->isConnected())
			return true;

	return false;
}

void Networking::update() {
	std::unique_lock lock(mMutex);
	const int targetPeeringCount = 4;
	if (int(mPeerings.size()) < targetPeeringCount) {
		auto nodes = node()->routing->table()->nodes();
		std::default_random_engine rng(std::random_device{}());
		std::shuffle(nodes.begin(), nodes.end(), rng);
		for (const auto &id : nodes) {
			if (mPeerings.find(id) == mPeerings.end()) {
				auto peering = createPeering(id);
				peering->connect();
				break;
			}
		}
	}
}

void Networking::notify(const events::variant &event) {}

void Networking::connectWebSocket(const string &url) {
	std::unique_lock lock(mMutex);
	std::cout << "Outgoing WebSocket to " << url << std::endl;
	auto webSocket = std::make_shared<rtc::WebSocket>();

	webSocket->onOpen([this, webSocket]() {
		std::cout << "Outgoing WebSocket opened" << std::endl;
		node()->routing->addChannel(webSocket);
	});

	webSocket->onClosed([this, webSocket]() {
		std::cout << "Outgoing WebSocket closed" << std::endl;
		node()->routing->removeChannel(webSocket);
	});

	webSocket->onError(
	    [](const string &error) { std::cerr << "WebSocket error: " << error << std::endl; });

	webSocket->open(url);
}

void Networking::connectPeer(Identifier remoteId) {
	std::unique_lock lock(mMutex);
	auto peering = createPeering(std::move(remoteId));
	peering->connect();
}

void Networking::receive(Identifier remoteId, binary payload) {
	std::unique_lock lock(mMutex);
	auto peering = createPeering(std::move(remoteId));
	peering->receive(std::move(payload));
}

shared_ptr<Peering> Networking::createPeering(Identifier remoteId) {
	// mMutex must be locked
	auto it = mPeerings.find(remoteId);
	if (it != mPeerings.end())
		return it->second;

	std::cout << "Creating peering for " << to_base64url(remoteId) << std::endl;

	auto peering = std::make_shared<Peering>(node()->routing, node()->provisioning, mTransport, remoteId);
	mPeerings.emplace(std::move(remoteId), peering);
	return peering;
}

} // namespace legio::impl
