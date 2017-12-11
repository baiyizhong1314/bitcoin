#ifndef SIGNALS_H
#define SIGNALS_H

#include "validationinterface.h"


class CChainInterface : public CValidationInterface
{
public:
    CChainInterface();
    virtual ~CChainInterface();

protected:
    // CValidationInterface
    void SyncTransaction(const CTransaction& tx, const CBlockIndex *pindex, const CBlock* pblock);
    void UpdatedBlockTip(const CBlockIndex *pindex);
};


#endif /* SIGNALS_H */
