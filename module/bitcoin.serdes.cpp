// SPDX-License-Identifier: BSL-1.0

module;

#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>

export module bitcoin:serdes;
import :vocabulary;

export namespace bitcoin::serdes {

template <class Sink>
concept byte_sink = requires(Sink& sink, std::span<std::byte const> bytes) {
  { sink.write(bytes) };
};

} // namespace bitcoin::serdes

namespace bitcoin::detail {

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

void serialize(block const& b, byte_sink_ref sink);
void serialize(block_header const& header, byte_sink_ref sink);
void serialize(transaction const& tx, byte_sink_ref sink);

} // namespace bitcoin::detail

export namespace bitcoin {

[[nodiscard]] auto parse_block(std::span<std::byte const> raw)
  -> std::optional<block>;

[[nodiscard]] auto parse_block_header(std::span<std::byte const> raw)
  -> std::optional<block_header>;

[[nodiscard]] auto parse_transaction(std::span<std::byte const> raw)
  -> std::optional<transaction>;

template <serdes::byte_sink Sink>
void serialize(block const& b, Sink& sink)
{
  detail::serialize(b, detail::byte_sink_ref{sink});
}

template <serdes::byte_sink Sink>
void serialize(block_header const& header, Sink& sink)
{
  detail::serialize(header, detail::byte_sink_ref{sink});
}

template <serdes::byte_sink Sink>
void serialize(transaction const& tx, Sink& sink)
{
  detail::serialize(tx, detail::byte_sink_ref{sink});
}

[[nodiscard]] auto serialized_size(block const& b) -> std::size_t;
[[nodiscard]] auto serialized_size(block_header const& header) -> std::size_t;
[[nodiscard]] auto serialized_size(transaction const& tx) -> std::size_t;

} // namespace bitcoin
