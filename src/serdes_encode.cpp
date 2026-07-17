// SPDX-License-Identifier: BSL-1.0

module;

#include <bit>
#include <cstring>
#include <ranges>
#include <span>

#include "hash.h"

module bitcoin;

namespace bitcoin {
namespace {

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

constexpr auto encode_u8 = [](auto& w, std::uint8_t v) {
  w.write(as_bytes(std::span{&v, 1}));
};

constexpr auto encode_u16 = [](auto& w, std::uint16_t v) {
  v = to_le(v);
  w.write(as_bytes(std::span{&v, 1}));
};

constexpr auto encode_u32 = [](auto& w, std::uint32_t v) {
  v = to_le(v);
  w.write(as_bytes(std::span{&v, 1}));
};

constexpr auto encode_u64 = [](auto& w, std::uint64_t v) {
  v = to_le(v);
  w.write(as_bytes(std::span{&v, 1}));
};

constexpr auto encode_hash256 = [](auto& w, auto const& v) {
  auto const bytes = as_bytes(v);
  static_assert(bytes.size() == 32);
  w.write(bytes);
};

constexpr auto encode_size = [](auto& w, std::size_t v) {
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

constexpr auto encode_bytes = [](auto& w, std::span<std::byte const> bytes) {
  encode_size(w, bytes.size());
  w.write(bytes);
};

constexpr auto encode_range = [](auto& w, auto&& range, auto encode_elem) {
  encode_size(w, std::ranges::size(range));
  for (auto&& elem : std::forward<decltype(range)>(range)) {
    encode_elem(w, elem);
  }
};

constexpr auto encode_outpoint = [](auto& w, outpoint const& p) {
  encode_hash256(w, p.txid());
  encode_u32(w, p.index());
};

constexpr auto encode_txin = [](auto& w, tx_input const& in) {
  encode_outpoint(w, in.prevout());
  encode_bytes(w, as_bytes(in.script()));
  encode_u32(w, in.sequence());
};

constexpr auto encode_txout = [](auto& w, tx_output const& out) {
  encode_u64(w, out.value().numerical_value_in(units::satoshi));
  encode_bytes(w, as_bytes(out.script()));
};

constexpr auto encode_tx_data =
  [](auto& w, std::uint32_t version, std::span<tx_input const> inputs,
     std::span<tx_output const> outputs, std::uint32_t locktime, bool segwit) {
    encode_u32(w, version);

    if (segwit) {
      encode_u8(w, 0);
      encode_u8(w, 1);
    }

    encode_range(w, inputs, encode_txin);
    encode_range(w, outputs, encode_txout);

    if (segwit) {
      for (auto const& in : inputs) {
        encode_range(w, in.witness(), encode_bytes);
      }
    }

    encode_u32(w, locktime);
  };

constexpr auto encode_tx = [](auto& w, transaction const& tx) {
  encode_tx_data(w, tx.version(), tx.inputs(), tx.outputs(), tx.locktime(),
                 is_segwit(tx));
};

constexpr auto encode_block_header = [](auto& w, block_header const& h) {
  encode_u32(w, h.version);
  encode_hash256(w, h.prev_block_hash);
  encode_hash256(w, h.merkle_root);
  encode_u32(w, h.time.time_since_epoch().count());
  encode_u32(w, h.bits);
  encode_u32(w, h.nonce);
};

constexpr auto encode_block = [](auto& w, block const& b) {
  encode_block_header(w, b.header());
  encode_range(w, b.transactions(),
               [](auto& w, transaction const& tx) { encode_tx(w, tx); });
};

template <serdes::byte_sink Sink, std::size_t BufferSize = 512>
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

static_assert(serdes::byte_sink<counting_sink>);
static_assert(serdes::byte_sink<buffered_sink<counting_sink>>);

inline bool has_witness(std::vector<tx_input> const& inputs)
{
  return std::ranges::any_of(
    inputs, [](tx_input const& input) { return !input.witness().empty(); });
}

inline auto calc_hash(std::uint32_t version, std::span<tx_input const> inputs,
                      std::span<tx_output const> outputs,
                      std::uint32_t locktime, bool segwit = false)
{
  auto hasher = HashWriter{};
  auto hash = std::array<std::byte, 32>{};
  encode_tx_data(hasher, version, inputs, outputs, locktime, segwit);
  std::ranges::transform(hasher.GetHash(), hash.begin(),
                         [](auto byte) { return std::byte{byte}; });
  return hash;
}

} // namespace

transaction::implementation::implementation(std::uint32_t version_,
                                            std::vector<tx_input> inputs_,
                                            std::vector<tx_output> outputs_,
                                            std::uint32_t locktime_)
  : version{version_}
  , inputs{std::move(inputs_)}
  , outputs{std::move(outputs_)}
  , locktime{locktime_}
  , is_segwit{has_witness(inputs)}
  , hash{calc_hash(version, inputs, outputs, locktime)}
  , witness_hash{calc_hash(version, inputs, outputs, locktime, is_segwit)}
{
}

void serdes::serialize(block const& b, byte_sink_ref sink)
{
  auto buf = buffered_sink<byte_sink_ref>{sink};
  encode_block(buf, b);
}

void serdes::serialize(block_header const& header, byte_sink_ref sink)
{
  auto buf = buffered_sink<byte_sink_ref, 80>{sink};
  encode_block_header(buf, header);
}

void serdes::serialize(transaction const& tx, byte_sink_ref sink)
{
  auto buf = buffered_sink<byte_sink_ref>{sink};
  encode_tx(buf, tx);
}

auto serialized_size(block const& b) -> std::size_t
{
  auto sink = counting_sink{};
  encode_block(sink, b);
  return sink.size();
}

auto serialized_size(block_header const& /*unused*/) -> std::size_t
{
  return 80;
}

auto serialized_size(transaction const& tx) -> std::size_t
{
  auto sink = counting_sink{};
  encode_tx(sink, tx);
  return sink.size();
}

auto block_hash_policy::operator()(block_header const& hdr)
  -> std::array<std::byte, 32>
{
  auto hasher = HashWriter{};
  auto hash = std::array<std::byte, 32>{};
  encode_block_header(hasher, hdr);
  std::ranges::transform(hasher.GetHash(), hash.begin(),
                         [](auto byte) { return std::byte{byte}; });
  return hash;
}

} // namespace bitcoin
