// SPDX-License-Identifier: BSL-1.0

module;

#include <chrono>
#include <format>
#include <string_view>
#include <vector>

#include <beman/any_view/any_view.hpp>
#include <beman/any_view/any_view_options.hpp>

export module bitcoin:vocabulary;

import :amount;
import :hash_id;
import :script;
import copy_on_write;

export namespace bitcoin {

class outpoint
{
public:
  outpoint() = default;
  constexpr outpoint(bitcoin::txid txid, std::size_t index)
    : _txid{txid}
    , _index{index}
  {
  }

  [[nodiscard]] constexpr auto txid() const -> bitcoin::txid { return _txid; }
  [[nodiscard]] constexpr auto index() const -> std::size_t { return _index; }

  friend bool operator==(outpoint const&, outpoint const&) = default;
  friend auto operator<=>(outpoint const& lhs, outpoint const&) = default;

private:
  bitcoin::txid _txid;
  std::size_t _index{};
};

class tx_input
{
public:
  struct witness_bytes : std::span<std::byte const>
  {
    using span<std::byte const>::span;
    friend bool operator==(witness_bytes left, witness_bytes right)
    {
      return std::ranges::equal(left, right);
    }
  };

  using witness_view =
    beman::any_view::any_view<witness_bytes,
                              beman::any_view::any_view_options::random_access
                                | beman::any_view::any_view_options::sized,
                              witness_bytes>;

  tx_input() = default;
  explicit tx_input(bitcoin::outpoint prevout, bitcoin::script script,
                    std::uint32_t sequence,
                    std::vector<std::vector<std::byte>> witness = {})
    : _prevout{prevout}
    , _script{std::move(script)}
    , _sequence{sequence}
    , _witness{std::move(witness)}
  {
  }

  explicit tx_input(tx_input&& other,
                    std::vector<std::vector<std::byte>> witness)
    : _prevout{other._prevout}
    , _script{std::move(other._script)}
    , _sequence{other._sequence}
    , _witness{std::move(witness)}
  {
  }

  [[nodiscard]] auto prevout() const -> outpoint { return _prevout; }
  [[nodiscard]] auto script() const -> script_ref { return _script; }
  [[nodiscard]] auto sequence() const -> std::uint32_t { return _sequence; }
  [[nodiscard]] auto witness() const -> witness_view { return _witness; }

  friend bool operator==(tx_input const&, tx_input const&) = default;

private:
  bitcoin::outpoint _prevout;
  bitcoin::script _script;
  std::uint32_t _sequence{};
  std::vector<std::vector<std::byte>> _witness;
};

class tx_output
{
public:
  tx_output() = default;
  constexpr tx_output(bitcoin::amount value, bitcoin::script script)
    : _value{value}
    , _script{std::move(script)}
  {
  }

  [[nodiscard]] auto value() const noexcept -> amount { return _value; }
  [[nodiscard]] auto script() const noexcept -> script_ref { return _script; }

  friend bool operator==(tx_output const&, tx_output const&) = default;

private:
  bitcoin::amount _value;
  bitcoin::script _script;
};

class transaction
{
public:
  using input_view = std::span<tx_input const>;
  using output_view = std::span<tx_output const>;

  transaction() = default;
  explicit transaction(std::uint32_t version, std::vector<tx_input> inputs,
                       std::vector<tx_output> outputs, std::uint32_t locktime)
    : _impl{std::in_place, version, std::move(inputs), std::move(outputs),
            locktime}
  {
  }

  [[nodiscard]] std::uint32_t version() const { return _impl->version; }
  [[nodiscard]] std::uint32_t locktime() const { return _impl->locktime; }
  [[nodiscard]] input_view inputs() const { return _impl->inputs; }
  [[nodiscard]] output_view outputs() const { return _impl->outputs; }

  friend bool operator==(transaction const&, transaction const&) = default;

private:
  struct implementation
  {
    implementation() = default;
    implementation(std::uint32_t version_, std::vector<tx_input> inputs_,
                   std::vector<tx_output> outputs_, std::uint32_t locktime_);

    std::uint32_t version{};
    std::vector<tx_input> inputs;
    std::vector<tx_output> outputs;
    std::uint32_t locktime{};

    bool is_segwit{};
    std::array<std::byte, 32> hash{};
    std::array<std::byte, 32> witness_hash{};

    friend constexpr bool operator==(implementation const& lhs,
                                     implementation const& rhs)
    {
      return lhs.witness_hash == rhs.witness_hash;
    }
  };

  friend struct txid_policy;
  friend struct wtxid_policy;
  friend bool is_segwit(transaction const& t);

  xyz::copy_on_write<implementation> _impl;
};

struct block_header
{
  std::uint32_t version;
  bitcoin::block_hash prev_block_hash;
  bitcoin::hash256 merkle_root;
  std::chrono::sys_seconds time;
  std::uint32_t bits;
  std::uint32_t nonce;

  friend bool operator==(block_header const&, block_header const&) = default;
  friend auto operator<=>(block_header const&, block_header const&) = default;
};

class block
{
public:
  using transaction_view = std::span<transaction const>;

  block() = default;
  explicit block(block_header header, std::vector<transaction> transactions)
    : _impl{std::in_place, header, std::move(transactions)}
  {
  }

  [[nodiscard]] auto header() const -> block_header const&
  {
    return _impl->header;
  }

  [[nodiscard]] auto transactions() const -> transaction_view
  {
    return _impl->transactions;
  }

  friend bool operator==(block const& lhs, block const& rhs) = default;

private:
  struct implementation
  {
    block_header header;
    std::vector<transaction> transactions;

    friend bool operator==(implementation const&,
                           implementation const&) = default;
  };

  xyz::copy_on_write<implementation> _impl;
};

inline auto txid_policy::operator()(transaction const& tx)
  -> std::array<std::byte, 32>
{
  return tx._impl->hash;
}

inline auto wtxid_policy::operator()(transaction const& tx)
  -> std::array<std::byte, 32>
{
  return tx._impl->witness_hash;
}

inline auto block_hash_policy::operator()(block const& b)
  -> std::array<std::byte, 32>
{
  return operator()(b.header());
}

} // namespace bitcoin

template <>
struct std::hash<bitcoin::outpoint>
{
  [[nodiscard]] auto operator()(bitcoin::outpoint const& value) const noexcept
    -> std::size_t
  {
    auto const txid = hash<bitcoin::txid>{}(value.txid());
    auto const index = hash<std::size_t>{}(value.index());
    constexpr auto mix = 0x9e3779b97f4a7c15ULL;
    return txid ^ (index + mix + (txid << 6U) + (txid >> 2U));
  }
};

template <>
struct std::formatter<bitcoin::outpoint> : std::formatter<std::string_view>
{
  auto format(bitcoin::outpoint const& obj, std::format_context& ctx) const
    -> std::format_context::iterator;
};

template <>
struct std::formatter<bitcoin::tx_input> : std::formatter<std::string_view>
{
  auto format(bitcoin::tx_input const& obj, std::format_context& ctx) const
    -> std::format_context::iterator;
};

template <>
struct std::formatter<bitcoin::tx_output> : std::formatter<std::string_view>
{
  auto format(bitcoin::tx_output const& obj, std::format_context& ctx) const
    -> std::format_context::iterator;
};

template <>
struct std::formatter<bitcoin::transaction> : std::formatter<std::string_view>
{
  auto format(bitcoin::transaction const& obj, std::format_context& ctx) const
    -> std::format_context::iterator;
};

template <>
struct std::formatter<bitcoin::block> : std::formatter<std::string_view>
{
  auto format(bitcoin::block const& obj, std::format_context& ctx) const
    -> std::format_context::iterator;
};
