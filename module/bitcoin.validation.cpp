// SPDX-License-Identifier: BSL-1.0

module;

#include <chrono>
#include <cstdint>
#include <format>
#include <type_traits>
#include <utility>

#include <beman/any_view/any_view.hpp>
#include <beman/any_view/any_view_options.hpp>

export module bitcoin:validation;

import :coin;
import :consensus_parameters;
import :vocabulary;

export namespace bitcoin {

template <typename T>
concept chain_view = std::ranges::view<T>
  && std::ranges::sized_range<T>
  && std::ranges::random_access_range<T>
  && std::convertible_to<std::ranges::range_reference_t<T>, block_header>;

template <typename T>
concept coin_index = requires(T const& m, outpoint p) {
  { m.lookup(p) } -> std::convertible_to<std::optional<coin>>;
};

class validation_status
{
public:
  constexpr validation_status() = default;
  constexpr validation_status(std::uint8_t status)
    : _status(status)
  {
  }

  [[nodiscard]] constexpr auto ok() const { return _status == 0; }
  constexpr explicit operator bool() const { return ok(); }

private:
  std::uint8_t _status = 0;
};

template <typename Fact>
class validation_result
{
public:
  constexpr validation_result(Fact value)
    : _status{0}
    , _fact{std::move(value)}
  {
  }

  constexpr validation_result(validation_status value)
    : _status{value}
    , _fact{}
  {
  }

  [[nodiscard]] constexpr auto ok() const { return _status.ok(); }
  [[nodiscard]] constexpr auto fact() const& { return _fact; }
  [[nodiscard]] constexpr auto fact() && { return std::move(_fact); }
  [[nodiscard]] constexpr auto status() const { return _status; }

  constexpr explicit operator bool() const { return ok(); }

private:
  validation_status _status;
  Fact _fact;
};

enum class validation_flags : std::uint_least32_t
{
  none = 0,
  p2sh = 1U << 0,                 // BIP16
  dersig = 1U << 2,               // BIP66
  nulldummy = 1U << 4,            // BIP147
  checklocktimeverify = 1U << 9,  // BIP65
  checksequenceverify = 1U << 10, // BIP112
  witness = 1U << 11,             // BIP141
  taproot = 1U << 17,             // BIPs 341 & 342
  all = p2sh
    | dersig
    | nulldummy
    | checklocktimeverify
    | checksequenceverify
    | witness
    | taproot,
};

[[nodiscard]] constexpr auto operator|(validation_flags l,
                                       validation_flags r) noexcept
{
  return validation_flags(static_cast<int>(l) | static_cast<int>(r));
}

} // namespace bitcoin

namespace bitcoin::detail {

class coin_index_ref
{
public:
  template <coin_index T>
    requires(!std::same_as<std::remove_cvref_t<T>, coin_index_ref>)
  constexpr coin_index_ref(T& index) noexcept
    : _object(std::addressof(index))
    , _lookup([](void* object, outpoint const& p) {
      return static_cast<T*>(object)->lookup(p);
    })
  {
  }

  [[nodiscard]] auto lookup(outpoint const& p) const
  {
    return _lookup(_object, p);
  }

private:
  using lookup_fn = std::optional<coin> (*)(void*, outpoint const&);

  void* _object;
  lookup_fn _lookup;
};

static_assert(coin_index<coin_index_ref>);

} // namespace bitcoin::detail

export namespace bitcoin {

class verifier
{
public:
  constexpr explicit verifier(consensus_parameters const& params) noexcept
    : _params(&params)
  {
  }

  //
  // Block header
  //

  [[nodiscard]] auto operator()(block_header const& h) const
  {
    return verify(h);
  }

  template <typename Chain>
    requires chain_view<std::remove_cvref_t<Chain>>
  [[nodiscard]] auto operator()(block_header const& h, Chain&& chain,
                                std::chrono::sys_seconds now) const
  {
    return verify(h, std::forward<Chain>(chain), now);
  }

  //
  // Block
  //

  [[nodiscard]] auto operator()(block const& b) const { return verify(b); }

  template <typename Chain>
    requires chain_view<std::remove_cvref_t<Chain>>
  [[nodiscard]] auto operator()(block const& b, Chain&& chain,
                                std::chrono::sys_seconds now) const
  {
    return verify(b, std::forward<Chain>(chain), now);
  }

  template <typename Chain, typename Coins>
    requires chain_view<std::remove_cvref_t<Chain>>
    && coin_index<std::remove_cvref_t<Coins>>
  [[nodiscard]] auto operator()(block const& b, Chain&& chain,
                                std::chrono::sys_seconds now,
                                Coins&& coins) const
  {
    return verify(b, std::forward<Chain>(chain), now,
                  std::forward<Coins>(coins));
  }

  //
  // Transaction
  //

  [[nodiscard]] auto operator()(transaction const& tx) const
  {
    return verify(tx);
  }

  template <typename Chain>
    requires chain_view<std::remove_cvref_t<Chain>>
  [[nodiscard]] auto operator()(transaction const& tx, Chain&& chain) const
  {
    return verify(tx, std::forward<Chain>(chain));
  }

  template <typename Chain, typename Coins>
    requires chain_view<std::remove_cvref_t<Chain>>
    && coin_index<std::remove_cvref_t<Coins>>
  [[nodiscard]] auto operator()(transaction const& tx, Chain&& chain,
                                Coins&& coins) const
  {
    return verify(tx, std::forward<Chain>(chain), std::forward<Coins>(coins));
  }

  //
  // Script
  //

  [[nodiscard]] auto operator()(script_ref script, amount value,
                                transaction const& tx, std::size_t input_index,
                                validation_flags flags) const
  {
    return verify(script, value, tx, input_index, flags, {});
  }

  template <typename Prevouts>
  [[nodiscard]] auto operator()(script_ref script, amount value,
                                transaction const& tx, std::size_t input_index,
                                validation_flags flags,
                                Prevouts&& prevouts) const
  {
    return verify(script, value, tx, input_index, flags,
                  std::forward<Prevouts>(prevouts));
  }

private:
  static constexpr auto sized_random_access =
    beman::any_view::any_view_options::copyable
    | beman::any_view::any_view_options::sized
    | beman::any_view::any_view_options::random_access;

  template <typename T>
  using any_sized_random_access_view =
    beman::any_view::any_view<T const, sized_random_access>;

  using any_chain_view = any_sized_random_access_view<block_header>;
  using any_prevouts_view = any_sized_random_access_view<tx_output>;

  //
  // Block header
  //

  [[nodiscard]] validation_status verify(block_header const& header) const;
  [[nodiscard]] validation_status verify(block_header const& header,
                                         any_chain_view chain,
                                         std::chrono::sys_seconds now) const;

  //
  // Block
  //

  [[nodiscard]] validation_status verify(bitcoin::block const& block) const;
  [[nodiscard]] validation_status verify(bitcoin::block const& block,
                                         any_chain_view chain,
                                         std::chrono::sys_seconds now) const;
  // validation_status verify(bitcoin::block const& block, chain, now, coins);

  //
  // Transaction
  //

  [[nodiscard]] validation_status verify(bitcoin::transaction const& tx) const;
  [[nodiscard]] validation_status verify(bitcoin::transaction const& tx,
                                         any_chain_view chain) const;
  // validation_status verify(bitcoin::transaction const& tx, chain,
  // coins)const;

  //
  // Script
  //

  [[nodiscard]] validation_status verify(script_ref script, amount value,
                                         transaction const& tx,
                                         std::size_t input_index,
                                         validation_flags flags,
                                         any_prevouts_view prevouts) const;

  //
  // Parameters
  //

  consensus_parameters const* _params;
};

inline constexpr auto verify = verifier{mainnet::params};

} // namespace bitcoin

namespace std {

template <>
struct formatter<bitcoin::validation_flags>
{
  constexpr auto parse(format_parse_context& ctx)
  {
    format_parse_context::iterator const it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      throw format_error("unexpected format specifier");
    }
    return it;
  }

  auto format(bitcoin::validation_flags flags, format_context& ctx) const
    -> format_context::iterator;
};

template <>
struct formatter<bitcoin::validation_status>
{
  constexpr auto parse(format_parse_context& ctx)
  {
    format_parse_context::iterator const it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      throw format_error("unexpected format specifier");
    }
    return it;
  }

  auto format(bitcoin::validation_status status, format_context& ctx) const
    -> format_context::iterator;
};

} // namespace std
