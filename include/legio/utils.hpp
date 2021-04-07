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

#ifndef LEGIO_UTILS_H
#define LEGIO_UTILS_H

#include <functional>
#include <memory>
#include <utility>

namespace legio {

// overloaded helper
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// weak_ptr bind helper
template <typename F, typename T, typename... Args> auto weak_bind(F &&f, T *t, Args &&..._args) {
	return [bound = std::bind(f, t, _args...), weak_this = t->weak_from_this()](auto &&...args) {
		if (auto shared_this = weak_this.lock())
			return bound(args...);
		else
			return static_cast<decltype(bound(args...))>(false);
	};
}

// scope_guard helper
class scope_guard final {
public:
	scope_guard(std::function<void()> func) : function(std::move(func)) {}
	scope_guard(scope_guard &&other) = delete;
	scope_guard(const scope_guard &) = delete;
	void operator=(const scope_guard &) = delete;

	~scope_guard() {
		if (function)
			function();
	}

private:
	std::function<void()> function;
};

// pimpl base class
template <typename T> using impl_ptr = std::shared_ptr<T>;
template <typename T> class CheshireCat {
public:
	CheshireCat(impl_ptr<T> impl) : mImpl(std::move(impl)) {}
	template <typename... Args>
	CheshireCat(Args... args) : mImpl(std::make_shared<T>(std::move(args)...)) {}
	CheshireCat(CheshireCat<T> &&cc) { *this = std::move(cc); }
	CheshireCat(const CheshireCat<T> &) = delete;

	virtual ~CheshireCat() = default;

	CheshireCat &operator=(CheshireCat<T> &&cc) {
		mImpl = std::move(cc.mImpl);
		return *this;
	};
	CheshireCat &operator=(const CheshireCat<T> &) = delete;

protected:
	impl_ptr<T> impl() { return mImpl; }
	impl_ptr<const T> impl() const { return mImpl; }

private:
	impl_ptr<T> mImpl;
};

// helper to combine hashes
template <typename T> inline void hash_combine(std::size_t &seed, const T &v) {
	std::hash<T> hash;
	seed ^= hash(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// network byte order functions
uint16_t htons(uint16_t n);
uint32_t htonl(uint32_t n);
uint64_t htonll(uint64_t n);
uint16_t ntohs(uint16_t n);
uint32_t ntohl(uint32_t n);
uint64_t ntohll(uint64_t n);

} // namespace legio

#endif
