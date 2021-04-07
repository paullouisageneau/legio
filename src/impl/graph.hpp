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

#ifndef LEGIO_IMPL_NETWORK_STATE_H
#define LEGIO_IMPL_NETWORK_STATE_H

#include "common.hpp"
#include "identifier.hpp"
#include "state.hpp"
#include "routing.hpp"
#include "routingtable.hpp"

#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace legio::impl {

class Graph final : public Component {
public:
	Graph(Node *node);
	~Graph();

	void update() override;
	void notify(const events::variant &event) override;

	Ecdh localEcdhPair() const;

	bool insert(State state);
	const State get(Identifier nodeId) const;
	std::vector<Identifier> nodes() const;
	int count() const;

	shared_ptr<RoutingTable> routingTable() const;

private:
	void broadcastHello();
	void broadcastState();

	struct Vertice {
		Vertice(Identifier _id) : mIdentifier(std::move(_id)) {}

		inline const Identifier &id() const { return mIdentifier; }

		optional<State> state;
		std::unordered_map<Identifier, shared_ptr<Vertice>, Identifier::hash> edges;

		shared_ptr<Vertice> nextHop;
		int distance = -1;
		bool visited = false;

	private:
		Identifier mIdentifier;
	};

	shared_ptr<Vertice> findVertice(const Identifier &id) const;
	bool updateVertice(State state);
	bool updateEdges(const Identifier &id, const std::set<Identifier> &neighbors);
	void computeRoutingTable();

	shared_ptr<RoutingTable> mRoutingTable;
	std::unordered_map<Identifier, shared_ptr<Vertice>, Identifier::hash> mVertices;

	Ecdh mEcdh;

	uint32_t mHelloSequence = 0;
	uint32_t mStateSequence = 0;

	mutable std::shared_mutex mMutex;
};

} // namespace legio::impl

#endif
