/**
 * Copyright (C) 2021 by Paul-Louis Ageneau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "legio/node.hpp"

#include <iostream>
#include <memory>
#include <chrono>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#include <thread>
#endif

using namespace std::chrono_literals;
using std::chrono::steady_clock;
using std::chrono::milliseconds;

const milliseconds period = 200ms;

std::unique_ptr<legio::Node> node;

void loop();
void update();

int main(int argc, char *argv[]) {
	try {
		std::cout << "Starting..." << std::endl;

		legio::Configuration config;
		config.port = argc <= 1 ? 8080 : 0;
		node = std::make_unique<legio::Node>(std::move(config));

		std::cout << "Local node is " << legio::to_base64url(node->id()) << std::endl;

		if(argc > 1) {
			std::string url = argv[1];
			std::cout << "Connecting to "  << url << std::endl;
			node->connect(url);
		}

#ifdef __EMSCRIPTEN__
		int frequency = 1000 / period.count();
		emscripten_set_main_loop(loop, frequency, 1);
#else
		loop();
#endif
	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		node.reset();
		return 1;
	}

	node.reset();
	return 0;
}

void loop() {
#ifdef __EMSCRIPTEN__
	try {
		update();
	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		emscripten_cancel_main_loop();
	}
#else
	while (true) {
		const auto tik = steady_clock::now();
		update();
		const auto tok = steady_clock::now();
		if(auto elapsed = tok - tik; elapsed < period)
			std::this_thread::sleep_for(period - elapsed);
	}
#endif
}

void update() {
	node->update();
}
