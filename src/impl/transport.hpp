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

#ifndef LEGIO_IMPL_TRANSPORT_H
#define LEGIO_IMPL_TRANSPORT_H

#include "common.hpp"
#include "identifier.hpp"
#include "message.hpp"
#include "graph.hpp"
#include "routing.hpp"

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace legio::impl {

class Transport : public Component {
public:
	using ReceiveCallback = std::function<void(Identifier remoteId, binary payload)>;

	Transport(Node *node, Message::Type type, ReceiveCallback receiveCallback);
	virtual ~Transport();

	virtual void update();
	virtual void notify(const events::variant &event);

	virtual void send(Identifier remoteId, binary payload);
	virtual void broadcast(binary payload);

protected:
	virtual void incoming(message_ptr message, shared_ptr<Channel> from);
	bool checkSequence(const Identifier &id, uint32_t sequence);

	const Message::Type mType;
	const ReceiveCallback mReceiveCallback;

	std::atomic<uint32_t> mSendSequence;

private:
	std::unordered_map<Identifier, uint32_t, Identifier::hash> mSequences;
	std::mutex mSequencesMutex;
};

} // namespace legio::impl

#endif
