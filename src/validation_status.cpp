// SPDX-License-Identifier: BSL-1.0

module;

#include <algorithm>
#include <format>
#include <string_view>

module bitcoin;

auto std::formatter<bitcoin::validation_status>::format(
  bitcoin::validation_status status, std::format_context& ctx) const
  -> std::format_context::iterator
{
  using namespace std::string_view_literals;

  auto out = ctx.out();
  auto str = status ? "OK"sv : "NOT OK"sv;
  return std::ranges::copy(str, out).out;
}
