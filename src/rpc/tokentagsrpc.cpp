/******************************************************************************
* Copyright © 2014-2021 The SuperNET Developers.                             *
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
#include "../cc/CCtokens.h"
#include "../cc/CCtokens_impl.h"
#include "../cc/CCtokentags.h"

using namespace std;
/*
extern void Lock2NSPV(const CPubKey &pk);
extern void Unlock2NSPV(const CPubKey &pk);
*/
UniValue tokentagaddress(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    struct CCcontract_info *cp,C; std::vector<unsigned char> pubkey;
    cp = CCinit(&C,EVAL_TOKENTAGS);
    if ( fHelp || params.size() > 1 )
        throw runtime_error("tokentagaddress [pubkey]\n");
    if ( ensure_CCrequirements(0) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    if ( params.size() == 1 )
        pubkey = ParseHex(params[0].get_str().c_str());
    return(CCaddress(cp,(char *)"TokenTags",pubkey));
}
/*
UniValue tokentagcreate(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ), jsonParams(UniValue::VOBJ); 
    std::string hex, name;

	uint8_t flags;
    uint256 tokenid;
	int64_t maxupdates;
    std::vector<uint256> tokenids;
    std::vector<CAmount> updateamounts;

    if (fHelp || params.size() < 2 || params.size() > 4)
        throw runtime_error(
            "tokentagcreate name {\"tokenid\":updateamount,...} [flags][maxupdates]\n"
            );
    if (ensure_CCrequirements(EVAL_TOKENTAGS) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    CCerror.clear();

    //Lock2NSPV(mypk); // nSPV for tokens is not supported at the time of writing - Dan

    if (!EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    LOCK2(cs_main, pwalletMain->cs_wallet);

    name = params[0].get_str();
    if (name.size() == 0 || name.size() > 32)   
        return MakeResultError("Token tag name must not be empty and up to 32 characters");

    // parse json params:
    if (params[1].getType() == UniValue::VOBJ)
        jsonParams = params[1].get_obj();
    else if (params[1].getType() == UniValue::VSTR)  // json in quoted string '{...}'
        jsonParams.read(params[1].get_str().c_str());
    if (jsonParams.getType() != UniValue::VOBJ || jsonParams.empty())
        return MakeResultError("Invalid parameter 1, expected object.");

    std::vector<std::string> keys = jsonParams.getKeys();
    int32_t i = 0;
    for (const std::string& key : keys)
    {
        tokenid = Parseuint256((char *)key.c_str());
        if (tokenid == zeroid)
            return MakeResultError("Invalid parameter, tokenid in object invalid or null"); 
        for (const auto &entry : tokenids) 
        {
            if (entry == tokenid)
                return MakeResultError(string("Invalid parameter, duplicated tokenid: ")+tokenid.GetHex());
        }
        tokenids.push_back(tokenid);

        CAmount nAmount = atoll(jsonParams[i].getValStr().c_str()); //AmountFromValue(jsonParams[i]) * COIN;
        if (nAmount <= 0)
            return MakeResultError("Invalid parameter, updateamount must be positive"); 
        updateamounts.push_back(nAmount);
        i++;
    }

    if (tokenids.size() != updateamounts.size())
        return MakeResultError("Invalid parameter, mismatched amount of specified tokenids vs updateamounts"); 

    flags = 0;
    if (params.size() >= 3)
        flags = atoll(params[2].get_str().c_str());
    
    maxupdates = 0;
    if (params.size() == 4)
    {
		maxupdates = atoll(params[3].get_str().c_str());
        if (maxupdates < -1)
            return MakeResultError("Invalid maxupdates, must be -1, 0 or any positive number"); 
    }

    //Unlock2NSPV(mypk);

    UniValue sigData = TokenTagCreate(mypk,0,name,tokenids,updateamounts,flags,maxupdates);
    RETURN_IF_ERROR(CCerror);
    if (ResultHasTx(sigData) > 0)
        result = sigData;
    else
        result = MakeResultError("Could not create token tag: " + ResultGetError(sigData));
    return(result);
}

UniValue tokentagupdate(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ), jsonParams(UniValue::VARR); 

	uint256 tokentagid;
	std::string data = "";
    std::vector<CAmount> updateamounts;

    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "tokentagupdate tokentagid \"data\" ( [updateamount1,...] )\n"
            );
    if (ensure_CCrequirements(EVAL_TOKENTAGS) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    Lock2NSPV(mypk);

    tokentagid = Parseuint256((char *)params[0].get_str().c_str());
	if (tokentagid == zeroid)
    {
		Unlock2NSPV(mypk);
        throw runtime_error("Token tag id invalid\n");
    }
    
    data = params[1].get_str();
    if (data.empty() || data.size() > 128)
    {
        Unlock2NSPV(mypk);
        throw runtime_error("Data string must not be empty and be up to 128 characters\n");
    }

    updateamounts.clear();
	if (params.size() == 3)
    {
        // parse json params:
        if (params[2].getType() == UniValue::VARR)
            jsonParams = params[2].get_array();
        else if (params[2].getType() == UniValue::VSTR)  // json in quoted string '[...]'
            jsonParams.read(params[2].get_str().c_str());
        if (jsonParams.getType() != UniValue::VARR || jsonParams.empty())
        {
            Unlock2NSPV(mypk);
            throw runtime_error("Invalid parameter 3, expected array.\n");
        }
        
        int32_t i = 0;
        for (const UniValue& entry : jsonParams.getValues())
        {
            CAmount nAmount = atoll(entry.getValStr().c_str());
            if (nAmount <= 0)
            {
                Unlock2NSPV(mypk);
                throw runtime_error("Invalid amount #"+std::to_string(i)+" in parameter 3, updateamount must be positive\n");
            }
            updateamounts.push_back(nAmount);
            i++;
        }
    }

    result = TokenTagUpdate(mypk,0,tokentagid,data,updateamounts);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue tokentagclose(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);

	uint256 tokentagid;
	std::string data = "";

    if (fHelp || params.size() != 2)
        throw runtime_error(
            "tokentagclose tokentagid \"data\"\n"
            );
    if (ensure_CCrequirements(EVAL_TOKENTAGS) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    Lock2NSPV(mypk);

    tokentagid = Parseuint256((char *)params[0].get_str().c_str());
	if (tokentagid == zeroid)
    {
		Unlock2NSPV(mypk);
        throw runtime_error("Token tag id invalid\n");
    }
    
    data = params[1].get_str();
    if (data.empty() || data.size() > 128)
    {
        Unlock2NSPV(mypk);
        throw runtime_error("Data string must not be empty and be up to 128 characters\n");
    }

    result = TokenTagClose(mypk,0,tokentagid,data);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue tokentaginfo(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 txid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error(
            "tokentaginfo txid\n"
            );
    if (ensure_CCrequirements(EVAL_TOKENTAGS) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    txid = Parseuint256((char *)params[0].get_str().c_str());
    return(TokenTagInfo(txid));
}

UniValue tokentagsamples(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 tokentagid;
    int64_t samplenum;
    bool bReverse;
    std::string typestr;

    if ( fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "tokentagsamples tokentagid [samplenum][reverse]\n"
            );
    if (ensure_CCrequirements(EVAL_TOKENTAGS) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    tokentagid = Parseuint256((char *)params[0].get_str().c_str());
    if (tokentagid == zeroid)
        throw runtime_error("Token tag id invalid\n");

    samplenum = 0;
    if (params.size() >= 2)
		samplenum = atoll(params[1].get_str().c_str());
    
    bReverse = false;
	if (params.size() == 3)
    {
        typestr = params[2].get_str(); // NOTE: is there a better way to parse a bool from the param array?
        if (STR_TOLOWER(typestr) == "1" || STR_TOLOWER(typestr) == "true")
           bReverse = true;
    }

    return(TokenTagSamples(tokentagid,samplenum,bReverse));
}

UniValue tokentaglist(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 tokenid = zeroid;
    CPubKey pubkey;

    if (fHelp || params.size() > 2)
        throw runtime_error(
            "tokentaglist [tokenid][pubkey]\n"
            );
    if (ensure_CCrequirements(EVAL_TOKENTAGS) < 0 || ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

	if (params.size() >= 1)
        tokenid = Parseuint256((char *)params[0].get_str().c_str());

    if (params.size() == 2)
        pubkey = pubkey2pk(ParseHex(params[1].get_str().c_str()));

    return(TokenTagList(tokenid, pubkey));
}
*/
// Additional RPCs for token transaction analysis

template <class V>
static UniValue tokenowners(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    uint256 tokenid;
    int64_t minbalance = 1, maxdepth = 1000;
    if ( fHelp || params.size() < 1 || params.size() > 3 )
        throw runtime_error("tokenowners tokenid [minbalance][maxdepth]\n");
    if ( ensure_CCrequirements(V::EvalCode()) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    tokenid = Parseuint256((char *)params[0].get_str().c_str());
    if (params.size() >= 2)
        minbalance = atoll(params[1].get_str().c_str()); 
    if (params.size() == 3)
        maxdepth = atoll(params[2].get_str().c_str()); 
    return (V::TokenOwners(tokenid,minbalance,maxdepth));
}

UniValue tokenowners(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenowners<TokensV1>("tokenowners", params, fHelp, remotepk);
}
UniValue tokenv2owners(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenowners<TokensV2>("tokenv2owners", params, fHelp, remotepk);
}

template <class V>
static UniValue tokeninventory(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    std::vector<unsigned char> vpubkey;
    int64_t minbalance = 1;
    if ( fHelp || params.size() > 2 )
        throw runtime_error("tokeninventory [minbalance][pubkey]\n");
    if ( ensure_CCrequirements(V::EvalCode()) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    if (params.size() >= 1)
        minbalance = atoll(params[0].get_str().c_str()); 
    if (params.size() == 2)
        vpubkey = ParseHex(params[1].get_str().c_str());
    else
		vpubkey = Mypubkey();

    return (V::TokenInventory(vpubkey,minbalance));
}

UniValue tokeninventory(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokeninventory<TokensV1>("tokeninventory", params, fHelp, remotepk);
}
UniValue tokenv2inventory(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokeninventory<TokensV2>("tokenv2inventory", params, fHelp, remotepk);
}

static const CRPCCommand commands[] =
{ //  category              name                actor (function)        okSafeMode
  //  -------------- ------------------------  -----------------------  ----------
    // token tags
	{ "tokentags", "tokentagaddress", &tokentagaddress, true },
    /*{ "tokentags", "tokentagcreate",  &tokentagcreate,  true },
    { "tokentags", "tokentagupdate",  &tokentagupdate,  true },
    { "tokentags", "tokentagclose",   &tokentagclose,   true },
	{ "tokentags", "tokentaginfo",    &tokentaginfo,	true },
    { "tokentags", "tokentagsamples", &tokentagsamples,	true },
    { "tokentags", "tokentaglist",    &tokentaglist,	true },*/
    // extended tokens
	{ "tokens",    "tokenowners",     &tokenowners,     true },
    { "tokens",    "tokenv2owners",   &tokenv2owners,   true },
    { "tokens",    "tokeninventory",  &tokeninventory,  true },
    { "tokens",    "tokenv2inventory",&tokenv2inventory,true },
};

void RegisterTokenTagsRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
