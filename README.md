# std-bitcoin

API specification papers for Bitcoin in C++, written in the style of WG21
standardization proposals.

---

## What this is

These papers define a **shared C++ API** for Bitcoin. The immediate goal is
convergence across independent C++ Bitcoin implementations — not a submission
to the ISO C++ committee.

The WG21 paper format is used deliberately: it is a precise, well-understood
way to specify a C++ API contract. Writing in this style forces every design
decision to be stated explicitly, keeps the specification implementer-neutral,
and makes the API easy to review, version, and extend. If the API ever gained
broad adoption, a path toward standardization would be straightforward. For
now, think of the format as a rigorous shared specification, not a
standards-track document.

> **These papers have not been submitted to the ISO C++ committee and are not
> under active consideration for standardization.**

---

## The problem

Every major C++ Bitcoin library independently defines the same concepts —
hashes, transactions, outputs, amounts, scripts — using structurally identical
yet source-incompatible types. Any code that needs to work across library
boundaries must introduce conversion layers and accept the impedance-mismatch
bugs that come with them.

The situation mirrors the pre-`<chrono>` world, where every project had its own
duration type. A common vocabulary fixes the same class of problem.

---

## Target implementations

The API specified here is intended to be implementable by — and useful to —
projects including:

- **[Bitcoin Core](https://github.com/bitcoin/bitcoin)** — the reference
  implementation of the Bitcoin protocol.
- **[Hornet](https://github.com/tobysharp/hornet)** — a minimal, executable
  specification of Bitcoin's consensus rules in modern declarative C++,
  including a lightweight full node with its own concurrent validation pipeline.
- **[libbitcoin-system](https://github.com/libbitcoin/libbitcoin-system)** — a
  cross-platform C++ development toolkit for Bitcoin applications.

---

## Papers

Papers are numbered and prefixed in reading order. Each is a self-contained
specification; later papers may depend on earlier ones but never modify them.

---

## Contributing

Feedback, corrections, and alternative design proposals are welcome as issues
or pull requests. The most useful contributions are:

- Concrete objections to a design decision with an alternative
- Missing types or functions that the target implementations already model
- Inconsistencies with existing WG21 conventions or with Bitcoin protocol
  specifications
