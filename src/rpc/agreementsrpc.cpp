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

    CPubKey destpub,arbitratorpub;
	uint256 agreementhash,refagreementtxid;
	std::string agreementname;
	int64_t payment,disputefee,deposit;

    if (fHelp || params.size() < 4 || params.size() > 8)
        throw runtime_error(
            "agreementcreate destpub agreementname agreementhash deposit [arbitratorpub][disputefee][refagreementtxid][payment]\n"
            );
    if (ensure_CCrequirements(EVAL_AGREEMENTS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    Lock2NSPV(mypk);
	
    destpub = pubkey2pk(ParseHex(params[0].get_str().c_str()));

    agreementname = params[1].get_str();
    if (agreementname.size() == 0 || agreementname.size() > 64)
    {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement name must not be empty and up to 64 characters\n");
    }

    agreementhash = Parseuint256((char *)params[2].get_str().c_str());
	if (agreementhash == zeroid)
    {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement hash empty or invalid\n");
    }

    deposit = atoll(params[3].get_str().c_str()) * COIN;
    if (deposit >= 0)
        deposit = AmountFromValue(params[3]);
    else
    {
        Unlock2NSPV(mypk);
        throw runtime_error("Required deposit too low\n");
    }
    
    if (params.size() >= 5)
        arbitratorpub = pubkey2pk(ParseHex(params[4].get_str().c_str()));

    disputefee = 0;
	if (params.size() >= 6)
    {
        if (arbitratorpub.IsFullyValid())
        {
            disputefee = atoll(params[5].get_str().c_str()) * COIN;
            if (disputefee == 0 || disputefee >= 10000)
                disputefee = AmountFromValue(params[5]);
            else
            {
                Unlock2NSPV(mypk);
                throw runtime_error("Dispute fee too low\n");
            }
        }
    }

    if (params.size() >= 7)
        refagreementtxid = Parseuint256((char *)params[6].get_str().c_str());

	payment = 0;
	if (params.size() == 8)
    {
        payment = atoll(params[7].get_str().c_str()) * COIN;
        if (payment == 0 || payment >= 10000)
            payment = AmountFromValue(params[7]);
        else
        {
            Unlock2NSPV(mypk);
            throw runtime_error("Required prepayment too low\n");
        }
    }
	
	result = AgreementCreate(mypk,0,destpub,agreementname,agreementhash,deposit,arbitratorpub,disputefee,refagreementtxid,payment);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementupdate(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);

	uint256 agreementhash,agreementtxid;
	std::string agreementname = "";
	int64_t payment;

    if (fHelp || params.size() < 2 || params.size() > 4)
        throw runtime_error(
            "agreementupdate agreementtxid agreementhash [agreementname][payment]\n"
            );
    if (ensure_CCrequirements(EVAL_AGREEMENTS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    Lock2NSPV(mypk);

    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid)   {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement transaction id invalid\n");
    }

    agreementhash = Parseuint256((char *)params[1].get_str().c_str());
	if (agreementhash == zeroid)
    {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement hash empty or invalid\n");
    }

    if (params.size() >= 3)
    {
        agreementname = params[2].get_str();
        if (agreementname.size() > 64)
        {
            Unlock2NSPV(mypk);
            throw runtime_error("Agreement name must be up to 64 characters\n");
        }
    }

	payment = 0;
	if (params.size() == 4)
    {
        payment = atoll(params[3].get_str().c_str()) * COIN;
        if (payment == 0 || payment >= 10000)
            payment = AmountFromValue(params[3]);
        else
        {
            Unlock2NSPV(mypk);
            throw runtime_error("Required payment too low\n");
        }
    }

	result = AgreementUpdate(mypk,0,agreementtxid,agreementhash,agreementname,payment);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementclose(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);

	uint256 agreementhash,agreementtxid;
	std::string agreementname = "";
	int64_t payment, depositcut;

    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "agreementclose agreementtxid agreementhash depositcut [agreementname][payment]\n"
            );
    if (ensure_CCrequirements(EVAL_AGREEMENTS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    Lock2NSPV(mypk);

    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid)   {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement transaction id invalid\n");
    }

    agreementhash = Parseuint256((char *)params[1].get_str().c_str());
	if (agreementhash == zeroid)
    {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement hash empty or invalid\n");
    }

    depositcut = atoll(params[2].get_str().c_str()) * COIN;
    if (depositcut >= 0)
        depositcut = AmountFromValue(params[2]);
    else
    {
        Unlock2NSPV(mypk);
        throw runtime_error("Required deposit cut can't be negative\n");
    }

    if (params.size() >= 4)
    {
        agreementname = params[3].get_str();
        if (agreementname.size() > 64)
        {
            Unlock2NSPV(mypk);
            throw runtime_error("Agreement name must be up to 64 characters\n");
        }
    }

	payment = 0;
	if (params.size() == 5)
    {
        payment = atoll(params[4].get_str().c_str()) * COIN;
        if (payment == 0 || payment >= 10000)
            payment = AmountFromValue(params[4]);
        else
        {
            Unlock2NSPV(mypk);
            throw runtime_error("Required payment too low\n");
        }
    }

    result = AgreementClose(mypk,0,agreementtxid,agreementhash,depositcut,agreementname,payment);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementstopproposal(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);

	uint256 proposaltxid;
	std::string cancelinfo = "";

    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "agreementstopproposal proposaltxid [cancelinfo]\n"
            );
    if (ensure_CCrequirements(EVAL_AGREEMENTS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    Lock2NSPV(mypk);

    proposaltxid = Parseuint256((char *)params[0].get_str().c_str());
	if (proposaltxid == zeroid)   {
		Unlock2NSPV(mypk);
        throw runtime_error("Proposal transaction id invalid\n");
    }

    if (params.size() == 2)
    {
        cancelinfo = params[1].get_str();
        if (cancelinfo.size() > 256)
        {
            Unlock2NSPV(mypk);
            throw runtime_error("Cancel info must be up to 256 characters\n");
        }
    }

    result = AgreementStopProposal(mypk,0,proposaltxid,cancelinfo);
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
        throw runtime_error(
            "agreementaccept proposaltxid\n"
            );
    if (ensure_CCrequirements(EVAL_AGREEMENTS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    Lock2NSPV(mypk);

    proposaltxid = Parseuint256((char *)params[0].get_str().c_str());
	if (proposaltxid == zeroid)   {
		Unlock2NSPV(mypk);
        throw runtime_error("Proposal transaction id invalid\n");
    }

    result = AgreementAccept(mypk,0,proposaltxid);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementdispute(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);

	uint256 agreementtxid;
	std::string typestr,disputeinfo = "";
    bool bFinalDispute;

    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "agreementdispute agreementtxid [disputeinfo][bFinalDispute]\n"
            );
    if (ensure_CCrequirements(EVAL_AGREEMENTS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    Lock2NSPV(mypk);

    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid)   {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement transaction id invalid\n");
    }

    if (params.size() >= 2)
    {
        disputeinfo = params[1].get_str();
        if (disputeinfo.size() > 256)
        {
            Unlock2NSPV(mypk);
            throw runtime_error("Dispute info must be up to 256 characters\n");
        }
    }

	bFinalDispute = false;
	if (params.size() == 3)
    {
        typestr = params[2].get_str(); // NOTE: is there a better way to parse a bool from the param array?
        if (STR_TOLOWER(typestr) == "1" || STR_TOLOWER(typestr) == "true")
            bFinalDispute = true;
    }

    result = AgreementDispute(mypk,0,agreementtxid,disputeinfo,bFinalDispute);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementresolve(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);

	uint256 disputetxid;
	std::string resolutioninfo = "";
    int64_t depositcut;

    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "agreementresolve disputetxid depositcut [resolutioninfo]\n"
            );
    if (ensure_CCrequirements(EVAL_AGREEMENTS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    Lock2NSPV(mypk);

    disputetxid = Parseuint256((char *)params[0].get_str().c_str());
	if (disputetxid == zeroid)   {
		Unlock2NSPV(mypk);
        throw runtime_error("Dispute transaction id invalid\n");
    }
    
    depositcut = atoll(params[1].get_str().c_str()) * COIN;
    if (depositcut >= 0)
        depositcut = AmountFromValue(params[1]);

	if (params.size() == 3)
    {
        resolutioninfo = params[2].get_str();
        if (resolutioninfo.size() > 256)
        {
            Unlock2NSPV(mypk);
            throw runtime_error("Dispute resolution info must be up to 256 characters\n");
        }
    }

    result = AgreementResolve(mypk,0,disputetxid,depositcut,resolutioninfo);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementunlock(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);

	uint256 agreementtxid,unlocktxid;

    if (fHelp || params.size() != 2)
        throw runtime_error(
            "agreementunlock agreementtxid unlocktxid\n"
            );
    if (ensure_CCrequirements(EVAL_AGREEMENTS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    Lock2NSPV(mypk);

    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid)   {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement transaction id invalid\n");
    }

    unlocktxid = Parseuint256((char *)params[0].get_str().c_str());
	if (unlocktxid == zeroid)   {
		Unlock2NSPV(mypk);
        throw runtime_error("Unlocking transaction id invalid\n");
    }

    result = AgreementUnlock(mypk,0,agreementtxid,unlocktxid);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementinfo(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 txid;
    if ( fHelp || params.size() != 1 )
        throw runtime_error(
            "agreementinfo txid\n"
            );
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    txid = Parseuint256((char *)params[0].get_str().c_str());
    return(AgreementInfo(txid));
}

UniValue agreementeventlog(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 agreementtxid;
    int64_t samplenum;
    uint8_t flags;
    bool bReverse;
    std::string typestr;

    if ( fHelp || params.size() < 1 || params.size() > 4)
        throw runtime_error(
            "agreementeventlog agreementtxid [all|updates|closures|disputes|resolutions][samplenum][reverse]\n"
            );
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
    if (agreementtxid == zeroid)
        throw runtime_error("Agreement transaction id invalid\n");
    
    flags = ASF_ALLEVENTS;
    if (params.size() >= 2)
    {
        if (STR_TOLOWER(params[1].get_str()) == "all")
            flags = ASF_ALLEVENTS;
        else if (STR_TOLOWER(params[1].get_str()) == "updates")
            flags = ASF_UPDATES;
        else if (STR_TOLOWER(params[1].get_str()) == "closures")
            flags = ASF_CLOSURES;
        else if (STR_TOLOWER(params[1].get_str()) == "disputes")
            flags = ASF_DISPUTES;
        else if (STR_TOLOWER(params[1].get_str()) == "resolutions")
            flags = ASF_RESOLUTIONS;
        else
            throw runtime_error("Incorrect search keyword used\n");
    }
    
    samplenum = 0;
    if (params.size() >= 3)
		samplenum = atoll(params[2].get_str().c_str());
    
    bReverse = false;
	if (params.size() == 4)
    {
        typestr = params[3].get_str(); // NOTE: is there a better way to parse a bool from the param array?
        if (STR_TOLOWER(typestr) == "1" || STR_TOLOWER(typestr) == "true")
            bReverse = true;
    }

    return(AgreementEventLog(agreementtxid,flags,samplenum,bReverse));
}

UniValue agreementreferences(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 agreementtxid;

    if ( fHelp || params.size() != 1 )
        throw runtime_error(
            "agreementreferences agreementtxid\n"
            );
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
    if (agreementtxid == zeroid)
        throw runtime_error("Agreement transaction id invalid\n");
    
    return(AgreementReferences(mypk,agreementtxid));
}

UniValue agreementlist(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	uint256 filtertxid;
    uint8_t flags;

    if (fHelp || params.size() > 2)
        throw runtime_error(
            "agreementlist [all|proposals|agreements][filtertxid]\n"
            );
    if (ensure_CCrequirements(EVAL_AGREEMENTS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    flags = ASF_ALL;
    if (params.size() >= 1)
    {
        if (STR_TOLOWER(params[0].get_str()) == "all")
            flags = ASF_ALL;
        else if (STR_TOLOWER(params[0].get_str()) == "proposals")
            flags = ASF_PROPOSALS;
        else if (STR_TOLOWER(params[0].get_str()) == "agreements")
            flags = ASF_AGREEMENTS;
        else
            throw runtime_error("Incorrect search keyword used\n");
    }
	
    if (params.size() == 2)
        filtertxid = Parseuint256((char *)params[1].get_str().c_str());

    return(AgreementList(mypk,flags,filtertxid));
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
	{ "agreements",  "agreementinfo",    &agreementinfo,	true },
	{ "agreements",  "agreementeventlog",&agreementeventlog,true },
	{ "agreements",  "agreementreferences",   &agreementreferences,   true },
	{ "agreements",  "agreementlist",    &agreementlist,    true },
};

void RegisterAgreementsRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
