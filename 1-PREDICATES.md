# Adding Bitcoin Protocol Predicates to the C++ Standard Library

> **Status:** Early draft — companion to *Adding Bitcoin Vocabulary Types to the
> C++ Standard Library* (0-PRIMITIVES.md).

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

This paper is a companion to *Adding Bitcoin Vocabulary Types to the C++
Standard Library*. That paper deliberately restricts itself to faithfully
carrying protocol data across library boundaries and omits interpretation of
protocol fields. This paper proposes the natural complement: a set of free
functions in `namespace bitcoin` that answer common protocol-level questions
about those vocabulary types. Because every predicate here is expressible
entirely in terms of the public interface of the vocabulary types, none requires
private access and all can be specified independently.

______________________________________________________________________

## Motivation and Scope

The vocabulary types paper exposes raw protocol fields (`sequence`, `locktime`,
`version`, …) without interpreting them. Many clients need to answer questions
such as:

- Is this transaction a coinbase?
- Does this transaction signal SegWit?
- Does this input opt in to replace-by-fee?
- Is this output provably unspendable?
- Is the transaction locktime a block height or a timestamp?

These predicates are trivially derivable from the public API, but spelling them
out correctly requires knowing several BIP-defined magic numbers and bit-field
layouts. Standardising them eliminates per-library reimplementations and the
subtle bugs that come with them.

### In scope

- Free-function predicates on `outpoint`, `tx_input`, `tx_output`,
  `transaction`, and `block`.
- Definitions expressed entirely in terms of the public interface of the
  vocabulary types.

### Out of scope

- Consensus validation.
- Strong types for raw integer fields (`sequence`, `locktime`, `version`, …) —
  see the vocabulary types paper.
- Script execution or script pattern matching beyond a single leading-byte check
  for provably unspendable outputs.

______________________________________________________________________

## Impact on the Standard

This paper adds free functions to `namespace bitcoin` inside the `<bitcoin>`
header. It depends on the vocabulary types paper and introduces no new types.

______________________________________________________________________

## Design Decisions

### D1 — Free functions, not member functions

Every predicate in this paper is expressible in terms of the public interface of
the corresponding vocabulary type. Making them free functions rather than members
has three advantages:

1. **ADL discovery.** Callers write `is_coinbase(op)` without a namespace
   qualifier; the compiler finds the function via argument-dependent lookup.
2. **Overloading across types.** A single name `is_coinbase` serves `txid`,
   `outpoint`, and `transaction`; callers do not need to remember which type
   carries which spelling.
3. **Independent evolution.** Predicates can be added in follow-on papers
   without touching the vocabulary type definitions or breaking ABI.

### D2 — Definitions are normative

Unlike the vocabulary types (whose internal representation is
implementation-defined), these predicates have fully normative *Returns:*
clauses expressed in terms of the public API. Implementations may not deviate.

### D3 — `sequence`-based predicates depend on transaction version

BIP-68 relative locktime is only active when the enclosing transaction's version
is ≥ 2. `has_relative_locktime` as specified here reflects only the
`tx_input`-level field condition (bit 31 clear, not final); enforcement of the
transaction-version gate is consensus logic and out of scope.

______________________________________________________________________

## Technical Specifications

All changes are relative to the C++ Working Draft and assume the wording of
*Adding Bitcoin Vocabulary Types to the C++ Standard Library* has been applied.

### [bitcoin.pred] Bitcoin protocol predicates

#### [bitcoin.pred.syn] Synopsis

All functions in [bitcoin.pred] are declared in `namespace bitcoin` inside the
`<bitcoin>` header.

```cpp
namespace bitcoin {

  // [bitcoin.pred.txid]
  [[nodiscard]] bool is_coinbase(const txid& t)               noexcept;

  // [bitcoin.pred.outpoint]
  [[nodiscard]] bool is_coinbase(const outpoint& o)           noexcept;

  // [bitcoin.pred.tx_input]
  [[nodiscard]] bool signals_rbf(const tx_input& i)           noexcept;
  [[nodiscard]] bool has_relative_locktime(const tx_input& i) noexcept;

  // [bitcoin.pred.tx_output]
  [[nodiscard]] bool is_unspendable(const tx_output& o)       noexcept;

  // [bitcoin.pred.transaction]
  [[nodiscard]] bool is_coinbase(const transaction& t)        noexcept;
  [[nodiscard]] bool is_segwit(const transaction& t)          noexcept;
  [[nodiscard]] bool locktime_is_height(const transaction& t) noexcept;
  [[nodiscard]] bool locktime_is_time(const transaction& t)   noexcept;

  // [bitcoin.pred.block]
  [[nodiscard]] bool has_coinbase(const block& b) noexcept;

} // namespace bitcoin
```

______________________________________________________________________

#### [bitcoin.pred.txid]

```cpp
[[nodiscard]] bool is_coinbase(const txid& t) noexcept;
```

*Returns:* `t.is_null()`.

______________________________________________________________________

#### [bitcoin.pred.outpoint]

```cpp
[[nodiscard]] bool is_coinbase(const outpoint& o) noexcept;
```

*Returns:* `is_coinbase(o.id())`.

______________________________________________________________________

#### [bitcoin.pred.tx_input]

```cpp
[[nodiscard]] bool signals_rbf(const tx_input& i) noexcept;
```

*Returns:* `i.sequence() < 0xFFFF'FFFEu`.

*Remarks:* Reflects BIP-125 opt-in replace-by-fee signaling. An input opts in
to RBF if its sequence number is less than `0xFFFFFFFE`.

```cpp
[[nodiscard]] bool has_relative_locktime(const tx_input& i) noexcept;
```

*Returns:* `(i.sequence() & 0x8000'0000u) == 0 && i.sequence() != 0xFFFF'FFFFu`.

*Remarks:* Reflects the BIP-68 relative lock-time enable flag (bit 31 clear,
sequence not final). Whether the relative locktime is enforced depends on the
enclosing transaction's version field (≥ 2 required); that check is consensus
logic and not part of this predicate.

______________________________________________________________________

#### [bitcoin.pred.tx_output]

```cpp
[[nodiscard]] bool is_unspendable(const tx_output& o) noexcept;
```

*Returns:* `!as_bytes(o.script()).empty() && as_bytes(o.script()).front() == std::byte{0x6a}`.

*Remarks:* An output whose script begins with `OP_RETURN` (`0x6a`) is provably
unspendable and conventionally used to embed arbitrary data in the blockchain.

______________________________________________________________________

#### [bitcoin.pred.transaction]

```cpp
[[nodiscard]] bool is_coinbase(const transaction& t) noexcept;
```

*Returns:* `t.inputs().size() == 1 && is_coinbase(t.inputs().front().previous_output())`.

```cpp
[[nodiscard]] bool is_segwit(const transaction& t) noexcept;
```

*Returns:* `true` if any element of `t.inputs()` has a non-empty `witness()`.

*Remarks:* Reflects BIP-141 segregated witness. A transaction is considered
SegWit if at least one of its inputs carries witness data.

```cpp
[[nodiscard]] bool locktime_is_height(const transaction& t) noexcept;
[[nodiscard]] bool locktime_is_time(const transaction& t)   noexcept;
```

`locktime_is_height(t)` returns `t.locktime() < 500'000'000`.
`locktime_is_time(t)` returns `t.locktime() >= 500'000'000`.

*Remarks:* A locktime value below 500,000,000 is interpreted as a block height;
a value at or above that threshold is a UNIX epoch timestamp in seconds (BIP-65,
BIP-112).

______________________________________________________________________

#### [bitcoin.pred.block]

```cpp
[[nodiscard]] bool has_coinbase(const block& b) noexcept;
```

*Returns:* `!b.transactions().empty() && is_coinbase(b.transactions().front())`.

______________________________________________________________________

## References

- **BIP-65** — OP_CHECKLOCKTIMEVERIFY\
  <https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki>

- **BIP-68** — Relative lock-time using consensus-enforced sequence numbers\
  <https://github.com/bitcoin/bips/blob/master/bip-0068.mediawiki>

- **BIP-112** — CHECKSEQUENCEVERIFY\
  <https://github.com/bitcoin/bips/blob/master/bip-0112.mediawiki>

- **BIP-125** — Opt-in Full Replace-by-Fee Signaling\
  <https://github.com/bitcoin/bips/blob/master/bip-0125.mediawiki>

- **BIP-141** — Segregated Witness (Consensus layer)\
  <https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki>

- **Bitcoin Developer Reference**\
  <https://developer.bitcoin.org/reference/>

- **Bitcoin Core source** — `src/primitives/`\
  <https://github.com/bitcoin/bitcoin>
