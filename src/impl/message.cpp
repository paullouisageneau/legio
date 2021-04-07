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

#include "message.hpp"
#include "aesgcm.hpp"
#include "sha.hpp"

#include <iostream>
#include <limits>

namespace legio::impl {

Message Message::Create(Type _type, uint32_t _sequence, binary _body,
                        optional<EcdsaPair> sourceEcdsaPair, optional<Identifier> destination) {
	Message message(_type, std::move(_body), std::move(destination));
	message.sequence = _sequence;
	if (sourceEcdsaPair)
		message.sign(*sourceEcdsaPair);

	return message;
}

Message::Message(Type _type, binary _body, optional<Identifier> _destination)
    : type(_type), body(std::move(_body)), destination(std::move(_destination)) {}

Message::Message(const binary &bin) {
	// OPTI: prevent copy
	binary_reader reader(bin);

	Header header;
	static_assert(sizeof(header) == 8, "Message header length must be 8 bytes");
	reader.read(reinterpret_cast<byte *>(&header), sizeof(header));

	type = static_cast<Message::Type>(header.type);
	sequence = ntohl(header.sequence);
	size_t length = ntohs(header.length);

	if (header.flags & HasSource)
		source = reader.read(Identifier::Size);

	if (header.flags & HasDestination)
		destination = reader.read(Identifier::Size);

	body = reader.read(length);
	signature = reader.left();

	if (source &&
	    !EcdsaPublic(*source).verify(bin.data(), bin.size() - signature.size(), signature))
		throw std::invalid_argument("Message signature is invalid");
}

void Message::sign(const EcdsaPair &sourceEcdsaPair) {
	source = Identifier(sourceEcdsaPair);

	// Clear the signature and sign the binary representation without signature
	signature.clear();
	signature = sourceEcdsaPair.sign(binary(*this));
}

Message::operator binary() const {
	if (body.size() > std::numeric_limits<uint16_t>::max())
		throw std::runtime_error("Message body is too long");

	binary_writer writer;

	Header header;
	static_assert(sizeof(header) == 8, "header length must be 8 bytes");
	header.type = static_cast<uint8_t>(type);
	header.flags = 0;
	header.length = htons(body.size());
	header.sequence = htonl(sequence);

	if (source)
		header.flags |= HasSource;

	if (destination)
		header.flags |= HasDestination;

	writer.write(reinterpret_cast<const byte *>(&header), sizeof(header));

	if (source)
		writer.write(*source);

	if (destination)
		writer.write(*destination);

	writer.write(body);
	writer.write(signature);

	return writer.data();
}

CipherBody CipherBody::Encrypt(const binary &cleartext, const Ecdh &ecdh, binary _destination) {
	binary sharedKey = Sha256(ecdh.agree(_destination));
	AesGcmEncryption encryption(std::move(sharedKey));
	CipherBody body;
	body.source = ecdh.publicKey();
	body.destination = std::move(_destination);
	body.iv = encryption.iv();
	body.ciphertext = encryption.encrypt(cleartext);
	return body;
}

CipherBody::CipherBody() {}

CipherBody::CipherBody(const binary &body) {
	// OPTI: prevent copy
	binary_reader reader(body);
	source = reader.read(Ecdh::KeySize);
	destination = reader.read(Ecdh::KeySize);
	iv = reader.read(16);
	ciphertext = reader.left();
}

binary CipherBody::decrypt(const Ecdh &ecdh) {
	if (ecdh.publicKey() != destination)
		throw std::runtime_error("Destination ECDH public key does not match");

	binary sharedKey = Sha256(ecdh.agree(source));
	AesGcmDecryption decryption(std::move(sharedKey), iv);
	return decryption.decrypt(ciphertext);
}

CipherBody::operator binary() const {
	binary_writer writer;
	writer.write(source);
	writer.write(destination);
	writer.write(iv);
	writer.write(ciphertext);
	return writer.data();
}

} // namespace legio::impl
