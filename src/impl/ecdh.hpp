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

#ifndef LEGIO_IMPL_ECDH_H
#define LEGIO_IMPL_ECDH_H

#include "common.hpp"

#include "cryptopp/eccrypto.h"
#include "cryptopp/oids.h"

namespace legio::impl {

class Ecdh final {
public:
	static const size_t KeySize;

	Ecdh(CryptoPP::OID curveId = CryptoPP::ASN1::secp256r1());
	~Ecdh();

	binary publicKey() const;

	binary agree(const binary &remotePublicKey) const;

private:
	CryptoPP::ECDH<CryptoPP::ECP>::Domain mDomain;
	CryptoPP::SecByteBlock mPublicKey;
	CryptoPP::SecByteBlock mPrivateKey;
};

} // namespace legio::impl

#endif
