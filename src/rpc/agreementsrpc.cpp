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
	std::string name;
	int64_t payment, arbitratorfee, deposit;
    if (fHelp || params.size() < 4 || params.size() > 9)
        throw runtime_error(
            "agreementcreate \"name\" datahash \"client\" \"arbitrator\" ( arbitratorfee payment deposit prevproposaltxid refagreementtxid )\n"
            "\nCreate a new agreement proposal transaction and return the raw hex. The agreement will be fully set up once this proposal is\n"
            "accepted by the owner of the designated recipient pubkey.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. \"name\"        (string, required) Name of the proposed agreement. (max 64 characters)\n"
            "2. datahash     (uint256, required) Field for arbitrary SHA256 hash, can be used to store a fingerprint of\n"
            "                                    a digital document or to reference a transaction in the blockchain.\n"
            "3. \"client\"      (string, required) Pubkey of proposal's intended recipient. If set to \"\" or 0, a proposal draft will be created.\n"
            "4. \"arbitrator\"  (string, required) Pubkey of proposed arbitrator for the agreement. If set to \"\" or 0, the agreement will have no\n"
            "                                    arbitrator.\n"
            "5. arbitratorfee   (numeric, optional, default=0) Fee that will be required to allocate to the arbitrator in order to create a dispute\n"
            "                                                  for the proposed agreement. If no arbitrator is set, always resets to 0, otherwise must\n"
            "                                                  be set to at least 10000 satoshis.\n"
            "6. payment         (numeric, optional, default=0) If set, recipient will have to send this amount of funds to the sender in order to\n"
            "                                                  accept this proposal successfully.\n"
            "7. deposit         (numeric, optional, default=0) Amount that the intended recipient will have to allocate to the agreement global address\n"
            "                                                  for deposit in order to accept this proposal successfully. If arbitrator is set, this\n"
            "                                                  must be set to at least 10000 satoshis.\n"
            "8. prevproposaltxid (uint256, optional) Transaction id of a previous open proposal (draft) to create an agreement by the same\n"
            "                                        sender pubkey. If set, this proposal will supersede the one specified here.\n"
            "9. refagreementtxid (uint256, optional) Transaction id of another agreement in the blockchain that shares at least one member pubkey with\n"
            "                                        the proposed agreement. If set, the proposed agreement will be a subcontract under the agreement\n"
            "                                        specified here."
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementcreate", "\"short draft with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 0 0")
            + HelpExampleCli("agreementcreate", "\"complex agreement with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 \"0237b502085b2552ae4ac6b2b9faf8b215b34a540ecdb5e0b22d2d3b82219a0aea\" \"0312b7f892c33da8fefbc5db6243d30c063031140fe0a130250aa79c66f8124b42\" 10000 10000 10000 b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607 56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a")
            + HelpExampleRpc("agreementcreate", "\"short draft with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 0 0")
            + HelpExampleRpc("agreementcreate", "\"complex agreement with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 \"0237b502085b2552ae4ac6b2b9faf8b215b34a540ecdb5e0b22d2d3b82219a0aea\" \"0312b7f892c33da8fefbc5db6243d30c063031140fe0a130250aa79c66f8124b42\" 10000 10000 10000 b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607 56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a")
        );
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    Lock2NSPV(mypk);
	
    name = params[0].get_str();
    if (name.size() == 0 || name.size() > 64)
    {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement name must not be empty and up to 64 characters\n");
    }
    datahash = Parseuint256((char *)params[1].get_str().c_str());
	if (datahash == zeroid)
    {
		Unlock2NSPV(mypk);
        throw runtime_error("Data hash empty or invalid\n");
    }
	std::vector<unsigned char> client(ParseHex(params[2].get_str().c_str()));
	std::vector<unsigned char> arbitrator(ParseHex(params[3].get_str().c_str()));
    arbitratorfee = 0;
	if (params.size() >= 5)
    {
		arbitratorfee = atoll(params[4].get_str().c_str());
        if (arbitratorfee != 0 && arbitratorfee < 10000)
        {
			Unlock2NSPV(mypk);
			throw runtime_error("Arbitrator fee too low\n");
		}
    }
	payment = 0;
	if (params.size() >= 6)
    {
		payment = atoll(params[5].get_str().c_str());
		if (payment != 0 && payment < 10000)
        {
			Unlock2NSPV(mypk);
			throw runtime_error("Prepayment too low\n");
		}
    }
	deposit = 0;
	if (params.size() >= 7)
    {
		deposit = atoll(params[6].get_str().c_str());
        if (deposit != 0 && deposit < 10000)
        {
			Unlock2NSPV(mypk);
			throw runtime_error("Deposit too low\n");
		}
    }
	prevproposaltxid = refagreementtxid = zeroid;
	if (params.size() >= 8)
        prevproposaltxid = Parseuint256((char *)params[7].get_str().c_str());
	if (params.size() == 9)
        refagreementtxid = Parseuint256((char *)params[8].get_str().c_str());
	result = AgreementCreate(mypk, 0, name, datahash, client, arbitrator, payment, arbitratorfee, deposit, prevproposaltxid, refagreementtxid);
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
        throw runtime_error(
            "agreementstopproposal proposaltxid\n"
            "\nCreate a proposal closure transaction and return the raw hex. The creator of this transaction must be either the creator or recipient of\n"
            "the proposal being closed for this RPC to be executed successfully.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. proposaltxid    (uint256, required) Transaction id of the proposal.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementstopproposal", "e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
            + HelpExampleRpc("agreementstopproposal", "e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
            );
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
        throw runtime_error(
            "agreementaccept proposaltxid\n"
            "\nCreate a proposal acceptance transaction and return the raw hex. The creator of this transaction must be the recipient of\n"
            "the proposal being accepted for this RPC to be executed successfully.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. proposaltxid    (uint256, required) Transaction id of the proposal.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementaccept", "e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
            + HelpExampleRpc("agreementaccept", "e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
            );
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
	std::string name;
	int64_t payment, arbitratorfee;
    if (fHelp || params.size() < 3 || params.size() > 6)
        throw runtime_error(
            "agreementupdate agreementtxid \"name\" datahash ( payment prevproposaltxid arbitratorfee )\n"
            "\nCreate an agreement update proposal transaction and return the raw hex. The agreement will be updated once this proposal is\n"
            "accepted by the owner of the designated recipient pubkey.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. agreementtxid (uint256, required) Transaction id of the agreement to be updated.\n"
            "2. \"name\"        (string, required) New name of the proposed agreement. (max 64 characters)\n"
            "3. datahash      (uint256, required) Field for arbitrary SHA256 hash, can be used to store a fingerprint of\n"
            "                                     a digital document or to reference a transaction in the blockchain.\n"
            "4. payment      (numeric, optional, default=0) If set, recipient will have to send this amount of funds to the sender in order to\n"
            "                                                  accept this proposal successfully.\n"
            "5. prevproposaltxid (uint256, optional) Transaction id of a previous open proposal to update an agreement by the same\n"
            "                                        sender pubkey. If set, this proposal will supersede the one specified here.\n"
            "6. arbitratorfee   (numeric, optional, default=0) If set, this will be the new fee that will be required to allocate to the\n"
            "                                                  arbitrator in order to create a dispute for the proposed agreement. If no\n"
            "                                                  arbitrator is set, always resets to 0, otherwise is set to the current arbitrator\n"
            "                                                  fee unless another amount is defined here (must be at least 10000 satoshis)."
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementupdate", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"short draft with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
            + HelpExampleCli("agreementupdate", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"complex agreement with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 10000 b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607 10001")
            + HelpExampleRpc("agreementupdate", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"short draft with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
            + HelpExampleRpc("agreementupdate", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"complex agreement with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 10000 b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607 10001")
        );
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    Lock2NSPV(mypk);
	
	agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement id invalid\n");
    }
	name = params[1].get_str();
    if (name.size() == 0 || name.size() > 64) {
		Unlock2NSPV(mypk);
        throw runtime_error("Update request name must not be empty and up to 64 characters\n");
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
	result = AgreementUpdate(mypk, 0, agreementtxid, name, datahash, payment, prevproposaltxid, arbitratorfee);
    if (result[JSON_HEXTX].getValStr().size() > 0)
        result.push_back(Pair("result", "success"));
    Unlock2NSPV(mypk);
    return(result);
}

UniValue agreementclose(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ);
	uint256 datahash, prevproposaltxid, agreementtxid;
	std::string name;
	int64_t payment, depositcut;
    if (fHelp || params.size() < 3 || params.size() > 6)
        throw runtime_error(
            "agreementclose agreementtxid \"name\" datahash ( depositcut payment prevproposaltxid )\n"
            "\nCreate an agreement closure proposal transaction and return the raw hex. The agreement will be closed once this proposal is\n"
            "accepted by the owner of the designated recipient pubkey.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. agreementtxid (uint256, required) Transaction id of the agreement to be closed.\n"
            "2. \"name\"        (string, required) Name or info describing the reason for closure. (max 64 characters)\n"
            "3. datahash      (uint256, required) Field for arbitrary SHA256 hash, can be used to store a fingerprint of\n"
            "                                     a digital document or to reference a transaction in the blockchain.\n"
            "4. depositcut   (numeric, optional, default=0) The amount taken from the deposit that will be sent to the sender if the\n"
            "                                               agreement is closed. The rest of the deposit will be given to the recipient.\n"
            "5. payment      (numeric, optional, default=0) If set, recipient will have to send this amount of funds to the sender in order to\n"
            "                                               accept this proposal successfully.\n"
            "6. prevproposaltxid (uint256, optional) Transaction id of a previous open proposal to close an agreement by the same\n"
            "                                        sender pubkey. If set, this proposal will supersede the one specified here.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementclose", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"short draft with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
            + HelpExampleCli("agreementclose", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"complex agreement with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 10000 10000 b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607")
            + HelpExampleRpc("agreementclose", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"short draft with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
            + HelpExampleRpc("agreementclose", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"complex agreement with info\" e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4 10000 10000 b8be8288b85f24b0f48c5eaf46125cc7703a215f38521b32d2b3cba060961607")
        );
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    Lock2NSPV(mypk);
	
	agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement id invalid\n");
    }
	name = params[1].get_str();
    if (name.size() == 0 || name.size() > 64) {
		Unlock2NSPV(mypk);
        throw runtime_error("Close request info must not be empty and up to 64 characters\n");
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
	result = AgreementClose(mypk, 0, agreementtxid, name, datahash, depositcut, payment, prevproposaltxid);
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
        throw runtime_error(
            "agreementdispute agreementtxid datahash\n"
            "\nCreate an agreement dispute transaction and return the raw hex. This transaction will cost the sender a fee equal to\n"
            "the latest arbitrator fee defined for the agreement. Only available if the agreement has an arbitrator.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. agreementtxid (uint256, required) Transaction id of the agreement to be disputed.\n"
            "2. datahash      (uint256, required) Field for arbitrary SHA256 hash, can be used to store a fingerprint of\n"
            "                                     a digital document or to reference a transaction in the blockchain.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementdispute", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
            + HelpExampleRpc("agreementdispute", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
        );
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
        throw runtime_error(
            "agreementresolve agreementtxid \"rewardedpubkey\"\n"
            "\nCreate an agreement dispute resolution transaction and return the raw hex. Only available to the arbitrator\n"
            "of the agreement. Sends the deposit to the chosen pubkey and retrieves the arbitrator fee from the dispute\n"
            "transaction, sending it to the arbitrator's wallet. This transaction will permanently close the agreement.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. agreementtxid    (uint256, required) Transaction id of the agreement to be resolved.\n"
            "2. \"rewardedpubkey\" (string, required) Pubkey to send the deposit to.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementresolve", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"0237b502085b2552ae4ac6b2b9faf8b215b34a540ecdb5e0b22d2d3b82219a0aea\"")
            + HelpExampleRpc("agreementresolve", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"0237b502085b2552ae4ac6b2b9faf8b215b34a540ecdb5e0b22d2d3b82219a0aea\"")
        );
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
	uint256 pawnshoptxid, agreementtxid;
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "agreementunlock agreementtxid pawnshoptxid\n"
            "\nCreate an agreement deposit unlock transaction and return the raw hex. Sends the deposit to the chosen\n"
            "Pawnshop instance escrow, and refunds any excess funds to the agreement client pubkey.\n"
            "Requires Pawnshop CC (and by extension Tokens CC) to be enabled for this RPC to work.\n"
            "Only available to the coin supplier of the Pawnshop instance, as long as it has this agreementtxid defined\n"
            "in its create transaction, has the deposit unlock requirement flag set and is able to have its required coin\n"
            "balance met by sending some or all of the deposit to the escrow.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. agreementtxid (uint256, required) Transaction id of the agreement to have its deposit unlocked.\n"
            "2. pawnshoptxid  (uint256, required) Transaction id of the Pawnshop instance.\n"
            "\nResult:\n"
            "\"result\"  (string) Whether this RPC was executed successfully.\n"
            "\"hex\"  (string) The signed raw transaction hex which can be broadcasted using the sendrawtransaction rpc.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementunlock", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
            + HelpExampleRpc("agreementunlock", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a e4815ed5db07f4ee56cd657d41df1022a7b4a169939d51cd28d66a393895b2c4")
        );
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 || ensure_CCrequirements(EVAL_PAWNSHOP) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);

    Lock2NSPV(mypk);
	
	agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	if (agreementtxid == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Agreement id invalid\n");
    }
    pawnshoptxid = Parseuint256((char *)params[1].get_str().c_str());
	if (pawnshoptxid == zeroid) {
		Unlock2NSPV(mypk);
        throw runtime_error("Pawnshop id invalid\n");
    }
	result = AgreementUnlock(mypk, 0, agreementtxid, pawnshoptxid);
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
            "\nReturns info about any Agreements CC transaction.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. txid (uint256, required) ID of an Agreements CC transaction.\n"
            "\nResult:\n"
            "\"result\"  (string) Info about the transaction.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementinfo", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a")
            + HelpExampleRpc("agreementinfo", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a")
        );
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
        throw runtime_error(
            "agreementupdatelog agreementtxid start_backwards ( num_samples )\n"
            "\nReturns array of agreement update transaction ids for the specified agreement transaction id.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. agreementtxid (uint256, required) Valid agreement transaction id.\n"
            "2. start_backwards (numeric, required) Whether or not to sort ids from latest to oldest.\n"
            "3. num_samples (numeric, optional, default=0) Max amount of ids to retrieve. If 0, returns all ids.\n"
            "\nResult:\n"
            "\"result\"  (array of strings) Transaction ids of accepted agreement updates.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementupdatelog", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a 1 6")
            + HelpExampleRpc("agreementupdatelog", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a 1 6")
        );
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
        throw runtime_error(
            "agreementinventory ( \"pubkey\" )\n"
            "Returns three arrays (one for seller, client and arbitrator) of agreement transaction ids that the\n"
            "specified pubkey is a member of.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. \"pubkey\" (string, optional, default=mypk) Pubkey to check for. If unset, pubkey used to launch\n"
            "the Komodo daemon is passed.\n"
            "\nResult:\n"
            "\"result\"  (json object) The agreement transaction ids that the pubkey is a member of.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementinventory", "\"0237b502085b2552ae4ac6b2b9faf8b215b34a540ecdb5e0b22d2d3b82219a0aea\"")
            + HelpExampleRpc("agreementinventory", "\"0237b502085b2552ae4ac6b2b9faf8b215b34a540ecdb5e0b22d2d3b82219a0aea\"")
        );
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
        throw runtime_error(
            "agreementproposals ( agreementtxid \"pubkey\" )\n"
            "\nReturns three arrays (one for seller, client and arbitrator) of agreement proposal transaction ids\n"
            "that the specified pubkey is referenced in.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. agreementtxid (uint256, optional) Valid agreement transaction id. If set, will filter out proposals\n"
            "unrelated to this agreement.\n"
            "2. \"pubkey\" (string, optional, default=mypk) Pubkey to check for. If unset, pubkey used to launch\n"
            "the Komodo daemon is passed.\n"
            "\nResult:\n"
            "\"result\"  (json object) The agreement proposal transaction ids that the pubkey is referenced in.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementproposals", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"0237b502085b2552ae4ac6b2b9faf8b215b34a540ecdb5e0b22d2d3b82219a0aea\"")
            + HelpExampleRpc("agreementproposals", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a \"0237b502085b2552ae4ac6b2b9faf8b215b34a540ecdb5e0b22d2d3b82219a0aea\"")
        );
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
        throw runtime_error(
            "agreementsubcontracts agreementtxid\n"
            "\nReturns array of agreement transaction ids that reference the specified agreement transaction id\n"
            "as the master agreement.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. agreementtxid (uint256, required) Valid agreement transaction id.\n"
            "\nResult:\n"
            "\"result\"  (array of strings) Transaction ids of subcontracts.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementsubcontracts", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a")
            + HelpExampleRpc("agreementsubcontracts", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a")
        );
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
    return(AgreementSubcontracts(agreementtxid));
}

UniValue agreementsettlements(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 agreementtxid;
    if ( fHelp || params.size() != 2 )
        throw runtime_error(
            "agreementsettlements agreementtxid active_only\n"
            "\nReturns array of Pawnshop transaction ids that reference the specified agreement transaction id.\n"
            "Note: Unlike the other Agreements RPCs, this RPC will only return non-empty arrays\n"
            "for members of this agreement (not including the arbitrator).\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. agreementtxid (uint256, required) Valid agreement transaction id.\n"
            "2. active_only (numeric, required) if set, filters out closed Pawnshop instances.\n"
            "\nResult:\n"
            "\"result\"  (array of strings) Transaction ids of settlements.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementsettlements", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a 1")
            + HelpExampleRpc("agreementsettlements", "56b9bae388690d42fb13c7431d935acbda209bdafa239531549ab4de4b20802a 1")
        );
    if ( ensure_CCrequirements(EVAL_AGREEMENTS) < 0 || ensure_CCrequirements(EVAL_PAWNSHOP) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
	std::string typestr;
	bool bActiveOnly;
    agreementtxid = Parseuint256((char *)params[0].get_str().c_str());
	typestr = params[1].get_str();
    if (STR_TOLOWER(typestr) == "1" || STR_TOLOWER(typestr) == "true")
        bActiveOnly = true;
    else if (STR_TOLOWER(typestr) == "0" || STR_TOLOWER(typestr) == "false")
        bActiveOnly = false;
    else 
        throw runtime_error("active_only flag invalid or empty\n");

    return(AgreementSettlements(mypk, agreementtxid, bActiveOnly));
}

UniValue agreementlist(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    if ( fHelp || params.size() > 0 )
        throw runtime_error(
            "agreementlist\n"
            "\nReturns array of every active proposal and agreement transaction id in the blockchain.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "none\n"
            "\nResult:\n"
            "\"result\"  (array of strings) Transaction ids of active proposals and agreements.\n"
            "\nExamples:\n"
            + HelpExampleCli("agreementlist", "")
            + HelpExampleRpc("agreementlist", "")
        );
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
