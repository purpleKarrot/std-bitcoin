// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>

#include <bitcoin/amount.hpp>
#include <bitcoin/block.hpp>
#include <bitcoin/script.hpp>

namespace bitcoin {

class coin
{
public:
  coin() noexcept = default;
  coin(bitcoin::amount value, bitcoin::script_ref output_script,
       std::size_t funding_height, bool coinbase = false)
    : _value{value}
    , _output_script{output_script}
    , _funding_height{funding_height}
    , _coinbase{coinbase}
  {
  }

  [[nodiscard]] auto value() const noexcept -> bitcoin::amount
  {
    return _value;
  }

  [[nodiscard]] auto output_script() const -> bitcoin::script_ref
  {
    return _output_script;
  }

  [[nodiscard]] auto funding_height() const noexcept -> std::size_t
  {
    return _funding_height;
  }

  friend auto is_coinbase(coin const& c) noexcept -> bool
  {
    return c._coinbase;
  }

  friend bool operator==(coin const& lhs, coin const& rhs) noexcept = default;

private:
  bitcoin::script _output_script;
  bitcoin::amount _value;
  std::size_t _funding_height = 0;
  bool _coinbase = false;
};

} // namespace bitcoin
