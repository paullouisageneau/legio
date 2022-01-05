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

#ifndef __EMSCRIPTEN__

#include "server.hpp"
#include "node.hpp"

#include <iostream>
#include <sstream>

namespace legio::impl {

Server::Server(const Configuration &config, Node *node) : Component(node), mMappingId(-1) {
	try {
		auto port = config.port.value_or(0);

		plum_config_t plumConfig = {};
		plumConfig.log_level = PLUM_LOG_LEVEL_WARN;
		plumConfig.log_callback = PlumLogCallback;

		if (config.dummyTlsService)
			plumConfig.dummytls_domain = config.dummyTlsService->c_str();

		if (plum_init(&plumConfig) < 0)
			throw std::runtime_error("Initialization failed");

		if (config.externalHost) {
			std::string externalHost = config.externalHost.value();
			uint16_t externalPort = config.externalPort.value_or(port);

			optional<CertificatePair> certPair;
			if (config.tlsPemCertificate && config.tlsPemKey) {
				certPair = CertificatePair{*config.tlsPemCertificate, *config.tlsPemKey};
			} else {
				char buffer[PLUM_MAX_HOST_LEN];
				if (plum_get_dummytls_host(externalHost.c_str(), buffer, PLUM_MAX_HOST_LEN) > 0)
					externalHost.assign(buffer);

				certPair = GetPlumCertificatePair();
			}

			createWebSocketServer(port, std::move(certPair));
			updateExternal(externalHost, externalPort);

		} else {
			createWebSocketServer(port);

			plum_mapping_t mapping = {};
			mapping.user_ptr = this;
			mapping.protocol = PLUM_IP_PROTOCOL_TCP;
			mapping.internal_port = mWebSocketServer->port();
			mapping.external_port = config.externalPort.value_or(port);
			mMappingId = plum_create_mapping(&mapping, PlumMappingCallback);
			if (mMappingId < 0)
				throw std::runtime_error("Mapping creation failed");
		}

	} catch (const std::exception &e) {
		throw std::runtime_error(string("Port mapping failed: ") + e.what());
	}
}

Server::~Server() {
	if (mMappingId >= 0)
		plum_destroy_mapping(mMappingId);

	plum_cleanup();
}

string Server::url() const {
	std::lock_guard lock(mMutex);
	return generateUrl();
}

void Server::update() {}

void Server::notify(const events::variant &event) {}

void Server::createWebSocketServer(uint16_t port, optional<CertificatePair> certPair) {
	try {
		std::lock_guard lock(mMutex);

		mWebSocketServer.reset();

		rtc::WebSocketServer::Configuration webSocketServerConfig;
		webSocketServerConfig.port = port;
		if (certPair) {
			webSocketServerConfig.enableTls = true;
			webSocketServerConfig.certificatePemFile = certPair->certificate;
			webSocketServerConfig.keyPemFile = certPair->key;
		}

		mTlsEnabled = webSocketServerConfig.enableTls;
		mWebSocketServer = std::make_unique<rtc::WebSocketServer>(std::move(webSocketServerConfig));

		mWebSocketServer->onClient([this](shared_ptr<rtc::WebSocket> webSocket) {
			std::cout << "Incoming WebSocket from "
			          << webSocket->remoteAddress().value_or("[unknown]") << std::endl;
			webSocket->onOpen([this, webSocket]() {
				std::cout << "Incoming WebSocket open" << std::endl;
				node()->routing->addChannel(webSocket);
			});
		});

	} catch (const std::exception &e) {
		throw std::runtime_error(string("WebSocket server creation failed: ") + e.what());
	}
}

void Server::updateExternal(optional<string> externalHost, optional<uint16_t> externalPort) {
	std::lock_guard lock(mMutex);
	mExternalHost = std::move(externalHost);
	mExternalPort = std::move(externalPort);

	std::cout << "WebSocket server URL: " << generateUrl() << std::endl;
}

string Server::generateUrl() const {
	if (!mWebSocketServer)
		throw std::runtime_error("WebSocket server is not started");

	uint16_t port = mExternalPort.value_or(mWebSocketServer->port());

	string hostname = mExternalHost.value_or(GetLocalAddress());
	if (hostname.find(':') != string::npos) // IPv6
		hostname = "[" + hostname + "]";

	std::ostringstream url;
	url << (mTlsEnabled ? "wss" : "ws") << "://" << hostname << ":" << port << "/";
	return url.str();
}

void Server::PlumLogCallback(plum_log_level_t level, const char *message) {
	const char *levelStr;
	switch (level) {
	case PLUM_LOG_LEVEL_FATAL:
		levelStr = "Fatal";
		break;
	case PLUM_LOG_LEVEL_ERROR:
		levelStr = "Error";
		break;
	case PLUM_LOG_LEVEL_WARN:
		levelStr = "Warning";
		break;
	case PLUM_LOG_LEVEL_INFO:
		levelStr = "Info";
		break;
	case PLUM_LOG_LEVEL_DEBUG:
		levelStr = "Debug";
		break;
	default:
		levelStr = "Verbose";
		break;
	}

	std::cout << "plum " << levelStr << ": " << message << std::endl;
}

void Server::PlumMappingCallback(int id, plum_state_t state, const plum_mapping_t *mapping) {
	auto server = static_cast<Server *>(mapping->user_ptr);
	if (id != server->mMappingId)
		return;

	if (state == PLUM_STATE_SUCCESS) {
		auto certPair = GetPlumCertificatePair();
		server->createWebSocketServer(mapping->internal_port, std::move(certPair));
		server->updateExternal(mapping->external_host, mapping->external_port);

	} else if (state == PLUM_STATE_FAILURE) {
		server->updateExternal(nullopt, nullopt);
	}
}

optional<Server::CertificatePair> Server::GetPlumCertificatePair() {
	CertificatePair result;

	try {
		const size_t bufferSize = 8192;
		char buffer[bufferSize];
		int len;

		len = plum_get_dummytls_certificate(PLUM_DUMMYTLS_PEM_FULLCHAIN, buffer, bufferSize);
		if (len < 0 || len >= bufferSize)
			throw std::runtime_error("Failed to retrieve DummyTLS certificate");

		result.certificate.assign(buffer);

		len = plum_get_dummytls_certificate(PLUM_DUMMYTLS_PEM_PRIVKEY, buffer, bufferSize);
		if (len < 0 || len >= bufferSize)
			throw std::runtime_error("Failed to retrieve DummyTLS key");

		result.key.assign(buffer);

	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return nullopt;
	}

	return result;
}

string Server::GetLocalAddress() {
	char local[PLUM_MAX_ADDRESS_LEN];
	if (plum_get_local_address(local, PLUM_MAX_ADDRESS_LEN) < 0)
		return "127.0.0.1";

	return local;
}

} // namespace legio::impl

#endif
