---
title: "Bitcoin Chains: `chain_view` and `any_chain_view`"
date: today
document: DXXXXR0
audience:
  - Library Evolution Working Group
  - SG14 (Low-Latency / Financial)
toc: false
references:
  - id: BIP34
    citation-label: BIP-34
    title: "BIP-34: Block v2, Height in Coinbase"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0034.mediawiki
  - id: BIP65
    citation-label: BIP-65
    title: "BIP-65: OP_CHECKLOCKTIMEVERIFY"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki
  - id: BIP112
    citation-label: BIP-112
    title: "BIP-112: CHECKSEQUENCEVERIFY"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0112.mediawiki
  - id: BitcoinCore
    citation-label: Bitcoin Core
    title: "Bitcoin Core source repository"
    URL: https://github.com/bitcoin/bitcoin
---

[For illustrative purposes only. This document is written in the style of a WG21
standardization paper. It has not been submitted to the ISO C++ committee and is
not under active consideration for standardization.]{.draftnote}

# Abstract

This paper proposes adding the concept `bitcoin::chain_view` and the type-erased
wrapper `bitcoin::any_chain_view` to the C++ Standard Library under the header
`<bitcoin>`. A type models `chain_view` if it is a random-access, sized view of
the sequence of `bitcoin::block_header` values on the path from the genesis
block to a particular tip. `any_chain_view` models
`std::ranges::random_access_range`, `std::ranges::sized_range`, and
`std::ranges::view`, and provides member operations `mismatch` and `starts_with`
for comparing chain prefixes and identifying divergence points between competing
chain tips. This paper specifies the concept and wrapper; the node-level
functions that produce chain objects are outside scope.

# Motivation and Scope

Bitcoin implementations must internally maintain a tree of candidate chain tips
and reason about longest-chain selection, divergence points, and ancestor
relationships [@BitcoinCore]. At library boundaries, the result of queries such
as "what is the current best chain?" or "find the chain containing this block?"
must be expressible in a common type, independent of any particular
implementation's internal storage or indexing strategy.

Today, C++ Bitcoin libraries generally return implementation-specific chain
representations. Cross-library tools — chain comparison utilities, testing
harnesses, analysis scripts — therefore require per-library adapters. A common
`chain_view` concept and an associated `any_chain_view` wrapper would improve
interoperability in the same way that a common `block_header` type reduces the
need for representation-specific adaptation.

## In scope

- `bitcoin::chain_view` — a concept satisfied by random-access, sized views of
  the block-header sequence from the genesis block to a particular tip.
- `bitcoin::any_chain_view` — a type-erased wrapper for values whose type models
  `bitcoin::chain_view`.
- `bitcoin::any_chain_view::iterator` — the associated random-access iterator.

## Out of scope

- Node-level functions that produce chain objects (e.g. `get_best_chain()`,
  `find_chain(const block_hash&)`). These depend on consensus logic and are
  outside the scope of a vocabulary API.
- Block body data. The element type is `bitcoin::block_header`, not
  `bitcoin::block`.
- Chain storage, indexing strategy, fork detection, and reorg handling.

# Impact on the Standard

This paper adds the concept `bitcoin::chain_view`, the class
`bitcoin::any_chain_view`, and the class `bitcoin::any_chain_view::iterator` to
`namespace bitcoin` inside the `<bitcoin>` header. It depends on the wording of
*Adding Bitcoin Vocabulary Types to the C++ Standard Library* and introduces no
new dependencies beyond `<algorithm>`, `<concepts>`, `<ranges>`,
`<type_traits>`, and `<utility>`, which are already required by that paper or by
the synopsis below.

# Design considerations

## D1 — View semantics

A type that models `chain_view` is a view of block headers on the path from the
genesis block to a particular tip. `any_chain_view` is a lightweight,
type-erased wrapper around such a value, intended to be inexpensive to copy and
pass by value. Neither the concept nor the wrapper mandates a particular storage
or ownership strategy for the underlying view object; implementations remain
free to use references, handles, small-buffer optimization, shared state, or
other representations appropriate to their chain-storage architecture.

## D2 — Use of `std::ranges::view_interface<any_chain_view>`

Deriving from `std::ranges::view_interface<any_chain_view>` and providing
`begin()` and `end()` integrates `any_chain_view` with `<algorithm>`,
`<ranges>`, and user-written generic code. Combined with explicit `operator[]`
and `size()` members, this also provides the usual view operations such as
`front()`, `back()`, and `empty()` through `view_interface`.

## D3 — Proxy iterator model

No known Bitcoin implementation stores block headers as a contiguous array of
`block_header` objects. An iterator over an `any_chain_view` must read or
reconstruct headers from implementation-defined storage on demand. Consequently
`any_chain_view::iterator::operator*()` returns `bitcoin::block_header` by value
rather than by reference, making the iterator a proxy iterator. Unlike the C++17
iterator model, C++20 does not require `reference` to be `value_type&`; it
requires only that dereferencing the iterator yields a readable value. An
`any_chain_view::iterator` otherwise satisfies the remaining
`std::random_access_iterator` requirements. The absence of a stable address for
the returned value means `operator->()` returns an unspecified proxy type rather
than a raw pointer.

## D4 — Member comparison operations

`std::ranges::mismatch` and `std::ranges::starts_with` are directly usable with
`any_chain_view` values and with arbitrary types whose type models
`bitcoin::chain_view`. They are additionally provided as member functions of
`any_chain_view` for two reasons:

1. Efficient implementations can exploit internal structure — ancestor pointers,
   skip lists, or accumulated-work metadata — to answer these queries in O(log
   n) or O(1) rather than the O(n) required by generic element-by-element
   comparison.
2. Providing corresponding member operations permits implementations to supply
   specialized implementations while preserving the semantics of the generic
   algorithms.

The semantics are identical to the corresponding range algorithms; callers who
prefer the generic form may always use `std::ranges::mismatch(a, b)`. An
implementation may use same-type member `starts_with` and `mismatch` operations
on the wrapped view type as optimization hooks. When such hooks are present,
`starts_with` must return exactly `bool`, and `mismatch` must return exactly
`std::ranges::mismatch_result<std::ranges::iterator_t<const T>, std::ranges::iterator_t<const T>>`,
where `T` is the wrapped view type.

## D5 — Named observer

The height of a chain — the zero-based index of its tip block — is a fundamental
domain concept referenced by [@BIP34], [@BIP65], [@BIP112], and others. Although
`height() == size() - 1` for any non-empty chain, exposing a named `height()`
avoids repeated `size() - 1` expressions and makes the non-empty precondition
explicit. `height()` follows the narrow-contract pattern used throughout these
specifications.

## D6 — Construction

`any_chain_view` objects are generally produced by querying a node's chain
state, but they can also be formed from any view whose type models
`bitcoin::chain_view` and whose optional optimization hooks, if present, have
the required signatures. This permits library interfaces to traffic in a stable
vocabulary type without specifying the backing storage or indexing strategy of
the wrapped view. The default constructor is an exception: it creates an empty
`any_chain_view` with no associated backing view. This provides a useful empty
state for failed lookups, default member initialization, and recursive chain
algorithms without requiring `std::optional<any_chain_view>` in each such use.

# Proposed wording

The wording in this section is relative to the C++ Working Draft and assumes
that the wording of *Adding Bitcoin Vocabulary Types to the C++ Standard
Library* has been applied.

[Add `template<class T> concept chain_view`, `class any_chain_view`, and
`class any_chain_view::iterator` to the `<bitcoin>` header synopsis in
[bitcoin.syn].]{.ednote}

## [bitcoin.chain.version] Feature test macro

```cpp
#define __cpp_lib_bitcoin_chain 202XXXL    // also in <bitcoin>
```

## [bitcoin.chain] Concept `chain_view` and class `any_chain_view`

### [bitcoin.chain.general] Concept `chain_view`

A type `T` models `chain_view` if and only if:

- `T` models `std::ranges::view`;
- `const T` models `std::ranges::random_access_range` and
  `std::ranges::sized_range`; and
- `std::ranges::range_reference_t<const T>` is convertible to
  `bitcoin::block_header`.

### [bitcoin.chain.syn] Synopsis

```cpp
namespace bitcoin {

  template<class T>
  concept chain_view =
    std::ranges::view<T> &&
    std::ranges::random_access_range<const T> &&
    std::ranges::sized_range<const T> &&
    std::convertible_to<std::ranges::range_reference_t<const T>,
                        bitcoin::block_header>;

  class any_chain_view : public std::ranges::view_interface<any_chain_view> {
  public:
    class iterator;
    using const_iterator = iterator;

    any_chain_view() noexcept;

    template<class T>
      requires chain_view<std::remove_cvref_t<T>> &&
               (!std::same_as<std::remove_cvref_t<T>, any_chain_view>)
    explicit any_chain_view(T&& object);

    [[nodiscard]] iterator begin() const;
    [[nodiscard]] iterator end() const;

    [[nodiscard]] bitcoin::block_header operator[](std::size_t index) const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::size_t height() const;

    [[nodiscard]] std::ranges::mismatch_result<iterator, iterator>
      mismatch(const any_chain_view& other) const;

    [[nodiscard]] bool starts_with(const any_chain_view& prefix) const;
  };

} // namespace bitcoin
```

### [bitcoin.any_chain_view.general] Class `any_chain_view`

- An `any_chain_view` object represents an ordered sequence of
  `bitcoin::block_header` values on the path from the genesis block to a
  particular block. Element `(*this)[0]` is the genesis block header; element
  `(*this)[n]` is the block header at height `n`.
- An `any_chain_view` is _empty_ if it contains no elements. An empty
  `any_chain_view` is a valid object; access to its elements is undefined
  behavior.
- `any_chain_view` derives from `std::ranges::view_interface<any_chain_view>`.
  In addition to the members listed in [bitcoin.chain.syn], `view_interface`
  provides `front()`, `back()`, and `empty()`.
- `any_chain_view` models `std::ranges::random_access_range`,
  `std::ranges::view`, `std::ranges::sized_range`, `std::copyable`, and
  `std::movable`.

### [bitcoin.any_chain_view.cons] Constructors

```cpp
any_chain_view() noexcept;
```

*Postconditions:* `empty()` is `true`.

```cpp
template<class T>
  requires chain_view<std::remove_cvref_t<T>> &&
           (!std::same_as<std::remove_cvref_t<T>, any_chain_view>)
any_chain_view(T&& object);
```

*Mandates:* Let `U` be `std::remove_cvref_t<T>`. If the expression
`std::declval<const U&>().starts_with(std::declval<const U&>())` is well-formed,
its type is exactly `bool`. If the expression
`std::declval<const U&>().mismatch(std::declval<const U&>())` is well-formed,
its type is exactly
`std::ranges::mismatch_result<std::ranges::iterator_t<const U>, std::ranges::iterator_t<const U>>`.

*Effects:* Constructs an `any_chain_view` that wraps `std::forward<T>(object)`.

*Postconditions:* If `std::ranges::empty(object)` is `true`, `empty()` is
`true`.

*Remarks:* If `U` provides `starts_with` and `mismatch` with the required
signatures, an implementation may use them to optimize the corresponding
operations on `any_chain_view`.

### [bitcoin.any_chain_view.obs] Observers

```cpp
[[nodiscard]] iterator begin() const;
```

*Returns:* An iterator to the element at height 0 (the genesis block header), or
`end()` if `*this` is empty.

```cpp
[[nodiscard]] iterator end() const;
```

*Returns:* The past-the-end iterator.

```cpp
[[nodiscard]] bitcoin::block_header operator[](std::size_t index) const;
```

*Preconditions:* `index < size()`.

*Returns:* The `bitcoin::block_header` at height `index`.

```cpp
[[nodiscard]] std::size_t size() const;
```

*Returns:* The number of block headers in `*this`.

```cpp
[[nodiscard]] std::size_t height() const;
```

*Preconditions:* `!empty()`.

*Returns:* `size() - 1`. The height of the tip block.

### [bitcoin.any_chain_view.ops] Operations

```cpp
[[nodiscard]] std::ranges::mismatch_result<iterator, iterator>
  mismatch(const any_chain_view& other) const;
```

*Returns:* Let `n` = `std::min(size(), other.size())`. Let `k` be the smallest
non-negative integer less than `n` such that `(*this)[k] != other[k]`. Returns
`{begin() + k, other.begin() + k}` if such a `k` exists; otherwise
`{begin() + n, other.begin() + n}`.

*Remarks:* Equivalent in result to `std::ranges::mismatch(*this, other)`, but
implementations should provide a sub-linear algorithm where their internal
representation permits.

```cpp
[[nodiscard]] bool starts_with(const any_chain_view& prefix) const;
```

*Returns:* `mismatch(prefix).in2 == prefix.end()`.

*Remarks:* Returns `true` if `prefix` is empty. Returns `true` if `*this` and
`prefix` agree on every block up to and including the tip of `prefix`.

## [bitcoin.any_chain_view.iterator] Class `any_chain_view::iterator`

### [bitcoin.any_chain_view.iterator.general]

`any_chain_view::iterator` is the random-access iterator type for
`bitcoin::any_chain_view`. It models `std::random_access_iterator` with
`value_type` equal to `bitcoin::block_header`. Dereferencing an iterator returns
a `block_header` by value; no reference into the implementation's backing
storage is exposed.

A default-constructed `any_chain_view::iterator` is singular: it is not
associated with any `any_chain_view` and may not be dereferenced, incremented,
decremented, or compared with any iterator other than another singular iterator.

Two iterators are _compatible_ if they were both obtained from the same
`any_chain_view` object, or both are singular.

### [bitcoin.any_chain_view.iterator.syn] Synopsis

```cpp
class bitcoin::any_chain_view::iterator {
public:
  using difference_type = std::ptrdiff_t;
  using value_type = bitcoin::block_header;
  using iterator_concept = std::random_access_iterator_tag;

  iterator() = default;

  [[nodiscard]] bitcoin::block_header operator*() const;
  [[nodiscard]] /* $unspecified$ */ operator->() const;
  [[nodiscard]] bitcoin::block_header operator[](difference_type n) const;

  iterator& operator++();
  iterator operator++(int);
  iterator& operator--();
  iterator operator--(int);
  iterator& operator+=(difference_type n);
  iterator& operator-=(difference_type n);

  friend iterator operator+(iterator it, difference_type n);
  friend iterator operator+(difference_type n, iterator it);
  friend iterator operator-(iterator it, difference_type n);
  friend difference_type operator-(iterator lhs, iterator rhs);

  friend bool operator==(iterator lhs, iterator rhs) noexcept;
  friend std::strong_ordering operator<=>(iterator lhs, iterator rhs)
    noexcept;
};
```

### [bitcoin.any_chain_view.iterator.ops] Operations

```cpp
[[nodiscard]] bitcoin::block_header operator*() const;
```

*Preconditions:* `*this` is dereferenceable.

*Returns:* The `bitcoin::block_header` at the position designated by `*this`.

```cpp
[[nodiscard]] /* $unspecified$ */ operator->() const;
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
friend iterator operator+(iterator it, difference_type n);
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

*Preconditions:* `lhs` and `rhs` are compatible
([bitcoin.any_chain_view.iterator.general]).

*Returns:* The signed distance from `rhs` to `lhs`.

```cpp
friend bool operator==(iterator lhs, iterator rhs) noexcept;
```

*Preconditions:* `lhs` and `rhs` are compatible
([bitcoin.any_chain_view.iterator.general]).

*Returns:* `true` if `lhs` and `rhs` designate the same position.

```cpp
friend std::strong_ordering operator<=>(iterator lhs, iterator rhs) noexcept;
```

*Preconditions:* `lhs` and `rhs` are compatible
([bitcoin.any_chain_view.iterator.general]).

*Returns:* The three-way comparison of the positions designated by `lhs` and
`rhs`, where a position at lower height compares less.
