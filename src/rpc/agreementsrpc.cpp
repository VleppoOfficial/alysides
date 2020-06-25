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

using namespace std;

extern void Lock2NSPV(const CPubKey &pk);
extern void Unlock2NSPV(const CPubKey &pk);

UniValue agreementaddress(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    struct CCcontract_info *cp,C; std::vector<unsigned char> pubkey;
    cp = CCinit(&C,EVAL_AGREEMENTS);
    if ( fHelp || params.size() > 1 )
        throw runtime_error("agreementaddress [pubkey]\n");
    if ( ensure_CCrequirements(0) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    if ( params.size() == 1 )
        pubkey = ParseHex(params[0].get_str().c_str());
    return(CCaddress(cp,(char *)"Agreements",pubkey));
}

UniValue agreementcreate(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 datahash, prevproposaltxid, refagreementtxid;
	std::string info;
	int64_t prepayment, arbitratorfee, deposit;
    if (fHelp || params.size() < 4 || params.size() > 9)
        throw runtime_error("agreementcreate info datahash buyer arbitrator [prepayment][arbitratorfee][deposit][prevproposaltxid][refagreementtxid]\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    Lock2NSPV(mypk);
	
	info = params[0].get_str();
    if (info.size() == 0 || info.size() > 2048) {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement info must not be empty and up to 2048 characters\n");
    }
    datahash = Parseuint256((char *)params[1].get_str().c_str());
	if (datahash == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Data hash empty or invalid\n");
    }
	std::vector<unsigned char> buyer(ParseHex(params[2].get_str().c_str()));
	std::vector<unsigned char> arbitrator(ParseHex(params[3].get_str().c_str()));
	prepayment = 0;
	if (params.size() >= 5) {
		prepayment = atoll(params[4].get_str().c_str());
		if (prepayment != 0 && prepayment < 10000) {
			Unlock2NSPV(mypk);
			throw runtime_error("Prepayment too low\n");
		}
    }
	arbitratorfee = 0;
	if (params.size() >= 6) {
		arbitratorfee = atoll(params[5].get_str().c_str());
        if (arbitratorfee != 0 && arbitratorfee < 10000)    {
			Unlock2NSPV(mypk);
			throw runtime_error("Arbitrator fee too low\n");
		}
    }
	deposit = 0;
	if (params.size() >= 7) {
		deposit = atoll(params[6].get_str().c_str());
        if (deposit != 0 && deposit < 10000)    {
			Unlock2NSPV(mypk);
			throw runtime_error("Deposit too low\n");
		}
    }
	prevproposaltxid = zeroid;
	if (params.size() >= 8)     {
        prevproposaltxid = Parseuint256((char *)params[7].get_str().c_str());
    }
	if (params.size() == 9)     {
        refagreementtxid = Parseuint256((char *)params[8].get_str().c_str());
    }
	result = AgreementCreate(mypk, 0, info, datahash, buyer, arbitrator, prepayment, arbitratorfee, deposit, prevproposaltxid, refagreementtxid);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementstopproposal(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 proposaltxid;
    if (fHelp || params.size() != 1)
        throw runtime_error("agreementstopproposal proposaltxid\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    Lock2NSPV(mypk);
	
	proposaltxid = Parseuint256((char *)params[0].get_str().c_str());
	if (proposaltxid == zeroid)   {
		Unlock2NSPV(mypk);
        throw runtime_error("Proposal transaction id invalid\n");
    }
	
	result = AgreementStopProposal(mypk, 0, proposaltxid);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementaccept(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 proposaltxid;
    if (fHelp || params.size() != 1)
        throw runtime_error("agreementaccept proposaltxid\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    Lock2NSPV(mypk);
	
	proposaltxid = Parseuint256((char *)params[0].get_str().c_str());
	if (proposaltxid == zeroid)   {
		Unlock2NSPV(mypk);
        throw runtime_error("Proposal transaction id invalid\n");
    }
	
	result = AgreementAccept(mypk, 0, proposaltxid);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementupdate(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 datahash, prevproposaltxid, agreementtxid;
	std::string info;
	int64_t payment, arbitratorfee;
    if (fHelp || params.size() < 3 || params.size() > 6)
        throw runtime_error("agreementupdate agreementtxid info datahash [payment][prevproposaltxid][arbitratorfee]\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    Lock2NSPV(mypk);
	
	agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement id invalid\n");
    }
	info = params[1].get_str();
    if (info.size() == 0 || info.size() > 2048) {
		Unlock2NSPV(mypk);
        throw runtime_error("Update request info must not be empty and up to 2048 characters\n");
    }
    datahash = Parseuint256((char *)params[2].get_str().c_str());
	if (datahash == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Data hash empty or invalid\n");
    }
	payment = 0;
	if (params.size() >= 4) {
		payment = atoll(params[3].get_str().c_str());
		if (payment != 0 && payment < 10000) {
			Unlock2NSPV(mypk);
			throw runtime_error("Payment too low\n");
		}
    }
	prevproposaltxid = zeroid;
	if (params.size() >= 5) {
        prevproposaltxid = Parseuint256((char *)params[4].get_str().c_str());
    }
	arbitratorfee = 0;
	if (params.size() == 6) {
		arbitratorfee = atoll(params[5].get_str().c_str());
        if (arbitratorfee != 0 && arbitratorfee < 10000) {
			Unlock2NSPV(mypk);
			throw runtime_error("Arbitrator fee too low\n");
		}
    }
	result = AgreementUpdate(mypk, 0, agreementtxid, info, datahash, payment, prevproposaltxid, arbitratorfee);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementclose(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 datahash, prevproposaltxid, agreementtxid;
	std::string info;
	int64_t payment, depositcut;
    if (fHelp || params.size() < 3 || params.size() > 6)
        throw runtime_error("agreementclose agreementtxid info datahash [depositcut][payment][prevproposaltxid]\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    Lock2NSPV(mypk);
	
	agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement id invalid\n");
    }
	info = params[1].get_str();
    if (info.size() == 0 || info.size() > 2048) {
		Unlock2NSPV(mypk);
        throw runtime_error("Close request info must not be empty and up to 2048 characters\n");
    }
    datahash = Parseuint256((char *)params[2].get_str().c_str());
	if (datahash == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Data hash empty or invalid\n");
    }
	depositcut = 0;
	if (params.size() >= 4) {
		depositcut = atoll(params[3].get_str().c_str());
		if (depositcut != 0 && depositcut < 10000) {
			Unlock2NSPV(mypk);
			throw runtime_error("Deposit cut too low\n");
		}
    }
	payment = 0;
	if (params.size() >= 5) {
		payment = atoll(params[4].get_str().c_str());
		if (payment != 0 && payment < 10000) {
			Unlock2NSPV(mypk);
			throw runtime_error("Payment too low\n");
		}
    }
	prevproposaltxid = zeroid;
	if (params.size() >= 6) {
        prevproposaltxid = Parseuint256((char *)params[5].get_str().c_str());
    }
	result = AgreementClose(mypk, 0, agreementtxid, info, datahash, depositcut, payment, prevproposaltxid);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementdispute(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 datahash, agreementtxid;
    if (fHelp || params.size() != 2)
        throw runtime_error("agreementdispute agreementtxid datahash\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    Lock2NSPV(mypk);
	
	agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement id invalid\n");
    }
    datahash = Parseuint256((char *)params[1].get_str().c_str());
	if (datahash == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Data hash empty or invalid\n");
    }
	result = AgreementDispute(mypk, 0, agreementtxid, datahash);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementresolve(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 agreementtxid;
    if (fHelp || params.size() != 2)
        throw runtime_error("agreementresolve agreementtxid rewardedpubkey\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    Lock2NSPV(mypk);
	
	agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement id invalid\n");
    }
	std::vector<unsigned char> rewardedpubkey(ParseHex(params[1].get_str().c_str()));
	result = AgreementResolve(mypk, 0, agreementtxid, rewardedpubkey);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementunlock(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 exchangetxid, agreementtxid;
    if (fHelp || params.size() != 2)
        throw runtime_error("agreementunlock agreementtxid exchangetxid\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 || ensure_CCrequirements(EVAL_EXCHANGES) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
	
    Lock2NSPV(mypk);
	
	agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement id invalid\n");
    }
    exchangetxid = Parseuint256((char *)params[1].get_str().c_str());
	if (exchangetxid == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Exchange id invalid\n");
    }
	result = AgreementUnlock(mypk, 0, agreementtxid, exchangetxid);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementinfo(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 txid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("agreementinfo txid\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    txid = Parseuint256((char *)params[0].get_str().c_str());
    return(AgreementInfo(txid));
}

UniValue agreementupdatelog(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 agreementtxid;
	int64_t samplenum;
	std::string typestr;
	bool start_backwards;
    if ( fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error("agreementupdatelog agreementtxid start_backwards [num_samples]\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	typestr = params[1].get_str();
    if (STR_TOLOWER(typestr) == "1" || STR_TOLOWER(typestr) == "true")
        start_backwards = true;
    else if (STR_TOLOWER(typestr) == "0" || STR_TOLOWER(typestr) == "false")
        start_backwards = false;
    else 
        throw runtime_error("Incorrect sort type\n");
    if (params.size() >= 3)
		samplenum = atoll(params[2].get_str().c_str());
    else
        samplenum = 0;
    return(AgreementUpdateLog(agreementtxid, samplenum, start_backwards));
}

UniValue agreementinventory(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    CPubKey pubkey;
    if ( fHelp || params.size() > 1 )
        throw runtime_error("agreementinventory [pubkey]\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    if ( params.size() == 1 )
        pubkey = pubkey2pk(ParseHex(params[0].get_str().c_str()));
    else
		pubkey = mypk.IsValid() ? mypk : pubkey2pk(Mypubkey());
	return(AgreementInventory(pubkey));
}

UniValue agreementproposals(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 agreementtxid = zeroid;
	std::vector<unsigned char> pubkey;
    if ( fHelp || params.size() > 2 )
        throw runtime_error("agreementproposals [agreementtxid][pubkey]\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
	if ( params.size() >= 1 )
        agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if ( params.size() == 2 )
		pubkey = ParseHex(params[1].get_str().c_str());
    return(AgreementProposals(pubkey2pk(pubkey), agreementtxid));
}

UniValue agreementsubcontracts(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 agreementtxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("agreementsubcontracts agreementtxid\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
    return(AgreementSubcontracts(agreementtxid));
}

UniValue agreementsettlements(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 agreementtxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("agreementsettlements agreementtxid\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 || ensure_CCrequirements(EVAL_EXCHANGES) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
	
	throw runtime_error("not implemented yet\n");
	
    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
    return(AgreementSettlements(agreementtxid));
}

UniValue agreementlist(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if ( fHelp || params.size() > 0 )
        throw runtime_error("agreementlist\n");
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    return(AgreementList());
}

static const CRPCCommand commands[] =
{ //  category              name                actor (function)        okSafeMode
  //  -------------- ------------------------  -----------------------  ----------
	// agreements
	{ "agreements",  "agreementcreate",  &agreementcreate,  true },
	{ "agreements",  "agreementstopproposal", &agreementstopproposal, true },
	{ "agreements",  "agreementaccept",  &agreementaccept,  true },
	{ "agreements",  "agreementupdate",  &agreementupdate,  true }, 
	{ "agreements",  "agreementclose",   &agreementclose,   true }, 
	{ "agreements",  "agreementdispute", &agreementdispute, true }, 
	{ "agreements",  "agreementresolve", &agreementresolve, true }, 
	{ "agreements",  "agreementunlock",  &agreementunlock,  true }, 
	{ "agreements",  "agreementaddress", &agreementaddress, true },
	{ "agreements",  "agreementinfo",	 &agreementinfo,	true },
	{ "agreements",  "agreementupdatelog",	  &agreementupdatelog,    true },
	{ "agreements",  "agreementinventory",	  &agreementinventory,    true },
	{ "agreements",  "agreementproposals",	  &agreementproposals,    true },
	{ "agreements",  "agreementsubcontracts", &agreementsubcontracts, true },
	{ "agreements",  "agreementsettlements",  &agreementsettlements,  true },
	{ "agreements",  "agreementlist",    &agreementlist,    true },
};

void RegisterAgreementsRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
