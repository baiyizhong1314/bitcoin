#include "mastercore_rpc_values.h"

#include "mastercore_sp.h"

#include "base58.h"
#include "rpcprotocol.h"

#include "json/json_spirit_value.h"

#include <string>


std::string ParseAddress(const json_spirit::Value& value)
{
    CBitcoinAddress address(value.get_str());
    if (!address.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }
    return address.ToString();
}

uint32_t ParsePropertyIdUnchecked(const json_spirit::Value& value)
{
    uint64_t propertyId = value.get_uint64();
    if (propertyId < 1 || 4294967295 < propertyId) { // TODO: avoid magic numbers
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property identifier");
    }
    return static_cast<uint32_t>(propertyId);
}

uint32_t ParsePropertyId(const json_spirit::Value& value, CMPSPInfo::Entry& info)
{
    uint32_t propertyId = ParsePropertyIdUnchecked(value);
    if (!mastercore::_my_sps->getSP(propertyId, info)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    }
    return propertyId;
}

uint32_t ParsePropertyId(const json_spirit::Value& value)
{
    uint32_t propertyId = ParsePropertyIdUnchecked(value);
    if (!mastercore::_my_sps->hasSP(propertyId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    }
    return propertyId;
}

