// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>

namespace bitcoin {
namespace serdes {

template <class Sink>
concept byte_sink = requires(Sink& sink, std::span<std::byte const> bytes) {
  { sink.write(bytes) };
};

} // namespace serdes

namespace detail {

class byte_sink_ref
{
public:
  template <serdes::byte_sink Sink>
    requires(!std::same_as<std::remove_cvref_t<Sink>, byte_sink_ref>)
  constexpr byte_sink_ref(Sink& sink) noexcept
    : _object(std::addressof(sink))
    , _write([](void* object, std::span<std::byte const> bytes) {
      static_cast<void>(static_cast<Sink*>(object)->write(bytes));
    })
  {
  }

  void write(std::span<std::byte const> bytes) { _write(_object, bytes); }

private:
  using write_fn = void (*)(void* object, std::span<std::byte const> bytes);

  void* _object;
  write_fn _write;
};

static_assert(serdes::byte_sink<byte_sink_ref>);

} // namespace detail
} // namespace bitcoin
