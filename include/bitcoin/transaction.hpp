// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <utility>

#include <beman/any_view/any_view.hpp>

#include <bitcoin/amount.hpp>
#include <bitcoin/hash_id.hpp>
#include <bitcoin/script.hpp>
#include <bitcoin/serdes/byte_sink.hpp>

namespace bitcoin {

class transaction;

namespace detail {

void serialize(transaction const& tx, byte_sink_ref sink);

} // namespace detail

class outpoint
{
public:
  constexpr outpoint() noexcept = default;
  constexpr outpoint(bitcoin::txid txid, std::size_t index) noexcept
    : _txid(txid)
    , _index(index)
  {
  }

  [[nodiscard]] constexpr auto txid() const noexcept -> bitcoin::txid
  {
    return _txid;
  }

  [[nodiscard]] constexpr auto index() const noexcept -> std::size_t
  {
    return _index;
  }

  constexpr friend bool operator==(outpoint const& lhs,
                                   outpoint const& rhs) noexcept = default;
  constexpr friend auto operator<=>(outpoint const& lhs,
                                    outpoint const& rhs) noexcept = default;

private:
  bitcoin::txid _txid;
  std::size_t _index{};
};

class tx_input
{
public:
  using witness_view =
    beman::any_view::any_view<std::span<std::byte const>,
                              beman::any_view::any_view_options::random_access
                                | beman::any_view::any_view_options::sized,
                              std::span<std::byte const>>;

  constexpr tx_input() = default;
  explicit tx_input(bitcoin::outpoint prevout, bitcoin::script script,
                    std::uint32_t sequence,
                    std::vector<std::vector<std::byte>> witness = {});

  explicit tx_input(tx_input&& other,
                    std::vector<std::vector<std::byte>> witness);

  [[nodiscard]] auto prevout() const noexcept -> outpoint;
  [[nodiscard]] auto script() const noexcept -> script_ref;
  [[nodiscard]] auto sequence() const noexcept -> std::uint32_t;
  [[nodiscard]] auto witness() const -> witness_view;

  friend bool operator==(tx_input const& lhs, tx_input const& rhs) noexcept;

private:
  bitcoin::outpoint _prevout;
  bitcoin::script _script;
  std::uint32_t _sequence{};
  std::vector<std::vector<std::byte>> _witness;
};

class tx_output
{
public:
  constexpr tx_output() = default;
  tx_output(bitcoin::amount value, bitcoin::script script);

  [[nodiscard]] auto value() const noexcept -> amount;
  [[nodiscard]] auto script() const noexcept -> script_ref;

  friend bool operator==(tx_output const& lhs, tx_output const& rhs) noexcept;

private:
  bitcoin::amount _value;
  bitcoin::script _script;
};

class transaction
{
public:
  using input_view = std::span<tx_input const>;
  using output_view = std::span<tx_output const>;

  transaction();
  explicit transaction(std::uint32_t version, std::vector<tx_input> inputs,
                       std::vector<tx_output> outputs, std::uint32_t locktime);

  [[nodiscard]] auto version() const noexcept -> std::uint32_t;
  [[nodiscard]] auto locktime() const noexcept -> std::uint32_t;
  [[nodiscard]] auto inputs() const -> input_view;
  [[nodiscard]] auto outputs() const -> output_view;

  friend bool operator==(transaction const& lhs,
                         transaction const& rhs) noexcept;
  friend void detail::serialize(transaction const& tx,
                                detail::byte_sink_ref sink);
  friend auto serialized_size(transaction const& tx) -> std::size_t;

private:
  std::uint32_t _version{};
  std::vector<tx_input> _inputs;
  std::vector<tx_output> _outputs;
  std::uint32_t _locktime{};

  bool _has_witness;
  std::array<std::byte, 32> _hash;
  std::array<std::byte, 32> _witness_hash;

  friend struct detail::txid_policy;
  friend struct detail::wtxid_policy;
};

auto parse_transaction(std::span<std::byte const> raw)
  -> std::optional<transaction>;

template <serdes::byte_sink Sink>
void serialize(transaction const& tx, Sink& sink)
{
  detail::serialize(tx, detail::byte_sink_ref{sink});
}

auto serialized_size(transaction const& tx) -> std::size_t;

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
