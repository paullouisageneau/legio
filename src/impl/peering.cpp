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

#include "peering.hpp"
#include "message.hpp"

namespace legio::impl {

const string DataChannelName = "legio";

Peering::Peering(shared_ptr<Routing> routing, shared_ptr<const Provisioning> provisioning,
                 shared_ptr<Transport> transport, Identifier remoteId)
    : mRouting(std::move(routing)), mProvisioning(std::move(provisioning)),
      mTransport(std::move(transport)), mRemoteId(std::move(remoteId)) {
	createPeerConnection();
}

Peering::~Peering() { disconnect(); }

const Identifier &Peering::remoteId() const { return mRemoteId; }

bool Peering::isConnected() const { return mDataChannel && mDataChannel->isOpen(); }

void Peering::connect() {
	disconnect();
	setDataChannel(mPeerConnection->createDataChannel(DataChannelName));
}

void Peering::disconnect() {
	if (mDataChannel) {
		mRouting->removeChannel(mDataChannel);
		mDataChannel->close();
		mDataChannel.reset();
	}
}

void Peering::receive(binary payload) {
	auto [type, sdp] = unpack_strings<2>(payload);

	rtc::Description description(std::move(sdp), std::move(type));
	std::cout << "Remote description, type=" << description.typeString() << ": " << description
	          << std::endl;

	using Type = rtc::Description::Type;
	using SignalingState = rtc::PeerConnection::SignalingState;
	if (description.type() == Type::Offer &&
	    mPeerConnection->signalingState() == SignalingState::HaveLocalOffer) {
		if (mRouting->localId() > mRemoteId) // Tiebreaker
			return;                          // Ignore offer
	}

	mPeerConnection->setRemoteDescription(description);
}

void Peering::createPeerConnection() {
	disconnect();

	rtc::Configuration config;
	// TODO: Make STUN server parametrable
	config.iceServers.emplace_back("stun:stun.ageneau.net:3478");

	for(const auto &entry : mProvisioning->pick(2))
		config.iceServers.emplace_back(entry.url());

	mPeerConnection = std::make_shared<rtc::PeerConnection>(config);

	using GatheringState = rtc::PeerConnection::GatheringState;

	mPeerConnection->onLocalDescription([this](rtc::Description description) {
		if (mPeerConnection->gatheringState() == GatheringState::Complete)
			sendLocalDescription(std::move(description));
	});

	mPeerConnection->onGatheringStateChange([this](GatheringState state) {
		if (state == GatheringState::Complete)
			if (auto description = mPeerConnection->localDescription())
				sendLocalDescription(std::move(*description));
	});

	mPeerConnection->onDataChannel([this](shared_ptr<rtc::DataChannel> dataChannel) {
		std::cout << "Data channel received" << std::endl;
		if (dataChannel->label() == DataChannelName)
			setDataChannel(dataChannel);
	});
}

void Peering::sendLocalDescription(rtc::Description description) {
	std::cout << "Local description, type=" << description.typeString() << ": " << description
	          << std::endl;
	mTransport->send(mRemoteId, pack_strings<2>({description.typeString(), string(description)}));
}

void Peering::setDataChannel(shared_ptr<rtc::DataChannel> dataChannel) {
	mDataChannel = dataChannel;

	mDataChannel->onOpen([this]() {
		std::cout << "Data channel open" << std::endl;
		mRouting->addChannel(mDataChannel);
		mRouting->addNeighbor(mRemoteId, mDataChannel);
	});

	mDataChannel->onClosed([this]() {
		std::cout << "Data channel closed" << std::endl;
		mRouting->removeChannel(mDataChannel);
	});

	mDataChannel->onError(
	    [](const string &error) { std::cerr << "Data channel error: " << error << std::endl; });
}

} // namespace legio::impl
