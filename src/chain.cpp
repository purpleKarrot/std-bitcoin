// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/chain.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

#include <bitcoin/block_header.hpp>
#include <bitcoin/detail/chain_erasure.hpp>
#include <bitcoin/detail/iterator.hpp>

namespace bitcoin {

auto any_chain_view::size() const -> std::size_t
{
  return _impl.size();
}

auto any_chain_view::height() const -> std::size_t
{
  assert(!empty());
  return size() - 1;
}

auto any_chain_view::mismatch(any_chain_view const& other) const
  -> std::ranges::mismatch_result<iterator, iterator>
{
  if (_impl.same_type(other._impl) && _impl.has_mismatch()) {
    auto const index = _impl.mismatch(other._impl);
    assert(index <= std::min(size(), other.size()));
    return {iterator(*this, index), iterator(other, index)};
  }

  return std::ranges::mismatch(*this, other);
}

auto any_chain_view::starts_with(any_chain_view const& prefix) const -> bool
{
  auto const prefix_size = prefix.size();
  if (prefix_size == 0) {
    return true;
  }

  if (size() < prefix_size) {
    return false;
  }

  if (_impl.same_type(prefix._impl) && _impl.has_starts_with()) {
    return _impl.starts_with(prefix._impl);
  }

  return mismatch(prefix).in2 == prefix.end();
}

} // namespace bitcoin
