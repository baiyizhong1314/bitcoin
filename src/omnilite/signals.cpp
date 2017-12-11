#include "omnilite/signals.h"

#include "omnicore/log.h"


CChainInterface::CChainInterface()
{
    PrintToConsole("Interface created\n");
}

CChainInterface::~CChainInterface()
{
    PrintToConsole("Interface shutdown\n");
}

void CChainInterface::UpdatedBlockTip(const CBlockIndex *pindex)
{
    PrintToConsole("UpdatedBlockTip\n");
}

void CChainInterface::SyncTransaction(const CTransaction& tx, const CBlockIndex* pindex, const CBlock* pblock)
{
    PrintToConsole("SyncTransaction\n");
}
