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

#include "graph.hpp"
#include "node.hpp"

#include <algorithm>
#include <deque>
#include <iostream>
#include <iterator>
#include <queue>

namespace legio::impl {

Graph::Graph(Node *node) : Component(node), mRoutingTable(std::make_shared<RoutingTable>()) {
	insert(State(node->ecdsaPair, mStateSequence - 1, mEcdh.publicKey()));
}

Graph::~Graph() {}

Ecdh Graph::localEcdhPair() const { return mEcdh; }

void Graph::update() {
	std::unique_lock lock(mMutex);
	broadcastHello();
}

void Graph::notify(const events::variant &event) {
	std::visit( //
	    overloaded{
	        [&](const events::Message &m) {
		        auto routing = node()->routing;
		        auto message = m.message;
		        auto channel = m.channel;
		        switch (message->type) {
		        case Message::Hello: {
			        std::cout << "New Hello from " << to_base64url(*message->source) << std::endl;
			        if (message->source && channel)
				        if (!routing->hasNeighbor(*message->source))
					        routing->addNeighbor(*message->source, channel);
			        break;
		        }
		        case Message::State: {
			        std::cout << "Got State from " << to_base64url(*message->source) << std::endl;
			        if (insert(State::FromMessage(message)))
				        routing->broadcast(message, channel);
			        break;
		        }
		        default: {
			        // Ignore
			        break;
		        }
		        }
	        },
	        [&](const events::Neighbor &n) {
		        auto routing = node()->routing;
		        auto neighbors = routing->neighbors();
		        std::cout << "Neighbors change, neighbors=" << neighbors.size() << std::endl;
		        if (updateEdges(node()->id(), neighbors))
			        broadcastState();
	        },
	    },
	    event);
}

bool Graph::insert(State state) {
	std::unique_lock lock(mMutex);
	return updateVertice(std::move(state));
}

const State Graph::get(Identifier nodeId) const {
	std::unique_lock lock(mMutex);
	auto it = mVertices.find(nodeId);
	if (it == mVertices.end())
		throw std::runtime_error("Attempted to get state for unknown node");

	if (!it->second->state)
		throw std::runtime_error("Unknown node state");

	return *it->second->state;
}

std::vector<Identifier> Graph::nodes() const {
	std::unique_lock lock(mMutex);
	std::vector<Identifier> result;
	result.reserve(mVertices.size());
	for (const auto &[id, vertice] : mVertices)
		if (vertice->state) // filter vertices with state
			result.push_back(id);

	return result;
}

int Graph::count() const {
	std::shared_lock lock(mMutex);
	return int(mVertices.size());
}

shared_ptr<RoutingTable> Graph::routingTable() const {
	std::shared_lock lock(mMutex);
	return mRoutingTable;
}

void Graph::broadcastHello() {
	auto message = make_message(Message::Hello, mHelloSequence++, binary(), node()->ecdsaPair);
	node()->routing->broadcast(std::move(message));
}

void Graph::broadcastState() {
	State localState(node()->id(), mStateSequence++, mEcdh.publicKey());
	auto neighbors = node()->routing->neighbors();
	for (const auto &id : neighbors)
		localState.neighbors.insert(id);

	auto message = localState.toMessage(node()->ecdsaPair);
	updateVertice(std::move(localState));
	node()->routing->broadcast(std::move(message));
}

shared_ptr<Graph::Vertice> Graph::findVertice(const Identifier &id) const {
	// mMutex needs to be locked

	auto it = mVertices.find(id);
	return it != mVertices.end() ? it->second : nullptr;
}

bool Graph::updateVertice(State state) {
	// mMutex needs to be uniquely locked

	shared_ptr<Vertice> vertice;
	if (auto it = mVertices.find(state.id()); it != mVertices.end()) {
		vertice = it->second;
		if (vertice->state && compare_sequence(state.sequence, vertice->state->sequence) <= 0)
			return false;

		if (!vertice->state || vertice->state->ecdhPublic != state.ecdhPublic)
			broadcastState(); // TODO: request broadcast method to limit rate

		vertice->state = std::move(state);

	} else {
		vertice = std::make_shared<Vertice>(state.id());
		vertice->state = std::move(state);
		mVertices.emplace(vertice->id(), vertice);
		broadcastState();
	}

	if (vertice->id() != node()->id())
		std::cout << "New state from " << to_base64url(vertice->id())
		          << ", sequence=" << vertice->state->sequence
		          << ", neighbors=" << vertice->state->neighbors.size() << std::endl;

	updateEdges(vertice->id(), vertice->state->neighbors);
	return true;
}

bool Graph::updateEdges(const Identifier &id, const std::set<Identifier> &neighbors) {
	// mMutex needs to be uniquely locked

	auto vertice = findVertice(id);
	if (!vertice)
		throw std::invalid_argument("Vertice not found");

	std::set<Identifier> added, removed;

	std::set<Identifier> currentNeighbors;
	for (const auto &[id, neighbor] : vertice->edges)
		currentNeighbors.insert(currentNeighbors.end(), id);

	std::set_difference(neighbors.begin(), neighbors.end(),               //
	                    currentNeighbors.begin(), currentNeighbors.end(), //
	                    std::inserter(added, added.end()));

	std::set_difference(currentNeighbors.begin(), currentNeighbors.end(), //
	                    neighbors.begin(), neighbors.end(),               //
	                    std::inserter(removed, removed.end()));

	if (added.empty() && removed.empty())
		return false;

	for (const auto &id : removed)
		vertice->edges.erase(id);

	for (const auto &id : added) {
		shared_ptr<Vertice> neighbor;
		if (auto it = mVertices.find(id); it != mVertices.end()) {
			neighbor = it->second;
		} else {
			neighbor = std::make_shared<Vertice>(id);
			mVertices.emplace(id, neighbor);
		}

		vertice->edges.emplace(id, std::move(neighbor));
	}

	computeRoutingTable();
	return true;
}

void Graph::computeRoutingTable() {
	// mMutex needs to be uniquely locked

	std::cout << "Recomputing routing table..." << std::endl;

	// Run Dijkstra's algorithm variant with priority queue to compute next hops
	using pair = std::pair<int, shared_ptr<Vertice>>;
	std::priority_queue<pair, std::deque<pair>, std::greater<pair>> queue;

	for (const auto &[id, vertice] : mVertices) {
		vertice->nextHop = nullptr;
		vertice->distance = -1;
		vertice->visited = false;
	}

	auto localVertice = findVertice(node()->id());
	if (!localVertice)
		throw std::runtime_error("Missing local node in network state");

	localVertice->nextHop = nullptr;
	localVertice->distance = 0;
	queue.push({localVertice->distance, localVertice});

	while (!queue.empty()) {
		const auto &[distance, node] = queue.top();
		if (!std::exchange(node->visited, true)) {
			for (const auto &[id, neighbor] : node->edges) {
				if (!neighbor->visited) {
					int tentative = distance + 1;
					if (!node->nextHop || tentative < neighbor->distance) {
						neighbor->nextHop = node->nextHop ? node->nextHop : neighbor;
						neighbor->distance = tentative;
						queue.push({neighbor->distance, neighbor});
					}
				}
			}
		}
		queue.pop();
	}

	mRoutingTable = std::make_shared<RoutingTable>();
	for (const auto &[id, node] : mVertices)
		if (node->nextHop)
			mRoutingTable->add(id, node->nextHop->id());

	std::cout << "Recomputed routing table, reacheable=" << mRoutingTable->count() << std::endl;
	node()->routing->setTable(mRoutingTable);
}

} // namespace legio::impl
