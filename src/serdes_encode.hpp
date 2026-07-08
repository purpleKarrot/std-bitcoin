// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <bit>
#include <cstring>
#include <ranges>
#include <span>

#include <bitcoin/block.hpp>
#include <bitcoin/block_header.hpp>
#include <bitcoin/predicates.hpp>
#include <bitcoin/serdes/byte_sink.hpp>
#include <bitcoin/transaction.hpp>

#include "bitcoin/amount.hpp"
#include "bitcoin/hash_id.hpp"

namespace bitcoin::serdes {

enum class witness
{
  disallow,
  allow,
};

template <std::integral T>
constexpr T to_le(T v) noexcept
{
  if constexpr (std::endian::native == std::endian::little) {
    return v;
  }
  else {
    return std::byteswap(v);
  }
}

inline constexpr auto encode_u8 = [](auto& w, std::uint8_t v) {
  w.write(as_bytes(std::span{&v, 1}));
};

inline constexpr auto encode_u16 = [](auto& w, std::uint16_t v) {
  v = to_le(v);
  w.write(as_bytes(std::span{&v, 1}));
};

inline constexpr auto encode_u32 = [](auto& w, std::uint32_t v) {
  v = to_le(v);
  w.write(as_bytes(std::span{&v, 1}));
};

inline constexpr auto encode_u64 = [](auto& w, std::uint64_t v) {
  v = to_le(v);
  w.write(as_bytes(std::span{&v, 1}));
};

inline constexpr auto encode_hash256 =
  []<class P>(auto& w, detail::basic_hash_id<P> const& v) {
    w.write(as_bytes(v));
  };

inline constexpr auto encode_size = [](auto& w, std::size_t v) {
  if (v < 253) {
    encode_u8(w, static_cast<std::uint8_t>(v));
  }
  else if (v <= 0xFFFF) {
    encode_u8(w, 253);
    encode_u16(w, static_cast<std::uint16_t>(v));
  }
  else if (v <= 0xFFFF'FFFF) {
    encode_u8(w, 254);
    encode_u32(w, static_cast<std::uint32_t>(v));
  }
  else {
    encode_u8(w, 255);
    encode_u64(w, static_cast<std::uint64_t>(v));
  }
};

inline constexpr auto encode_bytes = [](auto& w,
                                        std::span<std::byte const> bytes) {
  encode_size(w, bytes.size());
  w.write(bytes);
};

// TODO: investigate what any_view has against a range based loop here
inline constexpr auto encode_range = [](auto& w, auto&& range,
                                        auto encode_elem) {
  std::size_t const size = std::ranges::size(range);
  encode_size(w, size);
  for (std::size_t i = 0; i < size; ++i) {
    encode_elem(w, range[i]);
  }
};

inline constexpr auto encode_outpoint = [](auto& w, outpoint const& p) {
  encode_hash256(w, p.txid());
  encode_u32(w, p.index());
};

inline constexpr auto encode_txin = [](auto& w, tx_input const& in) {
  encode_outpoint(w, in.prevout());
  encode_bytes(w, as_bytes(in.script()));
  encode_u32(w, in.sequence());
};

inline constexpr auto encode_txout = [](auto& w, tx_output const& out) {
  encode_u64(w, out.value().numerical_value_in(units::satoshi));
  encode_bytes(w, as_bytes(out.script()));
};

inline constexpr auto encode_tx = [](witness wmode) {
  return [=](auto& w, transaction const& tx) {
    bool const with_witness = (wmode == witness::allow); // && has_witness(tx));

    encode_u32(w, tx.version());

    if (with_witness) {
      encode_u8(w, 0);
      encode_u8(w, 1);
    }

    encode_range(w, tx.inputs(), encode_txin);
    encode_range(w, tx.outputs(), encode_txout);

    if (with_witness) {
      for (auto const& in : tx.inputs()) {
        encode_range(w, in.witness(), encode_bytes);
      }
    }

    encode_u32(w, tx.locktime());
  };
};

inline constexpr auto encode_block_header = [](auto& w, block_header const& h) {
  encode_u32(w, h.version);
  encode_hash256(w, h.prev_block_hash);
  encode_hash256(w, h.merkle_root);
  encode_u32(w, h.time.time_since_epoch().count());
  encode_u32(w, h.bits);
  encode_u32(w, h.nonce);
};

inline constexpr auto encode_block = [](auto& w, block const& b) {
  encode_block_header(w, b.header());
  encode_range(w, b.transactions(), encode_tx(witness::allow));
};

template <byte_sink Sink, std::size_t BufferSize = 512>
class buffered_sink
{
public:
  static_assert(BufferSize > 0);

  constexpr explicit buffered_sink(Sink sink)
    : _sink{std::move(sink)}
  {
  }

  buffered_sink(buffered_sink const&) = delete;
  buffered_sink& operator=(buffered_sink const&) = delete;
  buffered_sink(buffered_sink&&) = default;
  buffered_sink& operator=(buffered_sink&&) = default;

  ~buffered_sink() noexcept(false)
  {
    if (_pos > 0) {
      _sink.write(std::span{_buffer}.subspan(0, _pos));
    }
  }

  constexpr void write(std::span<std::byte const> bytes)
  {
    while (!bytes.empty()) {
      std::size_t const n = std::min(bytes.size(), BufferSize - _pos);
      std::memcpy(_buffer.data() + _pos, bytes.data(), n);

      _pos += n;
      bytes = bytes.subspan(n);

      if (_pos == BufferSize) {
        _sink.write(_buffer);
        _pos = 0;
      }
    }
  }

private:
  Sink _sink;
  std::array<std::byte, BufferSize> _buffer{};
  std::size_t _pos = 0;
};

class counting_sink
{
public:
  constexpr void write(std::span<std::byte const> bytes) noexcept
  {
    _size += bytes.size();
  }

  [[nodiscard]] constexpr std::size_t size() const noexcept { return _size; }

private:
  std::size_t _size = 0;
};

static_assert(byte_sink<counting_sink>);
static_assert(byte_sink<buffered_sink<counting_sink>>);

} // namespace bitcoin::serdes
