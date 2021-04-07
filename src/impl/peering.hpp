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

#ifndef LEGIO_IMPL_PEERING_H
#define LEGIO_IMPL_PEERING_H

#include "common.hpp"
#include "identifier.hpp"
#include "routing.hpp"
#include "transport.hpp"

#include <rtc/rtc.hpp>

namespace legio::impl {

class Peering final {
public:
	Peering(shared_ptr<Routing> routing, shared_ptr<Transport> transport, Identifier remoteId);
	~Peering();

	const Identifier &remoteId() const;

	bool isConnected() const;
	void connect();
	void disconnect();

	void receive(binary body);

private:
	void createPeerConnection();
	void sendLocalDescription(rtc::Description description);
	void setDataChannel(shared_ptr<rtc::DataChannel> dataChannel);

	const shared_ptr<Routing> mRouting;
	const shared_ptr<Transport> mTransport;
	const Identifier mRemoteId;

	shared_ptr<rtc::PeerConnection> mPeerConnection;
	shared_ptr<rtc::DataChannel> mDataChannel;
};

} // namespace legio::impl

#endif
