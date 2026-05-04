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

The internal representation of `chain::iterator` — whether it stores an index,
a pointer, a reference-counted handle, or anything else — is deliberately
unspecified. The specification constrains only observable behaviour: what
dereferencing returns, how arithmetic moves the position, and when iterators
remain valid.

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

### D6 — Construction from a `ChainAccess`

`chain` is constructed by forwarding any object that satisfies the `ChainAccess`
concept (see D7, [bitcoin.chain.access]):

```cpp
template<ChainAccess A>
explicit chain(A&& access);
```

The constructor accepts a forwarding reference and arranges internal storage for
the backing object using perfect forwarding. How that storage is managed —
`shared_ptr`, `unique_ptr` with copy-on-write, a small-buffer optimisation, or
anything else — is an implementation detail that is not exposed in the public
interface. Passing a `shared_ptr` directly is intentionally not part of the API.

The natural implementation wraps the access object with
`std::make_shared<std::decay_t<A>>(std::forward<A>(access))`, because `chain`
must be O(1) copyable to model `std::ranges::view`, a requirement that
`unique_ptr` alone cannot satisfy. This paper notes `shared_ptr` as the most
likely implementation choice but does not mandate it.

The default constructor creates an empty `chain` with no associated backing
store. An empty chain is a useful sentinel — it can represent the result of a
failed lookup, an uninitialised chain member in a composite object, or the base
case of a recursive chain algorithm — without requiring `std::optional<chain>`
at every such site.

### D7 — Backing store abstraction (`ChainAccess`)

`ChainAccess` is the concept (or, under Option B, the polymorphic base class
`chain_access`) that `chain` requires of its backing object. It is an internal
extension point: implementations supply a concrete `ChainAccess` type that
provides `size()` and `at()`, and `chain` calls those members when it needs to
expose headers to callers.

`chain_access` / `ChainAccess` is not a vocabulary type in its own right. It
is not intended to appear at library boundaries independently of `chain`. Users
never store or pass `chain_access` values directly; they construct a `chain`
from their backing object and then work with `chain`. For this reason the
concept is specified in this paper rather than as a separate proposal.

### D8 — Type-erased backing store identity

`mismatch` and `starts_with` can exploit sub-linear internal algorithms — skip
lists, accumulated-work indices, ancestor pointers — but only when both `chain`
objects are backed by the same concrete type and that type provides an
optimised implementation. To enable this without relying on RTTI, each `chain`
object carries an opaque *type token* derived from its `ChainAccess` type at
construction time. The type token is the address of a per-type static variable
(the same technique used by `std::any`), obtained at the point of construction,
and stored alongside the backing object.

When `mismatch(other)` or `starts_with(other)` is called:

1. If `*this` and `other` carry the same type token, `chain` may dispatch to an
   optimised `mismatch_impl` provided by the shared concrete `ChainAccess` type,
   yielding a potentially sub-linear result.
2. If the type tokens differ — because the two chains were constructed from
   different `ChainAccess` types — `chain` falls back to a generic O(n)
   element-by-element comparison via `at`.

Both paths produce a correct result. Mixing two chains from different
implementations is well-defined; it simply cannot exploit internal structure.
This is fully transparent to callers.

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

### [bitcoin.chain.access] Concept `ChainAccess` and class `chain_access`

#### [bitcoin.chain.access.general]

`ChainAccess` is the concept that characterises types suitable as backing
stores for `bitcoin::chain`. A type that models `ChainAccess` provides indexed
access to block headers and reports how many headers it holds. Optionally, it
provides an optimised mismatch implementation that `chain` may dispatch to when
both operands share the same concrete `ChainAccess` type (see D8).

Two design options are available. Option A defines `ChainAccess` as a C++20
concept, enabling statically dispatched, zero-overhead type erasure at the
point of construction. Option B defines `chain_access` as an abstract base
class, enabling dynamically dispatched heterogeneous implementations. This
paper recommends Option A; the wording below presents both and notes where they
differ.

#### [bitcoin.chain.access.concept] Option A — Concept

```cpp
namespace bitcoin {

  template<typename A>
  concept ChainAccess =
    requires(const A& a, std::size_t n) {
      { a.size() } noexcept -> std::same_as<std::size_t>;
      { a.at(n)  }          -> std::same_as<bitcoin::block_header>;
    };

} // namespace bitcoin
```

A type `A` that additionally provides

```cpp
std::size_t mismatch_impl(const A& other, std::size_t limit) const noexcept;
```

may receive fast-path dispatch from `chain::mismatch` and `chain::starts_with`
when both chains were constructed from the same concrete type (see
[bitcoin.chain.ops]).

#### [bitcoin.chain.access.base] Option B — Abstract base class

```cpp
namespace bitcoin {

  class chain_access {
  public:
    virtual ~chain_access()                                         = default;
    virtual std::size_t          size() const noexcept             = 0;
    virtual bitcoin::block_header at(std::size_t height) const     = 0;
  };

} // namespace bitcoin
```

Any class derived from `chain_access` and providing concrete implementations of
`size()` and `at()` models `ChainAccess` under Option A. A derived class may
additionally override a virtual `mismatch_impl`:

```cpp
virtual std::size_t mismatch_impl(const chain_access& other,
                                  std::size_t         limit) const noexcept;
```

The default implementation performs element-by-element comparison via `at`.

> *Note:* `chain_access` is not a vocabulary type intended to appear at library
> boundaries independently of `chain`. It is an implementation detail of
> `chain`'s construction mechanism. — *end note*

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

    template</* ChainAccess */ A>
    explicit chain(A&& access);   // (Option A: requires ChainAccess<std::decay_t<A>>)
                                  // (Option B: requires std::is_base_of_v<chain_access, std::decay_t<A>>)

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

```cpp
template</* ChainAccess */ A>
explicit chain(A&& access);
```

*Constraints:* Under Option A, `ChainAccess<std::decay_t<A>>` is satisfied.
Under Option B, `std::decay_t<A>` is derived from `chain_access`.

*Effects:* Initialises an internal backing store from `std::forward<A>(access)`
using an unspecified ownership mechanism. How the storage is allocated and
managed is an implementation detail; no `shared_ptr` need appear in the
interface.

*Postconditions:* `!empty()`. The `chain` object carries a type token uniquely
identifying `std::decay_t<A>`, which is used by `mismatch` and `starts_with`
to determine whether a fast-path dispatch is possible (see [bitcoin.chain.ops]).

*Remarks:* The most likely implementation stores the backing object in a
`std::shared_ptr<std::decay_t<A>>` created via
`std::make_shared<std::decay_t<A>>(std::forward<A>(access))`, because `chain`
must be O(1) copyable to model `std::ranges::view`. This is a quality-of-
implementation recommendation, not a normative requirement.

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

If `*this` and `other` carry the same type token (i.e. were both constructed
from the same concrete `ChainAccess` type — see D8), the implementation may
dispatch to an optimised `mismatch_impl` provided by that type. Otherwise the
implementation falls back to generic O(n) element-by-element comparison via
`at`. Both paths produce correct results; callers need not be aware of which
path is taken.

```cpp
[[nodiscard]] bool starts_with(const chain& prefix) const noexcept;
```

*Returns:* `mismatch(prefix).second == prefix.end()`.

*Remarks:* Returns `true` if `prefix` is empty. Returns `true` if `*this`
and `prefix` agree on every block up to and including the tip of `prefix`.

If `*this` and `prefix` carry the same type token, the implementation may
dispatch to an optimised fast path via the shared `ChainAccess` type.
Otherwise the implementation falls back to O(n) element-by-element comparison.
Both paths produce correct results.

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

An iterator remains valid as long as the `chain` from which it was obtained
remains valid. The internal representation of a `chain::iterator` — whether it
stores an index, a pointer, a reference-counted handle, or anything else — is
unspecified. The specification constrains only observable behaviour.

> *Note:* Implementations should not be understood as required to store a
> `shared_ptr` or any other specific ownership mechanism inside an iterator.
> — *end note*

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
