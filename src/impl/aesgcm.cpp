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

#include "aesgcm.hpp"

#include "cryptopp/filters.h"
#include "cryptopp/osrng.h"

namespace legio::impl {

const size_t GCM_TAG_SIZE = 16;

AesGcmEncryption::AesGcmEncryption() : mKey(AES::DEFAULT_KEYLENGTH), mIv(AES::BLOCKSIZE) {
	CryptoPP::AutoSeededRandomPool prng;
	prng.GenerateBlock(mKey, mKey.size());
	prng.GenerateBlock(mIv, mIv.size());

	mEncryption.SetKeyWithIV(mKey, mKey.size(), mIv, mIv.size());
}

AesGcmEncryption::AesGcmEncryption(binary key) : mIv(AES::BLOCKSIZE) {
	if (key.size() < AES::BLOCKSIZE)
		throw std::invalid_argument("AES key too short");

	mKey.Assign(reinterpret_cast<const CryptoPP::byte *>(key.data()), key.size());

	CryptoPP::AutoSeededRandomPool prng;
	prng.GenerateBlock(mIv, mIv.size());

	mEncryption.SetKeyWithIV(mKey, mKey.size(), mIv, mIv.size());
}

AesGcmEncryption::~AesGcmEncryption() {}

binary AesGcmEncryption::key() const {
	auto b = reinterpret_cast<const byte *>(mKey.data());
	return binary(b, b + mKey.size());
}

binary AesGcmEncryption::iv() const {
	auto b = reinterpret_cast<const byte *>(mIv.data());
	return binary(b, b + mIv.size());
}

binary AesGcmEncryption::encrypt(const binary &data) {
	CryptoPP::ByteQueue queue;
	CryptoPP::ArraySource source(
	    reinterpret_cast<const CryptoPP::byte *>(data.data()), data.size(), true,
	    new CryptoPP::AuthenticatedEncryptionFilter(mEncryption, new CryptoPP::Redirector(queue), false, GCM_TAG_SIZE));

	binary cipher(queue.MaxRetrievable());
	queue.Get(reinterpret_cast<CryptoPP::byte *>(cipher.data()), cipher.size());
	return cipher;
}

AesGcmDecryption::AesGcmDecryption(binary key, binary iv) {
	if (key.size() < AES::BLOCKSIZE)
		throw std::invalid_argument("AES key too short");

	if (iv.size() < AES::BLOCKSIZE)
		throw std::invalid_argument("AES IV too short");

	mDecryption.SetKeyWithIV(reinterpret_cast<const CryptoPP::byte *>(key.data()), key.size(),
	                         reinterpret_cast<const CryptoPP::byte *>(iv.data()), iv.size());
}

AesGcmDecryption::~AesGcmDecryption() {}

binary AesGcmDecryption::decrypt(const binary &data) {
	binary plain(data.size());
	CryptoPP::ArraySink sink(reinterpret_cast<CryptoPP::byte *>(plain.data()), plain.size());
	CryptoPP::ArraySource source(
	    reinterpret_cast<const CryptoPP::byte *>(data.data()), data.size(), true,
	    new CryptoPP::AuthenticatedEncryptionFilter(mDecryption, new CryptoPP::Redirector(sink), false, GCM_TAG_SIZE));

	plain.resize(sink.TotalPutLength());
	return plain;
}

} // namespace legio::impl
