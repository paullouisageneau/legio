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

#include "node.hpp"
#include "binary.hpp"
#include "broadcastabletransport.hpp"
#include "message.hpp"

#include "rtc/rtc.hpp" // for rtc::InitLogger

#include <algorithm>

namespace {

using std::string;
using std::string_view;

inline bool match_prefix(string_view str, string_view prefix) {
	return str.size() >= prefix.size() &&
	       std::mismatch(prefix.begin(), prefix.end(), str.begin()).first == prefix.end();
}

} // namespace

namespace legio::impl {

using namespace std::placeholders;

Node::Node(Configuration _config)
    : config(std::move(_config)), scheduler(std::make_unique<Scheduler>()),
      routing(std::make_shared<Routing>(this)), graph(std::make_shared<Graph>(this)),
#ifndef __EMSCRIPTEN__
      server(config.port ? std::make_shared<Server>(config, this) : nullptr),
#endif
      networking(std::make_shared<Networking>(this)),
      userTransport(std::make_unique<BroadcastableTransport>(
          this, Message::User, std::bind(&Node::receive, this, _1, _2))) {

#ifndef __EMSCRIPTEN__
	rtc::InitLogger(rtc::LogLevel::Warning);
#endif
}

Node::~Node() {}

void Node::attach(Component *component) { mComponents.push_back(component); }

void Node::detach(Component *component) {
	auto it = std::find(mComponents.begin(), mComponents.end(), component);
	if (it != mComponents.end())
		mComponents.erase(it);
}

void Node::notify(const events::variant &event) {
	for (auto component : mComponents)
		component->notify(event);
}

void Node::update() {
	for (auto component : mComponents)
		component->update();

	scheduler->run();
}

string Node::url() const {
#ifndef __EMSCRIPTEN__
	return server->url();
#else
	return "";
#endif
}

bool Node::isConnected() const { return (routing->table()->count() > 0); }

void Node::connect(string url) {
	if (match_prefix(url, "ws:") || match_prefix(url, "wss:")) {
		networking->connectWebSocket(url);
	} else {
		networking->connectPeer(from_base64url(url));
	}
}

void Node::receive(Identifier id, binary payload) {
	std::lock_guard lock(messageCallbackMutex);
	if (messageCallback)
		messageCallback(std::move(id), std::move(payload));
}

} // namespace legio::impl
