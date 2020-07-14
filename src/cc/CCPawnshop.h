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
	PTF_NOLOAN = 1,
	PTF_NOTRADE = 2,
	PTF_REQUIREUNLOCK = 4,
	//PTF_NOSEIZE = 8,
	//PTF_EARLYEXCHANGE = 16,
	//PTF_EARLYRESCHEDULE = 32,
	//PTF_NOREDEEM = 64,
	//PTF_ = 128,
};

enum EPawnshopInputsFlags
{
	PIF_COINS = 0,
	PIF_TOKENS = 1,
};

CScript EncodePawnshopCreateOpRet(uint8_t version,CPubKey tokensupplier,CPubKey coinsupplier,uint8_t pawnshopflags,uint256 tokenid,int64_t numtokens,int64_t numcoins,uint256 agreementtxid);
uint8_t DecodePawnshopCreateOpRet(CScript scriptPubKey,uint8_t &version,CPubKey &tokensupplier,CPubKey &coinsupplier,uint8_t &pawnshopflags,uint256 &tokenid,int64_t &numtokens,int64_t &numcoins,uint256 &agreementtxid);
CScript EncodePawnshopScheduleOpRet(uint8_t version,uint256 createtxid,int64_t interest,int64_t duedate);
uint8_t DecodePawnshopScheduleOpRet(CScript scriptPubKey,uint8_t &version,uint256 &createtxid,int64_t &interest,int64_t &duedate);
CScript EncodePawnshopOpRet(uint8_t funcid,uint8_t version,uint256 createtxid,uint256 tokenid,CPubKey tokensupplier,CPubKey coinsupplier);
uint8_t DecodePawnshopOpRet(const CScript scriptPubKey,uint8_t &version,uint256 &createtxid,uint256 &tokenid);

bool PawnshopValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

int64_t IsPawnshopvout(struct CCcontract_info *cp,const CTransaction& tx,bool mode,CPubKey tokensupplier,CPubKey coinsupplier,int32_t v);
bool PawnshopExactAmounts(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, int64_t &coininputs, int64_t &tokeninputs);
bool GetLatestPawnshopTxid(uint256 createtxid, uint256 &latesttxid, uint8_t &funcid);
bool FindPawnshopTxidType(uint256 createtxid, uint8_t type, uint256 &typetxid);
int64_t CheckDepositUnlockCond(uint256 createtxid);
bool ValidatePawnshopCreateTx(CTransaction opentx, std::string &CCerror);
int64_t GetPawnshopInputs(struct CCcontract_info *cp,CTransaction createtx,bool mode,std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &validUnspentOutputs);
int64_t AddPawnshopInputs(struct CCcontract_info *cp,CMutableTransaction &mtx,CTransaction createtx,bool mode,int32_t maxinputs);

UniValue PawnshopCreate(const CPubKey& pk,uint64_t txfee,std::string name,CPubKey tokensupplier,CPubKey coinsupplier,int64_t numcoins,uint256 tokenid,int64_t numtokens,uint32_t pawnshopflags,uint256 agreementtxid);
UniValue PawnshopFund(const CPubKey& pk, uint64_t txfee, uint256 createtxid, int64_t amount, bool useTokens);
UniValue PawnshopSchedule(const CPubKey& pk, uint64_t txfee, uint256 createtxid, int64_t interest, int64_t duedate);
UniValue PawnshopCancel(const CPubKey& pk, uint64_t txfee, uint256 createtxid);
UniValue PawnshopBorrow(const CPubKey& pk, uint64_t txfee, uint256 createtxid, uint256 scheduletxid);
UniValue PawnshopSeize(const CPubKey& pk, uint64_t txfee, uint256 createtxid);
UniValue PawnshopExchange(const CPubKey& pk, uint64_t txfee, uint256 createtxid);

UniValue PawnshopInfo(const CPubKey& pk, uint256 createtxid);
UniValue PawnshopList(const CPubKey& pk);

#endif
