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

#include "ecdh.hpp"

#include "cryptopp/osrng.h"

#include <cassert>

namespace legio::impl {

const size_t Ecdh::KeySize = 65;

Ecdh::Ecdh(CryptoPP::OID curveId)
    : mDomain(curveId), mPublicKey(mDomain.PublicKeyLength()),
      mPrivateKey(mDomain.PrivateKeyLength()) {
	CryptoPP::AutoSeededRandomPool prng;
	mDomain.GenerateKeyPair(prng, mPrivateKey, mPublicKey);
}

Ecdh::~Ecdh() {}

binary Ecdh::publicKey() const {
	auto b = reinterpret_cast<const byte *>(mPublicKey.data());
	binary output(b, b + mPublicKey.size());

	assert(output.size() == KeySize);
	return output;
}

binary Ecdh::agree(const binary &remotePublicKey) const {
	if (remotePublicKey.size() != mDomain.PublicKeyLength())
		throw std::invalid_argument("Invalid remote ECDH public key size");

	CryptoPP::SecByteBlock secret(mDomain.AgreedValueLength());
	if (!mDomain.Agree(secret, mPrivateKey,
	                   reinterpret_cast<const CryptoPP::byte *>(remotePublicKey.data())))
		throw std::runtime_error("Failed to reach ECDH shared secret");

	auto b = reinterpret_cast<const byte *>(secret.data());
	return binary(b, b + secret.size());
}

} // namespace legio::impl
