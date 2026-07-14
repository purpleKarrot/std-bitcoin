// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdint>
#include <format>

namespace bitcoin {

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

} // namespace bitcoin

namespace std {

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
