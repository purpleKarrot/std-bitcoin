// SPDX-License-Identifier: BSL-1.0

import bitcoin;

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>

#include <doctest/doctest.h>
#include <mp-units/framework.h>

using namespace bitcoin::units;

namespace {
namespace coin_with_observers {

struct coin_t
{
  [[nodiscard]] bitcoin::amount value() const { return {}; }
  [[nodiscard]] bitcoin::script_ref output_script() const { return {}; }
  [[nodiscard]] std::size_t funding_height() const { return {}; }
  [[nodiscard]] bool is_coinbase() const { return {}; }
};

struct coin_map_t
{
  using mapped_type = coin_t;
  std::optional<coin_t> lookup(bitcoin::outpoint const&) const;
};

static_assert(bitcoin::coin<coin_t>);
static_assert(bitcoin::coin_index<coin_map_t>);

} // namespace coin_with_observers

namespace coin_with_hidden_friends {

class coin_t
{
  friend bitcoin::amount value(coin_t const&) { return {}; }
  friend bitcoin::script_ref output_script(coin_t const&) { return {}; }
  friend std::size_t funding_height(coin_t const&) { return {}; }
  friend bool is_coinbase(coin_t const&) { return {}; }
};

struct coin_map_t
{
  using mapped_type = coin_t;
  std::optional<coin_t> lookup(bitcoin::outpoint const&) const;
};

static_assert(bitcoin::coin<coin_t>);
static_assert(bitcoin::coin_index<coin_map_t>);

} // namespace coin_with_hidden_friends

namespace coin_with_free_functions {

struct coin_t
{
};

bitcoin::amount value(coin_t const&)
{
  return {};
}

bitcoin::script_ref output_script(coin_t const&)
{
  return {};
}

std::size_t funding_height(coin_t const&)
{
  return {};
}

bool is_coinbase(coin_t const&)
{
  return {};
}

struct coin_map_t
{
  using mapped_type = coin_t;
  std::optional<coin_t> lookup(bitcoin::outpoint const&) const;
};

static_assert(bitcoin::coin<coin_t>);
static_assert(bitcoin::coin_index<coin_map_t>);

} // namespace coin_with_free_functions
} // namespace
