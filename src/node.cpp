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

#include "node.hpp"

#include "impl/node.hpp"
#include "impl/identifier.hpp"

namespace legio {

Node::Node() : Node(Configuration()) {}

Node::Node(Configuration config) : CheshireCat<impl::Node>(std::move(config)) {}

Node::~Node() {}

binary Node::id() const { return impl()->id(); }

bool Node::isConnected() const { return impl()->isConnected(); }

void Node::update() { impl()->update(); }

void Node::connect(string url) { return impl()->connect(std::move(url)); }

void Node::send(binary id, binary message) {
	return impl()->userTransport->send(impl::Identifier(std::move(id)), std::move(message));
}

void Node::broadcast(binary message) {
	return impl()->userTransport->broadcast(std::move(message));
}

void Node::onMessage(std::function<void(binary id, binary message)> callback) {
	std::lock_guard lock(impl()->messageCallbackMutex);
	impl()->messageCallback = std::move(callback);
}

} // namespace legio
