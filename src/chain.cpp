// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/chain.hpp>

namespace bitcoin {

any_chain_view::any_chain_view(any_chain_view const& other)
  : _vtable(other._vtable)
{
  _vtable->copy(other.storage(), storage());
}

any_chain_view::any_chain_view(any_chain_view&& other) noexcept
  : _vtable(std::exchange(other._vtable, std::addressof(empty_vtable)))
{
  _vtable->move(other.storage(), storage());
}

auto any_chain_view::operator=(any_chain_view const& other) -> any_chain_view&
{
  if (this != std::addressof(other)) {
    _vtable->destroy(storage());
    _vtable = other._vtable;
    _vtable->copy(other.storage(), storage());
  }
  return *this;
}

auto any_chain_view::operator=(any_chain_view&& other) noexcept
  -> any_chain_view&
{
  if (this != std::addressof(other)) {
    _vtable->destroy(storage());
    _vtable = std::exchange(other._vtable, std::addressof(empty_vtable));
    _vtable->move(other.storage(), storage());
  }
  return *this;
}

any_chain_view::~any_chain_view()
{
  _vtable->destroy(storage());
}

auto any_chain_view::operator[](std::size_t index) const -> block_header
{
  return table().get(storage(), index);
}

auto any_chain_view::begin() const noexcept -> iterator
{
  return iterator(*this, 0);
}

auto any_chain_view::end() const noexcept(noexcept(size())) -> iterator
{
  return iterator(*this, size());
}

auto any_chain_view::size() const -> std::size_t
{
  return table().size(storage());
}

auto any_chain_view::height() const -> std::size_t
{
  assert(!empty());
  return size() - 1;
}

auto any_chain_view::mismatch(any_chain_view const& other) const
  -> std::ranges::mismatch_result<iterator, iterator>
{
  if (same_type(other) && table().mismatch != nullptr) {
    auto const index = table().mismatch(storage(), other.storage());
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

  if (same_type(prefix) && table().starts_with != nullptr) {
    return table().starts_with(storage(), prefix.storage());
  }

  return mismatch(prefix).in2 == prefix.end();
}

auto any_chain_view::storage() noexcept -> void*
{
  return _storage.data();
}

auto any_chain_view::storage() const noexcept -> void const*
{
  return _storage.data();
}

auto any_chain_view::table() const noexcept -> vtable const&
{
  return *_vtable;
}

bool any_chain_view::same_type(any_chain_view const& other) const noexcept
{
  return _vtable == other._vtable;
}

} // namespace bitcoin
