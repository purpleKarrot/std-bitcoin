// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/block.hpp>

#include <cassert>
#include <ranges>
#include <vector>

#include "detail.hpp"
#include "primitives/block.h"
#include "serialize.h"
#include "streams.h"

namespace bitcoin {

namespace {

auto make_block_header(CBlockHeader const& header) -> bitcoin::block_header
{
  auto stream = DataStream{};
  stream << header;
  auto const parsed = parse_block_header(
    std::span<std::byte const>{stream.data(), stream.size()});
  assert(parsed.has_value());
  return *parsed;
}

} // namespace

block::block()
  : block(detail::block_data{})
{
}

block::block(detail::block_data data)
  : _header{make_block_header(data)}
  , _data{std::make_shared<detail::block_data>(std::move(data))}
{
}

auto block::header() const noexcept -> block_header const&
{
  return _header;
}

auto block::transactions() const -> transaction_view
{
  constexpr auto convert = [](CTransactionRef const& in) {
    auto mut = CMutableTransaction{*in};
    auto ptr = std::make_shared<detail::transaction_data>(std::move(mut));
    return bitcoin::transaction{std::move(ptr)};
  };

  assert(_data != nullptr);
  return _data->vtx | std::views::transform(convert);
}

auto operator==(block const& lhs, block const& rhs) noexcept -> bool
{
  if (lhs._data == rhs._data) {
    return true;
  }

  if (lhs._data == nullptr || rhs._data == nullptr) {
    return lhs._data == rhs._data;
  }

  return (lhs._header == rhs._header)
    && std::ranges::equal(
           lhs._data->vtx, rhs._data->vtx,
           [](auto const& l, auto const& r) { return *l == *r; });
}

auto parse_block(std::span<std::byte const> raw) -> std::optional<block>
{
  try {
    auto stream = DataStream{raw};
    auto data = detail::block_data{};
    stream >> TX_WITH_WITNESS(data);
    if (!stream.empty()) {
      return std::nullopt;
    }
    return block{std::move(data)};
  }
  catch (std::ios_base::failure const&) {
    return std::nullopt;
  }
}

void serialize(block const& b, serialization::byte_sink_ref sink)
{
  assert(b._data != nullptr);
  ::Serialize(sink, TX_WITH_WITNESS(*b._data));
}

auto serialized_size(block const& b) -> std::size_t
{
  assert(b._data != nullptr);
  return ::GetSerializeSize(TX_WITH_WITNESS(*b._data));
}

} // namespace bitcoin
