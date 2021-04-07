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

#ifndef LEGIO_NODE_H
#define LEGIO_NODE_H

#include "common.hpp"
#include "configuration.hpp"
#include "binary.hpp"
#include "utils.hpp"

namespace legio {

namespace impl {

struct Node;

}

class Node final : CheshireCat<impl::Node> {
public:
	Node();
	Node(Configuration config);
	~Node();

	// Identifier of the local node
	binary id() const;

	bool isConnected() const;

	void update();

	// Bootstrap
	void connect(string url);

	// Message API
	void send(binary id, binary message);
	void broadcast(binary message);
	void onMessage(std::function<void(binary id, binary message)> callback);
};

} // namespace legio

#endif
