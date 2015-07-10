#!/bin/bash

# Get BUILDDIR and REAL_BITCOIND
CURDIR=$(cd $(dirname "$0"); pwd)
. "${CURDIR}/tests-config.sh"

TESTDIR="$BUILDDIR/qa/tmp/omnicore-rpc-tests"
DATADIR="$TESTDIR/.bitcoin"

# Start clean
rm -rf "$TESTDIR"

git clone https://github.com/OmniLayer/OmniJ.git $TESTDIR
mkdir -p "$DATADIR/regtest"
touch "$DATADIR/regtest/omnicore.log"
cd $TESTDIR
echo "Omni Core RPC test dir: "$TESTDIR
echo "Last OmniJ commit: "$(git log -n 1 --format="%H Author: %cn <%ce>")
if [ "x${EXEEXT}" = "x.exe" ]; then
  wine $REAL_BITCOIND -regtest -txindex -server -daemon -rpcuser=bitcoinrpc -rpcpassword=pass -debug=1 -omnidebug=all -omnialertallowsender=any -discover=0 -listen=0 -datadir="$DATADIR"
  wine $REAL_BITCOINCLI -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait getinfo
  wine $REAL_BITCOINCLI -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait getinfo_MP
else
  $REAL_BITCOIND -regtest -txindex -server -daemon -rpcuser=bitcoinrpc -rpcpassword=pass -debug=1 -omnidebug=all -omnialertallowsender=any -discover=0 -listen=0 -datadir="$DATADIR"
  $REAL_BITCOINCLI -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait getinfo
  $REAL_BITCOINCLI -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait getinfo_MP
fi
./gradlew --console plain :omnij-rpc:regTest
STATUS=$?
if [ "x${EXEEXT}" = "x.exe" ]; then
  wine $REAL_BITCOINCLI -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait stop
else
  $REAL_BITCOINCLI -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait stop
fi

# If $STATUS is not 0, the test failed.
if [ $STATUS -ne 0 ]; then tail -n 200 $DATADIR/regtest/omnicore.log; fi


exit $STATUS
