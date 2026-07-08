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
#include <copy_on_write.hpp>

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
                    std::vector<std::vector<std::byte>> witness = {})
    : _prevout{std::move(prevout)}
    , _script{std::move(script)}
    , _sequence{sequence}
    , _witness{std::move(witness)}
  {
  }

  explicit tx_input(tx_input&& other,
                    std::vector<std::vector<std::byte>> witness)
    : _prevout{std::move(other._prevout)}
    , _script{std::move(other._script)}
    , _sequence{other._sequence}
    , _witness{std::move(witness)}
  {
  }

  [[nodiscard]] auto prevout() const noexcept -> outpoint { return _prevout; }
  [[nodiscard]] auto script() const noexcept -> script_ref { return _script; }
  [[nodiscard]] auto sequence() const noexcept -> std::uint32_t
  {
    return _sequence;
  }
  [[nodiscard]] auto witness() const -> witness_view { return _witness; }

  friend bool operator==(tx_input const& lhs,
                         tx_input const& rhs) noexcept = default;

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
  tx_output(bitcoin::amount value, bitcoin::script script)
    : _value{value}
    , _script{std::move(script)}
  {
  }

  [[nodiscard]] auto value() const noexcept -> amount { return _value; }
  [[nodiscard]] auto script() const noexcept -> script_ref { return _script; }

  friend bool operator==(tx_output const& lhs,
                         tx_output const& rhs) noexcept = default;

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
                       std::vector<tx_output> outputs, std::uint32_t locktime);

  [[nodiscard]] auto version() const noexcept -> std::uint32_t
  {
    return _impl->version;
  }

  [[nodiscard]] auto locktime() const noexcept -> std::uint32_t
  {
    return _impl->locktime;
  }

  [[nodiscard]] auto inputs() const -> input_view { return _impl->inputs; }
  [[nodiscard]] auto outputs() const -> output_view { return _impl->outputs; }

  friend bool operator==(transaction const& lhs,
                         transaction const& rhs) noexcept = default;
  friend void detail::serialize(transaction const& tx,
                                detail::byte_sink_ref sink);
  friend auto serialized_size(transaction const& tx) -> std::size_t;

private:
  friend struct detail::txid_policy;
  friend struct detail::wtxid_policy;

  struct implementation
  {
    implementation() = default;
    implementation(std::uint32_t version_, std::vector<tx_input> inputs_,
                   std::vector<tx_output> outputs_, std::uint32_t locktime_);

    std::uint32_t version{};
    std::vector<tx_input> inputs;
    std::vector<tx_output> outputs;
    std::uint32_t locktime{};

    bool has_witness{};
    std::array<std::byte, 32> hash{};
    std::array<std::byte, 32> witness_hash{};

    friend bool operator==(implementation const& lhs,
                           implementation const& rhs) noexcept = default;
  };

  xyz::copy_on_write<implementation> _impl;
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
