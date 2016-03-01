#ifndef OMNICORE_RPCTX
#define OMNICORE_RPCTX

#include <univalue.h>

UniValue omni_sendtrade(const UniValue& params, bool fHelp);
UniValue omni_sendcanceltradesbyprice(const UniValue& params, bool fHelp);
UniValue omni_sendcanceltradesbypair(const UniValue& params, bool fHelp);
UniValue omni_sendcancelalltrades(const UniValue& params, bool fHelp);

#endif // OMNICORE_RPCTX
