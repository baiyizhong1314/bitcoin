// Smart Properties & Crowd Sales

#include "omnicore/sp.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"

#include "base58.h"
#include "clientversion.h"
#include "main.h"
#include "serialize.h"
#include "streams.h"
#include "tinyformat.h"
#include "uint256.h"
#include "utiltime.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

#include <stdint.h>

#include <map>
#include <string>
#include <vector>
#include <utility>

using json_spirit::Object;
using json_spirit::Value;
using json_spirit::Pair;

using namespace mastercore;

CMPSPInfo::Entry::Entry()
  : prop_type(0), prev_prop_id(0), num_tokens(0), property_desired(0),
    deadline(0), early_bird(0), percentage(0),
    close_early(false), max_tokens(false), missedTokens(0), timeclosed(0),
    fixed(false), manual(false)
{
}

bool CMPSPInfo::Entry::isDivisible() const
{
    switch (prop_type) {
        case MSC_PROPERTY_TYPE_DIVISIBLE:
        case MSC_PROPERTY_TYPE_DIVISIBLE_REPLACING:
        case MSC_PROPERTY_TYPE_DIVISIBLE_APPENDING:
            return true;
    }
    return false;
}

void CMPSPInfo::Entry::print() const
{
    PrintToConsole("%s:%s(Fixed=%s,Divisible=%s):%lu:%s/%s, %s %s\n",
            issuer,
            name,
            fixed ? "Yes" : "No",
            isDivisible() ? "Yes" : "No",
            num_tokens,
            category, subcategory, url, data);
}

CMPSPInfo::CMPSPInfo(const boost::filesystem::path& path, bool fWipe)
{
    leveldb::Status status = Open(path, fWipe);
    PrintToConsole("Loading smart property database: %s\n", status.ToString());

    // special cases for constant SPs MSC and TMSC
    implied_msc.issuer = ExodusAddress().ToString();
    implied_msc.prop_type = MSC_PROPERTY_TYPE_DIVISIBLE;
    implied_msc.num_tokens = 700000;
    implied_msc.category = "N/A";
    implied_msc.subcategory = "N/A";
    implied_msc.name = "MasterCoin";
    implied_msc.url = "www.mastercoin.org";
    implied_msc.data = "***data***";
    implied_tmsc.issuer = ExodusAddress().ToString();
    implied_tmsc.prop_type = MSC_PROPERTY_TYPE_DIVISIBLE;
    implied_tmsc.num_tokens = 700000;
    implied_tmsc.category = "N/A";
    implied_tmsc.subcategory = "N/A";
    implied_tmsc.name = "Test MasterCoin";
    implied_tmsc.url = "www.mastercoin.org";
    implied_tmsc.data = "***data***";

    init();
}

CMPSPInfo::~CMPSPInfo()
{
    if (msc_debug_persistence) PrintToLog("CMPSPInfo closed\n");
}

void CMPSPInfo::init(unsigned int nextSPID, unsigned int nextTestSPID)
{
    next_spid = nextSPID;
    next_test_spid = nextTestSPID;
}

unsigned int CMPSPInfo::peekNextSPID(unsigned char ecosystem) const
{
    unsigned int nextId = 0;

    switch (ecosystem) {
        case OMNI_PROPERTY_MSC: // Main ecosystem, MSC: 1, TMSC: 2, First available SP = 3
            nextId = next_spid;
            break;
        case OMNI_PROPERTY_TMSC: // Test ecosystem, same as above with high bit set
            nextId = next_test_spid;
            break;
        default: // Non-standard ecosystem, ID's start at 0
            nextId = 0;
    }

    return nextId;
}

unsigned int CMPSPInfo::updateSP(unsigned int propertyID, const Entry& info)
{
    // cannot update implied SP
    if (OMNI_PROPERTY_MSC == propertyID || OMNI_PROPERTY_TMSC == propertyID) {
        return propertyID;
    }

    unsigned int res = propertyID;

    // generate the SP id
    std::string spKey = strprintf(FORMAT_BOOST_SPKEY, propertyID);

    CDataStream ssValue(SER_DISK, CLIENT_VERSION);
    ssValue.reserve(ssValue.GetSerializeSize(info));
    ssValue << info;
    leveldb::Slice spValue(&ssValue[0], ssValue.size());

    std::string spPrevKey = strprintf("blk-%s:sp-%d", info.update_block.ToString(), propertyID);
    std::string spPrevValue;

    leveldb::WriteBatch commitBatch;

    // if a value exists move it to the old key
    if (false == pdb->Get(readoptions, spKey, &spPrevValue).IsNotFound()) {
        commitBatch.Put(spPrevKey, spPrevValue);
    }
    commitBatch.Put(spKey, spValue);
    pdb->Write(syncoptions, &commitBatch);

    PrintToLog("Updated LevelDB with SP data successfully\n");
    return res;
}

unsigned int CMPSPInfo::putSP(unsigned char ecosystem, const Entry& info)
{
    unsigned int res = 0;
    switch (ecosystem) {
        case OMNI_PROPERTY_MSC: // Main ecosystem, MSC: 1, TMSC: 2, First available SP = 3
            res = next_spid++;
            break;
        case OMNI_PROPERTY_TMSC: // Test ecosystem, same as above with high bit set
            res = next_test_spid++;
            break;
        default: // Non-standard ecosystem, ID's start at 0
            res = 0;
    }

    // generate the SP id
    std::string spKey = strprintf(FORMAT_BOOST_SPKEY, res);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);
    ssValue.reserve(ssValue.GetSerializeSize(info));
    ssValue << info;
    leveldb::Slice spValue(&ssValue[0], ssValue.size());

    std::string txIndexKey = strprintf(FORMAT_BOOST_TXINDEXKEY, info.txid.ToString());
    std::string txValue = strprintf("%d", res);

    // sanity checking
    std::string existingEntry;
    if (false == pdb->Get(readoptions, spKey, &existingEntry).IsNotFound() && false == boost::equals(spValue.ToString(), existingEntry)) {
        PrintToConsole("%s WRITING SP %d TO LEVELDB WHEN A DIFFERENT SP ALREADY EXISTS FOR THAT ID!!!\n", __FUNCTION__, res);
    } else if (false == pdb->Get(readoptions, txIndexKey, &existingEntry).IsNotFound() && false == boost::equals(txValue, existingEntry)) {
        PrintToConsole("%s WRITING INDEX TXID %s : SP %d IS OVERWRITING A DIFFERENT VALUE!!!\n", __FUNCTION__, info.txid.ToString(), res);
    }

    // atomically write both the the SP and the index to the database
    leveldb::WriteBatch commitBatch;
    commitBatch.Put(spKey, spValue);
    commitBatch.Put(txIndexKey, txValue);

    pdb->Write(syncoptions, &commitBatch);
    return res;
}

bool CMPSPInfo::getSP(unsigned int spid, Entry& info) const
{
    int64_t nTimeStart = GetTimeMicros();

    // special cases for constant SPs MSC and TMSC
    if (OMNI_PROPERTY_MSC == spid) {
        info = implied_msc;
        return true;
    } else if (OMNI_PROPERTY_TMSC == spid) {
        info = implied_tmsc;
        return true;
    }

    std::string spKey = strprintf(FORMAT_BOOST_SPKEY, spid);
    std::string spInfoStr;
    if (false == pdb->Get(readoptions, spKey, &spInfoStr).ok()) {
        return false;
    }

    int64_t nTimeGet = GetTimeMicros();
    int64_t nSpanGet = nTimeGet - nTimeStart;

    try {
        CDataStream ssValue(spInfoStr.data(), spInfoStr.data() + spInfoStr.size(), SER_DISK, CLIENT_VERSION);
        ssValue >> info;
    } catch (const std::exception& e) {
        PrintToConsole("%s(): ERROR: %s\n", __func__, e.what());
    }

    int64_t nTimeParse = GetTimeMicros();
    int64_t nSpanParse = nTimeParse - nTimeGet;

    int64_t nSpanTotal = nTimeParse - nTimeStart;

    PrintToConsole("%s(): %.3f ms total, get sp %d: %.3f ms, parse: %.3f ms\n",
            __func__,
            0.001 * nSpanTotal,
            spid,
            0.001 * nSpanGet,
            0.001 * nSpanParse);

    return true;
}

bool CMPSPInfo::hasSP(unsigned int spid) const
{
    // special cases for constant SPs MSC and TMSC
    if (OMNI_PROPERTY_MSC == spid || OMNI_PROPERTY_TMSC == spid) {
        return true;
    }

    std::string spKey = strprintf(FORMAT_BOOST_SPKEY, spid);
    leveldb::Iterator* iter = pdb->NewIterator(readoptions);
    iter->Seek(spKey);
    bool res = (iter->Valid() && iter->key().compare(spKey) == 0);
    // clean up the iterator
    delete iter;

    return res;
}

unsigned int CMPSPInfo::findSPByTX(const uint256& txid) const
{
    unsigned int res = 0;

    std::string txIndexKey = strprintf(FORMAT_BOOST_TXINDEXKEY, txid.ToString());
    std::string spidStr;
    if (pdb->Get(readoptions, txIndexKey, &spidStr).ok()) {
        res = boost::lexical_cast<unsigned int>(spidStr);
    }

    return res;
}

int CMPSPInfo::popBlock(const uint256& block_hash)
{
    int res = 0;
    int remainingSPs = 0;
    leveldb::WriteBatch commitBatch;

    leveldb::Iterator* iter = NewIterator();
    for (iter->Seek("sp-"); iter->Valid() && iter->key().starts_with("sp-"); iter->Next()) {
        // parse the encoded json, failing if it doesnt parse or is an object
        std::string spInfoVal = iter->value().ToString();
        Entry info;
        bool fSuccess = false;
        try {
            CDataStream ssValue(spInfoVal.data(), spInfoVal.data() + spInfoVal.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> info;
            fSuccess = true;
        } catch (const std::exception& e) {
            PrintToConsole("%s(): ERROR: %s\n", __func__, e.what());
        }
        if (fSuccess) {
            if (info.update_block == block_hash) {
                // need to roll this SP back
                if (info.update_block == info.creation_block) {
                    // this is the block that created this SP, so delete the SP and the tx index entry
                    std::string txIndexKey = strprintf(FORMAT_BOOST_TXINDEXKEY, info.txid.ToString());
                    commitBatch.Delete(iter->key());
                    commitBatch.Delete(txIndexKey);
                } else {
                    std::vector<std::string> vstr;
                    std::string key = iter->key().ToString();
                    boost::split(vstr, key, boost::is_any_of("-"), boost::token_compress_on);
                    unsigned int propertyID = boost::lexical_cast<unsigned int>(vstr[1]);

                    std::string spPrevKey = strprintf("blk-%s:sp-%d", info.update_block.ToString(), propertyID);
                    std::string spPrevValue;

                    if (false == pdb->Get(readoptions, spPrevKey, &spPrevValue).IsNotFound()) {
                        // copy the prev state to the current state and delete the old state
                        commitBatch.Put(iter->key(), spPrevValue);
                        commitBatch.Delete(spPrevKey);
                        remainingSPs++;
                    } else {
                        // failed to find a previous SP entry, trigger reparse
                        res = -1;
                    }
                }
            } else {
                remainingSPs++;
            }
        } else {
            // failed to parse the db json, trigger reparse
            res = -1;
        }
    }

    // clean up the iterator
    delete iter;

    pdb->Write(syncoptions, &commitBatch);

    if (res < 0) {
        return res;
    } else {
        return remainingSPs;
    }
}

void CMPSPInfo::setWatermark(const uint256& watermark)
{
    leveldb::WriteBatch commitBatch;
    commitBatch.Delete(watermarkKey);
    commitBatch.Put(watermarkKey, watermark.ToString());
    pdb->Write(syncoptions, &commitBatch);
}

int CMPSPInfo::getWatermark(uint256& watermark) const // TODO: return bool
{
    std::string watermarkVal;
    if (pdb->Get(readoptions, watermarkKey, &watermarkVal).ok()) {
        watermark.SetHex(watermarkVal);
        return 0;
    } else {
        return -1;
    }
}

void CMPSPInfo::printAll() const
{
    // print off the hard coded MSC and TMSC entries
    for (unsigned int idx = OMNI_PROPERTY_MSC; idx <= OMNI_PROPERTY_TMSC; idx++) {
        Entry info;
        PrintToConsole("%10d => ", idx);
        if (getSP(idx, info)) {
            info.print();
        } else {
            PrintToConsole("<Internal Error on implicit SP>\n");
        }
    }

    leveldb::Iterator* iter = NewIterator();
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        if (iter->key().starts_with("sp-")) {
            std::vector<std::string> vstr;
            std::string key = iter->key().ToString();
            boost::split(vstr, key, boost::is_any_of("-"), boost::token_compress_on);

            PrintToConsole("%10s => ", vstr[1]);

            // parse the encoded json, failing if it doesnt parse or is an object
            std::string spInfoVal = iter->value().ToString();
            Entry info;
            bool fSuccess = false;
            try {
                CDataStream ssValue(spInfoVal.data(), spInfoVal.data() + spInfoVal.size(), SER_DISK, CLIENT_VERSION);
                ssValue >> info;
                fSuccess = true;
            } catch (const std::exception& e) {
                PrintToConsole("%s(): ERROR: %s\n", __func__, e.what());
            }

            if (fSuccess) {
                info.print();
            } else {
                PrintToConsole("<Malformed JSON in DB>\n");
            }
        }
    }

    // clean up the iterator
    delete iter;
}

CMPCrowd::CMPCrowd()
  : propertyId(0), nValue(0), property_desired(0), deadline(0),
    early_bird(0), percentage(0), u_created(0), i_created(0)
{
}

CMPCrowd::CMPCrowd(unsigned int pid, uint64_t nv, unsigned int cd, uint64_t dl, unsigned char eb, unsigned char per, uint64_t uct, uint64_t ict)
  : propertyId(pid), nValue(nv), property_desired(cd), deadline(dl),
    early_bird(eb), percentage(per), u_created(uct), i_created(ict)
{
}

void CMPCrowd::insertDatabase(const std::string& txhash, const std::vector<uint64_t>& txdata)
{
    txFundraiserData.insert(std::make_pair(txhash, txdata));
}

void CMPCrowd::print(const std::string& address, FILE* fp) const
{
    fprintf(fp, "%34s : id=%u=%X; prop=%u, value= %lu, deadline: %s (%lX)\n", address.c_str(), propertyId, propertyId,
        property_desired, nValue, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", deadline).c_str(), deadline);
}

void CMPCrowd::saveCrowdSale(std::ofstream& file, SHA256_CTX* shaCtx, const std::string& addr) const
{
    // compose the outputline
    // addr,propertyId,nValue,property_desired,deadline,early_bird,percentage,created,mined
    std::string lineOut = strprintf("%s,%d,%d,%d,%d,%d,%d,%d,%d",
            addr,
            propertyId,
            nValue,
            property_desired,
            deadline,
            (int) early_bird,
            (int) percentage,
            u_created,
            i_created);

    // append N pairs of address=nValue;blockTime for the database
    std::map<std::string, std::vector<uint64_t> >::const_iterator iter;
    for (iter = txFundraiserData.begin(); iter != txFundraiserData.end(); ++iter) {
        lineOut.append(strprintf(",%s=", (*iter).first));
        std::vector<uint64_t> const &vals = (*iter).second;

        std::vector<uint64_t>::const_iterator valIter;
        for (valIter = vals.begin(); valIter != vals.end(); ++valIter) {
            if (valIter != vals.begin()) {
                lineOut.append(";");
            }

            lineOut.append(strprintf("%d", *valIter));
        }
    }

    // add the line to the hash
    SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

    // write the line
    file << lineOut << std::endl;
}

CMPCrowd* mastercore::getCrowd(const std::string& address)
{
    CrowdMap::iterator my_it = my_crowds.find(address);

    if (my_it != my_crowds.end()) return &(my_it->second);

    return (CMPCrowd *)NULL;
}

bool mastercore::isPropertyDivisible(unsigned int propertyId)
{
    // TODO: is a lock here needed
    CMPSPInfo::Entry sp;

    if (_my_sps->getSP(propertyId, sp)) return sp.isDivisible();

    return true;
}

std::string mastercore::getPropertyName(unsigned int propertyId)
{
    CMPSPInfo::Entry sp;
    if (_my_sps->getSP(propertyId, sp)) return sp.name;
    return "Property Name Not Found";
}

bool mastercore::isCrowdsaleActive(unsigned int propertyId)
{
    for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
        const CMPCrowd& crowd = it->second;
        unsigned int foundPropertyId = crowd.getPropertyId();
        if (foundPropertyId == propertyId) return true;
    }
    return false;
}

// save info from the crowdsale that's being erased
void mastercore::dumpCrowdsaleInfo(const std::string& address, const CMPCrowd& crowd, bool bExpired)
{
    boost::filesystem::path pathInfo = GetDataDir() / INFO_FILENAME;
    FILE* fp = fopen(pathInfo.string().c_str(), "a");

    if (!fp) {
        PrintToLog("\nPROBLEM writing %s, errno= %d\n", INFO_FILENAME, errno);
        return;
    }

    fprintf(fp, "\n%s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str());
    fprintf(fp, "\nCrowdsale ended: %s\n", bExpired ? "Expired" : "Was closed");

    crowd.print(address, fp);

    fflush(fp);
    fclose(fp);
}

// calculates and returns fundraiser bonus, issuer premine, and total tokens
// propType : divisible/indiv
// bonusPerc: bonus percentage
// currentSecs: number of seconds of current tx
// numProps: number of properties
// issuerPerc: percentage of tokens to issuer
int mastercore::calculateFractional(unsigned short int propType, unsigned char bonusPerc, uint64_t fundraiserSecs,
        uint64_t numProps, unsigned char issuerPerc, const std::map<std::string, std::vector<uint64_t> >& txFundraiserData,
        const uint64_t amountPremined)
{
    // initialize variables
    double totalCreated = 0;
    double issuerPercentage = (double) (issuerPerc * 0.01);

    std::map<std::string, std::vector<uint64_t> >::const_iterator it;

    // iterate through fundraiser data
    for (it = txFundraiserData.begin(); it != txFundraiserData.end(); it++) {

        // grab the seconds and amt transferred from this tx
        uint64_t currentSecs = it->second.at(1);
        double amtTransfer = it->second.at(0);

        // make calc for bonus given in sec
        uint64_t bonusSeconds = fundraiserSecs - currentSecs;

        // turn it into weeks
        double weeks = bonusSeconds / (double) 604800;

        // make it a %
        double ebPercentage = weeks * bonusPerc;
        double bonusPercentage = (ebPercentage / 100) + 1;

        // init var
        double createdTokens;

        // if indiv or div, do different truncation
        if (MSC_PROPERTY_TYPE_DIVISIBLE == propType) {
            // calculate tokens
            createdTokens = (amtTransfer / 1e8) * (double) numProps * bonusPercentage;

            // add totals up
            totalCreated += createdTokens;
        } else {
            // same here
            createdTokens = (uint64_t) ((amtTransfer / 1e8) * (double) numProps * bonusPercentage);

            totalCreated += createdTokens;
        }
    };

    // calculate premine
    double totalPremined = totalCreated * issuerPercentage;
    double missedTokens;

    // calculate based on div/indiv, truncation/not
    if (2 == propType) {
        missedTokens = totalPremined - amountPremined;
    } else {
        missedTokens = (uint64_t) (totalPremined - amountPremined);
    }

    return missedTokens;
}

// go hunting for whether a simple send is a crowdsale purchase
// TODO !!!! horribly inefficient !!!! find a more efficient way to do this
bool mastercore::isCrowdsalePurchase(const uint256& txid, const std::string& address, int64_t* propertyId, int64_t* userTokens, int64_t* issuerTokens)
{
    // 1. loop crowdsales (active/non-active) looking for issuer address
    // 2. loop those crowdsales for that address and check their participant txs in database

    int64_t nTimeStart = GetTimeMicros();

    // check for an active crowdsale to this address
    CMPCrowd* pcrowdsale = getCrowd(address);

    int64_t nTimeGetCrowd = GetTimeMicros();
    int64_t nSpanGetCrowd = nTimeGetCrowd - nTimeStart;

    if (pcrowdsale) {
        std::map<std::string, std::vector<uint64_t> >::const_iterator it;
        const std::map<std::string, std::vector<uint64_t> >& database = pcrowdsale->getDatabase();
        for (it = database.begin(); it != database.end(); it++) {
            uint256 tmpTxid(it->first); // construct from string
            if (tmpTxid == txid) {
                *propertyId = pcrowdsale->getPropertyId();
                *userTokens = it->second.at(2);
                *issuerTokens = it->second.at(3);
                return true;
            }
        }
    }

    int64_t nTimeLoop1 = GetTimeMicros();
    int64_t nSpanLoop1 = nTimeLoop1 - nTimeGetCrowd;

    // if we still haven't found txid, check non active crowdsales to this address
    unsigned int nextSPID = _my_sps->peekNextSPID(1);
    unsigned int nextTestSPID = _my_sps->peekNextSPID(2);

    int64_t nProps = 0;
    int64_t nDbs = 0;
    int64_t nSpanGetProp = 0;

    for (int64_t tmpPropertyId = 1; tmpPropertyId < nextSPID; tmpPropertyId++) {
        ++nProps;
        CMPSPInfo::Entry sp;

        int64_t nTime1 = GetTimeMicros();
        if (!_my_sps->getSP(tmpPropertyId, sp)) continue;
        nSpanGetProp += (GetTimeMicros() - nTime1);
        if (sp.issuer != address) continue;

        std::map<std::string, std::vector<uint64_t> >::const_iterator it;
        const std::map<std::string, std::vector<uint64_t> >& database = sp.historicalData;
        nDbs += database.size();

        for (it = database.begin(); it != database.end(); it++) {
            uint256 tmpTxid(it->first); // construct from string
            if (tmpTxid == txid) {
                *propertyId = tmpPropertyId;
                *userTokens = it->second.at(2);
                *issuerTokens = it->second.at(3);
                return true;
            }
        }
    }

    int64_t nTimeLoop2 = GetTimeMicros();
    int64_t nSpanLoop2 = nTimeLoop2 - nTimeLoop1;

    int64_t nProps2 = 0;
    int64_t nDbs2 = 0;
    int64_t nSpanGetProp2 = 0;

    for (int64_t tmpPropertyId = TEST_ECO_PROPERTY_1; tmpPropertyId < nextTestSPID; tmpPropertyId++) {
        ++nProps2;
        CMPSPInfo::Entry sp;

        int64_t nTime1 = GetTimeMicros();
        if (!_my_sps->getSP(tmpPropertyId, sp)) continue;
        nSpanGetProp2 += (GetTimeMicros() - nTime1);

        if (sp.issuer == address) continue;

        std::map<std::string, std::vector<uint64_t> >::const_iterator it;
        const std::map<std::string, std::vector<uint64_t> >& database = sp.historicalData;
        nDbs2 += database.size();

        for (it = database.begin(); it != database.end(); it++) {
            uint256 tmpTxid(it->first); // construct from string
            if (tmpTxid == txid) {
                *propertyId = tmpPropertyId;
                *userTokens = it->second.at(2);
                *issuerTokens = it->second.at(3);
                return true;
            }
        }
    }

    int64_t nTimeLoop3 = GetTimeMicros();
    int64_t nSpanLoop3 = nTimeLoop3 - nTimeLoop2;
    int64_t nSpanTotal = nTimeLoop3 - nTimeStart;

    PrintToConsole("%s(): %.3f ms total, getcrowdsale: %.3f ms, loop 1: %.3f ms, loop 2: %.3f ms (%d props, %d dbs, %.3f ms, %.3f ms/prop), loop 3: %.3f ms (%d props, %d dbs, %.3f ms, %.3f ms/prop)\n",
            __func__,
            0.001 * nSpanTotal,
            0.001 * nSpanGetCrowd,
            0.001 * nSpanLoop1,
            0.001 * nSpanLoop2,
            nProps,
            nDbs,
            0.001 * nSpanGetProp,
            0.001 * nSpanGetProp / nProps,
            0.001 * nSpanLoop3,
            nProps2,
            nDbs2,
            0.001 * nSpanGetProp2,
            0.001 * nSpanGetProp2 / nProps2);

    // didn't find anything, not a crowdsale purchase
    return false;
}

void mastercore::eraseMaxedCrowdsale(const std::string& address, uint64_t blockTime, int block)
{
    CrowdMap::iterator it = my_crowds.find(address);

    if (it != my_crowds.end()) {
        const CMPCrowd& crowdsale = it->second;
        PrintToLog("%s() FOUND MAXED OUT CROWDSALE from address= '%s', erasing...\n", __FUNCTION__, address);

        dumpCrowdsaleInfo(address, crowdsale);

        // get sp from data struct
        CMPSPInfo::Entry sp;
        _my_sps->getSP(crowdsale.getPropertyId(), sp);

        // get txdata
        sp.historicalData = crowdsale.getDatabase();
        sp.close_early = 1;
        sp.max_tokens = 1;
        sp.timeclosed = blockTime;

        // update SP with this data
        sp.update_block = chainActive[block]->GetBlockHash();
        _my_sps->updateSP(crowdsale.getPropertyId(), sp);

        // no calculate fractional calls here, no more tokens (at MAX)
        my_crowds.erase(it);
    }
}

unsigned int mastercore::eraseExpiredCrowdsale(const CBlockIndex* pBlockIndex)
{
    if (pBlockIndex == NULL) return 0;

    const int64_t blockTime = pBlockIndex->GetBlockTime();
    unsigned int how_many_erased = 0;
    CrowdMap::iterator my_it = my_crowds.begin();

    while (my_crowds.end() != my_it) {
        const std::string& address = my_it->first;
        const CMPCrowd& crowdsale = my_it->second;

        if (blockTime > (int64_t) crowdsale.getDeadline()) {
            PrintToLog("%s() FOUND EXPIRED CROWDSALE from address= '%s', erasing...\n", __FUNCTION__, address);

            // TODO: dump the info about this crowdsale being delete into a TXT file (JSON perhaps)
            dumpCrowdsaleInfo(address, crowdsale, true);

            // get sp from data struct
            CMPSPInfo::Entry sp;
            _my_sps->getSP(crowdsale.getPropertyId(), sp);

            // find missing tokens
            double missedTokens = calculateFractional(sp.prop_type,
                    sp.early_bird,
                    sp.deadline,
                    sp.num_tokens,
                    sp.percentage,
                    crowdsale.getDatabase(),
                    crowdsale.getIssuerCreated());

            // get txdata
            sp.historicalData = crowdsale.getDatabase();
            sp.missedTokens = (int64_t) missedTokens;

            // update SP with this data
            sp.update_block = pBlockIndex->GetBlockHash();
            _my_sps->updateSP(crowdsale.getPropertyId(), sp);

            // update values
            update_tally_map(sp.issuer, crowdsale.getPropertyId(), missedTokens, BALANCE);

            my_crowds.erase(my_it++);

            ++how_many_erased;

        } else my_it++;
    }

    return how_many_erased;
}

const char* mastercore::c_strPropertyType(int propertyType)
{
    switch (propertyType) {
        case MSC_PROPERTY_TYPE_DIVISIBLE: return "divisible";
        case MSC_PROPERTY_TYPE_INDIVISIBLE: return"indivisible";
    }

    return "*** property type error ***";
}

