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

#include "ecdsa.hpp"

#include "cryptopp/ecp.h"
#include "cryptopp/oids.h"
#include "cryptopp/osrng.h"
#include "cryptopp/queue.h"

#include <cassert>
#include <iostream>

namespace legio::impl {

const size_t EcdsaPublic::KeySize = 33;

EcdsaPublic::EcdsaPublic(CryptoPP::OID curveId) {
	mPublicKey.AccessGroupParameters().Initialize(curveId);
	mPublicKey.AccessGroupParameters().SetPointCompression(true);
}

EcdsaPublic::EcdsaPublic(const binary &key, CryptoPP::OID curveId) : EcdsaPublic(curveId) {
	const auto &curve = mPublicKey.GetGroupParameters().GetCurve();

	CryptoPP::ECP::Point point;
	curve.DecodePoint(
	    point, reinterpret_cast<const CryptoPP::byte *>(key.data()), key.size());
	mPublicKey.SetPublicElement(point);

	CryptoPP::AutoSeededRandomPool prng;
	if (!mPublicKey.Validate(prng, 3))
		throw std::runtime_error("Failed to validate external ECDSA public key");
}

EcdsaPublic::~EcdsaPublic() {}

binary EcdsaPublic::publicKey() const {
	const auto &point = mPublicKey.GetPublicElement();
	const auto &curve = mPublicKey.GetGroupParameters().GetCurve();

	CryptoPP::ByteQueue queue;
	curve.EncodePoint(queue, point, true); // compressed

	binary output(queue.MaxRetrievable());
	queue.Get(reinterpret_cast<CryptoPP::byte *>(output.data()), output.size());

	assert(output.size() == KeySize);
	return output;
}

bool EcdsaPublic::verify(const binary &message, const binary &signature) const {
	return verify(message.data(), message.size(), signature);
}

bool EcdsaPublic::verify(const byte *message, size_t size, const binary &signature) const {
	ECDSA::Verifier verifier(mPublicKey);
	return verifier.VerifyMessage(
	    reinterpret_cast<const CryptoPP::byte *>(message), size,
	    reinterpret_cast<const CryptoPP::byte *>(signature.data()), signature.size());
}

bool EcdsaPublic::operator==(const EcdsaPublic &other) const {
	return mPublicKey.GetPublicElement() == other.mPublicKey.GetPublicElement();
}

bool EcdsaPublic::operator!=(const EcdsaPublic &other) const {
	return !(mPublicKey.GetPublicElement() == other.mPublicKey.GetPublicElement());
}

bool EcdsaPublic::operator<(const EcdsaPublic &other) const {
	return mPublicKey.GetPublicElement() < other.mPublicKey.GetPublicElement();
}

bool EcdsaPublic::operator>(const EcdsaPublic &other) const {
	return other.mPublicKey.GetPublicElement() < mPublicKey.GetPublicElement();
}

std::size_t EcdsaPublic::hash::operator()(const EcdsaPublic &ecdsa) const noexcept {
	return binary_hash()(ecdsa.publicKey());
}

EcdsaPair::EcdsaPair(CryptoPP::OID curveId) : EcdsaPublic(curveId) {
	CryptoPP::AutoSeededRandomPool prng;
	mPrivateKey.Initialize(prng, curveId);
	if (!mPrivateKey.Validate(prng, 3))
		throw std::runtime_error("Failed to validate ECDSA private key");

	mPrivateKey.MakePublicKey(mPublicKey);
	if (!mPublicKey.Validate(prng, 3))
		throw std::runtime_error("Failed to validate ECDSA public key");

	mPublicKey.AccessGroupParameters().SetPointCompression(true);
}

EcdsaPair::~EcdsaPair() {}

binary EcdsaPair::sign(const binary &message) const {
	CryptoPP::AutoSeededRandomPool prng;
	ECDSA::Signer signer(mPrivateKey);
	binary signature(signer.MaxSignatureLength());
	size_t len =
	    signer.SignMessage(prng, // signature requires nonce
	                       reinterpret_cast<const CryptoPP::byte *>(message.data()), message.size(),
	                       reinterpret_cast<CryptoPP::byte *>(signature.data()));
	signature.resize(len);
	return signature;
}

} // namespace legio::impl
