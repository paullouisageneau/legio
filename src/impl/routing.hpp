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

#ifndef LEGIO_IMPL_ROUTING_H
#define LEGIO_IMPL_ROUTING_H

#include "common.hpp"
#include "component.hpp"
#include "message.hpp"
#include "routingtable.hpp"

#include <rtc/channel.hpp>

#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

namespace legio::impl {

using rtc::Channel;

class Routing final : public Component {
public:
	Routing(Node *node);
	~Routing();

	Identifier localId() const;

	void update() override;
	void notify(const events::variant &event) override;

	void addChannel(shared_ptr<Channel> channel);
	void removeChannel(shared_ptr<Channel> channel);

	void addNeighbor(const Identifier &remoteId, shared_ptr<Channel> channel);
	void removeNeighbor(const Identifier &remoteId, shared_ptr<Channel> channel);
	bool hasNeighbor(const Identifier &remoteId) const;
	std::set<Identifier> neighbors() const;

	void send(message_ptr message);
	void broadcast(message_ptr message, shared_ptr<Channel> from = nullptr);

	shared_ptr<RoutingTable> table() const;
	void setTable(shared_ptr<RoutingTable> table);

private:
	void route(message_ptr message, shared_ptr<Channel> from);
	shared_ptr<Channel> findRoute(const Identifier &destination);

	shared_ptr<RoutingTable> mTable;
	std::unordered_set<shared_ptr<Channel>> mChannels;
	std::unordered_map<Identifier, shared_ptr<Channel>, Identifier::hash> mNeighbors;
	mutable std::shared_mutex mMutex;
};

} // namespace legio::impl

#endif
