---
title: "Bitcoin Validation: `coin`, `coin_index`, `verification_status`, and `verify`"
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
  - id: PREDICATES
    citation-label: PREDICATES
    title: "Bitcoin Protocol Predicates"
    URL: https://purplekarrot.github.io/std-bitcoin/PREDICATES.html
  - id: SERDES
    citation-label: SERDES
    title: "Bitcoin Wire Formats: Parsing and Serialization"
    URL: https://purplekarrot.github.io/std-bitcoin/SERDES.html
  - id: SCRIPT
    citation-label: SCRIPT
    title: "Bitcoin Script Extensions"
    URL: https://purplekarrot.github.io/std-bitcoin/SCRIPT.html
  - id: CHAIN
    citation-label: CHAIN
    title: "Bitcoin Chains: `chain_view` and `any_chain_view`"
    URL: https://purplekarrot.github.io/std-bitcoin/CHAIN.html
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

This paper adds the vocabulary type `bitcoin::coin`, the concept
`bitcoin::coin_index`, the class `bitcoin::verification_status`, the class
template `bitcoin::verify_result<Fact>`, the aggregate
`bitcoin::consensus_parameters`, the class `bitcoin::verifier`, and predefined
`inline constexpr` `verify` objects for Bitcoin networks to the `<bitcoin>`
header.

`coin` represents an unspent transaction output — a `tx_output` together with
the height at which its containing transaction was confirmed. `coin_index`
provides a lookup interface from `outpoint` to `std::optional<coin>`.

`verification_status` represents the outcome of a consensus-rule evaluation. Its
constructors are unspecified; the set of failure values is
implementation-defined.

`verify_result<Fact>` carries either a successful verification's by-product (a
*fact*) or a failure status.

`consensus_parameters` is an aggregate bundling the network-specific constants
that govern Bitcoin consensus rules.

`verifier` is a lightweight function object holding a non-owning pointer to a
`consensus_parameters` instance. It exposes an overload set of `operator()` for
`block_header`, `transaction`, and `block`; the contextual overloads are
constrained member function templates over `chain_view` and `coin_index`. The
progression from intrinsic to contextual verification is expressed through
overloading: additional parameters represent additional consensus evidence.

Predefined `inline constexpr` `verify` objects are provided in network-specific
namespaces. Each object references a `consensus_parameters` instance with static
storage duration configured for its respective network. The object
`bitcoin::verify` is configured for the Bitcoin mainnet.

A negative verification result is one of two normal outcomes of consensus
evaluation and is reported by the return value. True failures — allocation
errors, I/O failures, and the like — remain exceptional and propagate via
exceptions.

This paper depends on the companion papers \[@VOCABULARY; @PREDICATES; @SERDES;
@SCRIPT; @CHAIN\].

# Motivation and Scope

Bitcoin software needs a common interface for consensus validation that is
independent of any particular node, chainstate, or UTXO-set implementation. The
companion papers standardize vocabulary types, predicates, serialization, script
utilities, and chain views; this paper layers on top of those facilities to
standardize how programs supply contextual evidence and receive validation
results.

The proposal separates the *interface* of validation from the complete
definition of Bitcoin consensus. It standardizes the `coin` vocabulary type,
the `coin_index` concept, validation outcomes, and validation by-products,
together with overloads that make progressively richer evidence explicit in the
function signature.

## In scope

- The vocabulary type `bitcoin::coin`.
- The concept `bitcoin::coin_index`.
- The class `bitcoin::verification_status`.
- The class template `bitcoin::verify_result<Fact>`.
- The fact types `bitcoin::header_fact`, `bitcoin::tx_fact`,
  `bitcoin::block_undo`, and `bitcoin::block_fact`.
- The aggregate `bitcoin::consensus_parameters`.
- The class `bitcoin::verifier`.
- The predefined `inline constexpr` object `bitcoin::verify`.
- The predefined `inline constexpr` object `bitcoin::testnet::verify`.
- The predefined `inline constexpr` object `bitcoin::signet::verify`.
- The predefined `inline constexpr` object `bitcoin::regtest::verify`.

## Out of scope

- The Bitcoin consensus rules themselves. Each `verifier::operator()` overload
  evaluates a defined subset of the consensus rules; which rules belong to which
  subset is determined by the overload's evidence parameters. The precise
  definition of each rule is outside the scope of this paper.
- Node-level functions that produce chain objects or manage chainstate.
- UTXO set storage, caching strategy, and database implementation.
- Mempool policy rules.
- Network or P2P protocol concerns.
- Script execution engine (the `verifier` invokes script evaluation as a
  subroutine but does not specify the interpreter).
- Chain-property derivation functions such as median-time-past and
  difficulty-target computation. These are internal implementation details of
  `verifier::operator()` and are not exposed.

# Impact on the Standard

This is a pure library addition. It adds declarations to `namespace bitcoin` and
nested namespaces inside the `<bitcoin>` header. It requires no core language
changes and introduces no new standard library dependencies beyond `<chrono>`,
`<concepts>`, `<cstdint>`, `<format>`, `<optional>`, and `<vector>`, which are
already required by the papers on which this paper depends or by the C++
standard library.

No existing names are modified or deprecated.

# Design considerations

## Negative verification is an ordinary result

A negative verification result is not an exceptional condition: consensus
validation is a query whose answer may legitimately be "no". The proposed API
therefore represents rule violations in `verification_status` and
`verify_result<Fact>` rather than by throwing dedicated validation exceptions.
Exceptions remain available for true failures such as allocation or I/O errors.

## Progressive contextual verification

Some consensus rules can be checked from the candidate object alone, while
others require external evidence such as the ancestor chain, the current time,
or the UTXO set. The overload set of `verifier::operator()` reflects this
progression directly: overloads with more parameters evaluate a superset of the
rules evaluated by overloads with fewer parameters.

## Constrained evidence parameters

Validation code often needs access to chain and UTXO evidence supplied by
implementation-specific storage layers. The standardized interface therefore
expresses those dependencies as the concepts `chain_view` [@CHAIN] and
`coin_index`, and the contextual `verifier::operator()` overloads are
constrained member function templates. Implementations may still adapt those
arguments to private non-owning, type-erased representations internally, but
that mechanism is not specified and is not part of the public API.

# Proposed wording

The wording in this section is relative to the C++ Working Draft and assumes
that the wording of [@VOCABULARY; @PREDICATES; @SERDES; @SCRIPT; @CHAIN] has
been applied.

[Add the following declarations to the `<bitcoin>` header synopsis in
[bitcoin.syn].]{.ednote}

## [bitcoin.validation.version] Feature test macro

```cpp
#define __cpp_lib_bitcoin_validation 214XXXL // also in <bitcoin>
```

## [bitcoin.validation] Validation vocabulary and functions

### [bitcoin.validation.coin] Vocabulary type `coin`

`coin` represents an unspent transaction output: the `tx_output` that was
created by a prior transaction, together with the height at which that
transaction was confirmed on the chain.

```cpp
namespace bitcoin {

  class coin {
  public:
    [[nodiscard]] const tx_output& output() const noexcept;
    [[nodiscard]] std::size_t height() const noexcept;

    friend bool operator==(const coin& lhs, const coin& rhs) noexcept;

  private:
    tx_output output_;   // exposition only
    std::size_t height_; // exposition only
  };

} // namespace bitcoin
```

#### [bitcoin.validation.coin.obs] Observers

```cpp
[[nodiscard]] const tx_output& output() const noexcept;
```

*Returns:* A reference to the transaction output represented by this coin.

```cpp
[[nodiscard]] std::size_t height() const noexcept;
```

*Returns:* The height of the block in which the transaction that created this
coin was confirmed.

### [bitcoin.validation.coinindex] Concept `coin_index`

A type `T` models `coin_index` if it provides a lookup from `outpoint` to
`std::optional<coin>`. The concept places no constraints on storage, caching,
persistence, or concurrency strategy.

```cpp
namespace bitcoin {

  template<class T>
  concept coin_index = requires (T const& m, outpoint p) {
    { m.lookup(p) } -> std::convertible_to<std::optional<coin>>;
  };

} // namespace bitcoin
```

### [bitcoin.validation.status] Class `verification_status`

`verification_status` represents the outcome of a consensus-rule evaluation. A
value of `verification_status` either indicates success — the verified object
satisfies all consensus rules evaluated by the call — or indicates failure,
identifying one or more violated consensus rules.

```cpp
namespace bitcoin {

  class verification_status {
  public:
    // constructors: unspecified

    [[nodiscard]] bool ok() const noexcept;
    [[nodiscard]] explicit operator bool() const noexcept;

    friend bool operator==(const verification_status& lhs,
                           const verification_status& rhs) noexcept;

  private:
    // exposition only
  };

} // namespace bitcoin
```

The constructors of `verification_status` are unspecified. Implementations
construct `verification_status` objects internally as return values of
`verifier::operator()`; user code receives them by copy or move and does not
construct them directly.

#### [bitcoin.validation.status.obs] Observers

```cpp
[[nodiscard]] bool ok() const noexcept;
[[nodiscard]] explicit operator bool() const noexcept;
```

*Returns:* `true` if `*this` represents a successful verification; otherwise
`false`. Both functions return the same value for a given `verification_status`
object.

#### [bitcoin.validation.status.eq] Equality

```cpp
friend bool operator==(const verification_status& lhs,
                       const verification_status& rhs) noexcept;
```

*Returns:* `true` if `lhs` and `rhs` represent equivalent verification outcomes;
otherwise `false`.

*Remarks:* Two `verification_status` objects that both represent success compare
equal. Two objects that both represent failure compare equal if and only if they
identify the same violated consensus rule. The set of distinct failure values,
their names, and their meanings are implementation-defined.

#### [bitcoin.validation.status.fmt] Formatter specialization

```cpp
namespace std {

  template<class CharT>
  struct formatter<bitcoin::verification_status, CharT> {
    constexpr typename basic_format_parse_context<CharT>::iterator
      parse(basic_format_parse_context<CharT>& ctx);

    typename basic_format_context<CharT>::iterator
      format(const bitcoin::verification_status& s,
             basic_format_context<CharT>& ctx) const;
  };

} // namespace std
```

The specialization of `formatter` for `bitcoin::verification_status` meets the
requirements of `BasicFormatter` ([format.requirements]) and `Formatter`
([format.requirements]).

`parse` parses the format specification as a `std-format-spec`
([format.string.std]).

`format` writes a textual representation of `s` to the output iterator of `ctx`.
The textual representation of a successful status is implementation-defined. The
textual representation of a failure status may include details identifying the
violated consensus rule, but is not required to; a minimal conforming
implementation may produce a generic failure message for all failure values.

### [bitcoin.validation.result] Class template `verify_result<Fact>`

```cpp
namespace bitcoin {

  template<class Fact>
  class verify_result {
  public:
    verify_result(Fact fact) noexcept(std::is_nothrow_move_constructible_v<Fact>);
    verify_result(verification_status status) noexcept;

    [[nodiscard]] bool ok() const noexcept;
    [[nodiscard]] explicit operator bool() const noexcept;

    [[nodiscard]] verification_status status() const &  noexcept;
    [[nodiscard]] verification_status status() &&      noexcept;
    [[nodiscard]] const Fact& fact() const &  noexcept;
    [[nodiscard]] Fact&&       fact() &&      noexcept;

  private:
    bool valid_;                   // exposition only
    verification_status status_;   // exposition only
    Fact fact_;                    // exposition only; valid when valid_ is true
  };

} // namespace bitcoin
```

#### [bitcoin.validation.result.cons] Constructors

```cpp
verify_result(Fact fact) noexcept(std::is_nothrow_move_constructible_v<Fact>);
```

*Postconditions:* `ok()` is `true`; `status().ok()` is `true`; `fact()` refers
to the stored fact.

```cpp
verify_result(verification_status status) noexcept;
```

*Preconditions:* `status.ok()` is `false`.

*Postconditions:* `ok()` is `false`; `status()` compares equal to `status`.

#### [bitcoin.validation.result.obs] Observers

```cpp
[[nodiscard]] bool ok() const noexcept;
[[nodiscard]] explicit operator bool() const noexcept;
```

*Returns:* `true` if `*this` was constructed from a `Fact`; otherwise `false`.

```cpp
[[nodiscard]] verification_status status() const &  noexcept;
[[nodiscard]] verification_status status() &&      noexcept;
```

*Returns:* The `verification_status` associated with this result. If `ok()` is
`true`, the returned status represents success; otherwise it represents the
violated consensus rule.

```cpp
[[nodiscard]] const Fact& fact() const &  noexcept;
[[nodiscard]] Fact&&       fact() &&      noexcept;
```

*Preconditions:* `ok()` is `true`.

*Returns:* The fact produced by successful verification.

### [bitcoin.validation.fact] Fact types

Each `verifier::operator()` overload that succeeds produces a by-product — a
*fact* — carrying information needed to apply state updates without recomputing
the same information.

#### [bitcoin.validation.fact.header] `header_fact`

```cpp
namespace bitcoin {

  struct header_fact {
    block_hash hash;
  };

} // namespace bitcoin
```

*Members:*

- `hash` — the block hash, equal to `block_hash{h}` where `h` is the verified
  header.

#### [bitcoin.validation.fact.tx] `tx_fact`

```cpp
namespace bitcoin {

  struct tx_fact {
    txid id;
    amount fee;
    std::uint64_t legacy_sigops = 0;
    std::uint64_t witness_sigops = 0;
  };

} // namespace bitcoin
```

*Members:*

- `id` — the transaction's identifier, equal to `txid{tx}`.
- `fee` — the difference between total input value and total output value. Zero
  for coinbase transactions.
- `legacy_sigops` — the number of signature operations counted in legacy script
  fields.
- `witness_sigops` — the number of signature operations counted in witness
  program data.

#### [bitcoin.validation.fact.block] `block_undo` and `block_fact`

```cpp
namespace bitcoin {

  struct block_undo {
    std::vector<coin> spent_coins;
  };

  struct block_fact {
    block_hash hash;
    amount total_fees;
    amount subsidy;
    std::uint64_t total_legacy_sigops;
    std::uint64_t total_witness_sigops;
    block_undo undo;
  };

} // namespace bitcoin
```

*Members of `block_undo`:*

- `spent_coins` — for each non-coinbase input in the block, in order, the `coin`
  that was spent by that input.

*Members of `block_fact`:*

- `hash` — the block's hash, equal to `block_hash{b}`.
- `total_fees` — the sum of fees across all non-coinbase transactions in the
  block.
- `subsidy` — the block subsidy at the block's height.
- `total_legacy_sigops` — the sum of `legacy_sigops` across all transactions in
  the block.
- `total_witness_sigops` — the sum of `witness_sigops` across all transactions
  in the block.
- `undo` — undo data for all non-coinbase transactions in the block.

### [bitcoin.validation.params] Aggregate `consensus_parameters`

`consensus_parameters` is an aggregate that bundles the network-specific
constants governing Bitcoin consensus rules. A `verifier` constructed from a
reference to a `consensus_parameters` instance evaluates consensus rules against
those constants.

```cpp
namespace bitcoin {

  struct consensus_parameters {
    // Proof of work
    std::uint32_t proof_of_work_limit   = 0x1d00ffff;
    std::size_t retarget_interval       = 2016;
    std::int64_t target_timespan_seconds = 14 * 24 * 60 * 60;

    // Block subsidy
    amount initial_subsidy              = amount{50 * 100'000'000};
    std::size_t halving_interval        = 210'000;

    // Coinbase maturity (confirmations required before a coinbase output may be spent)
    std::size_t coinbase_maturity       = 100;

    // Block limits
    std::size_t max_block_weight        = 4'000'000;
    std::size_t max_block_serialized_size = 4'000'000;
    std::size_t max_block_sigops        = 80'000;

    // Transaction limits
    std::size_t max_tx_weight           = 400'000;
    std::size_t max_tx_sigops           = 20'000;

    // Locktime and relative locktime
    bool locktime_enabled               = true;
    bool relative_locktime_enabled      = true;

    // BIP-34 height commitment
    bool bip34_enabled                  = true;
    std::size_t bip34_activation_height = 227'931;

    // SegWit
    bool segwit_enabled                 = true;
    std::size_t segwit_activation_height = 481'824;
  };

} // namespace bitcoin
```

The members of `consensus_parameters` are public data members. The default
values shown above correspond to the Bitcoin mainnet.

\[The set of parameters is illustrative. A production paper would enumerate
every consensus parameter that varies by network. The precise semantics of each
parameter are outside the scope of this paper; they govern the consensus rules
evaluated by `verifier::operator()`, whose definitions are likewise outside the
scope of this paper.\]{.ednote}

### [bitcoin.validation.verifier] Class `verifier`

`verifier` is a lightweight function object that holds a non-owning pointer to a
`consensus_parameters` instance and evaluates Bitcoin consensus rules for
`block_header`, `transaction`, and `block` objects. It is a literal type. It is
copyable, movable, and default-constructible. `sizeof(verifier)` is equal to
`sizeof(const consensus_parameters*)`.

```cpp
namespace bitcoin {

  class verifier {
  public:
    constexpr verifier() noexcept;
    constexpr explicit verifier(const consensus_parameters& params) noexcept;

    constexpr verifier(const verifier&) noexcept = default;
    constexpr verifier(verifier&&) noexcept = default;
    constexpr verifier& operator=(const verifier&) noexcept = default;
    constexpr verifier& operator=(verifier&&) noexcept = default;

    [[nodiscard]] constexpr const consensus_parameters& parameters() const noexcept;

    // --- Block header ---

    [[nodiscard]] verify_result<header_fact>
      operator()(const block_header& h) const;

    template<chain_view Chain>
    [[nodiscard]] verify_result<header_fact>
      operator()(const block_header& h, const Chain& chain,
                 std::chrono::sys_seconds now) const;

    // --- Transaction ---

    [[nodiscard]] verify_result<tx_fact>
      operator()(const transaction& tx) const;

    template<chain_view Chain>
    [[nodiscard]] verify_result<tx_fact>
      operator()(const transaction& tx, const Chain& chain) const;

    template<chain_view Chain, coin_index Coins>
    [[nodiscard]] verify_result<tx_fact>
      operator()(const transaction& tx, const Chain& chain,
                 const Coins& coins) const;

    // --- Block ---

    [[nodiscard]] verify_result<block_fact>
      operator()(const block& b) const;

    template<chain_view Chain>
    [[nodiscard]] verify_result<block_fact>
      operator()(const block& b, const Chain& chain,
                 std::chrono::sys_seconds now) const;

    template<chain_view Chain, coin_index Coins>
    [[nodiscard]] verify_result<block_fact>
      operator()(const block& b, const Chain& chain,
                 std::chrono::sys_seconds now,
                 const Coins& coins) const;

  private:
    const consensus_parameters* params_; // exposition only
  };

} // namespace bitcoin
```

`verifier::operator()` returns `verify_result<Fact>{fact}` when the verified
object satisfies all consensus rules evaluated by that overload. It returns
`verify_result<Fact>{status}` — where `status.ok()` is `false` — when the object
violates a consensus rule. If multiple rules are violated, the returned status
identifies one of the violated rules; the selection is unspecified.

`verifier::operator()` functions are not `noexcept`. Implementations may
propagate exceptions arising from ordinary execution, such as memory allocation
failure. An exception indicates a true failure, not a negative verification
result. A caller shall not interpret a propagated exception as a negative
verification outcome.

The contextual `verifier::operator()` overloads are constrained member
function templates. Chain evidence is accepted as any type that models
`chain_view` ([bitcoin.chain]); UTXO evidence is accepted as any type that
models `coin_index`. The standardized interface does not expose or specify any
type-erased adaptation mechanism.

Each overload evaluates the subset of Bitcoin consensus rules that can be
evaluated with the evidence it receives, parameterized by `*params_`. An
overload that accepts more parameters evaluates a superset of the rules
evaluated by an overload with fewer parameters. The `verification_status` value
returned on failure is implementation-defined. The precise definition of each
consensus rule is outside the scope of this paper.

An overload that takes no evidence parameter evaluates the *intrinsic* consensus
rules of the object — rules that can be checked from the object alone. An
overload that takes one or more evidence parameters evaluates a superset of the
intrinsic rules, incorporating *contextual* rules that require the supplied
evidence.

`verifier` is a non-owning reference to its `consensus_parameters`. The caller
is responsible for ensuring that the `consensus_parameters` object outlives the
`verifier`. Evidence arguments passed to `operator()` are borrowed only for the
duration of the call.

#### [bitcoin.validation.verifier.cons] Constructors

```cpp
constexpr verifier() noexcept;
```

*Effects:* Constructs a `verifier` whose `parameters()` function returns a
reference to a `consensus_parameters` object with default values (mainnet).

*Remarks:* The default `consensus_parameters` object referenced by the
default-constructed `verifier` has static storage duration.

```cpp
constexpr explicit verifier(const consensus_parameters& params) noexcept;
```

*Effects:* Stores `std::addressof(params)` in `*this`.

*Preconditions:* `params` outlives `*this`.

*Postconditions:* `parameters()` returns a reference to `params`.

#### [bitcoin.validation.verifier.obs] Observer

```cpp
[[nodiscard]] constexpr const consensus_parameters& parameters() const noexcept;
```

*Returns:* A reference to the consensus parameters referenced by `*this`.

#### [bitcoin.validation.verifier.header.intrinsic] `operator()(const block_header&)`

```cpp
[[nodiscard]] verify_result<header_fact>
  operator()(const block_header& h) const;
```

*Preconditions:* None.

*Returns:* `verify_result<header_fact>{header_fact{block_hash{h}}}` if `h`
satisfies all intrinsic header consensus rules; otherwise
`verify_result<header_fact>{status}`, where `status.ok()` is `false`.

*Remarks:* This overload evaluates only rules that can be checked against `h`
alone. It does not compare `h` against any ancestor chain.

#### [bitcoin.validation.verifier.header.contextual] `template<chain_view Chain> operator()(const block_header&, const Chain&, sys_seconds)`

```cpp
template<chain_view Chain>
[[nodiscard]] verify_result<header_fact>
  operator()(const block_header& h, const Chain& chain,
             std::chrono::sys_seconds now) const;
```

*Preconditions:* `chain` is non-empty and the hash of the last block header in
`chain` equals `h.prev_block_hash`.

*Returns:* `verify_result<header_fact>{header_fact{block_hash{h}}}` if `h`
satisfies all intrinsic and contextual header consensus rules; otherwise
`verify_result<header_fact>{status}`, where `status.ok()` is `false`.

*Remarks:* This overload evaluates a superset of the rules evaluated by
`operator()(const block_header&)`, incorporating rules that require the ancestor
chain and the current time.

#### [bitcoin.validation.verifier.tx.intrinsic] `operator()(const transaction&)`

```cpp
[[nodiscard]] verify_result<tx_fact>
  operator()(const transaction& tx) const;
```

*Preconditions:* None.

*Returns:*
`verify_result<tx_fact>{tx_fact{txid{tx}, /*fee*/, legacy_sigops, witness_sigops}}`
if `tx` satisfies all intrinsic transaction consensus rules; otherwise
`verify_result<tx_fact>{status}`, where `status.ok()` is `false`. The `fee`
member is zero for coinbase transactions.

#### [bitcoin.validation.verifier.tx.contextual1] `template<chain_view Chain> operator()(const transaction&, const Chain&)`

```cpp
template<chain_view Chain>
[[nodiscard]] verify_result<tx_fact>
  operator()(const transaction& tx, const Chain& chain) const;
```

*Preconditions:* `chain` is non-empty.

*Returns:* `verify_result<tx_fact>{tx_fact{...}}` if `tx` satisfies all
intrinsic and contextual transaction consensus rules given `chain`; otherwise
`verify_result<tx_fact>{status}`, where `status.ok()` is `false`.

*Remarks:* This overload evaluates a superset of the rules evaluated by
`operator()(const transaction&)`, incorporating rules that require the ancestor
chain.

#### [bitcoin.validation.verifier.tx.contextual2] `template<chain_view Chain, coin_index Coins> operator()(const transaction&, const Chain&, const Coins&)`

```cpp
template<chain_view Chain, coin_index Coins>
[[nodiscard]] verify_result<tx_fact>
  operator()(const transaction& tx, const Chain& chain,
             const Coins& coins) const;
```

*Preconditions:* `chain` is non-empty. `is_coinbase(tx)` is `false`.

*Returns:*
`verify_result<tx_fact>{tx_fact{txid{tx}, fee, legacy_sigops, witness_sigops}}`
if `tx` satisfies all intrinsic and contextual transaction consensus rules given
`chain` and `coins`; otherwise `verify_result<tx_fact>{status}`, where
`status.ok()` is `false`.

*Remarks:* This overload evaluates a superset of the rules evaluated by the
overload that accepts `tx` and `chain`, incorporating rules that require the
UTXO set via `coins`. The implementation invokes only `lookup` on `coins`.

#### [bitcoin.validation.verifier.block.intrinsic] `operator()(const block&)`

```cpp
[[nodiscard]] verify_result<block_fact>
  operator()(const block& b) const;
```

*Preconditions:* None.

*Returns:* `verify_result<block_fact>{block_fact{...}}` if `b` satisfies all
intrinsic block consensus rules; otherwise `verify_result<block_fact>{status}`,
where `status.ok()` is `false`. The `undo` member of the returned `block_fact`
is empty.

#### [bitcoin.validation.verifier.block.contextual1] `template<chain_view Chain> operator()(const block&, const Chain&, sys_seconds)`

```cpp
template<chain_view Chain>
[[nodiscard]] verify_result<block_fact>
  operator()(const block& b, const Chain& chain,
             std::chrono::sys_seconds now) const;
```

*Preconditions:* `chain` is non-empty and the hash of the last block header in
`chain` equals `b.header().prev_block_hash`.

*Returns:* `verify_result<block_fact>{block_fact{...}}` if `b` satisfies all
intrinsic and contextual block consensus rules given `chain` and `now`;
otherwise `verify_result<block_fact>{status}`, where `status.ok()` is `false`.
The `undo` member of the returned `block_fact` is empty.

*Remarks:* This overload evaluates a superset of the rules evaluated by
`operator()(const block&)`, incorporating header-contextual rules via
`operator()(b.header(), chain, now)` and per-transaction contextual rules. It
does not evaluate rules that require the UTXO set.

#### [bitcoin.validation.verifier.block.contextual2] `template<chain_view Chain, coin_index Coins> operator()(const block&, const Chain&, sys_seconds, const Coins&)`

```cpp
template<chain_view Chain, coin_index Coins>
[[nodiscard]] verify_result<block_fact>
  operator()(const block& b, const Chain& chain,
             std::chrono::sys_seconds now,
             const Coins& coins) const;
```

*Preconditions:* `chain` is non-empty and the hash of the last block header in
`chain` equals `b.header().prev_block_hash`.

*Returns:*
`verify_result<block_fact>{block_fact{block_hash{b}, total_fees, subsidy, total_legacy_sigops, total_witness_sigops, undo}}`
if `b` satisfies all intrinsic and contextual block consensus rules given
`chain`, `now`, and `coins`; otherwise `verify_result<block_fact>{status}`,
where `status.ok()` is `false`.

*Remarks:* This overload evaluates a superset of the rules evaluated by the
overload that accepts `b`, `chain`, and `now`, incorporating per-transaction
rules that require the UTXO set via `coins`. The `undo` member of the returned
`block_fact` is populated with the spent `coin` for each non-coinbase input in
`b`, in order.

### [bitcoin.validation.verify] Predefined `verify` objects

Predefined `inline constexpr` `verifier` objects are provided in
network-specific namespaces. Each object references a `consensus_parameters`
instance with static storage duration configured for its respective network.
Multiple copies of the same predefined `verify` object share the same
`consensus_parameters` instance.

```cpp
namespace bitcoin {

  inline constexpr verifier verify{};

} // namespace bitcoin
```

`bitcoin::verify` is a `verifier` whose `parameters()` returns a reference to a
`consensus_parameters` object with default values (mainnet). The referenced
`consensus_parameters` object has static storage duration.

```cpp
namespace bitcoin::testnet {

  inline constexpr consensus_parameters params{
    .proof_of_work_limit       = 0x1d00ffff,
    .retarget_interval         = 2016,
    .target_timespan_seconds   = 14 * 24 * 60 * 60,
    .initial_subsidy           = amount{50 * 100'000'000},
    .halving_interval          = 210'000,
    .coinbase_maturity         = 100,
    .max_block_weight          = 4'000'000,
    .max_block_serialized_size = 4'000'000,
    .max_block_sigops          = 80'000,
    .max_tx_weight             = 400'000,
    .max_tx_sigops             = 20'000,
    .locktime_enabled          = true,
    .relative_locktime_enabled = true,
    .bip34_enabled             = true,
    .bip34_activation_height   = 0,
    .segwit_enabled            = true,
    .segwit_activation_height  = 834'624,
  };

  inline constexpr verifier verify{params};

} // namespace bitcoin::testnet
```

```cpp
namespace bitcoin::signet {

  inline constexpr consensus_parameters params{
    .proof_of_work_limit       = 0x1e0377ae,
    .retarget_interval         = 2016,
    .target_timespan_seconds   = 14 * 24 * 60 * 60,
    .initial_subsidy           = amount{50 * 100'000'000},
    .halving_interval          = 210'000,
    .coinbase_maturity         = 100,
    .max_block_weight          = 4'000'000,
    .max_block_serialized_size = 4'000'000,
    .max_block_sigops          = 80'000,
    .max_tx_weight             = 400'000,
    .max_tx_sigops             = 20'000,
    .locktime_enabled          = true,
    .relative_locktime_enabled = true,
    .bip34_enabled             = true,
    .bip34_activation_height   = 0,
    .segwit_enabled            = true,
    .segwit_activation_height  = 0,
  };

  inline constexpr verifier verify{params};

} // namespace bitcoin::signet
```

```cpp
namespace bitcoin::regtest {

  inline constexpr consensus_parameters params{
    .proof_of_work_limit       = 0x207fffff,
    .retarget_interval         = std::numeric_limits<std::size_t>::max(),
    .target_timespan_seconds   = 14 * 24 * 60 * 60,
    .initial_subsidy           = amount{50 * 100'000'000},
    .halving_interval          = 150,
    .coinbase_maturity         = 100,
    .max_block_weight          = 4'000'000,
    .max_block_serialized_size = 4'000'000,
    .max_block_sigops          = 80'000,
    .max_tx_weight             = 400'000,
    .max_tx_sigops             = 20'000,
    .locktime_enabled          = true,
    .relative_locktime_enabled = true,
    .bip34_enabled             = true,
    .bip34_activation_height   = 0,
    .segwit_enabled            = true,
    .segwit_activation_height  = 0,
  };

  inline constexpr verifier verify{params};

} // namespace bitcoin::regtest
```

[The parameter values shown for testnet, signet, and regtest are illustrative.
A production paper would verify each value against the respective network's
consensus rules and may include additional parameters not shown here.]{.ednote}

A program calls `verify` as if it were a function:

```cpp
auto result = bitcoin::verify(b, chain, now, coins);          // mainnet
auto result = bitcoin::testnet::verify(b, chain, now, coins); // testnet
auto result = bitcoin::regtest::verify(h);                    // regtest
```

A program may also construct a `verifier` with a custom `consensus_parameters`
for networks not predefined by the standard:

```cpp
inline constexpr bitcoin::consensus_parameters my_params{
  .proof_of_work_limit = /* custom */,
  .halving_interval    = /* custom */,
  // ...
};

bitcoin::verifier my_net{my_params};
auto result = my_net(b, chain, now, coins);
```

Because `verifier` holds a non-owning pointer, the `consensus_parameters` object
must outlive the `verifier`. For `inline constexpr` objects and objects with
static storage duration, this is automatic. For local `consensus_parameters`
objects, the caller is responsible for lifetime management. The `chain` and
`coins` arguments supplied to `operator()` are borrowed only for the duration of
the call.
