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
 Pawnshop stuff
 */

#ifndef CC_PAWNSHOP_H
#define CC_PAWNSHOP_H

#define PAWNSHOPCC_VERSION 1
#define PAWNSHOPCC_MAXVINS 500
#define CC_TXFEE 10000
#define CC_MARKER_VALUE 10000
#define CC_BATON_VALUE 10000

#include "CCinclude.h"

enum EPawnshopTypeFlags
{
	PTF_TRADE = 1,
	PTF_LOAN = 2,
	PTF_DEPOSITUNLOCKABLE = 4,
};

enum EPawnshopInputsFlags
{
	PIF_COINS = 0,
	PIF_TOKENS = 1,
};

CScript EncodePawnshopOpenOpRet(uint8_t version,CPubKey tokensupplier,CPubKey coinsupplier,uint8_t pawnshoptype,uint256 tokenid,int64_t numtokens,int64_t numcoins,uint256 agreementtxid);
uint8_t DecodePawnshopOpenOpRet(CScript scriptPubKey,uint8_t &version,CPubKey &tokensupplier,CPubKey &coinsupplier,uint8_t &pawnshoptype,uint256 &tokenid,int64_t &numtokens,int64_t &numcoins,uint256 &agreementtxid);
CScript EncodePawnshopLoanTermsOpRet(uint8_t version,uint256 pawnshoptxid,int64_t interest,int64_t duedate);
uint8_t DecodePawnshopLoanTermsOpRet(CScript scriptPubKey,uint8_t &version,uint256 &pawnshoptxid,int64_t &interest,int64_t &duedate);
CScript EncodePawnshopOpRet(uint8_t funcid,uint8_t version,uint256 pawnshoptxid,uint256 tokenid,CPubKey tokensupplier,CPubKey coinsupplier);
uint8_t DecodePawnshopOpRet(const CScript scriptPubKey,uint8_t &version,uint256 &pawnshoptxid,uint256 &tokenid);

bool PawnshopValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

int64_t IsPawnshopvout(struct CCcontract_info *cp,const CTransaction& tx,bool mode,CPubKey tokensupplier,CPubKey coinsupplier,int32_t v);
bool PawnshopExactAmounts(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, int64_t &coininputs, int64_t &tokeninputs);
bool GetLatestPawnshopTxid(uint256 pawnshoptxid, uint256 &latesttxid, uint8_t &funcid);
bool FindPawnshopTxidType(uint256 pawnshoptxid, uint8_t type, uint256 &typetxid);
int64_t CheckDepositUnlockCond(uint256 pawnshoptxid);
bool ValidatePawnshopOpenTx(CTransaction opentx, std::string &CCerror);
int64_t GetPawnshopInputs(struct CCcontract_info *cp,CTransaction pawnshoptx,bool mode,std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &validUnspentOutputs);
int64_t AddPawnshopInputs(struct CCcontract_info *cp,CMutableTransaction &mtx,CTransaction pawnshoptx,bool mode,int32_t maxinputs);

UniValue PawnshopOpen(const CPubKey& pk,uint64_t txfee,CPubKey tokensupplier,CPubKey coinsupplier,uint256 tokenid,int64_t numcoins,int64_t numtokens,uint8_t pawnshoptype,uint256 agreementtxid,bool bSpendDeposit);
UniValue PawnshopFund(const CPubKey& pk, uint64_t txfee, uint256 pawnshoptxid, int64_t amount, bool useTokens);
UniValue PawnshopLoanTerms(const CPubKey& pk, uint64_t txfee, uint256 pawnshoptxid, int64_t interest, int64_t duedate);
UniValue PawnshopCancel(const CPubKey& pk, uint64_t txfee, uint256 pawnshoptxid);
UniValue PawnshopBorrow(const CPubKey& pk, uint64_t txfee, uint256 pawnshoptxid, uint256 loantermstxid);
UniValue PawnshopRepo(const CPubKey& pk, uint64_t txfee, uint256 pawnshoptxid);
UniValue PawnshopClose(const CPubKey& pk, uint64_t txfee, uint256 pawnshoptxid);

UniValue PawnshopInfo(const CPubKey& pk, uint256 pawnshoptxid);
UniValue PawnshopList(const CPubKey& pk);

#endif
