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

#ifndef LEGIO_IMPL_NODE_H
#define LEGIO_IMPL_NODE_H

#include "common.hpp"
#include "component.hpp"
#include "configuration.hpp"
#include "ecdsa.hpp"
#include "graph.hpp"
#include "identifier.hpp"
#include "networking.hpp"
#include "routing.hpp"
#include "scheduler.hpp"
#include "transport.hpp"

#ifndef __EMSCRIPTEN__
#include "server.hpp"
#endif

#include <mutex>

namespace legio::impl {

struct Node final : public std::enable_shared_from_this<Node> {
private:
	std::vector<Component *> mComponents;

public:
	Node(Configuration _config);
	~Node();

	inline Identifier id() const { return Identifier(ecdsaPair); }
	inline const EcdsaPublic &publicKey() const { return ecdsaPair; }

	void attach(Component *component);
	void detach(Component *component);
	void notify(const events::variant &event);
	void update();

	string url() const;
	bool isConnected() const;
	void connect(string url);

	void receive(Identifier id, binary payload);

	const Configuration config;
	const EcdsaPair ecdsaPair;
	const unique_ptr<Scheduler> scheduler;
	const shared_ptr<Routing> routing;
	const shared_ptr<Graph> graph;
#ifndef __EMSCRIPTEN__
	const shared_ptr<Server> server;
#endif
	const shared_ptr<Networking> networking;
	const shared_ptr<Transport> userTransport;

	using MessageCallback = std::function<void(Identifier remoteId, binary payload)>;
	MessageCallback messageCallback;
	std::mutex messageCallbackMutex;
};

} // namespace legio::impl

#endif
