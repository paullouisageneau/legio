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

#include "legio/utils.hpp"

namespace legio {

uint16_t htons(uint16_t n) {
	uint8_t *p = reinterpret_cast<uint8_t *>(&n);
	return (uint16_t(p[0]) << 8) | (uint16_t(p[1]));
}

uint32_t htonl(uint32_t n) {
	uint8_t *p = reinterpret_cast<uint8_t *>(&n);
	return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) |
	       (uint32_t(p[3]));
}

uint64_t htonll(uint64_t n) {
	uint8_t *p = reinterpret_cast<uint8_t *>(&n);
	return (uint64_t(p[0]) << 56) | (uint64_t(p[1]) << 48) | (uint64_t(p[2]) << 40) |
	       (uint64_t(p[3]) << 32) | (uint64_t(p[4]) << 24) | (uint64_t(p[5]) << 16) |
	       (uint64_t(p[6]) << 8) | (uint64_t(p[7]));
}

uint16_t ntohs(uint16_t n) { return htons(n); }

uint32_t ntohl(uint32_t n) { return htonl(n); }

uint64_t ntohll(uint64_t n) { return htonll(n); }

} // namespace legio

