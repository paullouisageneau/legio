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

#ifndef LEGIO_IMPL_STATE_H
#define LEGIO_IMPL_STATE_H

#include "common.hpp"
#include "ecdsa.hpp"
#include "identifier.hpp"
#include "message.hpp"

#include <set>

namespace legio::impl {

struct State final {
	State(EcdsaPublic _ecdsaPublic, uint32_t _sequence, binary _ecdhPublic);
	~State();

	inline Identifier id() const { return Identifier(ecdsaPublic); }
	inline const EcdsaPublic &publicKey() const { return ecdsaPublic; }

	message_ptr toMessage(const EcdsaPair &ecdsaPair) const;
	static State FromMessage(message_ptr message);

	EcdsaPublic ecdsaPublic;
	uint32_t sequence;

	binary ecdhPublic;
	std::set<Identifier> neighbors;

	enum ProvisionFlags {
		None = 0x0,
		HasWebSocket = 0x1,
		HasTurn = 0x2,
	};

	uint32_t provision = None;
};

} // namespace legio::impl

#endif
