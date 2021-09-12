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

#ifndef LEGIO_IMPL_PROVISIONING_H
#define LEGIO_IMPL_PROVISIONING_H

#include "common.hpp"
#include "identifier.hpp"
#include "component.hpp"
#include "transport.hpp"

#include <chrono>
#include <unordered_map>
#include <map>

namespace legio::impl {

class Provisioning final : public Component {
public:
	using clock = std::chrono::system_clock;
	using time_point = std::chrono::time_point<clock>;

	Provisioning(Node *node);
	~Provisioning();

	void update();
	void notify(const events::variant &event);

	struct Entry {
		Identifier source;
		string host;
		string username;
		string password;
		time_point time;

		string url() const;
		clock::duration age() const;
	};

	void insert(Entry entry);
	std::vector<Entry> pick(int count) const;

private:
	void receive(Identifier remoteId, binary payload);

	const shared_ptr<Transport> mTransport;

	std::unordered_map<Identifier, Entry, Identifier::hash> mEntries;
	mutable std::mutex mMutex;
};

} // namespace legio::impl

#endif
