// SPDX-License-Identifier: BSL-1.0

module;

export module bitcoin:customization_points;

import :vocabulary;

namespace bitcoin::customization_points {

void value() = delete;
void output_script() = delete;
void funding_height() = delete;
void is_coinbase() = delete;
void has_witness() = delete;

[[nodiscard]] inline bool is_coinbase(transaction const& t)
{
  auto inputs = t.inputs();
  if (inputs.size() != 1) {
    return false;
  }

  auto const& prevout = inputs.front().prevout();
  return !static_cast<bool>(prevout.txid()) && prevout.index() == 0xFFFF'FFFFu;
}

[[nodiscard]] inline auto output_script(tx_output const& output)
  -> decltype(auto)
{
  return output.script();
}

[[nodiscard]] inline bool has_witness(tx_input const& input)
{
  return !input.witness().empty();
}

[[nodiscard]] inline bool has_witness(transaction const& t)
{
  return t._impl->is_segwit;
}

struct value_fn
{
  template <typename T>
    requires requires(T const& x) { x.value(); }
  || requires(T const& x) { value(x); }
  constexpr decltype(auto) operator()(T const& x) const
  {
    if constexpr (requires { x.value(); }) {
      return x.value();
    }
    else {
      return value(x);
    }
  }
};

struct output_script_fn
{
  template <typename T>
    requires requires(T const& x) { x.output_script(); }
  || requires(T const& x) { output_script(x); }
  constexpr decltype(auto) operator()(T const& x) const
  {
    if constexpr (requires { x.output_script(); }) {
      return x.output_script();
    }
    else {
      return output_script(x);
    }
  }
};

struct funding_height_fn
{
  template <typename T>
    requires requires(T const& x) { x.funding_height(); }
  || requires(T const& x) { funding_height(x); }
  constexpr decltype(auto) operator()(T const& x) const
  {
    if constexpr (requires { x.funding_height(); }) {
      return x.funding_height();
    }
    else {
      return funding_height(x);
    }
  }
};

struct is_coinbase_fn
{
  template <typename T>
    requires requires(T const& x) { x.is_coinbase(); }
  || requires(T const& x) { is_coinbase(x); }
  constexpr decltype(auto) operator()(T const& x) const
  {
    if constexpr (requires { x.is_coinbase(); }) {
      return x.is_coinbase();
    }
    else {
      return is_coinbase(x);
    }
  }
};

struct has_witness_fn
{
  template <typename T>
    requires requires(T const& x) { x.has_witness(); }
  || requires(T const& x) { has_witness(x); }
  constexpr decltype(auto) operator()(T const& x) const
  {
    if constexpr (requires { x.has_witness(); }) {
      return x.has_witness();
    }
    else {
      return has_witness(x);
    }
  }
};

} // namespace bitcoin::customization_points

export namespace bitcoin {

constexpr auto value = customization_points::value_fn{};
constexpr auto output_script = customization_points::output_script_fn{};
constexpr auto funding_height = customization_points::funding_height_fn{};
constexpr auto is_coinbase = customization_points::is_coinbase_fn{};
constexpr auto has_witness = customization_points::has_witness_fn{};

} // namespace bitcoin
