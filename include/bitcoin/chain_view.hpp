// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <ranges>

#include <bitcoin/block_header.hpp>

namespace bitcoin {

template <typename T>
concept chain_view = std::ranges::view<T>
  && std::ranges::sized_range<T const>
  && std::ranges::random_access_range<T const>
  && std::convertible_to<std::ranges::range_reference_t<T const>, block_header>;

} // namespace bitcoin
