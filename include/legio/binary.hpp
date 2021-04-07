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

#ifndef LEGIO_BINARY_H
#define LEGIO_BINARY_H

#include <algorithm>
#include <array>
#include <string>
#include <cstddef>
#include <vector>

namespace legio {

using std::byte;
using std::string;
using binary = std::vector<byte>;

binary &operator^=(binary &a, const binary &b);
binary operator^(binary a, const binary &b);

binary &operator+=(binary &a, const binary &b);
binary operator+(binary a, const binary &b);

string to_string(const binary &bin);
binary to_binary(const string &str);

string to_hex(const binary &bin);
binary from_hex(const string &str);

string to_base64(const binary &bin, bool padding = false);
binary from_base64(const string &str);

string to_base64url(const binary &bin, bool padding = false);
binary from_base64url(const string &str);

// Pack zero-terminated strings in a binary
template <size_t C> binary pack_strings(const std::array<string, C> &strs) {
	size_t size = 0;
	for (const string &str : strs)
		size += str.size() + 1;

	binary result;
	result.reserve(size);
	for (const string &str : strs) {
		std::transform(str.begin(), str.end(), std::back_inserter(result),
		               [](char c) { return byte(c); });
		result.emplace_back(byte(0));
	}
	return result;
}

template <size_t C> std::array<string, C> unpack_strings(const binary &bin) {
	std::array<string, C> result;
	size_t i = 0;
	size_t j = 0;
	while (i < bin.size() && j < result.size()) {
		string &str = result[j];
		while (i < bin.size() && bin[i] != byte(0)) {
			str += std::to_integer<char>(bin[i]);
			++i;
		}
		++i;
		++j;
	}
	return result;
}

struct binary_hash {
	std::size_t operator()(const binary &b) const noexcept;
};

class binary_reader {
public:
	binary_reader(binary bin);

	size_t read(byte *buffer, size_t size);
	binary read(size_t size);
	void readInt(uint8_t &i);
	void readInt(uint16_t &i);
	void readInt(uint32_t &i);
	void readInt(uint64_t &i);

	size_t size() const;
	bool finished() const;
	binary left() const;

private:
	binary mBinary;
	size_t mPosition;
};

class binary_writer {
public:
	void write(const byte *data, size_t size);
	void write(const binary &data);
	void writeInt(uint8_t i);
	void writeInt(uint16_t i);
	void writeInt(uint32_t i);
	void writeInt(uint64_t i);

	size_t size() const;
	binary &data();
	const binary &data() const;

private:
	binary mBinary;
};

} // namespace legio

#endif
