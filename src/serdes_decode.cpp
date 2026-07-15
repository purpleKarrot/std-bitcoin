// SPDX-License-Identifier: BSL-1.0

module;

#include <algorithm>
#include <bit>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

#include <mp-units/framework.h>

module bitcoin;

namespace bitcoin {
namespace {

constexpr std::size_t MAX_SIZE = 4'000'000;
constexpr std::size_t MAX_VECTOR_ALLOCATE = 1'000'000;

template <std::integral T>
constexpr T from_le(T v) noexcept
{
  if constexpr (std::endian::native == std::endian::little) {
    return v;
  }
  else {
    return std::byteswap(v);
  }
}

template <class Source>
concept byte_source = requires(Source& source, std::span<std::byte> bytes) {
  { source.read_some(bytes) } -> std::same_as<std::size_t>;
};

class span_source
{
public:
  constexpr explicit span_source(std::span<std::byte const> data)
    : _data{data}
  {
  }

  constexpr std::size_t read_some(std::span<std::byte> out)
  {
    auto n = std::min(out.size(), _data.size());
    std::memcpy(out.data(), _data.data(), n);
    _data = _data.subspan(n);
    return n;
  }

  [[nodiscard]] constexpr bool empty() const { return _data.empty(); }

private:
  std::span<std::byte const> _data;
};

template <byte_source Source>
class decoder
{
public:
  constexpr explicit decoder(Source source)
    : _source{std::move(source)}
  {
  }

  [[nodiscard]] constexpr bool good() const { return !_failed; }

  constexpr void fail() { _failed = true; }

  constexpr void read(std::span<std::byte> out)
  {
    while (!out.empty() && good()) {
      auto n = _source.read_some(out);
      if (n == 0) {
        fail();
        return;
      }

      out = out.subspan(n);
    }
  }

  constexpr auto source() -> Source& { return _source; }

private:
  Source _source;
  bool _failed = false;
};

constexpr auto decode_u8 = [](auto& r) -> std::uint8_t {
  auto v = std::uint8_t{};
  r.read(as_writable_bytes(std::span{&v, 1}));
  return v;
};

constexpr auto decode_u16 = [](auto& r) -> std::uint16_t {
  auto v = std::uint16_t{};
  r.read(as_writable_bytes(std::span{&v, 1}));
  return from_le(v);
};

constexpr auto decode_u32 = [](auto& r) -> std::uint32_t {
  auto v = std::uint32_t{};
  r.read(as_writable_bytes(std::span{&v, 1}));
  return from_le(v);
};

constexpr auto decode_u64 = [](auto& r) -> std::uint64_t {
  auto v = std::uint64_t{};
  r.read(as_writable_bytes(std::span{&v, 1}));
  return from_le(v);
};

constexpr auto decode_uint256 = [](auto& r) {
  auto h = std::array<std::byte, 32>{};
  r.read(as_writable_bytes(std::span{h}));
  return h;
};

constexpr auto decode_size = [](auto& r) -> std::size_t {
  auto tag = decode_u8(r);
  if (!r.good()) {
    return 0;
  }

  if (tag < 253) {
    return tag;
  }

  if (tag == 253) {
    auto x = decode_u16(r);
    if (!r.good() || x < 253) {
      r.fail();
      return 0;
    }
    return x;
  }

  if (tag == 254) {
    auto x = decode_u32(r);
    if (!r.good() || x < 0x1'0000u) {
      r.fail();
      return 0;
    }
    return x;
  }

  // tag == 255
  auto x = decode_u64(r);
  if (!r.good() || x < 0x1'0000'0000ull || x > MAX_SIZE) {
    r.fail();
    return 0;
  }
  return x;
};

constexpr auto decode_bytes = [](auto& r) -> std::vector<std::byte> {
  auto size = decode_size(r);
  auto buf = std::vector<std::byte>(size);
  r.read(buf);
  return buf;
};

constexpr auto decode_range = [](auto& r, auto decode_elem) {
  using value_type = std::decay_t<decltype(decode_elem(r))>;
  constexpr auto batch_size = MAX_VECTOR_ALLOCATE / sizeof(value_type);

  auto const size = decode_size(r);
  auto out = std::vector<value_type>{};
  out.reserve(std::min(size, batch_size));

  while (out.size() < size && r.good()) {
    if (out.size() == out.capacity()) {
      out.reserve(std::min(size, out.size() + batch_size));
    }
    out.push_back(decode_elem(r));
  }

  return out;
};

constexpr auto decode_outpoint = [](auto& r) {
  auto hash = decode_uint256(r);
  auto index = decode_u32(r);
  return outpoint{txid{hash}, index};
};

constexpr auto decode_txin = [](auto& r) {
  auto prevout = decode_outpoint(r);
  auto script = bitcoin::script{decode_bytes(r)};
  auto sequence = decode_u32(r);
  return tx_input{prevout, std::move(script), sequence};
};

constexpr auto decode_txout = [](auto& r) {
  auto amount = decode_u64(r) * bitcoin::units::satoshi;
  auto script = bitcoin::script{decode_bytes(r)};
  return tx_output{amount, std::move(script)};
};

constexpr auto decode_witness = [](auto& r, std::vector<tx_input> in) {
  for (auto& elem : in) {
    auto witness = decode_range(r, decode_bytes);
    elem = tx_input{std::move(elem), std::move(witness)};
  }
  return in;
};

constexpr auto decode_tx = [](auto& r) -> transaction {
  auto const version = decode_u32(r);
  auto inputs = decode_range(r, decode_txin);
  auto outputs = std::vector<tx_output>{};

  if (!inputs.empty()) {
    outputs = decode_range(r, decode_txout);
  }
  else {
    auto const flags = decode_u8(r);
    if (flags == 1) {
      inputs = decode_range(r, decode_txin);
      outputs = decode_range(r, decode_txout);
    }
    else if (flags != 0) {
      r.fail();
      return {};
    }
    inputs = decode_witness(r, std::move(inputs));
    auto const has_witness = std::ranges::any_of(
      inputs, [](tx_input const& elem) { return !elem.witness().empty(); });
    if (!has_witness) {
      r.fail();
      return {};
    }
  }

  auto const locktime = decode_u32(r);
  return transaction{version, std::move(inputs), std::move(outputs), locktime};
};

constexpr auto decode_block_header = [](auto& r) {
  auto version = decode_u32(r);
  auto prev_block = decode_uint256(r);
  auto merkle_root = decode_uint256(r);
  auto time = decode_u32(r);
  auto bits = decode_u32(r);
  auto nonce = decode_u32(r);
  return block_header{
    .version = version,
    .prev_block_hash = block_hash{prev_block},
    .merkle_root = hash256{merkle_root},
    .time = std::chrono::sys_seconds{std::chrono::seconds{time}},
    .bits = bits,
    .nonce = nonce,
  };
};

constexpr auto decode_block = [](auto& r) {
  auto header = decode_block_header(r);
  auto transactions = decode_range(r, decode_tx);
  return block{std::move(header), std::move(transactions)};
};

} // namespace

auto parse_block(std::span<std::byte const> raw) -> std::optional<block>
{
  auto dec = decoder{span_source{raw}};
  auto b = decode_block(dec);
  if (!dec.good() || !dec.source().empty()) {
    return std::nullopt;
  }
  return b;
}

auto parse_block_header(std::span<std::byte const> raw)
  -> std::optional<block_header>
{
  auto dec = decoder{span_source{raw}};
  auto header = decode_block_header(dec);
  if (!dec.good() || !dec.source().empty()) {
    return std::nullopt;
  }
  return header;
}

auto parse_transaction(std::span<std::byte const> raw)
  -> std::optional<transaction>
{
  auto dec = decoder{span_source{raw}};
  auto tx = decode_tx(dec);
  if (!dec.good() || !dec.source().empty()) {
    return std::nullopt;
  }
  return tx;
}

} // namespace bitcoin
