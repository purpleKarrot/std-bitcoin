// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <bitcoin/amount.hpp>
#include <bitcoin/block.hpp>
#include <bitcoin/hash_id.hpp>
#include <bitcoin/transaction.hpp>

#include "primitives/block.h"
#include "primitives/transaction.h"

namespace legacy {

template <class P>
inline auto convert_uint256(bitcoin::detail::basic_hash_id<P> const& in)
{
  auto const bytes = as_bytes(in);
  auto const* data = reinterpret_cast<uint8_t const*>(bytes.data());
  return uint256{std::span{data, 32}};
}

inline constexpr auto convert_script = [](bitcoin::script_ref script) {
  auto const bytes = as_bytes(script);
  auto const* data = reinterpret_cast<uint8_t const*>(bytes.data());
  return CScript{data, data + bytes.size()};
};

inline constexpr auto convert_outpoint = [](bitcoin::outpoint const& in) {
  auto out = COutPoint{};
  out.hash = Txid::FromUint256(convert_uint256(in.txid()));
  out.n = in.index();
  return out;
};

inline constexpr auto convert_txin = [](bitcoin::tx_input const& in) {
  constexpr auto convert_witness = [](std::span<std::byte const> in) {
    auto const* data = reinterpret_cast<std::uint8_t const*>(in.data());
    return std::vector<std::uint8_t>{data, data + in.size()};
  };

  auto out = CTxIn{};
  out.prevout = convert_outpoint(in.prevout());
  out.scriptSig = convert_script(in.script());
  out.nSequence = in.sequence();
  // out.scriptWitness.stack = in.witness()
  //   | std::views::transform(convert_witness)
  //   | std::ranges::to<std::vector>();
  auto witness = in.witness();
  out.scriptWitness.stack.reserve(witness.size());
  for (std::size_t i = 0; i < witness.size(); ++i) {
    out.scriptWitness.stack.push_back(convert_witness(witness[i]));
  }
  return out;
};

inline constexpr auto convert_txout = [](bitcoin::tx_output const& in) {
  auto out = CTxOut{};
  out.nValue = in.value().numerical_value_in(bitcoin::units::satoshi);
  out.scriptPubKey = convert_script(in.script());
  return out;
};

inline constexpr auto convert_tx = [](bitcoin::transaction const& tx) {
  auto out = CMutableTransaction{};
  out.version = tx.version();
  out.vin = tx.inputs()
    | std::views::transform(convert_txin)
    | std::ranges::to<std::vector>();
  out.vout = tx.outputs()
    | std::views::transform(convert_txout)
    | std::ranges::to<std::vector>();
  out.nLockTime = tx.locktime();
  return CTransaction{out};
};

inline constexpr auto convert_header = [](bitcoin::block_header const& in) {
  auto out = CBlockHeader{};
  out.nVersion = in.version;
  out.hashPrevBlock = convert_uint256(in.prev_block_hash);
  out.hashMerkleRoot = convert_uint256(in.merkle_root);
  out.nTime = in.time.time_since_epoch().count();
  out.nBits = in.bits;
  out.nNonce = in.nonce;
  return out;
};

inline constexpr auto convert_block = [](bitcoin::block const& in) {
  constexpr auto make_ref = [](CTransaction tx) {
    return MakeTransactionRef(std::move(tx));
  };
  auto out = CBlock{convert_header(in.header())};
  out.vtx = in.transactions()
    | std::views::transform(convert_tx)
    | std::views::transform(make_ref)
    | std::ranges::to<std::vector>();
  return out;
};

} // namespace legacy
