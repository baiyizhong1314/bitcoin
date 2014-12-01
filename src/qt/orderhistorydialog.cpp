// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "orderhistorydialog.h"
#include "ui_orderhistorydialog.h"

#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "wallet.h"
#include "base58.h"
#include "ui_interface.h"

#include <boost/filesystem.hpp>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"

// potentially overzealous includes here
#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "util.h"
#include <fstream>
#include <algorithm>
#include <vector>
#include <utility>
#include <string>
#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
// end potentially overzealous includes

using namespace json_spirit;
#include "mastercore.h"
using namespace mastercore;

// potentially overzealous using here
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace leveldb;
// end potentially overzealous using

#include "mastercore_dex.h"
#include "mastercore_tx.h"
#include "mastercore_sp.h"
#include "mastercore_parse_string.h"

#include <QDateTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

OrderHistoryDialog::OrderHistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::orderHistoryDialog),
    model(0)
{
    ui->setupUi(this);
    this->model = model;

    // setup
    ui->orderHistoryTable->setColumnCount(7);
    ui->orderHistoryTable->setHorizontalHeaderItem(0, new QTableWidgetItem(" "));
    ui->orderHistoryTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Date"));
    ui->orderHistoryTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Status"));
    ui->orderHistoryTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Trade Details"));
    ui->orderHistoryTable->setHorizontalHeaderItem(4, new QTableWidgetItem("Sold"));
    ui->orderHistoryTable->setHorizontalHeaderItem(5, new QTableWidgetItem("Received"));
    ui->orderHistoryTable->verticalHeader()->setVisible(false);
//    ui->txHistoryTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
//    ui->txHistoryTable->setShowGrid(false);
    ui->orderHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->orderHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->orderHistoryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->orderHistoryTable->horizontalHeader()->setResizeMode(3, QHeaderView::Stretch);
    ui->orderHistoryTable->setColumnWidth(0, 23);
    ui->orderHistoryTable->setColumnWidth(1, 150);
    ui->orderHistoryTable->setColumnWidth(2, 110);
    ui->orderHistoryTable->setColumnWidth(4, 180);
    ui->orderHistoryTable->setColumnWidth(5, 180);
    ui->orderHistoryTable->setColumnWidth(6, 0);

    CWallet *wallet = pwalletMain;
    string sAddress = "";
    string addressParam = "";
    bool addressFilter;

    addressFilter = false;
    int64_t nCount = 10;
    int64_t nFrom = 0;
    int64_t nStartBlock = 0;
    int64_t nEndBlock = 999999;

    Array response; //prep an array to hold our output
    int rowcount = 0;

    // rewrite to use original listtransactions methodology from core
    LOCK(wallet->cs_wallet);
    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, "*");

    // iterate backwards 
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0)
        {
            uint256 hash = pwtx->GetHash();
            CTransaction wtx;
            uint256 blockHash = 0;
            if (!GetTransaction(hash, wtx, blockHash, true)) continue;
            // get the time of the tx
            int64_t nTime = pwtx->GetTxTime();
            // get the height of the transaction and check it's within the chosen parameters
            blockHash = pwtx->hashBlock;
            if ((0 == blockHash) || (NULL == mapBlockIndex[blockHash])) continue;
            CBlockIndex* pBlockIndex = mapBlockIndex[blockHash];
            if (NULL == pBlockIndex) continue;
            int blockHeight = pBlockIndex->nHeight;
            if ((blockHeight < nStartBlock) || (blockHeight > nEndBlock)) continue; // ignore it if not within our range
            // check if the transaction exists in txlist, and if so is it correct type (21)
            if (p_txlistdb->exists(hash))
            {
                // get type from levelDB
                string strValue;
                if (!p_txlistdb->getTX(hash, strValue)) continue;
                std::vector<std::string> vstr;
                boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
                if (4 <= vstr.size())
                {
                    // if tx21, get the details for the list
                    if(21 == atoi(vstr[2]))
                    {
                        string statusText;
                        unsigned int propertyIdForSale = 0;
                        unsigned int propertyIdDesired = 0;
                        uint64_t amountForSale = 0;
                        uint64_t amountDesired = 0;
                        string address;
                        bool divisibleForSale;
                        bool divisibleDesired;
                        bool valid;
                        Array tradeArray;
                        uint64_t totalBought = 0;
                        uint64_t totalSold = 0;
                        bool orderOpen = false;

                        CMPMetaDEx temp_metadexoffer;
                        CMPTransaction mp_obj;
                        int parseRC = parseTransaction(true, wtx, blockHeight, 0, &mp_obj);
                        if (0 <= parseRC) //negative RC means no MP content/badly encoded TX, we shouldn't see this if TX in levelDB but check for sanity
                        {
                            if (0<=mp_obj.step1())
                            {
                                //MPTxType = mp_obj.getTypeString();
                                //MPTxTypeInt = mp_obj.getType();
                                address = mp_obj.getSender();
                                //if (!filterAddress.empty()) if ((senderAddress != filterAddress) && (refAddress != filterAddress)) return -1; // return negative rc if filtering & no match

                                int tmpblock=0;
                                uint32_t tmptype=0;
                                uint64_t amountNew=0;
                                valid=getValidMPTX(hash, &tmpblock, &tmptype, &amountNew);

                                if (0 == mp_obj.step2_Value())
                                {
                                    propertyIdForSale = mp_obj.getProperty();
                                    amountForSale = mp_obj.getAmount();
                                    divisibleForSale = isPropertyDivisible(propertyIdForSale);
                                    if (0 <= mp_obj.interpretPacket(NULL,&temp_metadexoffer))
                                    {
                                        propertyIdDesired = temp_metadexoffer.getDesProperty();
                                        divisibleDesired = isPropertyDivisible(propertyIdDesired);
                                        amountDesired = temp_metadexoffer.getAmountDesired();
                                        //mdex_action = temp_metadexoffer.getAction();
                                        t_tradelistdb->getMatchingTrades(hash, propertyIdForSale, &tradeArray, &totalSold, &totalBought);

                                        // status - is order cancelled/closed-filled/open/open-partialfilled?
                                        // is the sell offer still open - need more efficient way to do this
                                        for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
                                        {
                                            if (my_it->first == propertyIdForSale) //at minimum only go deeper if it's the right property id
                                            {
                                                md_PricesMap & prices = my_it->second;
                                                for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
                                                {
                                                    md_Set & indexes = (it->second);
                                                    for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
                                                    {
                                                        CMPMetaDEx obj = *it;
                                                        if( obj.getHash().GetHex() == hash.GetHex() ) orderOpen = true;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // work out status
                        bool partialFilled = false;
                        bool filled = false;
                        if(totalSold>0) partialFilled = true;
                        if(totalSold>=amountForSale) filled = true;
                        statusText = "UNKNOWN";
                        if((!orderOpen) && (!partialFilled)) statusText = "CANCELLED";
                        if((!orderOpen) && (partialFilled)) statusText = "PART CANCEL";
                        if((!orderOpen) && (filled)) statusText = "FILLED";
                        if((orderOpen) && (!partialFilled)) statusText = "OPEN";
                        if((orderOpen) && (partialFilled)) statusText = "PART FILLED";

                        // add to list
                        string displayText = "Sell ";
                        string displayIn = "+";
                        string displayOut = "-";
                        string displayInToken;
                        string displayOutToken;

                        if(divisibleForSale) { displayText += FormatDivisibleShortMP(amountForSale); } else { displayText += FormatIndivisibleMP(amountForSale); }
                        if(propertyIdForSale < 3)
                        {
                            if(propertyIdForSale == 1) { displayText += " MSC for "; displayOutToken = " MSC"; }
                            if(propertyIdForSale == 2) { displayText += " TMSC for "; displayOutToken = " TMSC"; }
                        }
                        else
                        {
                            string s = to_string(propertyIdForSale);
                            displayText += " SPT#" + s + " for ";
                            displayOutToken = " SPT#" + s;
                        }
                        if(divisibleDesired) { displayText += FormatDivisibleShortMP(amountDesired); } else { displayText += FormatIndivisibleMP(amountDesired); }
                        if(propertyIdDesired < 3)
                        {
                            if(propertyIdDesired == 1) { displayText += " MSC"; displayInToken = " MSC"; }
                            if(propertyIdDesired == 2) { displayText += " TMSC"; displayInToken = " TMSC"; }
                        }
                        else
                        {
                            string s = to_string(propertyIdDesired);
                            displayText += " SPT#" + s;
                            displayInToken = " SPT#" + s;
                        }
                        if(divisibleDesired) { displayIn += FormatDivisibleShortMP(totalBought); } else { displayIn += FormatIndivisibleMP(totalBought); }
                        if(divisibleForSale) { displayOut += FormatDivisibleShortMP(totalSold); } else { displayOut += FormatIndivisibleMP(totalSold); }
                        if(totalBought == 0) displayIn = "0";
                        if(totalSold == 0) displayOut = "0";
                        displayIn += displayInToken;
                        displayOut += displayOutToken;
                        QDateTime txTime;
                        txTime.setTime_t(nTime);
                        QString txTimeStr = txTime.toString(Qt::SystemLocaleShortDate);

                        //icon
                        QIcon ic = QIcon(":/icons/transaction_0");
                        // add to order history
                        ui->orderHistoryTable->setRowCount(rowcount+1);
                        QTableWidgetItem *dateCell = new QTableWidgetItem(txTimeStr);
                        QTableWidgetItem *statusCell = new QTableWidgetItem(QString::fromStdString(statusText));
                        QTableWidgetItem *infoCell = new QTableWidgetItem(QString::fromStdString(displayText));
                        QTableWidgetItem *amountOutCell = new QTableWidgetItem(QString::fromStdString(displayOut));
                        QTableWidgetItem *amountInCell = new QTableWidgetItem(QString::fromStdString(displayIn));
                        QTableWidgetItem *iconCell = new QTableWidgetItem;
                        QTableWidgetItem *txidCell = new QTableWidgetItem(QString::fromStdString(hash.GetHex()));
                        iconCell->setIcon(ic);
                        //addressCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
                        //addressCell->setForeground(QColor("#707070"));
                        amountOutCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                        amountOutCell->setForeground(QColor("#EE0000"));
                        amountInCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                        amountInCell->setForeground(QColor("#00AA00"));
                        if (rowcount % 2)
                        {
                            dateCell->setBackground(QColor("#F0F0F0"));
                            statusCell->setBackground(QColor("#F0F0F0"));
                            infoCell->setBackground(QColor("#F0F0F0"));
                            amountOutCell->setBackground(QColor("#F0F0F0"));
                            amountInCell->setBackground(QColor("#F0F0F0"));
                            iconCell->setBackground(QColor("#F0F0F0"));
                            txidCell->setBackground(QColor("#F0F0F0"));
                        }
                        if((!orderOpen) && (filled)) //make filled orders background
                        {
                            dateCell->setForeground(QColor("#707070"));
                            statusCell->setForeground(QColor("#707070"));
                            infoCell->setForeground(QColor("#707070"));
                            amountOutCell->setForeground(QColor("#993333"));
                            amountInCell->setForeground(QColor("#33CC66"));
                        }
                        ui->orderHistoryTable->setItem(rowcount, 0, iconCell);
                        ui->orderHistoryTable->setItem(rowcount, 1, dateCell);
                        ui->orderHistoryTable->setItem(rowcount, 2, statusCell);
                        ui->orderHistoryTable->setItem(rowcount, 3, infoCell);
                        ui->orderHistoryTable->setItem(rowcount, 4, amountOutCell);
                        ui->orderHistoryTable->setItem(rowcount, 5, amountInCell);
                        ui->orderHistoryTable->setItem(rowcount, 6, txidCell);
                        rowcount += 1;

/*
                        qItem->setData(Qt::UserRole + 1, QString::fromStdString(displayText));
                        qItem->setData(Qt::UserRole + 2, QString::fromStdString(displayIn));
                        qItem->setData(Qt::UserRole + 3, QString::fromStdString(displayOut));
                        qItem->setData(Qt::UserRole + 4, QString::fromStdString(statusText));
                        qItem->setData(Qt::UserRole + 5, QString::fromStdString(address));
                        qItem->setData(Qt::UserRole + 6, txTimeStr);
                        ui->orderHistoryLW->addItem(qItem);
*/
                    }
                }
            }
            // don't burn time doing more work than we need to
//            if ((int)response.size() >= (nCount+nFrom)) break;
        }
    }
    // sort array here and cut on nFrom and nCount
//    if (nFrom > (int)response.size())
//        nFrom = response.size();
//    if ((nFrom + nCount) > (int)response.size())
//        nCount = response.size() - nFrom;
//    Array::iterator first = response.begin();
//    std::advance(first, nFrom);
//    Array::iterator last = response.begin();
//    std::advance(last, nFrom+nCount);

//    if (last != response.end()) response.erase(last, response.end());
//    if (first != response.begin()) response.erase(response.begin(), first);

//    std::reverse(response.begin(), response.end()); // return oldest to newest?
 //   return response;   // return response array for JSON serialization

}

void OrderHistoryDialog::setModel(WalletModel *model)
{
    this->model = model;
    //connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(OrderRefresh()));
}

