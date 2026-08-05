# Cryptocurrency scanner proposal

Tracking issue: #651. This proposal is offline-first; it does not add a scanner implementation or make network requests.

## Scope

The scanner finds high-confidence cryptocurrency addresses and records why each one matched. It recognises public keys and secret-bearing material as distinct evidence classes. It must not query an address, public key, private key, seed phrase, encrypted wallet, or ambiguous candidate while scanning.

The initial target is Bitcoin (BTC), Ethereum-compatible networks (ETH, BSC, Avalanche C-Chain, and EVM tokens), XRP, and Solana. Later adapters can add BCH, DOGE/LTC, Cardano, Stellar, TRON, Avalanche X/P, TON, Hedera, Sui, and Monero.

This scanner is not a wallet-recovery tool. It does not derive a private key from an address, attempt passwords, or report a network lookup as proof of ownership.

## Finding classes

| Class | Meaning | Eligible for optional enrichment |
| --- | --- | --- |
| address | A chain/network public address passed validation rules. | Yes, if high confidence. |
| public_key | A recognised serialized public key, not an address. | No. |
| private_key | A recognised private-key serialization. | Never. |
| seed_phrase | A recovery phrase. | Never. |
| encrypted_wallet | A recognised encrypted key or wallet container. | Never. |
| ambiguous | Plausible text lacking validation/context. | No. |

Secret-bearing values are high-sensitivity evidence. Normal reports should redact their values and retain only location, type, and a keyed evidence identifier. The scanner must not log the value, recurse into it, or make it available to a remote service.

## Architecture

The scanner tokenizes once per PHASE_SCAN buffer, dispatches candidates to local validators, and writes normalized findings through a feature recorder. Validators are pure and immutable after PHASE_INIT2, so concurrent PHASE_SCAN calls need no mutable global state. It must follow doc/scanner_api.md: define the recorder in PHASE_INIT, acquire it in PHASE_INIT2, keep sp.sbuf read-only, and retain no pointer into it after the callback.

    tokenizer and context scorer
              |
    family registry: prefixes, lengths, networks, confidence policy
              |
    shared codecs: Base58Check, Bech32/Bech32m, CashAddr, Base32/CRC, hex, EIP-55
              |
    family adapters: BTC, EVM, XRP, SOL, ...
              |
    normalized feature record

BTC, DOGE, LTC, and legacy BCH share Base58Check framing; BCH adds CashAddr. BTC Bech32/Bech32m, Cardano, and Avalanche X/P share Bech32 primitives but own their human-readable prefixes, versions, and payload rules. ETH, BSC, Avalanche C-Chain, Hyperliquid EVM, and EVM-hosted tokens share 20-byte hex parsing and EIP-55, while the adapter supplies chain ID and token contract/mint. XRP and TRON may reuse Base58 machinery but not alphabets or version bytes. Solana and Monero may reuse only Base58 decoding.

## Validation and confidence

An address finding records family, network, encoding, checksum_status, context_status, and confidence. A full checksum plus network/version match is high confidence. Context is required for all-lowercase EVM addresses and raw Sui hex. A generic hex regex is not sufficient.

Common Base58Check forms have conditional random acceptance near one in 2^32, and Bech32/Bech32m/CashAddr near one in 2^30. EIP-55 is useful only when mixed case is present. Solana 32-byte Base58 keys and unchecksummed hex need conservative boundaries/context. These are not corpus-wide rates; measure candidates per GiB by confidence tier against labelled images.

## Optional post-scan enrichment

Enrichment is a separate command or bounded worker pool after scanning. It deduplicates high-confidence addresses, caches by chain, network, address, query kind, and block height/time, rate-limits, and records provider/local-node identity and errors. It reports observed, not_observed, or unknown; neither of the latter two means invalid. A locally operated node/indexer is preferred for sensitive cases.

## Test corpus

tests/Data/crypto-public-key-samples.pem contains deliberately generated secp256k1 and Ed25519 public keys. tests/Data/crypto-encrypted-test-keys.pem contains only their encrypted private-key fixtures. They are non-production test material; the password is literally password and must never be reused. tests/Data/crypto-address-vectors.txt supplies public address examples and negative boundary/checksum cases.

The encrypted fixtures exist only to test classification as encrypted_wallet. No normal scan test may decrypt them.

