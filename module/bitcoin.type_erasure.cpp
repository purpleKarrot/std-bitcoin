// SPDX-License-Identifier: BSL-1.0

module;

#include <beman/any_view/any_view.hpp>
#include <beman/any_view/any_view_options.hpp>

export module bitcoin:type_erasure;

import :concepts;
import :vocabulary;

namespace bitcoin::type_erasure {

static constexpr auto sized_random_access =
  beman::any_view::any_view_options::copyable
  | beman::any_view::any_view_options::sized
  | beman::any_view::any_view_options::random_access;

template <typename T>
using any_sized_random_access_view =
  beman::any_view::any_view<T const, sized_random_access>;

using any_chain_view = any_sized_random_access_view<block_header>;
using any_prevouts_view = any_sized_random_access_view<tx_output>;

static_assert(chain_view<any_chain_view>);

class coin_t
{
public:
  coin_t(bitcoin::coin auto const& c)
    : _value{bitcoin::value(c)}
    , _script{bitcoin::output_script(c)}
    , _height{bitcoin::funding_height(c)}
    , _coinbase{bitcoin::is_coinbase(c)}
  {
  }

  [[nodiscard]] auto value() const -> amount { return _value; }
  [[nodiscard]] auto output_script() const -> script_ref { return _script; }
  [[nodiscard]] auto funding_height() const -> std::size_t { return _height; }
  [[nodiscard]] auto is_coinbase() const -> bool { return _coinbase; }

private:
  bitcoin::amount _value;
  bitcoin::script_ref _script;
  std::size_t _height = 0;
  bool _coinbase = false;
};

static_assert(coin<coin_t>);

class coin_index_ref
{
public:
  using mapped_type = coin_t;

  template <coin_index T>
    requires(!std::same_as<std::remove_cvref_t<T>, coin_index_ref>)
  constexpr coin_index_ref(T const& index) noexcept
    : _object(std::addressof(index))
    , _lookup(lookup_fn<T>)
  {
  }

  [[nodiscard]] auto lookup(outpoint const& p) const
    -> std::optional<mapped_type>
  {
    return _lookup(_object, p);
  }

private:
  template <typename T>
  static constexpr auto lookup_fn = [](void const* object, outpoint const& p) {
    auto const& m = *static_cast<T const*>(object);
    return std::optional{m.lookup(p)}.transform(
      [](auto const& c) { return coin_t{c}; });
  };

  void const* _object;
  std::optional<mapped_type> (*_lookup)(void const*, outpoint const&);
};

static_assert(coin_index<coin_index_ref>);

} // namespace bitcoin::type_erasure
