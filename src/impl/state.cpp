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

#include "state.hpp"
#include "ecdh.hpp" // for Ecdh::KeySize

#include <iostream>

namespace legio::impl {

State::State(EcdsaPublic _ecdsaPublic, uint32_t _sequence, binary _ecdhPublic)
    : ecdsaPublic(std::move(_ecdsaPublic)), sequence(_sequence), ecdhPublic(std::move(_ecdhPublic)) {}

State::~State() {}

message_ptr State::toMessage(const EcdsaPair &ecdsaPair) const {
	binary_writer writer;

	writer.writeInt(provision);
	writer.write(ecdhPublic);

	for (const Identifier &id : neighbors)
		writer.write(id);

	return make_message(Message::State, sequence, std::move(writer.data()), ecdsaPair);
}

State State::FromMessage(message_ptr message) {
	if (!message || message->type != Message::State || !message->source)
		throw std::invalid_argument("Not a State message");

	binary_reader reader(message->body);

	if (reader.size() < 4 + Ecdh::KeySize)
		throw std::invalid_argument("Truncated State message");

	uint32_t provision = 0;
	reader.readInt(provision);
	binary ecdhPublic = reader.read(Ecdh::KeySize);

	State result(*message->source, message->sequence, std::move(ecdhPublic));
	result.provision = provision;

	while (reader.size() >= Identifier::Size) {
		binary id = reader.read(Identifier::Size);
		result.neighbors.insert(std::move(id));
	}

	if (reader.size() > 0)
		std::cerr << "Warning: " << reader.size() << " bytes left in State message" << std::endl;

	return result;
}

} // namespace legio::impl
