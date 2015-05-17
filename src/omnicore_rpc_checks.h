#ifndef OMNICORE_RPC_CHECKS_H
#define OMNICORE_RPC_CHECKS_H

#include <stdint.h>
#include <string>

void RequireSaneReferenceAmount(int64_t amount);

void RequireSufficientBalance(const std::string fromAddress, uint32_t propertyId, int64_t amount);

void RequireNonEmptyPropertyName(const std::string& name);

void RequireOnlyMSC(uint32_t propertyId);

void RequireActiveCrowdsale(uint32_t propertyId);

void RequireTokenAdministrator(const std::string& sender, uint32_t propertyId);

void RequireMatchingDexOffer(const std::string& toAddress, uint32_t propertyId);

void RequireNoOtherDexOffer(const std::string& fromAddress, uint32_t propertyId);


#endif // OMNICORE_RPC_CHECKS_H
