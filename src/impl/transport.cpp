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

#include "transport.hpp"
#include "node.hpp"

namespace legio::impl {

Transport::Transport(Node *node, Message::Type type, ReceiveCallback receiveCallback)
    : Component(node), mType(type), mReceiveCallback(std::move(receiveCallback)) {}

Transport::~Transport() {}

void Transport::update() {}

void Transport::notify(const events::variant &event) {
	if (!std::holds_alternative<events::Message>(event))
		return;

	const auto &m = std::get<events::Message>(event);
	if (m.message->type != mType || !m.message->source)
		return;

	incoming(m.message, m.channel);
}

void Transport::send(Identifier remoteId, binary payload) {
	auto graph = node()->graph;
	auto remoteState = graph->get(remoteId);
	auto cipherBody = CipherBody::Encrypt(payload, graph->localEcdhPair(), remoteState.ecdhPublic);
	auto message = make_message(mType, mSendSequence++, binary(cipherBody), node()->ecdsaPair,
	                            EcdsaPublic(remoteId));
	node()->routing->send(std::move(message));
}

void Transport::broadcast(binary payload) {
	throw std::logic_error("Transport does not support broadcasting");
}

void Transport::incoming(message_ptr message, shared_ptr<Channel> from) {
	Identifier remoteId(*message->source);

	if (!checkSequence(remoteId, message->sequence))
		return;

	if (!message->destination)
		return; // Do not accept broadcast

	// Direct send, decrypt message
	CipherBody cipherBody(message->body);

	auto localEcdhPair = node()->graph->localEcdhPair();
	if (cipherBody.destination != localEcdhPair.publicKey())
		return; // TODO: handle ECDH key rotation

	// TODO: take new remote key into account

	binary payload = cipherBody.decrypt(localEcdhPair);
	mReceiveCallback(std::move(remoteId), std::move(payload));
}

bool Transport::checkSequence(const Identifier &id, uint32_t sequence) {
	std::lock_guard lock(mSequencesMutex);
	auto it = mSequences.find(id);
	if (it == mSequences.end()) {
		mSequences.emplace(id, sequence);
		return true;
	}

	if (compare_sequence(sequence, it->second) > 0) {
		it->second = sequence;
		return true;
	}

	return false;
}

} // namespace legio::impl
