# Adding a Bitcoin Chain View Type to the C++ Standard Library

**Document number:** DXXXXR0\
**Date:** 2025-07-14\
**Audience:** LEWG, SG14 (Low-Latency / Financial)\
**Reply-to:** TBD

______________________________________________________________________

> **Note — For illustrative purposes only.**\
> This document is written in the style of a WG21 standardization paper. It has
> not been submitted to the ISO C++ committee and is not under active
> consideration for standardization.

______________________________________________________________________

## Table of Contents

1. [Abstract](#abstract)
2. [Motivation and Scope](#motivation-and-scope)
3. [Impact on the Standard](#impact-on-the-standard)
4. [Design Decisions](#design-decisions)
5. [Technical Specifications](#technical-specifications)
6. [References](#references)

______________________________________________________________________

## Abstract

This paper proposes adding `bitcoin::chain` to the C++ Standard Library under
the header `<bitcoin>`. A `chain` is a random-access view of the sequence of
`bitcoin::block_header` values on the path from the genesis block to a
particular tip. It models `std::ranges::random_access_range` and
`std::ranges::view`, and provides member operations `mismatch` and
`starts_with` for efficient comparison of competing chain tips. This paper
specifies the type itself; the node-level functions that produce `chain`
objects are outside scope.

______________________________________________________________________

## Motivation and Scope

Bitcoin implementations must internally maintain a tree of candidate chain tips
and reason about longest-chain selection, divergence points, and ancestor
relationships. At library boundaries, the result of queries such as "what is
the current best chain?" or "find the chain containing this block?" must be
expressible in a common type, independent of any particular implementation's
internal storage or indexing strategy.

Today, every C++ Bitcoin library returns its own opaque chain representation.
Cross-library tools — chain comparison utilities, testing harnesses, analysis
scripts — must write per-library adapters. A common `chain` type eliminates
this friction in the same way that a common `block_header` type eliminates the
need to adapt hash representations.

### In scope

- `bitcoin::chain` — a random-access view of the sequence of block headers
  from the genesis block to a particular tip.
- `bitcoin::chain::iterator` — the associated random-access iterator.

### Out of scope

- Node-level functions that produce `chain` objects (e.g.
  `get_best_chain()`, `find_chain(const block_hash&)`). These depend on
  consensus logic and are outside the scope of a vocabulary API.
- Block body data. The element type is `bitcoin::block_header`, not
  `bitcoin::block`.
- Chain storage, indexing strategy, fork detection, and reorg handling.

______________________________________________________________________

## Impact on the Standard

This paper adds `bitcoin::chain` and `bitcoin::chain::iterator` to
`namespace bitcoin` inside the `<bitcoin>` header. It depends on the wording
of *Adding Bitcoin Vocabulary Types to the C++ Standard Library* and
introduces no new dependencies beyond `<ranges>`, which is already required
by that paper.

______________________________________________________________________

## Design Decisions

### D1 — `chain` is a view, not a container

A `chain` does not own the block headers it exposes. Implementations
maintain block data in their own storage (on-disk databases, in-memory trees,
flat files). `chain` is a lightweight handle — a view over that storage —
intended to be cheap to copy and pass by value. This follows the same
rationale as the *value-range*<T> return types in the vocabulary types paper,
generalised to a first-class named type.

### D2 — `chain` derives from `std::ranges::view_interface<chain>`

Deriving from `std::ranges::view_interface<chain>` and providing `begin()` and
`end()` yields `operator[]`, `front()`, `back()`, `size()`, and `empty()` at
no cost, and ensures `chain` composes naturally with `<algorithm>`, `<ranges>`,
and user-written generic code. This is the standard mechanism for user-defined
views in C++20 and avoids reimplementing derived range operations by hand.

### D3 — The iterator is a proxy random-access iterator

No known Bitcoin implementation stores block headers as a contiguous array of
`block_header` objects. An iterator over a `chain` must read or reconstruct
headers from implementation-defined storage on demand. Consequently
`chain::iterator::operator*()` returns `bitcoin::block_header` by value rather
than by reference, making the iterator a proxy iterator. Unlike the C++17
iterator model, C++20 does not require `reference` to be `value_type&`; it
requires only that dereferencing the iterator yields a readable value. A
`chain::iterator` satisfies all remaining `std::random_access_iterator`
requirements in the normal way. The absence of a stable address for the returned
value means `operator->()` returns an unspecified proxy type rather than a raw
pointer.

### D4 — `mismatch` and `starts_with` are member functions

`std::ranges::mismatch` and `std::ranges::starts_with` are directly usable
with `chain` values, since `chain` models `std::ranges::random_access_range`.
They are additionally provided as member functions for two reasons:

1. Efficient implementations can exploit internal structure — ancestor
   pointers, skip lists, or accumulated-work metadata — to answer these
   queries in O(log n) or O(1) rather than the O(n) required by generic
   element-by-element comparison.
2. Making them members signals that implementations are expected to provide
   such an optimised override, rather than inheriting a linear fallback.

The semantics are identical to the corresponding range algorithms; callers who
prefer the generic form may always use `std::ranges::mismatch(a, b)`.

### D5 — `height()` as a named observer

The height of a chain — the zero-based index of its tip block — is a
fundamental domain concept referenced by BIP-34, BIP-65, BIP-112, and others.
Although `height() == size() - 1` for any non-empty chain, exposing a named
`height()` is more idiomatic and eliminates the off-by-one error that the
subtraction invites. `height()` carries a precondition that the chain is
non-empty, consistent with the narrow-contract pattern established across these
specifications.

### D6 — Construction is implementation-defined, except for the default constructor

`chain` objects are generally produced by querying a node's chain state.
Specifying constructors that accept block data would require specifying how
headers are associated with a chain and how the backing store is managed,
coupling the type to implementation internals. Such constructors are therefore
implementation-defined, following the same approach as `bitcoin::transaction`
and `bitcoin::block` in the vocabulary types paper.

The default constructor is an exception: it creates an empty `chain` with no
associated backing store. An empty chain is a useful sentinel — it can
represent the result of a failed lookup, an uninitialised chain member in a
composite object, or the base case of a recursive chain algorithm — without
requiring `std::optional<chain>` at every such site.

______________________________________________________________________

## Technical Specifications

All additions are relative to the C++ Working Draft and assume the wording of
*Adding Bitcoin Vocabulary Types to the C++ Standard Library* has been applied.

> *Editorial note — Add `class chain` and `class chain::iterator` to the
> `<bitcoin>` header synopsis in [bitcoin.syn].*

______________________________________________________________________

### [bitcoin.chain.version] Feature test macro

```cpp
#define __cpp_lib_bitcoin_chain 202XXXL    // also in <bitcoin>
```

______________________________________________________________________

### [bitcoin.chain] Class `chain`

#### [bitcoin.chain.general]

- A `chain` object represents a non-owning, ordered sequence of
  `bitcoin::block_header` values on the path from the genesis block to a
  particular block. Element `chain[0]` is the genesis block header; element
  `chain[n]` is the block header at height `n`.
- A `chain` is _empty_ if it contains no elements. An empty `chain` is a
  valid object; access to its elements is undefined behaviour.
- `chain` derives from `std::ranges::view_interface<chain>`. In addition to
  the members listed in [bitcoin.chain.syn], `view_interface` provides
  `operator[]`, `front()`, `back()`, `size()`, and `empty()`.
- `chain` models `std::ranges::random_access_range`, `std::ranges::view`,
  `std::ranges::sized_range`, `std::copyable`, and `std::movable`.

#### [bitcoin.chain.syn] Synopsis

```cpp
namespace bitcoin {

  class chain : public std::ranges::view_interface<chain> {
  public:
    class iterator;
    using const_iterator = iterator;

    chain() noexcept;

    [[nodiscard]] iterator  begin() const noexcept;
    [[nodiscard]] iterator  end()   const noexcept;

    [[nodiscard]] std::size_t height() const noexcept;

    [[nodiscard]] std::pair<iterator, iterator>
      mismatch(const chain& other)    const noexcept;

    [[nodiscard]] bool
      starts_with(const chain& prefix) const noexcept;
  };

} // namespace bitcoin
```

#### [bitcoin.chain.cons] Constructors

```cpp
chain() noexcept;
```

*Postconditions:* `empty()` is `true`.

______________________________________________________________________

#### [bitcoin.chain.obs] Observers

```cpp
[[nodiscard]] iterator begin() const noexcept;
```

*Returns:* An iterator to the element at height 0 (the genesis block header),
or `end()` if `*this` is empty.

```cpp
[[nodiscard]] iterator end() const noexcept;
```

*Returns:* The past-the-end iterator.

```cpp
[[nodiscard]] std::size_t height() const noexcept;
```

*Preconditions:* `!empty()`.

*Returns:* `size() - 1`. The height of the tip block.

#### [bitcoin.chain.ops] Operations

```cpp
[[nodiscard]] std::pair<iterator, iterator>
  mismatch(const chain& other) const noexcept;
```

*Returns:* Let `n` = `std::min(size(), other.size())`. Let `k` be the
smallest non-negative integer less than `n` such that
`(*this)[k] != other[k]`. Returns `{begin() + k, other.begin() + k}` if such
a `k` exists; otherwise `{begin() + n, other.begin() + n}`.

*Remarks:* Equivalent in result to
`std::ranges::mismatch(*this, other)`, but implementations should provide a
sub-linear algorithm where their internal representation permits.

```cpp
[[nodiscard]] bool starts_with(const chain& prefix) const noexcept;
```

*Returns:* `mismatch(prefix).second == prefix.end()`.

*Remarks:* Returns `true` if `prefix` is empty. Returns `true` if `*this`
and `prefix` agree on every block up to and including the tip of `prefix`.

______________________________________________________________________

### [bitcoin.chain.iterator] Class `chain::iterator`

#### [bitcoin.chain.iterator.general]

`chain::iterator` is the random-access iterator type for `bitcoin::chain`. It
models `std::random_access_iterator` with `value_type` equal to
`bitcoin::block_header`. Dereferencing an iterator returns a `block_header` by
value; no reference into the implementation's backing storage is exposed.

A default-constructed `chain::iterator` is singular: it is not associated
with any `chain` and may not be dereferenced, incremented, decremented, or
compared with any iterator other than another singular iterator.

Two iterators are _compatible_ if they were both obtained from the same
`chain` object (or copies thereof), or both are singular.

#### [bitcoin.chain.iterator.syn] Synopsis

```cpp
class bitcoin::chain::iterator {
public:
  using difference_type  = std::ptrdiff_t;
  using value_type       = bitcoin::block_header;
  using iterator_concept = std::random_access_iterator_tag;

  iterator() = default;

  [[nodiscard]] bitcoin::block_header  operator*()                    const;
  [[nodiscard]] /* unspecified */       operator->()                   const;
  [[nodiscard]] bitcoin::block_header  operator[](difference_type n)  const;

  iterator& operator++();      iterator operator++(int);
  iterator& operator--();      iterator operator--(int);
  iterator& operator+=(difference_type n);
  iterator& operator-=(difference_type n);

  friend iterator        operator+(iterator it,    difference_type n);
  friend iterator        operator+(difference_type n, iterator it);
  friend iterator        operator-(iterator it,    difference_type n);
  friend difference_type operator-(iterator lhs,   iterator rhs);

  friend bool                 operator==(iterator lhs, iterator rhs)  noexcept;
  friend std::strong_ordering operator<=>(iterator lhs, iterator rhs) noexcept;
};
```

#### [bitcoin.chain.iterator.ops] Operations

```cpp
[[nodiscard]] bitcoin::block_header operator*() const;
```

*Preconditions:* `*this` is dereferenceable.

*Returns:* The `bitcoin::block_header` at the position designated by `*this`.

```cpp
[[nodiscard]] /* unspecified */ operator->() const;
```

*Preconditions:* `*this` is dereferenceable.

*Returns:* An object `p` such that `p->m` is equivalent to `(**this).m` for
every member `m` of `bitcoin::block_header`. The return type is unspecified.

```cpp
[[nodiscard]] bitcoin::block_header operator[](difference_type n) const;
```

*Preconditions:* `*this + n` is dereferenceable.

*Returns:* `*(*this + n)`.

```cpp
iterator& operator++();
```

*Preconditions:* `*this` is incrementable.

*Effects:* Advances `*this` by one position.

*Returns:* `*this`.

```cpp
iterator operator++(int);
```

*Effects:* Equivalent to `{ auto tmp = *this; ++(*this); return tmp; }`.

```cpp
iterator& operator--();
```

*Preconditions:* `*this` is decrementable.

*Effects:* Retreats `*this` by one position.

*Returns:* `*this`.

```cpp
iterator operator--(int);
```

*Effects:* Equivalent to `{ auto tmp = *this; --(*this); return tmp; }`.

```cpp
iterator& operator+=(difference_type n);
```

*Effects:* Advances `*this` by `n` positions (retreats if `n` is negative).

*Returns:* `*this`.

```cpp
iterator& operator-=(difference_type n);
```

*Effects:* Equivalent to `return *this += -n;`.

*Returns:* `*this`.

```cpp
friend iterator operator+(iterator it,    difference_type n);
friend iterator operator+(difference_type n, iterator it);
```

*Returns:* A copy of `it` advanced by `n` positions.

```cpp
friend iterator operator-(iterator it, difference_type n);
```

*Returns:* A copy of `it` retreated by `n` positions.

```cpp
friend difference_type operator-(iterator lhs, iterator rhs);
```

*Preconditions:* `lhs` and `rhs` are compatible ([bitcoin.chain.iterator.general]).

*Returns:* The signed distance from `rhs` to `lhs`.

```cpp
friend bool operator==(iterator lhs, iterator rhs) noexcept;
```

*Preconditions:* `lhs` and `rhs` are compatible ([bitcoin.chain.iterator.general]).

*Returns:* `true` if `lhs` and `rhs` designate the same position.

```cpp
friend std::strong_ordering operator<=>(iterator lhs, iterator rhs) noexcept;
```

*Preconditions:* `lhs` and `rhs` are compatible ([bitcoin.chain.iterator.general]).

*Returns:* The three-way comparison of the positions designated by `lhs` and
`rhs`, where a position at lower height compares less.

______________________________________________________________________

## References

- **BIP-34** — Block v2; Height in Coinbase\
  <https://github.com/bitcoin/bips/blob/master/bip-0034.mediawiki>

- **BIP-65** — OP_CHECKLOCKTIMEVERIFY\
  <https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki>

- **BIP-112** — CHECKSEQUENCEVERIFY\
  <https://github.com/bitcoin/bips/blob/master/bip-0112.mediawiki>

- **Bitcoin Developer Reference** — Block chain\
  <https://developer.bitcoin.org/reference/block_chain.html>

- **Bitcoin Core source** — `src/chain.h`\
  <https://github.com/bitcoin/bitcoin>
