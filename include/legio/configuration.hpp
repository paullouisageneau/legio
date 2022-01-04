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

#ifndef LEGIO_CONFIGURATION_H
#define LEGIO_CONFIGURATION_H

#include "common.hpp"

namespace legio {

const uint16_t DefaultPort = 8080;
const string DefaultDummyTlsService = "legio-p2p.net";

struct Configuration {
	optional<uint16_t> port = DefaultPort;
	optional<string> externalHost;
	optional<uint16_t> externalPort;
	optional<string> tlsPemCertificate;
	optional<string> tlsPemKey;
	optional<string> dummyTlsService = DefaultDummyTlsService;
};

} // namespace legio

#endif
