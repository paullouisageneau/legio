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

#include "routing.hpp"
#include "node.hpp"

#include <iostream>

namespace legio::impl {

Routing::Routing(Node *node) : Component(node), mTable(std::make_shared<RoutingTable>()) {}

Routing::~Routing() {}

Identifier Routing::localId() const { return node()->id(); }

void Routing::update() {}

void Routing::notify(const events::variant &event) {}

void Routing::addChannel(shared_ptr<Channel> channel) {
	std::unique_lock lock(mMutex);

	channel->onMessage(
	    [this, channel](rtc::binary data) {
		    // This can be called on non-main thread
		    try {
			    auto message = std::make_shared<Message>(data);
			    route(std::move(message), channel);

		    } catch (const std::exception &e) {
			    std::cerr << "Invalid message: " << e.what() << std::endl;
		    }
	    },
	    [](rtc::string data) { std::cerr << "Unexpected non-binary message" << std::endl; });

	mChannels.emplace(std::move(channel));
}

void Routing::removeChannel(shared_ptr<Channel> channel) {
	std::unique_lock lock(mMutex);

	// Remove channel
	if (auto it = mChannels.find(channel); it != mChannels.end()) {
		auto channel = *it;
		channel->onMessage([](const binary &data) {}, [](const string &data) {});
		mChannels.erase(it);
	}

	// Remove links using channel
	auto it = mNeighbors.begin();
	while (it != mNeighbors.end()) {
		if (it->second == channel)
			it = mNeighbors.erase(it);
		else
			++it;
	}
}

void Routing::addNeighbor(const Identifier &remoteId, shared_ptr<Channel> channel) {
	std::unique_lock lock(mMutex);
	if (mChannels.find(channel) != mChannels.end()) {
		if (mNeighbors.find(remoteId) == mNeighbors.end()) {
			mNeighbors.emplace(remoteId, channel);
			lock.unlock();
			emit(events::Neighbor{remoteId, channel});
		}
	}
}

void Routing::removeNeighbor(const Identifier &remoteId, shared_ptr<Channel> channel) {
	std::unique_lock lock(mMutex);
	auto it = mNeighbors.find(remoteId);
	if (it != mNeighbors.end() && it->second == channel) {
		mNeighbors.erase(it);
		lock.unlock();
		emit(events::Neighbor{remoteId, nullptr});
	}
}

bool Routing::hasNeighbor(const Identifier &remoteId) const {
	std::unique_lock lock(mMutex);
	return mNeighbors.find(remoteId) != mNeighbors.end();
}

std::set<Identifier> Routing::neighbors() const {
	std::unique_lock lock(mMutex);
	std::set<Identifier> result;
	for (const auto &[id, channel] : mNeighbors)
		result.insert(id);

	return result;
}

shared_ptr<RoutingTable> Routing::table() const {
	std::shared_lock lock(mMutex);
	return mTable;
}

void Routing::setTable(shared_ptr<RoutingTable> routingTable) {
	std::unique_lock lock(mMutex);
	mTable = std::move(routingTable);
}

void Routing::send(message_ptr message) {
	if (message->destination)
		route(message, nullptr);
	else
		broadcast(message);
}

void Routing::broadcast(message_ptr message, shared_ptr<Channel> from) {
	std::unique_lock lock(mMutex);

	for (const auto &channel : mChannels) {
		if (channel != from && channel->isOpen()) {
			try {
				channel->send(binary(*message));
			} catch (const std::exception &e) {
				std::cerr << e.what() << std::endl;
			}
		}
	}
}

void Routing::route(message_ptr message, shared_ptr<Channel> from) {
	if (!message->source)
		throw std::runtime_error("Missing message source");

	if (!message->destination || *message->destination == localId()) {
		emit(events::Message{message, from});
	} else {
		if (auto channel = findRoute(*message->destination))
			channel->send(binary(*message));
	}
}

shared_ptr<Channel> Routing::findRoute(const Identifier &remoteId) {
	std::shared_lock lock(mMutex);
	if (!mTable)
		return nullptr; // missing table

	auto nextHop = mTable->findNextHop(remoteId);
	if (!nextHop)
		return nullptr; // missing next hop

	auto it = mNeighbors.find(remoteId);
	if (it == mNeighbors.end())
		return nullptr; // missing channel for next hop

	return it->second;
}

} // namespace legio::impl
