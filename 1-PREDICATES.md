---
title: Bitcoin Protocol Predicates
date: today
document: DXXXXR0
audience:
  - Library Evolution Working Group
  - SG14 (Low-Latency / Financial)
toc: false
references:
  - id: BIP65
    citation-label: BIP-65
    title: "BIP-65: OP_CHECKLOCKTIMEVERIFY"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki
  - id: BIP68
    citation-label: BIP-68
    title: >-
      BIP-68: Relative lock-time using consensus-enforced sequence
      numbers
    URL: https://github.com/bitcoin/bips/blob/master/bip-0068.mediawiki
  - id: BIP112
    citation-label: BIP-112
    title: "BIP-112: CHECKSEQUENCEVERIFY"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0112.mediawiki
  - id: BIP125
    citation-label: BIP-125
    title: "BIP-125: Opt-in Full Replace-by-Fee Signaling"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0125.mediawiki
  - id: BIP141
    citation-label: BIP-141
    title: "BIP-141: Segregated Witness (Consensus layer)"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki
---

[For illustrative purposes only. This document is written in the style of a WG21
standardization paper. It has not been submitted to the ISO C++ committee and is
not under active consideration for standardization.]{.draftnote}

# Abstract

This paper is a companion to *Adding Bitcoin Vocabulary Types to the C++
Standard Library*. That paper deliberately restricts itself to faithfully
carrying protocol data across library boundaries and omits interpretation of
protocol fields. This paper proposes a set of free functions in
`namespace bitcoin` that answer common protocol-level questions about those
vocabulary types. Each predicate can be specified entirely in terms of the
public interface of the vocabulary types, so no private access is required.

# Motivation and Scope

The vocabulary types paper exposes raw protocol fields (`sequence`, `locktime`,
`version`, …) without interpreting them. Many clients need to answer questions
such as:

- Is this transaction a coinbase?
- Does this transaction signal SegWit?
- Does this input opt in to replace-by-fee?
- Is this output provably unspendable?
- Is the transaction locktime a block height or a timestamp?

These predicates are straightforward to derive from the public API, but their
definitions depend on protocol constants and bit-field layouts specified by
several BIPs [@BIP65; @BIP68; @BIP112; @BIP125; @BIP141]. Standardizing these
operations would provide a common specification and reduce duplicated,
divergent implementations.

## In scope

- Free-function predicates on `outpoint`, `tx_input`, `tx_output`,
  `transaction`, and `block`.
- Definitions expressed entirely in terms of the public interface of the
  vocabulary types.

## Out of scope

- Consensus validation.
- Strong types for raw integer fields (`sequence`, `locktime`, `version`, …) —
  see the vocabulary types paper.
- Script execution or script pattern matching beyond a single leading-byte check
  for provably unspendable outputs.

# Impact on the Standard

This paper adds free functions to `namespace bitcoin` inside the `<bitcoin>`
header. It depends on the vocabulary types paper and introduces no new types.

# Design considerations

## D1 — Non-member predicates

Every predicate in this paper is expressible in terms of the public interface of
the corresponding vocabulary type. Specifying them as non-member functions
allows calls such as `is_coinbase(op)` to participate in argument-dependent
lookup, permits a single function name to be overloaded across related
vocabulary types, and allows additional predicates to be added without
modifying the associated class definitions.

## D2 — Normative semantic requirements

Unlike the vocabulary types (whose internal representation is
implementation-defined), these predicates have complete *Returns:* clauses
expressed in terms of the public API. The specification therefore gives their
semantics entirely in terms of the public interface.

## D3 — Transaction-version dependence of sequence-based predicates

The relative locktime rules from [@BIP68] are only active when the enclosing
transaction's version is ≥ 2. `has_relative_locktime` as specified here
reflects only the `tx_input`-level field condition (bit 31 clear, not final);
enforcement of the transaction-version gate is consensus logic and out of
scope.

# Proposed wording

The wording in this section is relative to the C++ Working Draft and assumes
that the wording of *Adding Bitcoin Vocabulary Types to the C++ Standard
Library* has been applied.

## [bitcoin.pred] Bitcoin protocol predicates

### [bitcoin.pred.syn] Synopsis

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
  [[nodiscard]] bool is_coinbase(const transaction& t);
  [[nodiscard]] bool is_segwit(const transaction& t);
  [[nodiscard]] bool locktime_is_height(const transaction& t) noexcept;
  [[nodiscard]] bool locktime_is_time(const transaction& t)   noexcept;

  // [bitcoin.pred.block]
  [[nodiscard]] bool has_coinbase(const block& b);

} // namespace bitcoin
```

### [bitcoin.pred.txid]

```cpp
[[nodiscard]] bool is_coinbase(const txid& t) noexcept;
```

*Returns:* `!static_cast<bool>(t)`.

### [bitcoin.pred.outpoint]

```cpp
[[nodiscard]] bool is_coinbase(const outpoint& o) noexcept;
```

*Returns:* `is_coinbase(o.txid())`.

### [bitcoin.pred.tx_input]

```cpp
[[nodiscard]] bool signals_rbf(const tx_input& i) noexcept;
```

*Returns:* `i.sequence() < 0xFFFF'FFFEu`.

*Remarks:* Reflects the opt-in replace-by-fee signaling specified in
[@BIP125]. An input opts in to RBF if its sequence number is less than
`0xFFFFFFFE`.

```cpp
[[nodiscard]] bool has_relative_locktime(const tx_input& i) noexcept;
```

*Returns:* `(i.sequence() & 0x8000'0000u) == 0 && i.sequence() != 0xFFFF'FFFFu`.

*Remarks:* Reflects the [@BIP68] relative lock-time enable flag (bit 31 clear,
sequence not final). Whether the relative locktime is enforced depends on the
enclosing transaction's version field (≥ 2 required); that check is consensus
logic and not part of this predicate.

### [bitcoin.pred.tx_output]

```cpp
[[nodiscard]] bool is_unspendable(const tx_output& o) noexcept;
```

*Returns:* `true` if `as_bytes(o.script())` is non-empty and its first
 element is `std::byte{0x6a}`; otherwise `false`.

*Remarks:* An output whose script begins with `OP_RETURN` (`0x6a`) is provably
unspendable. Such outputs are commonly used to carry data.

### [bitcoin.pred.transaction]

```cpp
[[nodiscard]] bool is_coinbase(const transaction& t);
```

*Returns:* `true` if `t.inputs().size() == 1` and
`is_coinbase(t.inputs().front().prevout())` are both `true`;
otherwise `false`.

```cpp
[[nodiscard]] bool is_segwit(const transaction& t);
```

*Returns:* `true` if any element of `t.inputs()` has a non-empty `witness()`.

*Remarks:* Reflects segregated witness as specified by [@BIP141]. A
transaction is considered SegWit if at least one of its inputs carries witness
data.

```cpp
[[nodiscard]] bool locktime_is_height(const transaction& t) noexcept;
[[nodiscard]] bool locktime_is_time(const transaction& t)   noexcept;
```

`locktime_is_height(t)` returns `t.locktime() < 500'000'000`.
`locktime_is_time(t)` returns `t.locktime() >= 500'000'000`.

*Remarks:* A locktime value below 500,000,000 is interpreted as a block height;
a value at or above that threshold is a UNIX epoch timestamp in seconds
[@BIP65; @BIP112].

### [bitcoin.pred.block]

```cpp
[[nodiscard]] bool has_coinbase(const block& b);
```

*Returns:* `!b.transactions().empty() && is_coinbase(b.transactions().front())`.
