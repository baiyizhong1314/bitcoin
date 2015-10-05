#ifndef OMNICORE_UTILSBITCOIN_H
#define OMNICORE_UTILSBITCOIN_H

class CBlockIndex;
class uint256;

#include <stdint.h>

namespace mastercore
{
/** Returns the current chain length. */
int GetHeight();
/** Returns the timestamp of the latest block. */
uint32_t GetLatestBlockTime();
/** Returns the CBlockIndex for a given block hash, or NULL. */
CBlockIndex* GetBlockIndex(const uint256& hash);
/** Checks, whether the block is in the main chain. */
bool ChainContains(const CBlockIndex* pindex);

bool MainNet();
bool TestNet();
bool RegTest();
bool UnitTest();
bool isNonMainNet();
}

#endif // OMNICORE_UTILSBITCOIN_H
