---
title: "Bitcoin Consensus Validation"
date: today
document: VALIDATION
audience:
  - Library Evolution Working Group
  - SG14 (Low-Latency / Financial)
author:
  - name: Daniel Pfeifer
    email: <daniel@pfeifer-mail.de>
toc: false
references:
  - id: VOCABULARY
    citation-label: VOCABULARY
    title: "Adding Bitcoin Vocabulary Types to the C++ Standard Library"
    URL: https://purplekarrot.github.io/std-bitcoin/VOCABULARY.html
  - id: CPO
    citation-label: CPO
    title: "Bitcoin Customization Point Objects"
    URL: https://purplekarrot.github.io/std-bitcoin/CPO.html
  - id: BIP34
    citation-label: BIP-34
    title: "BIP-34: Block v2, Height in Coinbase"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0034.mediawiki
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
  - id: BIP141
    citation-label: BIP-141
    title: "BIP-141: Segregated Witness (Consensus layer)"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki
  - id: BIP144
    citation-label: BIP-144
    title: "BIP-144: Segregated Witness (Peer Services)"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0144.mediawiki
---

[For illustrative purposes only. This document is written in the style of a WG21
standardization paper. It has not been submitted to the ISO C++ committee and is
not under active consideration for standardization.]{.draftnote}

# Abstract

This paper proposes a standard C++ interface for Bitcoin consensus validation.

It defines a `verifier` function object that checks `block_header`,
`transaction`, and `block` objects, as well as forward ranges of blocks, and
returns ordinary values for both successful and unsuccessful validation. The
overload set makes required validation evidence explicit: some overloads need
only the candidate object, while others additionally accept a `chain_view`, the
current time, or a `coin_index`.

To support this interface, the paper specifies the `chain_view`, `coin`, and
`coin_index` concepts, the result type `validation_status`, the
network-parameter aggregate `consensus_parameters`, and predefined `verify`
objects for the standard Bitcoin networks.

The design keeps storage strategy, caching, and any type-erasure out of the
public API. A failed verification is reported as a normal return value; true
execution failures remain exceptional and propagate via exceptions.

This paper depends on the companion papers [@VOCABULARY] and [@CPO].

# Motivation

Bitcoin software needs a common interface for consensus validation that is
independent of any particular node, chainstate, or UTXO-set implementation. This
paper standardizes how programs supply validation evidence and receive
validation results, while leaving node architecture and storage choices outside
the specification.

The proposal separates the *interface* of validation from the complete
definition of Bitcoin consensus. It standardizes the `chain_view`, `coin`, and
`coin_index` abstractions and validation outcomes, together with overloads that
make progressively richer evidence explicit in the function signature. The
public interface uses protocol vocabulary types from [@VOCABULARY] and
observational customization point objects from [@CPO].

# Impact on the Standard

This is a pure library addition to the `<bitcoin>` header. It requires no core
language changes and does not modify or deprecate existing names.

# Design considerations

## Negative verification is an ordinary result

A negative verification result is not an exceptional condition: consensus
validation is a query whose answer may legitimately be "no". This follows the
error model articulated in [@P0709R4]: a function reports an error when it
cannot do what it advertised despite its preconditions having been met. An
invalid block, transaction, or header is instead an ordinary outcome of the
validation query. The proposed API therefore represents rule violations in
`validation_status` rather than by throwing dedicated validation exceptions.
Exceptions remain available for true failures such as allocation or I/O errors.

## Progressive verification

Some consensus rules can be checked from the candidate object alone, while
others require additional evidence such as the ancestor chain, the current time,
or the UTXO set. The overload set of `verifier::operator()` reflects this
progression directly: overloads with more parameters evaluate a superset of the
rules evaluated by overloads with fewer parameters.

## Constrained evidence parameters

Validation code needs access to chain and UTXO evidence supplied by
implementation-specific storage layers. The standardized interface therefore
expresses those dependencies as the concepts `chain_view` and `coin_index`, and
the overloads of `verifier::operator()` that take evidence parameters are
constrained member function templates. This keeps the public API concept-based
while still allowing implementations to adapt those arguments to private,
non-owning, type-erased representations internally. Any such type erasure is an
implementation detail and is not part of the public API.

## Validating a sequence of blocks

A forward range of blocks gives an implementation access to an entire candidate
chain segment. Multiple passes permit gathering outputs and chain context before
checking transactions, while leaving the range's storage representation and the
size of the segment to the application. The range need not be sized, random
access, contiguous, or a view.

Range order specifies consensus order, not execution order. The result is defined
by connecting the blocks in that order, starting from the supplied ancestor chain
and UTXO snapshot. An implementation may instead index outputs across the
segment, precompute transaction data, perform script and signature checks in
parallel, and aggregate spend-uniqueness checks. These strategies must preserve
the same success or failure outcome, including input availability at the point
of each spend and the rules applicable to each block.

The range overload returns one `validation_status` for the entire segment. It
does not require identifying the first invalid block or a maximal valid prefix.
This permits aggregate checks that detect a violation without immediately
locating it. No validation work or borrowed evidence remains outstanding when
the call returns.

## Alignment with standard lookup interfaces

The `coin_index` concept requires a `mapped_type` that models `coin` and a
`lookup` result convertible to `std::optional<mapped_type>`. The mapped type is
chosen by the evidence provider; this paper does not standardize a concrete
coin class. Its required properties are queried through the customization
point objects in [@CPO]. This permits stored values and lightweight handles
without requiring a `tx_output` subobject or a particular storage layout.

The name `lookup` follows the direction of the standard lookup proposals for
C++29. [@P3091R5] provides the optional-reference lookup model, while [@P4139R2]
argues for naming that operation `lookup` rather than `get`. An optional-reference
result is also permitted when it is convertible to the required optional value
and preserves the coin's observable properties. Under that design,
`std::map<outpoint, C>`, `std::unordered_map<outpoint, C>`, and
`std::flat_map<outpoint, C>` can model `coin_index` when `C` models `coin` and
their lookup results meet the conversion requirement.

That is valuable even if production nodes use custom UTXO data structures.
Tests, examples, and small implementations can use standard containers directly,
while full nodes remain free to provide more specialized storage and caching
architectures behind the same abstraction.

# Proposed wording

The wording in this section is relative to the C++ Working Draft and assumes
that the wording of [@VOCABULARY] and [@CPO] has been applied.

[Add the following declarations to the `<bitcoin>` header synopsis in
[bitcoin.syn].]{.ednote}

## [bitcoin.validation.version] Feature test macro

```cpp
#define __cpp_lib_bitcoin_validation 214XXXL // also in <bitcoin>
```

## [bitcoin.validation] Validation vocabulary and functions

### [bitcoin.validation.coin] Concept `coin`

The `coin` concept describes the evidence about a transaction output needed for
validation: its value, locking script, funding height, and whether the funding
transaction is coinbase. It does not require a particular representation or a
public vocabulary class.

```cpp
namespace bitcoin {

  template<class T>
  concept coin = requires (T const& c) {
    { bitcoin::value(c) } -> std::convertible_to<amount>;
    { bitcoin::output_script(c) } -> std::convertible_to<script_ref>;
    { bitcoin::funding_height(c) } -> std::convertible_to<std::size_t>;
    { bitcoin::is_coinbase(c) } -> std::convertible_to<bool>;
  };

} // namespace bitcoin
```

A type `T` models `coin` only if it satisfies the concept and, for an object
`c` representing a transaction output, the following semantic requirements
hold after conversion to the respective required types:

- `bitcoin::value(c)` is the amount of the represented output.
- `bitcoin::output_script(c)` refers to the locking script of that output.
- `bitcoin::funding_height(c)` is the height of the block containing the
  transaction that created that output.
- `bitcoin::is_coinbase(c)` is `true` if and only if that transaction is a
  coinbase transaction.

These operations are observational: they do not consume the represented output
or change its properties. Repeated queries on unchanged evidence yield the
same amount, script bytes, height, and coinbase provenance. Equality of coin
objects is not required.

The `script_ref` obtained from `bitcoin::output_script(c)` is non-owning. Its
backing storage must remain valid while that reference is used. Additional
lifetime requirements when a coin is supplied through `coin_index` are specified
in [bitcoin.validation.coinindex].

### [bitcoin.validation.chain] Concept `chain_view`

A type `T` models `chain_view` if and only if:

- `T` models `std::ranges::view`;
- `T` models `std::ranges::sized_range` and
  `std::ranges::random_access_range`;
- `std::ranges::range_reference_t<T>` is convertible to
  `bitcoin::block_header`; and
- concurrent observation of the same view and traversal using distinct iterator
  objects do not introduce data races or change the represented header sequence.

A `chain_view` represents the sequence of block headers on the path from the
genesis block to a particular tip. The concurrent-access requirement also applies
to any internal caching performed by its observation and traversal operations.

```cpp
namespace bitcoin {

  template<class T>
  concept chain_view =
    std::ranges::view<T> &&
    std::ranges::sized_range<T> &&
    std::ranges::random_access_range<T> &&
    std::convertible_to<std::ranges::range_reference_t<T>,
                        bitcoin::block_header>;

} // namespace bitcoin
```

### [bitcoin.validation.coinindex] Concept `coin_index`

The `coin_index` concept describes a lookup from an `outpoint` to optional coin
evidence. The provider's `mapped_type` models `coin`.

```cpp
namespace bitcoin {

  template<class T>
  concept coin_index =
    coin<typename T::mapped_type> &&
    requires (T const& m, outpoint const& p) {
      { m.lookup(p) } ->
        std::convertible_to<std::optional<typename T::mapped_type>>;
    };

} // namespace bitcoin
```

A type `T` models `coin_index` only if it satisfies the concept and the
following semantic requirements hold. Let `result` be the result of
`m.lookup(p)` converted to `std::optional<typename T::mapped_type>`:

- `result` is engaged if and only if `p` identifies an unspent output in the
  UTXO state represented by `m`.
- If engaged, `*result` represents that output and supplies its value, locking
  script, funding height, and coinbase provenance as specified by `coin`.
- Lookup does not spend, remove, or otherwise consume an output. Internal
  caching is permitted provided that it does not change the represented UTXO
  state or the observable coin evidence.
- An empty result denotes absence from the represented UTXO state. An execution
  failure, such as an I/O failure while retrieving evidence, shall not be
  reported as an empty result; it may propagate as an exception.

When supplied to `verifier::operator()`, the index represents a fixed UTXO
snapshot for the duration of the call. Repeated lookups of the same outpoint
shall agree on presence and, when present, on all four coin properties. The
represented UTXO state and the backing storage of scripts obtained from it
shall remain valid and unchanged until the call returns. In particular,
destroying a temporary lookup result shall not invalidate script
references obtained from that result during the call. This permits an
implementation to retain the required properties in a private non-owning
representation.

Concurrent calls to `lookup` on the same index, including calls for the same
outpoint, and concurrent observation of the returned coin evidence shall not
introduce data races or alter the represented UTXO state or observable coin
properties. This requirement also applies when lookup performs internal caching.

The order and number of lookups performed by a verifier are unspecified. An
implementation may invoke lookup and observe the returned coin evidence
concurrently. Storage, caching, persistence, and synchronization strategies are
otherwise unspecified.

### [bitcoin.validation.status] Class `validation_status`

`validation_status` represents the outcome of a consensus-rule evaluation. A
value of `validation_status` either indicates success — the verified object
satisfies all consensus rules evaluated by the call — or indicates failure.

```cpp
namespace bitcoin {

  class validation_status {
  public:
    // constructors: unspecified

    [[nodiscard]] bool ok() const noexcept;
    [[nodiscard]] explicit operator bool() const noexcept;

  private:
    // exposition only
  };

} // namespace bitcoin
```

The constructors of `validation_status` are unspecified. Implementations
construct `validation_status` objects internally as return values of
`verifier::operator()`; user code receives them by copy or move and does not
construct them directly.

#### [bitcoin.validation.status.obs] Observers

```cpp
[[nodiscard]] bool ok() const noexcept;
[[nodiscard]] explicit operator bool() const noexcept;
```

*Returns:* `true` if `*this` represents a successful verification; otherwise
`false`. Both functions return the same value for a given `validation_status`
object.

#### [bitcoin.validation.status.fmt] Formatter specialization

```cpp
namespace std {

  template<>
  struct formatter<bitcoin::validation_status> {
    constexpr format_parse_context::iterator
      parse(format_parse_context& ctx);

    format_context::iterator
      format(bitcoin::validation_status s,
             format_context& ctx) const;
  };

} // namespace std
```

The specialization of `formatter` for `bitcoin::validation_status` meets the
requirements of `Formatter` ([format.requirements]).

`parse` parses the format specification as a `std-format-spec`
([format.string.std]).

`format` writes a textual representation of `s` to the output iterator of `ctx`.
The textual representation of a successful status is implementation-defined. The
textual representation of a failure status may include details identifying the
violated consensus rule, but is not required to; a minimal conforming
implementation may produce a generic failure message for all failure values.

### [bitcoin.validation.params] Aggregate `consensus_parameters`

`consensus_parameters` is an aggregate that bundles the network-specific
constants governing Bitcoin consensus rules. A `verifier` constructed from a
reference to a `consensus_parameters` instance evaluates consensus rules against
those constants.

```cpp
namespace bitcoin {

  struct consensus_parameters {
    // not yet specified in this revision
  };

} // namespace bitcoin
```

The members of `consensus_parameters`, their semantics, and their default member
initializers are not yet specified in this revision of the paper.
A later revision will specify them.

\[This paper intentionally does not yet specify the exact set of consensus
parameters or the exact parameter values for the standard Bitcoin networks. It
specifies only the interface through which such parameters are supplied to
validation.\]{.ednote}

### [bitcoin.validation.verifier] Class `verifier`

`verifier` is a lightweight function object that holds a non-owning pointer to a
`consensus_parameters` instance and evaluates Bitcoin consensus rules for
`block_header`, `transaction`, and `block` objects and forward ranges of blocks.

```cpp
namespace bitcoin {

  class verifier {
  public:
    constexpr explicit verifier(const consensus_parameters& params) noexcept;

    // --- Block header ---

    [[nodiscard]] validation_status
      operator()(const block_header& h) const;

    template<class Chain>
      requires chain_view<std::remove_cvref_t<Chain>>
    [[nodiscard]] validation_status
      operator()(const block_header& h, Chain&& chain,
                 std::chrono::sys_seconds now) const;

    // --- Block ---

    [[nodiscard]] validation_status
      operator()(const block& b) const;

    template<class Chain>
      requires chain_view<std::remove_cvref_t<Chain>>
    [[nodiscard]] validation_status
      operator()(const block& b, Chain&& chain,
                 std::chrono::sys_seconds now) const;

    template<class Chain, class Coins>
      requires chain_view<std::remove_cvref_t<Chain>> &&
               coin_index<std::remove_cvref_t<Coins>>
    [[nodiscard]] validation_status
      operator()(const block& b, Chain&& chain,
                 std::chrono::sys_seconds now,
                 const Coins& coins) const;

    // --- Sequence of blocks ---

    template<std::ranges::forward_range Blocks, class Chain, class Coins>
      requires std::same_as<std::ranges::range_value_t<Blocks>, block> &&
               std::convertible_to<std::ranges::range_reference_t<Blocks>,
                                   block> &&
               chain_view<std::remove_cvref_t<Chain>> &&
               coin_index<std::remove_cvref_t<Coins>>
    [[nodiscard]] validation_status
      operator()(Blocks&& blocks, Chain&& chain,
                 std::chrono::sys_seconds now,
                 const Coins& coins) const;

    // --- Transaction ---

    [[nodiscard]] validation_status
      operator()(const transaction& tx) const;

    template<class Chain>
      requires chain_view<std::remove_cvref_t<Chain>>
    [[nodiscard]] validation_status
      operator()(const transaction& tx, Chain&& chain) const;

    template<class Chain, class Coins>
      requires chain_view<std::remove_cvref_t<Chain>> &&
               coin_index<std::remove_cvref_t<Coins>>
    [[nodiscard]] validation_status
      operator()(const transaction& tx, Chain&& chain,
                 const Coins& coins) const;

  private:
    const consensus_parameters* params_; // exposition only
  };

} // namespace bitcoin
```

`verifier::operator()` returns a `validation_status` whose value is `true` if
the verified object satisfies all consensus rules evaluated by that overload,
and `false` otherwise. If multiple rules are violated, the returned failure
status identifies an implementation-defined failure condition.

`verifier::operator()` functions are not `noexcept`. Implementations may
propagate exceptions arising from ordinary execution, such as memory allocation
failure. An exception indicates a true failure, not a negative verification
result. A caller shall not interpret a propagated exception as a negative
verification outcome.

The overloads of `verifier::operator()` that take evidence parameters are
constrained member function templates. Chain evidence is accepted as any type
that models `chain_view` ([bitcoin.validation.chain]); UTXO evidence is accepted
as any type that models `coin_index` ([bitcoin.validation.coinindex]). The
standardized interface does not expose or specify any type-erased adaptation
mechanism.

Each overload evaluates the subset of Bitcoin consensus rules that can be
evaluated with the evidence it receives, parameterized by `*params_`. An
overload that accepts more parameters evaluates a superset of the rules
evaluated by an overload with fewer parameters. The precise definition of those
consensus rules is outside the scope of this paper.

`verifier` is a non-owning reference to its `consensus_parameters`. The caller
is responsible for ensuring that the `consensus_parameters` object outlives the
`verifier`. Evidence arguments passed to `operator()` are borrowed only for the
duration of the call. The candidate range and its elements are likewise borrowed
only for the duration of a range validation call.

#### [bitcoin.validation.verifier.cons] Constructor

```cpp
constexpr explicit verifier(const consensus_parameters& params) noexcept;
```

*Effects:* Stores `std::addressof(params)` in `*this`.

*Preconditions:* `params` outlives `*this`.

#### [bitcoin.validation.verifier.header.intrinsic] `operator()(const block_header&)`

```cpp
[[nodiscard]] validation_status
  operator()(const block_header& h) const;
```

*Returns:* A successful `validation_status` if `h` satisfies all intrinsic
header consensus rules; otherwise a failing `validation_status`.

*Remarks:* This overload evaluates only rules that can be checked against `h`
alone. It does not compare `h` against any ancestor chain.

#### [bitcoin.validation.verifier.header.chain_time] `template<class Chain> operator()(const block_header&, Chain&&, sys_seconds)`

```cpp
template<class Chain>
  requires chain_view<std::remove_cvref_t<Chain>>
[[nodiscard]] validation_status
  operator()(const block_header& h, Chain&& chain,
             std::chrono::sys_seconds now) const;
```

*Returns:* A successful `validation_status` if `h` satisfies all header
consensus rules evaluated by this overload; otherwise a failing
`validation_status`.

*Remarks:* This overload evaluates a superset of the rules evaluated by
`operator()(const block_header&)`, incorporating rules that require the ancestor
chain and the current time.

#### [bitcoin.validation.verifier.block.intrinsic] `operator()(const block&)`

```cpp
[[nodiscard]] validation_status
  operator()(const block& b) const;
```

*Returns:* A successful `validation_status` if `b` satisfies all intrinsic block
consensus rules; otherwise a failing `validation_status`.

#### [bitcoin.validation.verifier.block.chain_time] `template<class Chain> operator()(const block&, Chain&&, sys_seconds)`

```cpp
template<class Chain>
  requires chain_view<std::remove_cvref_t<Chain>>
[[nodiscard]] validation_status
  operator()(const block& b, Chain&& chain,
             std::chrono::sys_seconds now) const;
```

*Returns:* A successful `validation_status` if `b` satisfies all block
consensus rules evaluated by this overload; otherwise a failing
`validation_status`.

*Remarks:* This overload evaluates a superset of the rules evaluated by
`operator()(const block&)`, incorporating rules that require the ancestor chain
and the current time.

#### [bitcoin.validation.verifier.block.chain_time_coins] `template<class Chain, class Coins> operator()(const block&, Chain&&, sys_seconds, Coins&&)`

```cpp
template<class Chain, class Coins>
  requires chain_view<std::remove_cvref_t<Chain>> &&
           coin_index<std::remove_cvref_t<Coins>>
[[nodiscard]] validation_status
  operator()(const block& b, Chain&& chain,
             std::chrono::sys_seconds now,
             Coins&& coins) const;
```

*Preconditions:* `coins` represents the UTXO state at the tip of `chain`, before
processing `b`, and meets the snapshot and lifetime requirements in
[bitcoin.validation.coinindex].

*Returns:* A successful `validation_status` if `b` satisfies all block
consensus rules evaluated by this overload; otherwise a failing
`validation_status`.

*Remarks:* This overload evaluates a superset of the rules evaluated by the
overload that accepts `b`, `chain`, and `now`, incorporating rules that require
UTXO-set evidence via `coins`.

The verifier accounts for outputs created and spent within `b` according to
transaction order without applying those changes to the UTXO state represented
by `coins`. The verifier resolves outputs created within `b` and tracks spends
internally; lookup continues to report membership in the initial snapshot.
Missing inputs and double spends in the candidate block are unsuccessful
validation outcomes.

#### [bitcoin.validation.verifier.blocks.chain_time_coins] `template<class Blocks, class Chain, class Coins> operator()(Blocks&&, Chain&&, sys_seconds, const Coins&)`

```cpp
template<std::ranges::forward_range Blocks, class Chain, class Coins>
  requires std::same_as<std::ranges::range_value_t<Blocks>, block> &&
           std::convertible_to<std::ranges::range_reference_t<Blocks>, block> &&
           chain_view<std::remove_cvref_t<Chain>> &&
           coin_index<std::remove_cvref_t<Coins>>
[[nodiscard]] validation_status
  operator()(Blocks&& blocks, Chain&& chain,
             std::chrono::sys_seconds now,
             const Coins& coins) const;
```

*Preconditions:* `chain` represents an already validated ancestor chain and
`coins` represents its UTXO state, before processing any candidate block. If
`chain` is empty, `coins` represents the empty UTXO state preceding the genesis
block. The evidence meets the snapshot and lifetime requirements in
[bitcoin.validation.coinindex]. The candidate range remains valid and yields
the same sequence of block values on repeated traversal for the duration of the
call. Concurrent observation of `blocks`, traversal using distinct iterator
objects, and observation of the yielded blocks and their transaction, script,
and witness data do not introduce data races or change the candidate values.
This requirement also applies to any internal caching performed by those
operations. Concurrent access to `chain` meets the requirements in
[bitcoin.validation.chain].

*Returns:* A successful `validation_status` if the entire sequence can be
connected to `chain`, starting from the UTXO state represented by `coins`, under
the block consensus rules evaluated by the single-block overload accepting chain,
time, and UTXO evidence; otherwise a failing `validation_status`. An empty range
returns a successful `validation_status`.

For determining this outcome, blocks are considered in range order and
transactions within each block in transaction order. Each block is evaluated
against `chain` extended by the preceding candidate headers, using the UTXO state
resulting from the preceding candidate blocks and transactions. The supplied
`now` is used for every candidate block. Heights, median times, difficulty, and
activation-dependent rules are determined in the context of each candidate
block, not solely from the initial ancestor tip.

*Remarks:* Candidate linkage is checked by the verifier. A first block that does
not extend the supplied ancestor chain, or a later block that does not extend
its predecessor in the range, produces unsuccessful validation. With an empty
ancestor chain, the first candidate must be the network's genesis block.

The verifier resolves outputs created within the segment and accounts for
spends according to consensus order without changing the UTXO state represented
by `coins`. Lookup continues to report membership in the initial snapshot.
Missing inputs, references to outputs that do not yet exist at the point of
spending, and double spends within or across candidate blocks produce
unsuccessful validation. Output resolution and spend accounting shall preserve
the applicable consensus rules for duplicate transaction identifiers and
historical exceptions; an outpoint is not assumed to identify only one output
occurrence over the entire history.

The implementation may traverse `blocks` multiple times and reorder, combine,
or execute validation computations concurrently, provided that the returned
success or failure is unchanged. This includes collecting output evidence before
its creating block has been fully validated and deferring spend-uniqueness checks
until other checks have completed. Successful validation shall not be reported
until all checks required to establish the outcome have completed. The
implementation may read `blocks`, its yielded block data, and `chain`
concurrently under their concurrent-access requirements.

The failure condition reported is implementation-defined. A failure status does
not certify any prefix of the range, and the implementation is not required to
identify the first invalid block. All validation work initiated by the call
shall have completed before the call returns or propagates an exception; the
implementation shall not retain borrowed arguments or references into them
beyond that point.

#### [bitcoin.validation.verifier.tx.intrinsic] `operator()(const transaction&)`

```cpp
[[nodiscard]] validation_status
  operator()(const transaction& tx) const;
```

*Returns:* A successful `validation_status` if `tx` satisfies all intrinsic
transaction consensus rules; otherwise a failing `validation_status`.

#### [bitcoin.validation.verifier.tx.chain] `template<class Chain> operator()(const transaction&, Chain&&)`

```cpp
template<class Chain>
  requires chain_view<std::remove_cvref_t<Chain>>
[[nodiscard]] validation_status
  operator()(const transaction& tx, Chain&& chain) const;
```

*Returns:* A successful `validation_status` if `tx` satisfies all transaction
consensus rules evaluated by this overload; otherwise a failing
`validation_status`.

*Remarks:* This overload evaluates a superset of the rules evaluated by
`operator()(const transaction&)`, incorporating rules that require chain
context.

#### [bitcoin.validation.verifier.tx.chain_coins] `template<class Chain, class Coins> operator()(const transaction&, Chain&&, Coins&&)`

```cpp
template<class Chain, class Coins>
  requires chain_view<std::remove_cvref_t<Chain>> &&
           coin_index<std::remove_cvref_t<Coins>>
[[nodiscard]] validation_status
  operator()(const transaction& tx, Chain&& chain,
             Coins&& coins) const;
```

*Preconditions:* `coins` represents the UTXO state at the tip of `chain`, before
processing `tx`, and meets the snapshot and lifetime requirements in
[bitcoin.validation.coinindex].

*Returns:* A successful `validation_status` if `tx` satisfies all transaction
consensus rules evaluated by this overload; otherwise a failing
`validation_status`.

*Remarks:* This overload evaluates a superset of the rules evaluated by the
overload that accepts `tx` and `chain`, incorporating rules that require UTXO
information via `coins`.

Validation leaves the UTXO state represented by `coins` unchanged. Missing
inputs and duplicate spends within the candidate transaction are unsuccessful
validation outcomes.

### [bitcoin.validation.verify] Predefined `params` and `verify` objects

Predefined `inline constexpr` objects are provided for the standard Bitcoin
networks. Each `verify` object references a `consensus_parameters` instance with
static storage duration configured for its respective network.

```cpp
namespace bitcoin {

  namespace mainnet {
    inline constexpr consensus_parameters params = { /* mainnet */ };
  }

  inline constexpr verifier verify{mainnet::params};

} // namespace bitcoin
```

`bitcoin::verify` is a `verifier` for the main Bitcoin network.

```cpp
namespace bitcoin::testnet {

  inline constexpr consensus_parameters params = { /* testnet */ };
  inline constexpr verifier verify{params};

} // namespace bitcoin::testnet
```

```cpp
namespace bitcoin::signet {

  inline constexpr consensus_parameters params = { /* signet */ };
  inline constexpr verifier verify{params};

} // namespace bitcoin::signet
```

```cpp
namespace bitcoin::regtest {

  inline constexpr consensus_parameters params = { /* regtest */ };
  inline constexpr verifier verify{params};

} // namespace bitcoin::regtest
```

The exact contents of these predefined `params` objects are not yet specified in
this revision of the paper. A later revision will specify the network-specific
parameter sets.

A program calls `verify` as if it were a function:

```cpp
auto status = bitcoin::verify(b, chain, now, coins);          // mainnet
auto status = bitcoin::testnet::verify(b, chain, now, coins); // testnet
auto status = bitcoin::regtest::verify(h);                    // regtest
```

A forward range can be validated with the same evidence interface:

```cpp
// blocks is a forward range of bitcoin::block values in candidate chain order.
// chain and coins describe the validated state preceding the entire segment.
auto status = bitcoin::verify(blocks, chain, now, coins);
```

A program may also construct a `verifier` with custom `consensus_parameters` for
networks not predefined by the standard:

```cpp
auto my_params = bitcoin::mainnet::params;
// adjust selected members of my_params

bitcoin::verifier my_net{my_params};
auto status = my_net(b, chain, now, coins);
```

Because `verifier` holds a non-owning pointer, the `consensus_parameters` object
must outlive the `verifier`. For the predefined `params` objects this is
automatic. For local `consensus_parameters` objects, the caller is responsible
for lifetime management. The `chain` and `coins` arguments supplied to
`operator()` are borrowed only for the duration of the call.
