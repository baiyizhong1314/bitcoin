Omni Core (Beta) address-indexed
================================

Modifications
-------------

This branch is based on the [mscore-0.0.9](https://github.com/dexX7/bitcoin/tree/mscore-0.0.9) ([diff](https://github.com/mastercoin-MSC/mastercore/compare/mscore-0.0.9...dexX7:addrindex-mscore-0.0.9)) branch of Omni Core and includes the required changes to allow extended transaction lookups based on an address.

The extensions are available for different bases:

- [Bitcoin Core 0.9](https://github.com/dexX7/bitcoin/tree/addrindex-0.9) ([diff](https://github.com/bitcoin/bitcoin/compare/0.9...dexX7:addrindex-0.9))
- [Bitcoin Core 0.10](https://github.com/dexX7/bitcoin/tree/addrindex-0.10) ([diff](https://github.com/bitcoin/bitcoin/compare/0.10...dexX7:addrindex-0.10))
- [Bitcoin Core 0.11](https://github.com/dexX7/bitcoin/tree/addrindex-0.11) ([diff](https://github.com/bitcoin/bitcoin/compare/0.11...dexX7:addrindex-0.11))
- [Omni Core 0.0.10](https://github.com/dexX7/bitcoin/tree/addrindex-omnicore-0.0.10) ([diff](https://github.com/OmniLayer/omnicore/compare/omnicore-0.0.10...dexX7:addrindex-omnicore-0.0.10))

### Setup and configuration

Use `-addrindex=1` to enable address-based indexing of transactions.
Reindexing via `-reindex` is required the first time.

### RPC commands

The following new RPC commands are available:

```
> searchrawtransactions "address" (verbose skip count includeorphans)

Description:
Returns an array of all confirmed transactions associated with address.

Note: as per default, orphaned transactions, which are not part of the active chain, are 
included in the results.

Arguments:
1. address          (string, required) The Bitcoin address
2. verbose          (numeric, optional, default=1) If 0, return only transaction hex
3. skip             (numeric, optional, default=0) The number of transactions to skip
4. count            (numeric, optional, default=100) The number of transactions to return
5. includeorphans   (numeric, optional, default=1) If 0, exclude orphaned transactions
```

```
> listallunspent "address" ( verbose minconf maxconf maxreqsigs )

Description:
Returns an array of confirmed, unspent transaction outputs with between minconf and maxconf 
(inclusive) confirmations, spendable by the provided address, whereby maximal maxreqsigs 
signatures are required to redeem the output.

Arguments:
1. address          (string, required) The Bitcoin address
2. verbose          (numeric, optional, default=0) If 0, exclude reqSigs, addresses, scriptPubKey (asm, hex), blockhash, blocktime, blockheight
3. minconf          (numeric, optional, default=1) The minimum confirmations to filter.
4. maxconf          (numeric, optional, default=9999999) The maximum confirmations to filter
5. maxreqsigs       (numeric, optional, default=1) The number of signatures required to spend the output
```

```
> getallbalance "address" ( minconf maxreqsigs )

Description:
Returns the sum of confirmed, spendable transaction outputs by address with at least minconf 
confirmations, whereby maximal maxreqsigs signatures are allowed to be required to redeem an 
output.

Arguments:
1. address          (string, required) The Bitcoin address
2. minconf          (numeric, optional, default=1) The minimum confirmations to filter
3. maxreqsigs       (numeric, optional, default=1) The number of signatures required to spend an output
```

```
> gettxposition "txid"

Description:
Returns information related to the position of transaction.

Arguments:
1. txid          (string, required) The transaction id

Result:
{
  "txid" : "hash",        (string) The transaction id (same as provided)
  "blockhash" : "hash",   (string) The block hash
  "blockheight" : n,      (numeric) The block height (if orphaned: -1, unconfirmed: 0)
  "position" : n          (numeric) The position of transaction within block (if unconfirmed: -1)
}
```

Copyright (c) 2013-2015 Omni Core Developers

Copyright (c) 2009-2015 Bitcoin Core Developers

What is the Omni Layer
----------------------
The Omni Layer is a communications protocol that uses the Bitcoin block chain to enable features such as smart contracts, user currencies and decentralized peer-to-peer exchanges. A common analogy that is used to describe the relation of the Omni Layer to Bitcoin is that of HTTP to TCP/IP: HTTP, like the Omni Layer, is the application layer to the more fundamental transport and internet layer of TCP/IP, like Bitcoin.

http://www.omnilayer.org

What is Omni Core
-----------------

Omni Core is a fast, portable Omni Layer implementation that is based off the Bitcoin Core codebase (currently 0.9.5). This implementation requires no external dependencies extraneous to Bitcoin Core, and is native to the Bitcoin network just like other Bitcoin nodes. It currently supports a wallet mode and it will be seamlessly available on 3 platforms: Windows, Linux and Mac OS. Omni Layer extensions are exposed via the UI and the JSON-RPC interface. Development has been consolidated on the Omni Core product, and once officially released it will become the reference client for the Omni Layer.

Disclaimer, warning
-------------------
This software is EXPERIMENTAL software. USE ON MAINNET AT YOUR OWN RISK.

By default this software will use your existing Bitcoin wallet, including spending bitcoins contained therein (for example for transaction fees or trading).
The protocol and transaction processing rules for the Omni Layer are still under active development and are subject to change in future.
Omni Core should be considered an alpha-level product, and you use it at your own risk. Neither the Omni Foundation nor the Omni Core developers assumes any responsibility for funds misplaced, mishandled, lost, or misallocated.

Further, please note that this installation of Omni Core should be viewed as EXPERIMENTAL. Your wallet data, bitcoins and Omni Layer tokens may be lost, deleted, or corrupted, with or without warning due to bugs or glitches. Please take caution.

This software is provided open-source at no cost. You are responsible for knowing the law in your country and determining if your use of this software contravenes any local laws.

PLEASE DO NOT use wallet(s) with significant amounts of bitcoins or Omni Layer tokens while testing!

Testnet
-------------------

Testnet mode allows Omni Core to be run on the Bitcoin Testnet blockchain for safe testing.
1. To run Omni Core in testnet mode, run Omni Core with the following option in place: ``` -testnet ```.
2. To receive MSC (and TMSC) on testnet please send TBTC to moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP. For each 1 TBTC you will receive 100 MSC and 100 TMSC.


Dependencies
------------
Boost >= 1.53

Installation
------------

*NOTE: This will only build on Ubuntu Linux for now.*

You will need appropriate libraries to run Omni Core on Unix, 
please see [doc/build-unix.md](doc/build-unix.md) for the full listing.

You will need to install git & pkg-config:

```
sudo apt-get install git
sudo apt-get install pkg-config
```

Clone the Omni Core repository:

```
 git clone https://github.com/mastercoin-MSC/mastercore.git
 cd mastercore/
```

Then, run:

```
./autogen.sh
./configure
make
```
Once complete:

```
cd src/
```
and start Omni Core using ```./mastercored``` (or ```./qt/mastercore-qt``` if built with UI). The inital parse step for a first time run
will take up to 60 minutes or more, during this time your client will scan the blockchain for Omni Layer transactions. You can view the 
output of the parsing at any time by viewing the log located in your datadir, by default: ```~/.bitcoin/mastercore.log```.

Omni Core requires the transaction index to be enabled.   Add an entry to your bitcoin.conf file for ```txindex=1``` to enable it or Omni Core will refuse to start.

If a message is returned asking you to reindex, pass the ```-reindex``` flag to mastercored. The reindexing process can take serveral hours.

To issue RPC commands to Master Core you may add the '-server=1' CLI flag or add an entry to the bitcoin.conf file (located in '~/.bitcoin/' by default).

In bitcoin.conf:
```
server=1
```

After this step completes, check that the installation went smoothly by issuing the following command ```./mastercore-cli getinfo_MP``` which should return the 'mastercoreversion' as well as some
additional information related to the client.

The documentation for the RPC interface and command-line is located in [doc/apidocumentation.md] (doc/apidocumentation.md).

Current feature set:
--------------------

* Broadcasting of simple send (tx0), and send to owners (tx3) [doc] (doc/apidocumentation.md#broadcasting-a-simple-send-transaction)

* Obtaining a Master Protocol balance [doc] (doc/apidocumentation.md#obtaining-a-master-protocol-balance)

* Obtaining all balances (including smart property) for an address [doc] (doc/apidocumentation.md#obtaining-all-master-protocol-balances-for-an-address)

* Obtaining all balances associated with a specific smart property [doc] (doc/apidocumentation.md#obtaining-all-master-protocol-balances-for-a-property-id)

* Retrieving information about any Master Protocol transaction [doc] (doc/apidocumentation.md#retrieving-a-master-protocol-transaction)

* Listing historical transactions of addresses in the wallet [doc] (doc/apidocumentation.md#listing-historical-transactions)                            

* Retreiving detailed information about a smart property [doc] (doc/apidocumentation.md#retrieving-information-about-a-master-protocol-property)

* Retreiving active and expired crowdsale information [doc] (doc/apidocumentation.md#retrieving-information-for-a-master-protocol-crowdsale)

* Sending a specific BTC amount to a receiver with referenceamount in send_MP

* Creating and broadcasting transactions based on raw Master Protocol data with sendrawtx_MP

* Functional UI for balances, sending and historical transactions

Pending additions:
-------------------

* Meta-DEx support

* DEx support (making offer, making accept, making payment)

* Crowdsales (issuing smart properties, fundraisers, changing currency, closing fundraisers)

* gettransaction_MP output should include matched sell offer transaction reference

Support:
------------------

* Open a [GitHub issue] (https://github.com/mastercoin-MSC/mastercore/issues) to file a bug submission.
