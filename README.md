# Mycryptobank presale contract
## Overview
This smart contract is designed to issue tokens on a pre-sale round.

## Code
**transfer**
```cs
void transfer( account_name from, account_name to, asset quantity )
```
Allows holdes to transfer tokens.

**get_balance**
```cs
asset get_balance(account_name owner) const 
```
Get balance.

**mint**
```cs
void mint( account_name to, asset quantity)
```
Mints tokens.

**burnTokens**
```cs
void finish_minting()
```
Finish minting and unfreeze token transfers.
