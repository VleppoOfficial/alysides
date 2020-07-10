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
#include <stdint.h>
#include <string.h>
#include <numeric>
#include "univalue.h"
#include "amount.h"
#include "rpc/server.h"
#include "rpc/protocol.h"

#include "../wallet/crypter.h"
#include "../wallet/rpcwallet.h"

#include "sync_ext.h"

#include "../cc/CCinclude.h"
#include "../cc/CCagreements.h"
#include "../cc/CCtokens.h"
#include "../cc/CCPawnshop.h"

using namespace std;

extern void Lock2NSPV(const CPubKey &pk);
extern void Unlock2NSPV(const CPubKey &pk);

UniValue pawnshopaddress(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    struct CCcontract_info *cp,C; std::vector<unsigned char> pubkey;
    cp = CCinit(&C,EVAL_PAWNSHOP);
    if ( fHelp || params.size() > 1 )
        throw runtime_error("pawnshopaddress [pubkey]\n");
    if ( ensure_CCrequirements(0) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    if ( params.size() == 1 )
        pubkey = ParseHex(params[0].get_str().c_str());
    return(CCaddress(cp,(char *)"Pawnshop",pubkey));
}

UniValue pawnshopopen(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 tokenid, agreementtxid = zeroid;
	int64_t numcoins, numtokens;
	bool bSpendDeposit = false;
	std::string typestr, depositspendstr;
	uint8_t typenum = 0;
	
	if (fHelp || params.size() < 6 || params.size() > 8)
		throw runtime_error("pawnshopopen tokensupplier coinsupplier tokenid numcoins numtokens trade|loan [agreementtxid][enable_deposit_spend]\n");
	if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

	Lock2NSPV(mypk);
	
	CPubKey tokensupplier(pubkey2pk(ParseHex(params[0].get_str().c_str())));
	if (STR_TOLOWER(params[0].get_str()) == "mypk")
	{
        tokensupplier = mypk.IsValid() ? mypk : pubkey2pk(Mypubkey());
	}

	CPubKey coinsupplier(ParseHex(params[1].get_str().c_str()));
	if (STR_TOLOWER(params[1].get_str()) == "mypk")
	{
        coinsupplier = mypk.IsValid() ? mypk : pubkey2pk(Mypubkey());
	}
	
	tokenid = Parseuint256((char *)params[2].get_str().c_str());
    if (tokenid == zeroid)
	{
        Unlock2NSPV(mypk);
		throw runtime_error("Invalid tokenid\n");
	}
	
	//numcoins = atof((char *)params[3].get_str().c_str()) * COIN + 0.00000000499999;
	numcoins = atoll(params[3].get_str().c_str());
	if (numcoins < 1)
	{
		Unlock2NSPV(mypk);
		throw runtime_error("Required coin amount must be above 0\n");
	}
	
	numtokens = atoll(params[4].get_str().c_str());
	if (numtokens < 1)
	{
		Unlock2NSPV(mypk);
		throw runtime_error("Required token amount must be above 0\n");
	}
	
	typestr = params[5].get_str();
	if (STR_TOLOWER(typestr) == "trade")
        typenum = EXTF_TRADE;
	else if (STR_TOLOWER(typestr) == "loan")
        typenum = EXTF_LOAN;
    else
	{
		Unlock2NSPV(mypk);
        throw runtime_error("Incorrect type, must be 'trade' or 'loan'\n");
	}

	if (params.size() >= 7)
	{
        agreementtxid = Parseuint256((char *)params[6].get_str().c_str());
		if (agreementtxid == zeroid)
		{
			Unlock2NSPV(mypk);
			throw runtime_error("Agreement transaction id invalid\n");
		}
    }
	
	if (params.size() == 8)
	{
		depositspendstr = params[7].get_str();
		if (STR_TOLOWER(depositspendstr) == "true" || STR_TOLOWER(depositspendstr) == "1")
		{
			bSpendDeposit = true;
		}
		else if (STR_TOLOWER(depositspendstr) == "false" || STR_TOLOWER(depositspendstr) == "0")
		{
			bSpendDeposit = false;
		}
    }
	
	result = PawnshopOpen(mypk,0,tokensupplier,coinsupplier,tokenid,numcoins,numtokens,typenum,agreementtxid,bSpendDeposit);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	
	Unlock2NSPV(mypk);
	return(result);
}

UniValue pawnshopfund(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 pawnshoptxid;
	int64_t amount;
	bool useTokens = false;
	std::string typestr;
	if (fHelp || params.size() != 3)
		throw runtime_error("pawnshopfund pawnshoptxid amount coins|tokens\n");
	if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
	pawnshoptxid = Parseuint256((char *)params[0].get_str().c_str());
	if (pawnshoptxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Pawnshoptxid is invalid\n");
	}
	amount = atoll(params[1].get_str().c_str());
	if (amount < 0) {
		Unlock2NSPV(mypk);
		throw runtime_error("Amount must be positive\n");
	}
	typestr = params[2].get_str();
	if (STR_TOLOWER(typestr) == "coins")
		useTokens = false;
	else if (STR_TOLOWER(typestr) == "tokens")
		useTokens = true;
	else {
		Unlock2NSPV(mypk);
		throw runtime_error("Incorrect type, must be 'coins' or 'tokens'\n");
	}
	result = PawnshopFund(mypk, 0, pawnshoptxid, amount, useTokens);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue pawnshoploanterms(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 pawnshoptxid;
	int64_t interest, duedate;
    if ( fHelp || params.size() != 3 )
        throw runtime_error("pawnshoploanterms pawnshoptxid interest duedate\n");
    if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
    pawnshoptxid = Parseuint256((char *)params[0].get_str().c_str());
	if (pawnshoptxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Pawnshoptxid is invalid\n");
	}
	interest = atoll(params[1].get_str().c_str());
	if (interest < 0) {
		Unlock2NSPV(mypk);
		throw runtime_error("Interest amount must be positive\n");
	}
	duedate = atoll(params[2].get_str().c_str());
	result = PawnshopLoanTerms(mypk, 0, pawnshoptxid, interest, duedate);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue pawnshopcancel(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 pawnshoptxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("pawnshopcancel pawnshoptxid\n");
    if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
    pawnshoptxid = Parseuint256((char *)params[0].get_str().c_str());
	if (pawnshoptxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Pawnshoptxid is invalid\n");
	}
    result = PawnshopCancel(mypk, 0, pawnshoptxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue pawnshopborrow(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 pawnshoptxid, loanparamtxid;
    if ( fHelp || params.size() != 2 )
        throw runtime_error("pawnshoprelease pawnshoptxid loanparamtxid\n");
    if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
    pawnshoptxid = Parseuint256((char *)params[0].get_str().c_str());
	if (pawnshoptxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Pawnshoptxid is invalid\n");
	}
	loanparamtxid = Parseuint256((char *)params[1].get_str().c_str());
	if (loanparamtxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Loan parameter txid is invalid\n");
	}
	result = PawnshopBorrow(mypk, 0, pawnshoptxid, loanparamtxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue pawnshoprepo(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 pawnshoptxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("pawnshoprepo pawnshoptxid\n");
    if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
    pawnshoptxid = Parseuint256((char *)params[0].get_str().c_str());
	if (pawnshoptxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Pawnshoptxid is invalid\n");
	}
    result = PawnshopRepo(mypk, 0, pawnshoptxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue pawnshopclose(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 pawnshoptxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("pawnshopclose pawnshoptxid\n");
    if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
    pawnshoptxid = Parseuint256((char *)params[0].get_str().c_str());
	if (pawnshoptxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Pawnshoptxid is invalid\n");
	}
    result = PawnshopClose(mypk, 0, pawnshoptxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue pawnshopinfo(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 pawnshoptxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("pawnshopinfo pawnshoptxid\n");
    if ( ensure_CCrequirements(EVAL_PAWNSHOP) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    pawnshoptxid = Parseuint256((char *)params[0].get_str().c_str());
	if (pawnshoptxid == zeroid)
		throw runtime_error("Pawnshoptxid is invalid\n");
    return(PawnshopInfo(mypk,pawnshoptxid));
}

UniValue pawnshoplist(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if ( fHelp || params.size() > 0 )
        throw runtime_error("pawnshoplist\n");
    if ( ensure_CCrequirements(EVAL_PAWNSHOP) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    return(PawnshopList(mypk));
}

static const CRPCCommand commands[] =
{ //  category              name                actor (function)        okSafeMode
  //  -------------- ------------------------  -----------------------  ----------
	// pawnshop
	{ "pawnshop",  "pawnshopaddress",	&pawnshopaddress, 	true },
	{ "pawnshop",  "pawnshopopen",		&pawnshopopen, 		true },
	{ "pawnshop",  "pawnshopfund",		&pawnshopfund, 		true },
	{ "pawnshop",  "pawnshopinfo",		&pawnshopinfo, 		true },
	{ "pawnshop",  "pawnshoplist",		&pawnshoplist,		true },
	{ "pawnshop",  "pawnshopcancel",	&pawnshopcancel,	true },
	{ "pawnshop",  "pawnshoprepo",		&pawnshoprepo,		true },
	{ "pawnshop",  "pawnshoploanterms",&pawnshoploanterms,	true },
	{ "pawnshop",  "pawnshopborrow",	&pawnshopborrow,	true },
	{ "pawnshop",  "pawnshopclose",	&pawnshopclose,		true },
};

void RegisterPawnshopRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
