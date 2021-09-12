/***************************************************************************
 *   Copyright (C) 2017-2021 by Paul-Louis Ageneau                         *
 *   paul-louis (at) ageneau (dot) org                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#include "provisioning.hpp"
#include "node.hpp"

#include <algorithm>
#include <random>
#include <sstream>

namespace legio::impl {

using namespace std::placeholders;
using namespace std::chrono_literals;

Provisioning::Provisioning(Node *node)
    : Component(node),
      mTransport(std::make_shared<Transport>(node, Message::Provisioning,
                                             std::bind(&Provisioning::receive, this, _1, _2))) {}

Provisioning::~Provisioning() {}

void Provisioning::update() {
	mTransport->update();

	std::unique_lock lock(mMutex);

	int fresh = 0;
	auto it = mEntries.begin();
	while (it != mEntries.end()) {
		const auto &entry = it->second;
		if (entry.age() >= 30min) {
			it = mEntries.erase(it);
			continue;
		}
		if (entry.age() < 10min) {
			++fresh;
		}
		++it;
	}

	if (fresh < 4) {
		auto nodes = node()->graph->nodes(State::HasTurn);
		std::default_random_engine rng(std::random_device{}());
		std::shuffle(nodes.begin(), nodes.end(), rng);
		for (const auto &id : nodes) {
			if (mEntries.find(id) == mEntries.end()) {
				mTransport->send(id, pack_strings<1>({"turn"}));
				break;
			}
		}
	}
}

void Provisioning::notify(const events::variant &event) {}

void Provisioning::insert(Entry entry) {
	std::lock_guard lock(mMutex);
	mEntries.emplace(entry.source, std::move(entry));
}

std::vector<Provisioning::Entry> Provisioning::pick(int count) const {
	std::lock_guard lock(mMutex);

	std::vector<std::reference_wrapper<const Entry>> vec;
	vec.reserve(mEntries.size());
	for (const auto &[id, entry] : mEntries)
		vec.push_back(std::ref(entry));

	std::default_random_engine rng(std::random_device{}());
	std::shuffle(vec.begin(), vec.end(), rng);

	std::vector<Provisioning::Entry> result;
	result.reserve(count);
	for (int i = 0; i < std::min(count, int(vec.size())); ++i)
		result.push_back(vec[i].get());

	return result;
}

void Provisioning::receive(Identifier remoteId, binary payload) {
	std::unique_lock lock(mMutex);
	auto [type, host, username, password] = unpack_strings<4>(payload);
	if (type != "turn")
		return;

	insert({std::move(remoteId),
	        std::move(host),     // host
	        std::move(username), // username
	        std::move(password), // password
	        clock::now()});
}

string Provisioning::Entry::url() const {
	std::ostringstream oss;
	oss << "turn:" << username << "@" << password << ":" << host;
	return oss.str();
}

Provisioning::clock::duration Provisioning::Entry::age() const {
	return std::max(clock::now() - time, clock::duration::zero());
}

} // namespace legio::impl
