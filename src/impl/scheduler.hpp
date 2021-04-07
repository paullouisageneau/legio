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

#ifndef RTC_IMPL_SCHEDULER_H
#define RTC_IMPL_SCHEDULER_H

#include "common.hpp"

#include <chrono>
#include <map>
#include <mutex>

namespace legio::impl {

class Scheduler final {
public:
	using clock = std::chrono::steady_clock;

	Scheduler();
	~Scheduler();

	void run();
	bool runOne();

	using TaskIdentifier = std::pair<clock::time_point, int>; // time, number

	template <class F, class... Args>
	TaskIdentifier enqueue(F &&f, Args &&...args);

	template <class F, class... Args>
	TaskIdentifier schedule(clock::duration delay, F &&f, Args &&...args);

	template <class F, class... Args>
	TaskIdentifier schedule(clock::time_point time, F &&f, Args &&...args);

	bool cancel(const TaskIdentifier &id);

protected:
	std::function<void()> dequeue();

	struct Task {
		clock::time_point time;
		int number;
		std::function<void()> func;

		bool operator>(const Task &other) const { return time > other.time; }
		bool operator<(const Task &other) const { return time < other.time; }
	};

	std::map<TaskIdentifier, Task> mTasks;
	int mNextTaskNumber = 0;

	mutable std::mutex mMutex;
};

template <class F, class... Args>
Scheduler::TaskIdentifier Scheduler::enqueue(F &&f, Args &&...args) {
	return schedule(clock::now(), std::forward<F>(f), std::forward<Args>(args)...);
}

template <class F, class... Args>
Scheduler::TaskIdentifier Scheduler::schedule(clock::duration delay, F &&f, Args &&...args) {
	return schedule(clock::now() + delay, std::forward<F>(f), std::forward<Args>(args)...);
}

template <class F, class... Args>
Scheduler::TaskIdentifier Scheduler::schedule(clock::time_point time, F &&f, Args &&...args) {
	std::unique_lock lock(mMutex);
	int number = mNextTaskNumber++;
	TaskIdentifier id{time, number};
	auto bound = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
	mTasks.emplace(id, Task{time, number, [bound = std::move(bound)]() { return bound(); }});
	return id;
}

} // namespace legio::impl

#endif
