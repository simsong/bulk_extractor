# Cryptocurrency scanning design

Tracking issue: [#651](https://github.com/simsong/bulk_extractor/issues/651).

This is the design specification for cryptocurrency evidence discovery in
bulk_extractor. It intentionally does not implement a scanner, decrypt a
wallet, recover a password, or make a network request.

## Goals

The feature must distinguish public addresses and public keys from
secret-bearing material, discover structured wallet containers, state their
encryption status explicitly, and avoid false claims about ownership or
validity. It is offline-first and deterministic: a normal image scan never
contacts an RPC node or third-party API.

Initial address families are Bitcoin (BTC), Ethereum-compatible networks
(ETH, BSC, Avalanche C-Chain, and EVM tokens), XRP, and Solana. Follow-up
families are DOGE/LTC, BCH, TRON, XLM, ADA, AVAX X/P, TON, HBAR, SUI, and XMR.

## Components

```
scan_crypto.flex           bounded text candidates
      |                         |
      v                         v
crypto_validate.cpp      scan_crypto_wallet.cpp
address/key validation  structured container recognition
      \\                         /
       \\                       /
        crypto address, sensitive, and wallet feature records
                                  |
                                  v
                    optional python/crypto_enrich.py
```

### Flex text scanner

`src/scan_crypto.flex` follows the existing `scan_accts.flex` design:
Flex performs inexpensive bounded candidate discovery and C++ actions call a
validator before writing a feature. It uses `sbuf_flex_scanner.h`, initializes
its feature definitions in `PHASE_INIT`, obtains recorders in `PHASE_INIT2`,
and has no mutable state during concurrent `PHASE_SCAN` calls.

### Validation library

`src/crypto_validate.{h,cpp}` provides typed, pure validators for Base58Check,
Bech32/Bech32m, CashAddr, StrKey/CRC, hex, EIP-55, and family-specific
versions/networks. It returns a structured result rather than a Boolean:

`crypto_kind` describes unstructured address and key candidates only. Wallet
containers are parsed separately by `scan_crypto_wallet.cpp` and written to
`crypto_wallets.txt`; they do not use `crypto_match.kind`.

```cpp
enum class crypto_kind {
    address, public_key, private_key, seed_phrase,
    encrypted_wallet, ambiguous
};
enum class crypto_confidence { low, high };

struct crypto_match {
    crypto_kind kind;
    crypto_confidence confidence;
    std::string family;
    std::string network;
    std::string encoding;
    bool checksum_valid;
    bool encrypted;
    bool remote_lookup_allowed;
};
```

### Wallet-container scanner

`src/scan_crypto_wallet.cpp` recognizes and parses structured artifacts:
PKCS#8 encrypted keys, Ethereum V3 keystores, Bitcoin wallet databases,
wallet manifests, and supported application vault formats. A filename,
high-entropy blob, or generic ciphertext is never enough to call something a
wallet. Each adapter needs a structural signature, bounded parser, and
format-specific encryption evidence.

Product-specific adapters may be added only after fixtures for that product's
actual format exist. The test-envelope vectors described below define the
output contract; they do not claim that a synthetic envelope is a real wallet
format.

## Finding classes and sensitive-data policy

| Class | Meaning | May be enriched online |
| --- | --- | --- |
| `address` | Validated chain/network public address. | Only if high confidence. |
| `public_key` | Recognized serialized public key, not an address. | No. |
| `private_key` | Recognized plaintext private-key serialization. | Never. |
| `seed_phrase` | Recovery phrase or HD seed material. | Never. |
| `encrypted_wallet` | Parsed encrypted key, keystore, or vault. | Never. |
| `wallet_container` | Parsed wallet with no private material exposed. | Never. |
| `ambiguous` | Plausible shape without sufficient validation/context. | No. |

Secret-bearing values are high-sensitivity evidence. The normal report stores
only location, type, encryption status, and a per-run keyed fingerprint. It
does not store a raw private key, mnemonic, decrypted value, password, or API
credential. Normal operation never decrypts a wallet.

## Outputs

### crypto_addresses.txt

This file contains public identifiers only.

```text
# path offset length family network kind encoding checksum confidence value
image.dd 1048576 34 bitcoin mainnet address base58check valid high 1AGNa15ZQXAZUgFiqJ2i7Z2DPU2J6hW62i
image.dd 2097152 42 evm unknown address eip55 valid high 0x52908400098527886E0F7030069857D2E4169EE7
image.dd 3145728 44 solana mainnet public_key base58 n/a high Vote111111111111111111111111111111111111111
```

### crypto_sensitive.txt

This opt-in, redacted file records material that must not be sent to a remote
service.

```text
# path offset length kind family algorithm encoding encrypted confidence fingerprint
image.dd 4194304 241 encrypted_wallet unknown secp256k1 pkcs8-pem yes high HMAC-SHA256:...
image.dd 5242880 52 private_key bitcoin secp256k1 wif no high HMAC-SHA256:...
```

### crypto_wallets.txt

This is the wallet-discovery output. Encryption is explicit and never inferred
from a filename or entropy alone.

```text
# wallet_id path offset length wallet_format product parse_state container_encryption sensitive_encryption currency_scope currencies key_material confidence
wallet:... image.dd 0 241 pkcs8-pem unknown parsed encrypted all_detected_sensitive_material_encrypted single unknown encrypted_private_key high
wallet:... image.dd 0 512 ethereum-v3-json unknown parsed encrypted all_detected_sensitive_material_encrypted single EVM encrypted_private_key high
wallet:... image.dd 0 1024 application-vault ExampleWallet parsed encrypted unknown multi BTC,ETH,SOL encrypted_vault high
```

The fields use these values:

| Field | Values |
| --- | --- |
| `parse_state` | `parsed`, `candidate`, `corrupt`, `unsupported` |
| `container_encryption` | `encrypted`, `not_encrypted`, `mixed`, `not_a_container`, `unknown` |
| `sensitive_encryption` | `all_detected_sensitive_material_encrypted`, `some_sensitive_material_encrypted`, `not_encrypted`, `not_inspected`, `unknown` |
| `currency_scope` | `single`, `multi`, `multi_capable`, `unknown` |
| `key_material` | `none`, `public_only`, `encrypted_private_key`, `private_key`, `seed_phrase`, `encrypted_vault`, `unknown` |

An encrypted wallet often leaves public addresses and account metadata
plaintext. Therefore `container_encryption=encrypted` and
`sensitive_encryption=all_detected_sensitive_material_encrypted` are
different statements. The latter is used only when the parser has inspected
every recognized sensitive field. Otherwise it is `unknown`.

## Multi-currency wallets

A multi-currency result needs evidence:

| Result | Meaning |
| --- | --- |
| `single` | Parsed data identifies one chain/network. |
| `multi` | Parsed account/asset metadata identifies two or more chains. |
| `multi_capable` | An HD seed or equivalent could derive several chains, but data does not prove installed accounts/assets. |
| `unknown` | No justified currency conclusion. |

A BIP-39 phrase or BIP-32 extended key is not automatically a discovered
multi-currency wallet; it is secret material that may be multi-currency-capable.

## Confidence modes

```text
-S crypto_mode=conservative    # default
-S crypto_mode=discovery
```

| Mode | Address policy | Wallet policy |
| --- | --- | --- |
| `conservative` | Only validated checksum/version matches. All-lower EVM and Solana-shaped Base58 require strong context. | Only structurally parsed containers with format-based encryption evidence. |
| `discovery` | Adds contextual candidates marked `low`. | Adds candidate artifacts from path/schema/magic evidence, always `parse_state=candidate`; encryption remains `unknown` unless proven. |

Base58Check generally offers about 32 checksum bits and Bech32-family encodings
about 30. These conditional figures are not corpus-wide false-positive rates.
The implementation must measure candidates per GiB by confidence tier against
labelled image corpora.

## Currency-family implementation matrix

Market-cap ordering changes, and stablecoins/tokens use their host-chain
address format. The table therefore groups implementation work by address
family. Effort is one engineer's implementation, documented vectors, and
substantive tests; it is not image-scan time. Add roughly one to three days
for each separately supported RPC/indexer adapter.

| Currency/family | Local validation | Wallet/key considerations | Initial effort |
| --- | --- | --- | --- |
| Bitcoin (BTC) | Base58Check plus Bech32/Bech32m and mainnet versions | secp256k1; WIF, BIP-32, BIP-39, wallet databases | 3-5 days |
| Ethereum (ETH) | 20-byte hex; EIP-55 when mixed case | secp256k1; JSON keystores; EVM address does not identify chain | 2-3 days |
| USDT, USDC, LINK, stETH | Host-chain address, not token-specific text | token contract/mint is required to enrich holdings | low increment after host chain |
| BNB Smart Chain, Hyperliquid EVM, Avalanche C | EVM family | same EVM key handling; chain ID distinguishes network | low increment after EVM |
| XRP | XRPL Base58 checksum; classic and X-address forms | secp256k1/Ed25519 seed/key formats; destination tags matter | 1-2 days |
| Solana (SOL) | Base58 32-byte public-key shape; conservative context policy | Ed25519; a raw 32-byte value may be public key or seed | 2-3 days |
| TRON (TRX) | Base58Check with TRON version | secp256k1/EVM-related key material | 1-2 days |
| DOGE, LTC | Bitcoin-family version rules | secp256k1, WIF, HD-wallet support | low increment after BTC |
| Bitcoin Cash (BCH) | CashAddr plus optional legacy Base58 | secp256k1 | 1-2 days |
| Cardano (ADA) | Bech32 header/network/payload rules | Ed25519 payment and stake material | 3-5 days |
| Stellar (XLM) | StrKey/Base32 CRC; public and muxed forms | Ed25519 public/secret encodings | 1-2 days |
| Avalanche X/P | chain-specific Bech32-like forms | secp256k1; distinguish from C-chain EVM | 2-3 days |
| Sui (SUI) | 32-byte hex with strict context | scheme-tagged key material | 1-2 days plus policy |
| Hedera (HBAR) | numeric account IDs and EVM aliases | Ed25519/ECDSA keys; numeric form is collision-prone | 2-3 days |
| TON (TON) | friendly/raw address flags and checksum | Ed25519 keys and contract-derived account addresses | 2-3 days |
| Monero (XMR) | Base58 blocks, network/type bytes, checksum | private spend/view keys and mnemonic; public lookup is limited | 3-5 days |

Address validation is not private-key validation. A private key cannot feasibly
be derived from an address or public key. Private-key recognition is instead a
format/context classification problem and is always high-sensitivity output.

## Optional online enrichment

Online verification belongs in a separate Python command, not the C++
executable:

```text
python/crypto_enrich.py --input report/crypto_addresses.txt \\
  --output report/crypto_enrichment.jsonl --provider-config case-rpc.toml \\
  --minimum-confidence high --lookup-budget 10000
```

The tool uses Pydantic models for configuration, input records, provider
responses, cache entries, and output records. It deduplicates, caches by
chain/network/address/query-kind/block-height, rate-limits, and records
provider/local-node identity and timestamp.

Only high-confidence `address` records are accepted. It rejects public keys,
private keys, seed phrases, encrypted wallets, wallet containers, and ambiguous
values. Results are `observed`, `not_observed`, or `unknown`; neither of
the latter two means invalid. Local nodes/indexers are preferred for sensitive
cases.

Python preserves forensic reproducibility, keeps API credentials and HTTP
dependencies outside the core scanner, and prevents remote failures from
blocking image scanning.

## Test vectors

The test corpus is under `tests/Data`:

| Path | Purpose |
| --- | --- |
| `crypto-address-vectors.txt` | Public address, checksum, and ambiguity examples. |
| `crypto-public-key-samples.pem` | Generated secp256k1 and Ed25519 public keys. |
| `crypto-encrypted-test-keys.pem` | Generated encrypted PKCS#8 private-key fixtures. |
| `crypto-wallet-vectors/plaintext-single-wallet.json` | Test-envelope single-currency wallet with generated plaintext private material. |
| `crypto-wallet-vectors/encrypted-multicurrency-wallet.json` | Test-envelope encrypted multi-currency wallet. |
| `crypto-wallet-vectors/encrypted-single-wallet.pem` | PKCS#8 encrypted single-key wallet material. |
| `crypto-wallet-vectors/unknown-ciphertext.txt` | Negative control: ciphertext alone is not a wallet. |

All private material is deliberately generated test material with no funds or
production use. The only fixture password is literally `password`; it must
never be reused. The plaintext-wallet fixture exists specifically to verify an
explicit `not_encrypted` classification and must never be included in normal
reports.

### Fixture-generation commands

The generated key pairs were created in a disposable directory. Re-running
these random commands creates different valid fixtures; it does not reproduce
the committed bytes.

```sh
# secp256k1 public and encrypted PKCS#8 fixture
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:secp256k1 \\
  -out secp256k1-private.pem
openssl pkey -in secp256k1-private.pem -pubout \\
  -out secp256k1-public.pem
openssl pkey -in secp256k1-private.pem -aes-256-cbc \\
  -passout pass:password -out secp256k1-encrypted-private.pem

# Ed25519 public and encrypted PKCS#8 fixture
openssl genpkey -algorithm ED25519 -out ed25519-private.pem
openssl pkey -in ed25519-private.pem -pubout -out ed25519-public.pem
openssl pkey -in ed25519-private.pem -aes-256-cbc \\
  -passout pass:password -out ed25519-encrypted-private.pem

# Encrypt the exact JSON payload used by the multi-currency envelope.
openssl enc -aes-256-cbc -pbkdf2 -salt -pass pass:password -a \\
  -in multi-currency-wallet-plaintext.json \\
  -out multi-currency-wallet-ciphertext.txt
```

The first two encrypted outputs are concatenated in
`crypto-encrypted-test-keys.pem`. The last ciphertext is embedded in
`encrypted-multicurrency-wallet.json`; the envelope records its algorithm,
KDF, and test password. Delete plaintext intermediate files after fixture
creation. The wallet-vector README identifies which fixtures are test-envelope
contracts rather than product-specific wallet formats.

## Implementation order

1. Add the Flex scanner, typed validation library, and conservative BTC/EVM/XRP/SOL rules.
2. Add redacted sensitive-material classification.
3. Add the generic wallet-container interface and PKCS#8/Ethereum V3 adapters.
4. Add wallet fixtures produced by supported real applications before adding a product adapter.
5. Add the Python enricher for local-node and explicitly configured provider use.
6. Measure confidence-tier false positives before enabling discovery mode broadly.
