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

UniValue pawnshopcreate(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 tokenid, agreementtxid = zeroid;
	int64_t numcoins, numtokens;
	bool bSpendDeposit = false;
	std::string name;
	uint32_t flags = 0;
	
	if (fHelp || params.size() < 6 || params.size() > 8)
		throw runtime_error(
            "pawnshopcreate \"name\" \"coinsupplier\" \"tokensupplier\" numcoins tokenid numtokens ( flags agreementtxid )\n"
            "\nCreate a new Pawnshop instance transaction and return the raw hex.\n"
			"Note: Pawnshop CC requires Tokens CC to be enabled in order to work properly.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. \"name\"                   (string, required) Name of the Pawnshop instance. (max 32 characters)\n"
            "2. \"coinsupplier\"   (string, required) Pubkey of the coin provider for this instance. The special \"mypk\" keyword can\n"
			"                                       be used to pass the pubkey used to launch the Komodo daemon to this parameter.\n"
            "3. \"tokensupplier\" (string, required) Pubkey of the token provider for this instance. The special \"mypk\" keyword can\n"
			"                                       be used to pass the pubkey used to launch the Komodo daemon to this parameter.\n"
            "4. numcoins      (numeric, required) The amount of coins that will be required for exchange.\n"
            "5. tokenid       (uint256, required) Token id of the tokens being exchanged.\n"
			"6. numtokens     (numeric, required) The amount of tokens with the specified token id that will be required for exchange.\n"
            "7. flags         (numeric/binary, optional) Optional flags for altering behavior & permissions for this Pawnshop instance.\n"
            "                                            List of currently available flags:\n"
            "                                            - PTF_REQUIREUNLOCK (1 or 00000001): Pawnshop instance will require usage\n"
			"                                              of agreementunlock before pawnshopexchange can be used. Requires valid\n"
			"                                              agreementtxid defined.\n"
			"                                            - PTF_NOLOAN (2 or 00000010): any loan-related rpcs like pawnshopborrow are\n"
			"                                              disabled. Currently mandatory as loan functionality isn't built yet.\n"
			"                                            - PTF_NOTRADE (4 or 00000100): disables pawnshopexchange before a loan is\n"
			"                                              initiated. Currently must be disabled.\n"
			"                                            Note: more flags may be introduced in the future.\n"
            "8. agreementtxid (uint256, optional) Transaction id of an agreement in the blockchain that shares at least one member pubkey with\n"
            "                                     the Pawnshop instance. If set, the Pawnshop instance will be linked as a settlement to the agreement\n"
            "                                     specified here. Note that in order to set up an agreement, the Agreements CC must be enabled.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("pawnshopcreate", "\"name1\" \"0237b502085b2552ae4ac6b2b9faf8b215b34a540ecdb5e0b22d2d3b82219a0aea\" \"0312b7f892c33da8fefbc5db6243d30c063031140fe0a130250aa79c66f8124b42\" 10000 e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 10000 00000010")
            + HelpExampleCli("pawnshopcreate", "\"name2\" \"mypk\" \"0312b7f892c33da8fefbc5db6243d30c063031140fe0a130250aa79c66f8124b42\" 10000 e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 10000 3 b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607")
            + HelpExampleRpc("pawnshopcreate", "\"name1\" \"0237b502085b2552ae4ac6b2b9faf8b215b34a540ecdb5e0b22d2d3b82219a0aea\" \"0312b7f892c33da8fefbc5db6243d30c063031140fe0a130250aa79c66f8124b42\" 10000 e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 10000 00000010")
            + HelpExampleRpc("pawnshopcreate", "\"name2\" \"mypk\" \"0312b7f892c33da8fefbc5db6243d30c063031140fe0a130250aa79c66f8124b42\" 10000 e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 10000 3 b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607")
        );
	if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

	Lock2NSPV(mypk);
	
	name = params[0].get_str();
    if (name.size() == 0 || name.size() > 32) {
		Unlock2NSPV(mypk);
        throw runtime_error("Name must not be empty and up to 32 chars\n");
    }
	
	CPubKey coinsupplier(ParseHex(params[1].get_str().c_str()));
	if (STR_TOLOWER(params[1].get_str()) == "mypk")
	{
        coinsupplier = mypk.IsValid() ? mypk : pubkey2pk(Mypubkey());
	}
	
	
	CPubKey tokensupplier(pubkey2pk(ParseHex(params[2].get_str().c_str())));
	if (STR_TOLOWER(params[2].get_str()) == "mypk")
	{
        tokensupplier = mypk.IsValid() ? mypk : pubkey2pk(Mypubkey());
	}

	//numcoins = atoll(params[3].get_str().c_str());
	numcoins = AmountFromValue(params[3]);
	if (numcoins < 1)
	{
		Unlock2NSPV(mypk);
		throw runtime_error("Required coin amount must be above 0\n");
	}
	
	tokenid = Parseuint256((char *)params[4].get_str().c_str());
    if (tokenid == zeroid)
	{
        Unlock2NSPV(mypk);
		throw runtime_error("Invalid tokenid\n");
	}
	
	numtokens = atoll(params[5].get_str().c_str());
	if (numtokens < 1)
	{
		Unlock2NSPV(mypk);
		throw runtime_error("Required token amount must be above 0\n");
	}
	
	if (params.size() >= 7)
	{
        flags = atoll(params[6].get_str().c_str());
    }
	
	if (params.size() == 8)
	{
        agreementtxid = Parseuint256((char *)params[7].get_str().c_str());
		if (agreementtxid == zeroid)
		{
			Unlock2NSPV(mypk);
			throw runtime_error("Agreement transaction id invalid\n");
		}
    }
	
	result = PawnshopCreate(mypk,0,name,tokensupplier,coinsupplier,numcoins,tokenid,numtokens,flags,agreementtxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	
	Unlock2NSPV(mypk);
	return(result);
}

UniValue pawnshopfund(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 createtxid;
	int64_t amount;
	if (fHelp || params.size() != 2)
		throw runtime_error(
            "pawnshopfund createtxid amount\n"
            "\nSend an amount of coins to the escrow of the specified Pawnshop instance.\n"
			"Only available to the coinsupplier pubkey specified at the creation of the Pawnshop instance.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
			"1. createtxid    (uint256, required) The Pawnshop instance's creation transaction id.\n"
            "2. amount        (numeric, required) The amount of coins to send.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("pawnshopfund", "b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607 10000")
            + HelpExampleRpc("pawnshopfund", "b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607 10000")
		);
	if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	
	Lock2NSPV(mypk);
	
	createtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (createtxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Create txid is invalid\n");
	}
	//amount = atoll(params[1].get_str().c_str());
	amount = AmountFromValue(params[1]);
	if (amount < 0) {
		Unlock2NSPV(mypk);
		throw runtime_error("Amount must be positive\n");
	}
	
	result = PawnshopFund(mypk, 0, createtxid, amount);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue pawnshoppledge(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 createtxid;
	if (fHelp || params.size() != 1)
		throw runtime_error(
            "pawnshoppledge createtxid\n"
            "\nSend an amount of tokens to the escrow of the specified Pawnshop instance.\n"
			"The amount sent is equal to the numtokens amount defined at the creation of the Pawnshop instance.\n"
			"Only available to the tokensupplier pubkey specified at the creation of the Pawnshop instance.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
			"1. createtxid    (uint256, required) The Pawnshop instance's creation transaction id.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("pawnshoppledge", "b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607")
            + HelpExampleRpc("pawnshoppledge", "b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607")
		);
	if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

	Lock2NSPV(mypk);
	
	createtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (createtxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Create txid is invalid\n");
	}
	
	result = PawnshopPledge(mypk, 0, createtxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

///// not implemented yet /////
UniValue pawnshopschedule(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 createtxid;
	int64_t interest;
	uint64_t duedate;
	std::string typestr;
	bool bRelative = false;
    if ( fHelp || params.size() < 3 || params.size() > 4 )
        throw runtime_error("pawnshopschedule createtxid interest duedate [bRelative]\n");
    if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	throw runtime_error("not implemented yet");
	
	Lock2NSPV(mypk);
	
    createtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (createtxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Create txid is invalid\n");
	}
	interest = atoll(params[1].get_str().c_str());
	if (interest < 0) {
		Unlock2NSPV(mypk);
		throw runtime_error("Interest amount must be positive\n");
	}
	duedate = atoll(params[2].get_str().c_str());
	if (params.size() == 4)
	{
        typestr = params[3].get_str();
		if (STR_TOLOWER(typestr) == "1" || STR_TOLOWER(typestr) == "true")
			bRelative = true;
		else if (STR_TOLOWER(typestr) == "0" || STR_TOLOWER(typestr) == "false")
			bRelative = false;
		else 
			throw runtime_error("Incorrect sort type\n");
    }
	
	result = PawnshopSchedule(mypk, 0, createtxid, interest, duedate, bRelative);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}
///// not implemented yet /////

UniValue pawnshopcancel(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 createtxid;
    if ( fHelp || params.size() != 1 )
		throw runtime_error(
            "pawnshopcancel createtxid\n"
            "\nCancel and permanently close the specified Pawnshop instance.\n"
			"Any coins and tokens left in the escrow will be refunded to their respective senders.\n"
			"Only available if the escrow does not have enough of the required token & coin balance.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
			"1. createtxid    (uint256, required) The Pawnshop instance's creation transaction id.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("pawnshopcancel", "b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607")
            + HelpExampleRpc("pawnshopcancel", "b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607")
		);
    if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	
	Lock2NSPV(mypk);
	
    createtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (createtxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Create txid is invalid\n");
	}
	
    result = PawnshopCancel(mypk, 0, createtxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

///// not implemented yet /////
UniValue pawnshopborrow(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 createtxid, loanparamtxid;
    if ( fHelp || params.size() != 2 )
        throw runtime_error("pawnshoprelease createtxid loanparamtxid\n");
    if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	throw runtime_error("not implemented yet");
	
	Lock2NSPV(mypk);
	
    createtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (createtxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Create txid is invalid\n");
	}
	loanparamtxid = Parseuint256((char *)params[1].get_str().c_str());
	if (loanparamtxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Loan parameter txid is invalid\n");
	}
	
	result = PawnshopBorrow(mypk, 0, createtxid, loanparamtxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}
UniValue pawnshopseize(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 createtxid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error("pawnshopseize createtxid\n");
    if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	throw runtime_error("not implemented yet");
	
	Lock2NSPV(mypk);
	
    createtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (createtxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Create txid is invalid\n");
	}
	
    result = PawnshopSeize(mypk, 0, createtxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}
///// not implemented yet /////

UniValue pawnshopexchange(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	UniValue result(UniValue::VOBJ);
	uint256 createtxid;
    if ( fHelp || params.size() != 1 )
		throw runtime_error(
            "pawnshopexchange createtxid\n"
            "\nComplete and permanently close the specified Pawnshop instance.\n"
			"All coins and tokens in the escrow will be exchanged between members.\n"
			"Only available if the escrow meets the required token & coin balance.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
			"1. createtxid    (uint256, required) The Pawnshop instance's creation transaction id.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("pawnshopexchange", "b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607")
            + HelpExampleRpc("pawnshopexchange", "b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607")
		);
    if (ensure_CCrequirements(EVAL_PAWNSHOP) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	
	Lock2NSPV(mypk);
	
    createtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (createtxid == zeroid) {
		Unlock2NSPV(mypk);
		throw runtime_error("Create txid is invalid\n");
	}
	
    result = PawnshopExchange(mypk, 0, createtxid);
	if (result[JSON_HEXTX].getValStr().size() > 0)
		result.push_back(Pair("result", "success"));
	Unlock2NSPV(mypk);
	return(result);
}

UniValue pawnshopinfo(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 createtxid;
    if ( fHelp || params.size() != 1 )
		throw runtime_error(
            "pawnshopinfo createtxid\n"
            "\nReturns info about any Pawnshop instance.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. createtxid    (uint256, required) The Pawnshop instance's creation transaction id.\n"
            "\nResult:\n"
            "\"result\"  (string) Info about the transaction.\n"
            "\nExamples:\n"
            + HelpExampleCli("pawnshopinfo", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a")
            + HelpExampleRpc("pawnshopinfo", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a")
        );
    if ( ensure_CCrequirements(EVAL_PAWNSHOP) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    createtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (createtxid == zeroid)
		throw runtime_error("Create txid is invalid\n");
    return(PawnshopInfo(mypk,createtxid));
}

UniValue pawnshoplist(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if ( fHelp || params.size() > 0 )
		throw runtime_error(
            "pawnshoplist\n"
            "\nReturns array of every Pawnshop instance that the pubkey used to launch the Komodo daemon.\n"
			"is a member of.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "none\n"
            "\nResult:\n"
            "\"result\"  (array of strings) Transaction ids of Pawnshop instances.\n"
            "\nExamples:\n"
            + HelpExampleCli("pawnshoplist", "")
            + HelpExampleRpc("pawnshoplist", "")
        );
    if ( ensure_CCrequirements(EVAL_PAWNSHOP) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    return(PawnshopList(mypk));
}

static const CRPCCommand commands[] =
{ //  category              name                actor (function)        okSafeMode
  //  -------------- ------------------------  -----------------------  ----------
	{ "pawnshop",  "pawnshopaddress",	&pawnshopaddress, 	true },
	{ "pawnshop",  "pawnshopcreate",	&pawnshopcreate, 	true },
	{ "pawnshop",  "pawnshopfund",		&pawnshopfund, 		true },
	{ "pawnshop",  "pawnshoppledge",	&pawnshoppledge, 	true },
	{ "pawnshop",  "pawnshopinfo",		&pawnshopinfo, 		true },
	{ "pawnshop",  "pawnshoplist",		&pawnshoplist,		true },
	{ "pawnshop",  "pawnshopcancel",	&pawnshopcancel,	true },
	{ "pawnshop",  "pawnshopseize",		&pawnshopseize,		true },
	{ "pawnshop",  "pawnshopschedule",  &pawnshopschedule,	true },
	{ "pawnshop",  "pawnshopborrow",	&pawnshopborrow,	true },
	{ "pawnshop",  "pawnshopexchange",	&pawnshopexchange,	true },
};

void RegisterPawnshopRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
