# std-bitcoin

API specification papers for Bitcoin in C++, written in the style of WG21
standardization proposals.

______________________________________________________________________

## What this is

These papers define a **shared C++ API** for Bitcoin. The goal is a standardized
API for a **stateless, side-effect-free consensus validation library** — not a
submission to the ISO C++ committee.

The WG21 paper format is used deliberately: it is a precise, well-understood way
to specify a C++ API contract. Writing in this style forces every design
decision to be stated explicitly, keeps the specification implementer-neutral,
and makes the API easy to review, version, and extend. If the API ever gained
broad adoption, a path toward standardization would be straightforward. For now,
think of the format as a rigorous shared specification, not a standards-track
document.

> **These papers have not been submitted to the ISO C++ committee and are not
> under active consideration for standardization.**

______________________________________________________________________

## The problem

Bitcoin consensus is one of the world's most valuable pieces of software, yet it
remains tightly coupled to a single node implementation. Validation logic,
networking, storage, concurrency, and node management have evolved together for
more than fifteen years into a large and complex system, but not into a reusable
one.

This project specifies the reusable parts: vocabulary types, customization
points, serialization and deserialization, script helpers, and stateless,
side-effect-free consensus validation.

______________________________________________________________________

## Target implementations

The API specified here is meant to admit multiple conforming implementations and
to be useful to projects including:

- **[Bitcoin Core](https://github.com/bitcoin/bitcoin)** — the reference
  implementation of the Bitcoin protocol.
- **[Hornet](https://github.com/tobysharp/hornet)** — a minimal, executable
  specification of Bitcoin's consensus rules in modern declarative C++,
  including a lightweight full node with its own concurrent validation pipeline.
- **[libbitcoin-system](https://github.com/libbitcoin/libbitcoin-system)** — a
  cross-platform C++ development toolkit for Bitcoin applications.

______________________________________________________________________

## Why several papers instead of one?

The specification is split so that implementations can adopt it incrementally. A
library may implement only one paper and rely on other libraries for the
facilities specified by the others. For example, one implementation might
provide vocabulary types and serialization support, while another provides
validation.

At the same time, a library that implements more than one paper can exploit
cross-cutting implementation details that are not visible in the public API. For
example, it may precompute observational queries such as
`has_witness(transaction)` when constructing a transaction value, while still
presenting the same API.

This split keeps the specification modular without preventing efficient,
integrated implementations.

______________________________________________________________________

## Papers

### Active

- Bitcoin Vocabulary Types (
  [html](https://purplekarrot.github.io/std-bitcoin/VOCABULARY.html),
  [pdf](https://purplekarrot.github.io/std-bitcoin/VOCABULARY.pdf))
- Bitcoin Customization Point Objects (
  [html](https://purplekarrot.github.io/std-bitcoin/CPO.html),
  [pdf](https://purplekarrot.github.io/std-bitcoin/CPO.pdf))
- Bitcoin Wire Formats: Parsing and Serialization (
  [html](https://purplekarrot.github.io/std-bitcoin/SERDES.html),
  [pdf](https://purplekarrot.github.io/std-bitcoin/SERDES.pdf))
- Bitcoin Script Extensions (
  [html](https://purplekarrot.github.io/std-bitcoin/SCRIPT.html),
  [pdf](https://purplekarrot.github.io/std-bitcoin/SCRIPT.pdf))
- Bitcoin Consensus Validation (
  [html](https://purplekarrot.github.io/std-bitcoin/VALIDATION.html),
  [pdf](https://purplekarrot.github.io/std-bitcoin/VALIDATION.pdf))

### Retired

- Retired: Bitcoin Protocol Predicates (
  [html](https://purplekarrot.github.io/std-bitcoin/PREDICATES.html),
  [pdf](https://purplekarrot.github.io/std-bitcoin/PREDICATES.pdf))
- Retired: Bitcoin Chains (
  [html](https://purplekarrot.github.io/std-bitcoin/CHAIN.html),
  [pdf](https://purplekarrot.github.io/std-bitcoin/CHAIN.pdf))

______________________________________________________________________

## Contributing

Feedback, corrections, and alternative design proposals are welcome as issues or
pull requests. The most useful contributions are:

- Concrete objections to a design decision with an alternative
- Missing types or functions that the target implementations already model
- Inconsistencies with existing WG21 conventions or with Bitcoin protocol
  specifications
