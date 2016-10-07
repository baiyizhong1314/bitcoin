#!/bin/bash

# Get BUILDDIR and REAL_BITCOIND
CURDIR=$(cd $(dirname "$0"); pwd)
. "${CURDIR}/tests_config.py"

OMNICORED="$BUILDDIR/src/omnicored$EXEEXT"
OMNICORECLI="$BUILDDIR/src/omnicore-cli$EXEEXT"
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
if [ "$@" = "true" ]; then
    echo "Debug logging level: minimum"
    $OMNICORED -regtest -txindex -server -daemon -rpcuser=bitcoinrpc -rpcpassword=pass -debug=0 -omnidebug=none -omnialertallowsender=any -omniactivationallowsender=any -paytxfee=0.0001 -minrelaytxfee=0.00001 -discover=0 -listen=0 -datadir="$DATADIR" &
else
    echo "Debug logging level: maximum"
    $OMNICORED -regtest -txindex -server -daemon -rpcuser=bitcoinrpc -rpcpassword=pass -debug=1 -omnidebug=all -omnialertallowsender=any -omniactivationallowsender=any -paytxfee=0.0001 -minrelaytxfee=0.00001 -discover=0 -listen=0 -datadir="$DATADIR" &
fi
$OMNICORECLI -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait getinfo
$OMNICORECLI -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait omni_getinfo
./gradlew --console plain :omnij-rpc:regTest
STATUS=$?
$OMNICORECLI -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait stop

# If $STATUS is not 0, the test failed.
if [ $STATUS -ne 0 ]; then cat $DATADIR/regtest/omnicore.log; fi


exit $STATUS