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

#include "scheduler.hpp"

#include <iostream>

namespace legio::impl {

Scheduler::Scheduler() {}

Scheduler::~Scheduler() {}

void Scheduler::run() {
	while (runOne()) {
	}
}

bool Scheduler::runOne() {
	if (auto task = dequeue()) {
		try {
			task();
		}
		catch(const std::exception &e) {
			std::cerr << "Unhandled exception in task: " << e.what() << std::endl;
		}
		return true;
	}
	return false;
}

bool Scheduler::cancel(const TaskIdentifier &id) {
	std::unique_lock lock(mMutex);
	return mTasks.erase(id) != 0;
}

std::function<void()> Scheduler::dequeue() {
	std::unique_lock lock(mMutex);
	if (mTasks.empty())
		return nullptr;

	auto it = mTasks.begin();
	if (it->second.time > clock::now())
		return nullptr;

	auto func = std::move(it->second.func);
	mTasks.erase(it);
	return func;
}

} // namespace legio::impl
