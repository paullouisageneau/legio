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

#ifndef LEGIO_IMPL_EVENT_H
#define LEGIO_IMPL_EVENT_H

#include "common.hpp"
#include "message.hpp"

#include <variant>

#include <rtc/channel.hpp>

namespace legio::impl {

using rtc::Channel;
using channel_ptr = shared_ptr<Channel>;

namespace events {

struct Message {
	message_ptr message;
	channel_ptr channel;
};

struct Neighbor {
	Identifier id;
	channel_ptr channel;
};

using variant = std::variant<Message, Neighbor>;

}

} // namespace legio::impl

#endif
