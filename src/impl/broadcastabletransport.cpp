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

#include "broadcastabletransport.hpp"
#include "node.hpp"

namespace legio::impl {

BroadcastableTransport::BroadcastableTransport(Node *node, Message::Type type,
                                               ReceiveCallback receiveCallback)
    : Transport(node, type, std::move(receiveCallback)) {}

BroadcastableTransport::~BroadcastableTransport() {}

void BroadcastableTransport::broadcast(binary payload) {
	auto message = make_message(mType, mSendSequence++, std::move(payload), node()->ecdsaPair);
	node()->routing->broadcast(std::move(message));
}

void BroadcastableTransport::incoming(message_ptr message, shared_ptr<Channel> from) {
	if (message->destination) {
		Transport::incoming(message, from);
		return;
	}

	if (message->type != mType || !message->source)
		return;

	Identifier remoteId(*message->source);

	if (!checkSequence(remoteId, message->sequence))
		return;

	// Broadcast, message is not encrypted
	node()->routing->broadcast(message, from);
	mReceiveCallback(std::move(remoteId), message->body);
}

} // namespace legio::impl
