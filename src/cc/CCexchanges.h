/******************************************************************************
 * Copyright © 2014-2020 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

/*
 Exchanges stuff
 */

#ifndef CC_EXCHANGES_H
#define CC_EXCHANGES_H

#define EXCHANGECC_VERSION 1
#define CC_TXFEE 10000
#define CC_MARKER_VALUE 10000
#define CC_BATON_VALUE 10000

#include "CCinclude.h"

enum EExchangeTypeFlags
{
	EXTF_TRADE = 1,
	EXTF_LOAN = 2,
	EXTF_DEPOSITUNLOCKABLE = 4,
};

enum EExchangeInputsFlags
{
	EIF_COINS = 0,
	EIF_TOKENS = 1,
};

CScript EncodeExchangeOpenOpRet(uint8_t version,CPubKey tokensupplier,CPubKey coinsupplier,uint8_t exchangetype,uint256 tokenid,int64_t numtokens,int64_t numcoins,uint256 agreementtxid);
uint8_t DecodeExchangeOpenOpRet(CScript scriptPubKey,uint8_t &version,CPubKey &tokensupplier,CPubKey &coinsupplier,uint8_t &exchangetype,uint256 &tokenid,int64_t &numtokens,int64_t &numcoins,uint256 &agreementtxid);
CScript EncodeExchangeLoanTermsOpRet(uint8_t version,uint256 exchangetxid,int64_t interest,int64_t duedate);
uint8_t DecodeExchangeLoanTermsOpRet(CScript scriptPubKey,uint8_t &version,uint256 &exchangetxid,int64_t &interest,int64_t &duedate);
CScript EncodeExchangeOpRet(uint8_t funcid,uint8_t version,uint256 exchangetxid,uint256 tokenid,CPubKey tokensupplier,CPubKey coinsupplier);
uint8_t DecodeExchangeOpRet(const CScript scriptPubKey,uint8_t &version,uint256 &exchangetxid,uint256 &tokenid);

bool ExchangesValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

int64_t IsExchangesvout(struct CCcontract_info *cp,const CTransaction& tx,bool mode,CPubKey tokensupplier,CPubKey coinsupplier,int32_t v);
bool ExchangesExactAmounts(struct CCcontract_info *cp,Eval* eval,const CTransaction &tx);
bool GetLatestExchangeTxid(uint256 exchangetxid, uint256 &latesttxid, uint8_t &funcid);
bool FindExchangeTxidType(uint256 exchangetxid, uint8_t type, uint256 &typetxid);
bool ValidateExchangeOpenTx(CTransaction opentx, std::string &CCerror);
int64_t GetExchangesInputs(struct CCcontract_info *cp,CTransaction exchangetx,bool mode,std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &validUnspentOutputs);
int64_t AddExchangesInputs(struct CCcontract_info *cp,CMutableTransaction &mtx,CTransaction exchangetx,bool mode,int32_t maxinputs);

UniValue ExchangeOpen(const CPubKey& pk,uint64_t txfee,CPubKey tokensupplier,CPubKey coinsupplier,uint256 tokenid,int64_t numcoins,int64_t numtokens,uint8_t exchangetype,uint256 agreementtxid,bool bSpendDeposit);
UniValue ExchangeFund(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid, int64_t amount, bool useTokens);
UniValue ExchangeLoanTerms(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid, int64_t interest, int64_t duedate);
UniValue ExchangeCancel(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid);
UniValue ExchangeBorrow(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid, uint256 loantermstxid);
UniValue ExchangeRepo(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid);
UniValue ExchangeClose(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid);

UniValue ExchangeInfo(const CPubKey& pk, uint256 exchangetxid);
UniValue ExchangeList(const CPubKey& pk);

#endif
