// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstddef>
#include <span>

namespace bitcoin::serialization {

template <class Sink>
concept byte_sink = requires(Sink& sink, std::span<std::byte const> bytes) {
  { sink.write(bytes) };
};

} // namespace bitcoin::serialization
