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

#include "identifier.hpp"

#include <cassert>

namespace legio::impl {

const size_t Identifier::Size = 33;

Identifier::Identifier(const binary &bin) : mPublicKey(bin) {}

Identifier::Identifier(EcdsaPublic publicKey) : mPublicKey(std::move(publicKey)) {}

Identifier::~Identifier() {}

Identifier::operator binary() const {
	binary bin(mPublicKey.publicKey());
	assert(bin.size() == Size);
	return bin;
}

Identifier::operator EcdsaPublic() const { return mPublicKey; }

bool Identifier::operator==(const Identifier &other) const { return mPublicKey == other.mPublicKey; }

bool Identifier::operator!=(const Identifier &other) const { return mPublicKey != other.mPublicKey; }

bool Identifier::operator<(const Identifier &other) const { return mPublicKey < other.mPublicKey; }

bool Identifier::operator>(const Identifier &other) const { return mPublicKey > other.mPublicKey; }

std::size_t Identifier::hash::operator()(const Identifier &id) const noexcept {
	return binary_hash()(binary(id));
}

} // namespace legio::impl
