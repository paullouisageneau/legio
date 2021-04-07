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

#ifndef LEGIO_IMPL_MESSAGE_H
#define LEGIO_IMPL_MESSAGE_H

#include "common.hpp"
#include "ecdh.hpp"
#include "ecdsa.hpp"
#include "identifier.hpp"

namespace legio::impl {

#pragma pack(push, 1)
struct Header {
	uint8_t type;
	uint8_t flags;
	uint16_t length;
	uint32_t sequence;
};
#pragma pack(pop)

struct Message {
public:
	enum Type : uint8_t {
		Dummy = 0x00,

		// Routing
		Hello = 0x01,
		State = 0x02,

		// Signaling
		Signaling = 0x10,
		Provisioning = 0x11,

		// User
		User = 0x80
	};

	enum Flags : uint8_t { None = 0x00, HasSource = 0x01, HasDestination = 0x02 };

	static Message Create(Type _type, uint32_t sequence, binary _body = binary(),
	                      optional<EcdsaPair> sourceEcdsaPair = nullopt,
	                      optional<Identifier> destination = nullopt);

	Message(const binary &bin);

	void sign(const EcdsaPair &sourceEcdsaPair);

	operator binary() const;

	Type type;
	uint32_t sequence;
	optional<Identifier> source;
	optional<Identifier> destination;
	binary body;
	binary signature;

private:
	Message(Type type, binary body, optional<Identifier> destination = nullopt);
};

struct CipherBody {
	static CipherBody Encrypt(const binary &cleartext, const Ecdh &ecdh, binary _destination);

	CipherBody(const binary &body);

	binary decrypt(const Ecdh &ecdh);

	operator binary() const;

	binary source;
	binary destination;
	binary iv;
	binary ciphertext;

private:
	CipherBody();
};

using message_ptr = shared_ptr<Message>;

inline message_ptr make_message(Message::Type _type, uint32_t sequence, binary _body = binary(),
                                optional<EcdsaPair> sourceEcdsaPair = nullopt,
                                optional<Identifier> destination = nullopt) {
	return std::make_shared<Message>(Message::Create(
	    _type, sequence, std::move(_body), std::move(sourceEcdsaPair), std::move(destination)));
}

inline int compare_sequence(uint32_t s1, uint32_t s2) {
	if(s1 == s2)
		return 0;
	else if(s1 - s2 < std::numeric_limits<uint32_t>::max() / 2)
		return 1; // s1 > s2
	else
		return -1; // s1 < s2
}

} // namespace legio::impl

#endif
