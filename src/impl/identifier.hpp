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

#ifndef LEGIO_IMPL_IDENTIFIER_H
#define LEGIO_IMPL_IDENTIFIER_H

#include "common.hpp"
#include "ecdsa.hpp"

namespace legio::impl {

class Identifier final {
public:
	static const size_t Size;

	Identifier(const binary &bin);
	Identifier(EcdsaPublic publicKey);
	~Identifier();

	operator binary() const;
	operator EcdsaPublic() const;

	bool operator==(const Identifier &other) const;
	bool operator!=(const Identifier &other) const;
	bool operator<(const Identifier &other) const;
	bool operator>(const Identifier &other) const;

	struct hash {
        std::size_t operator()(const Identifier &id) const noexcept;
    };

private:
	EcdsaPublic mPublicKey;
};

} // namespace legio

#endif
