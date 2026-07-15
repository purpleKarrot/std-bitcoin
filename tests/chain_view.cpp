// SPDX-License-Identifier: BSL-1.0

import bitcoin;

#include <span>
#include <vector>

#include <doctest/doctest.h>

namespace {

class pointer_chain_view : public std::ranges::view_base
{
public:
  pointer_chain_view() = default;
  pointer_chain_view(bitcoin::block_header const* data, std::size_t count)
    : _data(data)
    , _count(count)
  {
  }

  [[nodiscard]] auto begin() const { return _data; }
  [[nodiscard]] auto end() const { return _data + _count; }
  [[nodiscard]] auto size() const { return _count; }

private:
  bitcoin::block_header const* _data = nullptr;
  std::size_t _count = 0;
};

} // namespace

static_assert(bitcoin::chain_view<std::span<bitcoin::block_header const>>);
static_assert(bitcoin::chain_view<pointer_chain_view>);
static_assert(!bitcoin::chain_view<std::vector<bitcoin::block_header>>);
