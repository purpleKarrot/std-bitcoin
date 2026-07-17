// SPDX-License-Identifier: BSL-1.0

module;

#include <ranges>

#include "consensus/params.h"
#include "primitives/block.h"
#include "primitives/transaction.h"

export module legacy;

import bitcoin;

export namespace legacy {

auto convert_uint256(auto const& in)
{
  auto const bytes = as_bytes(in);
  static_assert(bytes.size() == 32);
  auto const* data = reinterpret_cast<uint8_t const*>(bytes.data());
  return uint256{std::span{data, 32}};
}

constexpr auto convert_script = [](bitcoin::script_ref script) {
  auto const bytes = as_bytes(script);
  auto const* data = reinterpret_cast<uint8_t const*>(bytes.data());
  return CScript{data, data + bytes.size()};
};

constexpr auto convert_outpoint = [](bitcoin::outpoint const& in) {
  auto out = COutPoint{};
  out.hash = Txid::FromUint256(convert_uint256(in.txid()));
  out.n = in.index();
  return out;
};

constexpr auto convert_txin = [](bitcoin::tx_input const& in) {
  constexpr auto convert_witness = [](std::span<std::byte const> in) {
    auto const* data = reinterpret_cast<std::uint8_t const*>(in.data());
    return std::vector<std::uint8_t>{data, data + in.size()};
  };

  auto out = CTxIn{};
  out.prevout = convert_outpoint(in.prevout());
  out.scriptSig = convert_script(in.script());
  out.nSequence = in.sequence();
  out.scriptWitness.stack = in.witness()
    | std::views::transform(convert_witness)
    | std::ranges::to<std::vector>();
  return out;
};

constexpr auto convert_txout = [](bitcoin::tx_output const& in) {
  auto out = CTxOut{};
  out.nValue = in.value().numerical_value_in(bitcoin::units::satoshi);
  out.scriptPubKey = convert_script(in.script());
  return out;
};

constexpr auto convert_tx = [](bitcoin::transaction const& tx) {
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

constexpr auto convert_header = [](bitcoin::block_header const& in) {
  auto out = CBlockHeader{};
  out.nVersion = static_cast<std::int32_t>(in.version);
  out.hashPrevBlock = convert_uint256(in.prev_block_hash);
  out.hashMerkleRoot = convert_uint256(in.merkle_root);
  out.nTime = in.time.time_since_epoch().count();
  out.nBits = in.bits;
  out.nNonce = in.nonce;
  return out;
};

constexpr auto convert_block = [](bitcoin::block const& in) {
  constexpr auto make_ref = [](CTransaction tx) {
    return std::make_shared<CTransaction const>(std::move(tx));
  };
  auto out = CBlock{convert_header(in.header())};
  out.vtx = in.transactions()
    | std::views::transform(convert_tx)
    | std::views::transform(make_ref)
    | std::ranges::to<std::vector>();
  return out;
};

constexpr auto convert_consensus = [](bitcoin::consensus_parameters const& in) {
  auto out = Consensus::Params{};

  out.hashGenesisBlock = convert_uint256(in.genesis_block_hash);
  out.nSubsidyHalvingInterval = static_cast<int>(in.halving_interval);
  // out.script_flag_exceptions;
  // out.BIP34Height;
  // out.BIP34Hash;
  // out.BIP65Height;
  // out.BIP66Height;
  // out.CSVHeight;
  // out.SegwitHeight;
  // out.MinBIP9WarningHeight;
  // out.vDeployments;
  // out.powLimit;
  // out.fPowAllowMinDifficultyBlocks;
  // out.enforce_BIP94;
  // out.fPowNoRetargeting;
  // out.nPowTargetSpacing;
  // out.nPowTargetTimespan;
  // out.nMinimumChainWork;
  // out.defaultAssumeValid;
  // out.signet_blocks;
  // out.signet_challenge;

  return out;
};

} // namespace legacy
