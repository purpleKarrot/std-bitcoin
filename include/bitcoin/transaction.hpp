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
#include <bitcoin/serialization/byte_sink.hpp>

namespace bitcoin {

class transaction;

namespace detail {

struct transaction_data;
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

  [[nodiscard]] auto prevout() const noexcept -> outpoint;
  [[nodiscard]] auto script() const noexcept -> script_ref;
  [[nodiscard]] auto sequence() const noexcept -> std::uint32_t;
  [[nodiscard]] auto witness() const -> witness_view;

  friend bool operator==(tx_input const& lhs, tx_input const& rhs) noexcept;

  tx_input(std::shared_ptr<detail::transaction_data const> data,
           std::size_t index) noexcept;

private:
  std::shared_ptr<detail::transaction_data const> _data;
  std::size_t _index{};
};

class tx_output
{
public:
  tx_output(amount value, script_ref script);

  [[nodiscard]] auto value() const noexcept -> amount;
  [[nodiscard]] auto script() const noexcept -> script_ref;

  friend bool operator==(tx_output const& lhs, tx_output const& rhs) noexcept;

  tx_output(std::shared_ptr<detail::transaction_data const> data,
            std::size_t index) noexcept;

private:
  std::shared_ptr<detail::transaction_data const> _data;
  std::size_t _index{};
  friend struct _impl_access;
};

class transaction
{
  static constexpr auto sized_random_access =
    beman::any_view::any_view_options::random_access
    | beman::any_view::any_view_options::sized;

public:
  using input_view =
    beman::any_view::any_view<tx_input, sized_random_access, tx_input>;
  using output_view =
    beman::any_view::any_view<tx_output, sized_random_access, tx_output>;

  transaction();
  explicit transaction(std::shared_ptr<detail::transaction_data const> data);

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
  std::shared_ptr<detail::transaction_data const> _data;
  friend struct _impl_access;
  friend struct detail::txid_policy;
  friend struct detail::wtxid_policy;
};

auto parse_transaction(std::span<std::byte const> raw)
  -> std::optional<transaction>;

template <serialization::byte_sink Sink>
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
