// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <utility>

#include <bitcoin/validation_status.hpp>

namespace bitcoin {

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

} // namespace bitcoin
