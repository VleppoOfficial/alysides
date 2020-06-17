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
#include "../cc/CCexchanges.h"

using namespace std;

extern void Lock2NSPV(const CPubKey &pk);
extern void Unlock2NSPV(const CPubKey &pk);

UniValue exchangeaddress(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    struct CCcontract_info *cp,C; std::vector<unsigned char> pubkey;
    cp = CCinit(&C,EVAL_EXCHANGES);
    if ( fHelp || params.size() > 1 )
        throw runtime_error("exchangeaddress [pubkey]\n");
    if ( ensure_CCrequirements(0) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    if ( params.size() == 1 )
        pubkey = ParseHex(params[0].get_str().c_str());
    return(CCaddress(cp,(char *)"Exchanges",pubkey));
}

UniValue exchangeopen(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 tokenid, agreementtxid = zeroid;
	int64_t numcoins, numtokens;
	bool bSpendDeposit = false;
	std::string typestr, depositspendstr;
	uint8_t typenum = 0;
	
	if (fHelp || params.size() < 6 || params.size() > 8)
		throw runtime_error("exchangeopen tokensupplier coinsupplier tokenid numcoins numtokens trade|loan [agreementtxid][enable_deposit_spend]\n");
	if (ensure_CCrequirements(EVAL_EXCHANGES) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
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
	if (numcoins < 0)
	{
		Unlock2NSPV(mypk);
		throw runtime_error("Required coin amount must be positive\n");
	}
	
	numtokens = atoll(params[4].get_str().c_str());
	if (numtokens < 0)
	{
		Unlock2NSPV(mypk);
		throw runtime_error("Required token amount must be positive\n");
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
	
	result = ExchangeOpen(mypk,0,tokensupplier,coinsupplier,tokenid,numcoins,numtokens,typenum,agreementtxid,bSpendDeposit);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	
	Unlock2NSPV(mypk);
	return(result);
}

UniValue exchangefund(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 exchangetxid;
	int64_t amount;
	bool useTokens = false;
	std::string typestr;
	if (fHelp || params.size() != 3)
		throw runtime_error("exchangefund exchangetxid amount coins|tokens\n");
	if (ensure_CCrequirements(EVAL_EXCHANGES) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
	exchangetxid = Parseuint256((char *)params[0].get_str().c_str());
	if (exchangetxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Exchangetxid is invalid\n");
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
	result = ExchangeFund(mypk, 0, exchangetxid, amount, useTokens);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue exchangeloanterms(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 exchangetxid;
	int64_t interest, duedate;
    if ( fHelp || params.size() != 3 )
        throw runtime_error("exchangeloanterms exchangetxid interest duedate\n");
    if (ensure_CCrequirements(EVAL_EXCHANGES) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
    exchangetxid = Parseuint256((char *)params[0].get_str().c_str());
	if (exchangetxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Exchangetxid is invalid\n");
	}
	interest = atoll(params[1].get_str().c_str());
	if (interest < 0) {
		Unlock2NSPV(mypk);
		throw runtime_error("Interest amount must be positive\n");
	}
	duedate = atoll(params[2].get_str().c_str());
	result = ExchangeLoanTerms(mypk, 0, exchangetxid, interest, duedate);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue exchangecancel(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 exchangetxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("exchangecancel exchangetxid\n");
    if (ensure_CCrequirements(EVAL_EXCHANGES) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
    exchangetxid = Parseuint256((char *)params[0].get_str().c_str());
	if (exchangetxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Exchangetxid is invalid\n");
	}
    result = ExchangeCancel(mypk, 0, exchangetxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue exchangeborrow(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 exchangetxid, loanparamtxid;
    if ( fHelp || params.size() != 2 )
        throw runtime_error("exchangerelease exchangetxid loanparamtxid\n");
    if (ensure_CCrequirements(EVAL_EXCHANGES) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
    exchangetxid = Parseuint256((char *)params[0].get_str().c_str());
	if (exchangetxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Exchangetxid is invalid\n");
	}
	loanparamtxid = Parseuint256((char *)params[1].get_str().c_str());
	if (loanparamtxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Loan parameter txid is invalid\n");
	}
	result = ExchangeBorrow(mypk, 0, exchangetxid, loanparamtxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue exchangerepo(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 exchangetxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("exchangerepo exchangetxid\n");
    if (ensure_CCrequirements(EVAL_EXCHANGES) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
    exchangetxid = Parseuint256((char *)params[0].get_str().c_str());
	if (exchangetxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Exchangetxid is invalid\n");
	}
    result = ExchangeRepo(mypk, 0, exchangetxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue exchangeclose(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 exchangetxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("exchangeclose exchangetxid\n");
    if (ensure_CCrequirements(EVAL_EXCHANGES) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	Lock2NSPV(mypk);
	
    exchangetxid = Parseuint256((char *)params[0].get_str().c_str());
	if (exchangetxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Exchangetxid is invalid\n");
	}
    result = ExchangeClose(mypk, 0, exchangetxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue exchangeinfo(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 exchangetxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("exchangeinfo exchangetxid\n");
    if ( ensure_CCrequirements(EVAL_EXCHANGES) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    exchangetxid = Parseuint256((char *)params[0].get_str().c_str());
	if (exchangetxid == zeroid)
		throw runtime_error("Exchangetxid is invalid\n");
    return(ExchangeInfo(mypk,exchangetxid));
}

UniValue exchangelist(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if ( fHelp || params.size() > 0 )
        throw runtime_error("exchangelist\n");
    if ( ensure_CCrequirements(EVAL_EXCHANGES) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    return(ExchangeList(mypk));
}

static const CRPCCommand commands[] =
{ //  category              name                actor (function)        okSafeMode
  //  -------------- ------------------------  -----------------------  ----------
	// exchanges
	{ "exchanges",  "exchangeaddress",	&exchangeaddress, 	true },
	{ "exchanges",  "exchangeopen",		&exchangeopen, 		true },
	{ "exchanges",  "exchangefund",		&exchangefund, 		true },
	{ "exchanges",  "exchangeinfo",		&exchangeinfo, 		true },
	{ "exchanges",  "exchangelist",		&exchangelist,		true },
	{ "exchanges",  "exchangecancel",	&exchangecancel,	true },
	{ "exchanges",  "exchangerepo",		&exchangerepo,		true },
	{ "exchanges",  "exchangeloanterms",&exchangeloanterms,	true },
	{ "exchanges",  "exchangeborrow",	&exchangeborrow,	true },
	{ "exchanges",  "exchangeclose",	&exchangeclose,		true },
};

void RegisterExchangesRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
