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

#include "routingtable.hpp"

namespace legio::impl {

RoutingTable::RoutingTable() {}

RoutingTable::~RoutingTable() {}

void RoutingTable::add(const Identifier &node, const Identifier &nextHop) {
	std::unique_lock lock(mMutex);
	mNextHops.emplace(node, nextHop);
}

void RoutingTable::remove(const Identifier &node, const Identifier &nextHop) {
	std::unique_lock lock(mMutex);
	auto it = mNextHops.find(node);
	if(it != mNextHops.end() && it->second == nextHop)
		mNextHops.erase(it);
}

void RoutingTable::clear() {
	std::unique_lock lock(mMutex);
	mNextHops.clear();
}

optional<Identifier> RoutingTable::findNextHop(const Identifier &id) {
	std::shared_lock lock(mMutex);
	auto it = mNextHops.find(id);
	return it != mNextHops.end() ? std::make_optional(it->second) : nullopt;
}

std::vector<Identifier> RoutingTable::nodes() const {
	std::shared_lock lock(mMutex);
	std::vector<Identifier> result;
	result.reserve(mNextHops.size());
	for(const auto &[id, nextHop] : mNextHops)
		result.push_back(id);

	return result;
}

int RoutingTable::count() const {
	std::shared_lock lock(mMutex);
	return int(mNextHops.size());
}

} // namespace legio::impl
