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

#include "binary.hpp"
#include "common.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace legio {

binary &operator^=(binary &a, const binary &b) {
	a.resize(std::max(a.size(), b.size()), byte(0));
	for (size_t i = 0; i < std::min(a.size(), b.size()); ++i)
		a[i] ^= b[i];
	return a;
}

binary operator^(binary a, const binary &b) { return a ^= b; }

binary &operator+=(binary &a, const binary &b) {
	a.insert(a.end(), b.begin(), b.end());
	return a;
}

binary operator+(binary a, const binary &b) { return a += b; }

string to_string(const binary &bin) {
	string r;
	r.reserve(bin.size());
	std::transform(bin.begin(), bin.end(), std::back_inserter(r),
	               [](byte b) { return to_integer<char>(b); });
	return r;
}

binary to_binary(const string &str) {
	binary r;
	r.reserve(str.size());
	std::transform(str.begin(), str.end(), std::back_inserter(r), [](char c) { return byte(c); });
	return r;
}

string to_hex(const binary &bin) {
	std::ostringstream oss;
	for (int i = 0; i < bin.size(); ++i) {
		oss << std::hex << std::uppercase;
		oss << std::setfill('0') << std::setw(2);
		oss << unsigned(uint8_t(bin[i]));
	}
	return oss.str();
}

binary from_hex(const string &str) {
	binary out;
	if (str.empty())
		return out;

	int count = (str.size() + 1) / 2;
	out.reserve(count);
	for (int i = 0; i < count; ++i) {
		std::string s;
		s += str[i * 2];
		if (i * 2 + 1 != str.size())
			s += str[i * 2 + 1];
		else
			s += '0';

		unsigned value = 0;
		std::istringstream iss(s);
		if (!(iss >> std::hex >> value))
			throw std::invalid_argument("invalid hexadecimal representation");

		out.push_back(byte(value & 0xFF));
	}

	return out;
}

namespace {

string to_base64_impl(const binary &bin, const char *tab, bool padding) {
	string out;
	out.reserve(4 * ((bin.size() + 2) / 3));
	int i = 0;
	while (bin.size() - i >= 3) {
		auto d0 = to_integer<uint8_t>(bin[i]);
		auto d1 = to_integer<uint8_t>(bin[i + 1]);
		auto d2 = to_integer<uint8_t>(bin[i + 2]);
		out += tab[d0 >> 2];
		out += tab[((d0 & 3) << 4) | (d1 >> 4)];
		out += tab[((d1 & 0x0F) << 2) | (d2 >> 6)];
		out += tab[d2 & 0x3F];
		i += 3;
	}

	int left = bin.size() - i;
	if (left) {
		auto d0 = to_integer<uint8_t>(bin[i]);
		out += tab[d0 >> 2];
		if (left == 1) {
			out += tab[(d0 & 3) << 4];
			if (padding)
				out += '=';
		} else { // left == 2
			auto d1 = to_integer<uint8_t>(bin[i + 1]);
			out += tab[((d0 & 3) << 4) | (d1 >> 4)];
			out += tab[(d1 & 0x0F) << 2];
		}
		if (padding)
			out += '=';
	}

	return out;
}

binary from_base64_impl(const string &str) {
	binary out;
	out.reserve(3 * ((str.size() + 3) / 4));
	int i = 0;
	while (i < str.size() && str[i] != '=') {
		byte tab[4] = {};
		int j = 0;
		while (i < str.size() && j < 4) {
			uint8_t c = str[i];
			if (std::isspace(c))
				continue;
			if (c == '=')
				break;

			if ('A' <= c && c <= 'Z')
				tab[j] = byte(c - 'A');
			else if ('a' <= c && c <= 'z')
				tab[j] = byte(c + 26 - 'a');
			else if ('0' <= c && c <= '9')
				tab[j] = byte(c + 52 - '0');
			else if (c == '+' || c == '-')
				tab[j] = byte(62);
			else if (c == '/' || c == '_')
				tab[j] = byte(63);
			else
				throw std::invalid_argument("Invalid character in base64");

			++i;
			++j;
		}

		if (j > 0) {
			out.push_back((tab[0] << 2) | (tab[1] >> 4));
			if (j > 1) {
				out.push_back((tab[1] << 4) | (tab[2] >> 2));
				if (j > 2)
					out.push_back((tab[2] << 6) | (tab[3]));
			}
		}
	}

	return out;
}

} // namespace

string to_base64(const binary &bin, bool padding) {
	static const char *tab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	return to_base64_impl(bin, tab, padding);
}

string to_base64url(const binary &bin, bool padding) {
	static const char *tab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	return to_base64_impl(bin, tab, padding);
}

binary from_base64(const string &str) { return from_base64_impl(str); }

binary from_base64url(const string &str) { return from_base64_impl(str); }

size_t binary_hash::operator()(const binary &bin) const noexcept {
	size_t seed = 0;
	for (const byte b : bin)
		hash_combine(seed, b);
	return seed;
}

binary_reader::binary_reader(binary bin) : mBinary(std::move(bin)), mPosition(0) {}

size_t binary_reader::read(byte *buffer, size_t size) {
	if (mPosition + size > mBinary.size())
		throw std::runtime_error("read out of bounds");

	const byte *b = mBinary.data() + mPosition;
	mPosition += size;
	std::copy(b, b + size, buffer);
	return size;
}

binary binary_reader::read(size_t size) {
	binary result(size);
	read(result.data(), size);
	return result;
}

void binary_reader::readInt(uint8_t &i) { read(reinterpret_cast<byte *>(&i), 1); }

void binary_reader::readInt(uint16_t &i) {
	read(reinterpret_cast<byte *>(&i), 2);
	i = ntohs(i);
}

void binary_reader::readInt(uint32_t &i) {
	read(reinterpret_cast<byte *>(&i), 4);
	i = ntohl(i);
}

void binary_reader::readInt(uint64_t &i) {
	read(reinterpret_cast<byte *>(&i), 8);
	i = ntohll(i);
}

size_t binary_reader::size() const { return mBinary.size() - mPosition; }

bool binary_reader::finished() const { return mPosition == mBinary.size(); }

binary binary_reader::left() const { return binary(mBinary.begin() + mPosition, mBinary.end()); }

void binary_writer::write(const byte *data, size_t size) {
	mBinary.insert(mBinary.end(), data, data + size);
}

void binary_writer::write(const binary &data) { mBinary += data; }

void binary_writer::writeInt(uint8_t i) { write(reinterpret_cast<const byte *>(&i), 1); }

void binary_writer::writeInt(uint16_t i) {
	i = htons(i);
	write(reinterpret_cast<const byte *>(&i), 2);
}

void binary_writer::writeInt(uint32_t i) {
	i = htonl(i);
	write(reinterpret_cast<const byte *>(&i), 4);
}

void binary_writer::writeInt(uint64_t i) {
	i = htonll(i);
	write(reinterpret_cast<const byte *>(&i), 8);
}

size_t binary_writer::size() const { return mBinary.size(); }

binary &binary_writer::data() { return mBinary; }

const binary &binary_writer::data() const { return mBinary; }

} // namespace legio
