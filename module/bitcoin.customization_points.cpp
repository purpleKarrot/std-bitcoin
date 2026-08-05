// SPDX-License-Identifier: BSL-1.0

export module bitcoin:customization_points;

namespace bitcoin::customization_points {

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

export namespace bitcoin::inline adl_barrier {

constexpr auto value = customization_points::value_fn{};
constexpr auto output_script = customization_points::output_script_fn{};
constexpr auto funding_height = customization_points::funding_height_fn{};
constexpr auto is_coinbase = customization_points::is_coinbase_fn{};
constexpr auto has_witness = customization_points::has_witness_fn{};

} // namespace bitcoin::inline adl_barrier
