/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
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

#include "CCagreements.h"

/*

Note: version numbers should be reset after contract acceptance

Agreements transaction types:
	
	'p' - agreement proposal (possibly doesn't have CC inputs):
	vins.* normal input
	vin.n-1 previous proposal baton (optional)
		CC input - unlocks validation
	vout.0 marker
		can't be spent (most likely)
		sent to global address
	vout.1 response hook
		can be spent by 'p', 'c' or 'u' transactions
		sent to seller/buyer 1of2 address
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'p'
		proposaltype (proposal create, proposal update, contract update, contract cancel)
		datahash
		initiatorpubkey
		receiverpubkey (can't be changed in subsequent updates, enforced by validation)
		[description]
		[mediatorpubkey]
		[deposit] (can't be changed if proposal was accepted)
		[mediatorfee](can't be changed if proposal was accepted)
		[agreementtxid]
		[prevproposaltxid]
		[depositsplit](only if tx is a contract cancel request)
	
	'c' - proposal acceptance and contract creation:
	vins.* normal input
	vin.n-1 latest proposal by seller
	vout.0 marker
		can't be spent (most likely)
		sent to global address
	vout.1 update baton
		can be spent by 'u' transactions, provided they also spend the appropriate 'p' transaction
		sent to seller/buyer 1of2 address
	vout.2 seller dispute baton
		can be spent by 'd' transactions
		sent to seller CC address
	vout.3 buyer dispute baton
		can be spent by 'd' transactions
		sent to buyer CC address
	vout.4 invoice hook (can also be deposit)
		sent to agreements global CC address
		if no mediator:
			can be spent by a Settlements Payment transaction
		if mediator exists:
			can be spent by a Settlements Payment transaction (if buyer) or a 'r' transaction (if mediator)
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'c'
		proposaltxid
		sellerpubkey
		buyerpubkey
	
	'u' - contract update:
	vins.* normal input
	vin.n-1 latest proposal by other party
	vout.0 marker
		can't be spent (most likely)
		sent to global address
	vout.1 next update baton
		can be spent by 'u' transactions, provided they also spend the appropriate 'p' transaction
		sent to seller/buyer 1of2 address
	vout.2 deposit split to party 1
		sent to party 1 normal address
	vout.3 deposit split to party 2
		sent to party 2 normal address
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'u'
		lastupdatetxid
		updateproposaltxid
		type (contract update, contract cancel)
	
	'd' - contract dispute:
	vins.* normal input
	vin.n-1 previous dispute by disputer
	vout.0 marker
		can't be spent (most likely)
		sent to global address
	vout.1 next dispute baton
		can be spent by 'd' transactions
		sent to disputer CC address
	vout.2 response hook (can also be mediator fee)
		can be spent by 'r' transactions
		if no mediator:
			sent to disputer CC address
		if mediator exists:
			sent to mediator CC address
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'd'
		lastdisputetxid(is this needed?)
		disputetype(light, heavy)
		initiatorpubkey(is this needed?)
		receiverpubkey(is this needed?)
		[description]
		[disputehash]
		
	'r' - dispute resolve:
	vins.* normal input
	vin.n-1 dispute that is being resolved
	vout.0 marker
		can't be spent (most likely)
		sent to global address
	vout.1 mediator fee OR change
		if no mediator:
			sent to disputer CC address
		if mediator exists:
			sent to mediator CC address
	vout.2 deposit redeem
		sent to either party 1 or 2, dependent on mediator
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'r'
		disputetxid
		verdict(closed by disputer, closed by mediator, deposit redeemed)
		[rewardedpubkey]
		[message]
	
Agreements RPCs:
	
	agreementaddress
		Gets relevant agreement CC, normal, global addresses, etc.
	agreementlist
		Gets every marker in the Agreements global address.
	agreementpropose(datahash receiverpubkey [description][mediatorpubkey][deposit][mediatorfee][prevproposaltxid])
		Creates a proposal(update) type transaction, which will need to be confirmed by the buyer.
	agreementupdate(agreementtxid datahash [description][mediatorpubkey][prevproposaltxid])
		Creates a contract update type transaction, which will need to be confirmed by the other party.
	agreementcancel(agreementtxid datahash [description][depositsplit][prevproposaltxid])
		Creates a contract cancel type transaction, which will need to be confirmed by the other party.
	agreementaccept(proposaltxid)
		Accepts a proposal type transaction. 
		This RPC is context aware:
			if the 'p' tx is a non-contract proposal, it will create a 'c' tx
			if it is update request, then it will create 'u' tx that is of update type
			if it is cancel request, then it will create 'u' tx that is of cancel type
	agreementdispute(agreementtxid disputetype [description][disputehash])
		Creates a new agreement dispute.
	agreementresolve(agreementtxid disputetxid verdict [rewardedpubkey][message])
		Resolves the specified agreement dispute.
	agreementinfo(txid)
		Retrieves info about the specified Agreements transaction.
		TODO: Add what info will be displayed for each tx
	agreementviewupdates(agreementtxid)
		Retrieves a list of updates for the specified agreement.
	agreementviewdisputes(agreementtxid)
		Retrieves a list of disputes for the specified agreement.
	agreementinventory([pubkey])
		Retrieves every agreement wherein the specified pubkey is the buyer, seller or mediator.
		Can look up both current and past agreements.

*/

//===========================================================================
//
// Opret encoding/decoding functions
//
//===========================================================================

CScript EncodeAgreementCreateOpRet(std::string name, uint256 datahash, std::vector<uint8_t> creatorpubkey, std::vector<uint8_t> clientpubkey, int64_t deposit, int64_t timelock)
{
    CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'n';
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << name << datahash << creatorpubkey << clientpubkey << deposit << timelock);
    return(opret);
}

uint8_t DecodeAgreementCreateOpRet(CScript scriptPubKey, std::string &name, uint256 &datahash, std::vector<uint8_t> &creatorpubkey, std::vector<uint8_t> &clientpubkey, int64_t &deposit, int64_t &timelock)
{
    std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
    GetOpReturnData(scriptPubKey, vopret);
    if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> name; ss >> datahash; ss >> creatorpubkey; ss >> clientpubkey; ss >> deposit; ss >> timelock) != 0 && evalcode == EVAL_AGREEMENTS)
    {
        return(funcid);
    }
    return(0);
}

//===========================================================================
//
// Validation
//
//===========================================================================

bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	return(eval->Invalid("no validation yet"));
}

//===========================================================================
//
// Helper functions
//
//===========================================================================

int64_t IsAgreementsvout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v)
{
    char destaddr[64];
    if ( tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0 )
    {
        if ( Getscriptaddress(destaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(destaddr,cp->unspendableCCaddr) == 0 )
            return(tx.vout[v].nValue);
    }
    return(0);
}

//===========================================================================
//
// RPCs
//
//===========================================================================

UniValue AgreementCreate(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> clientpubkey, int64_t deposit, int64_t timelock)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
    struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	
    if ( txfee == 0 )
        txfee = 10000;
    mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "not done yet");
	
    /*for(auto txid : bindtxids)
    {
        if (myGetTransaction(txid,tx,hashBlock)==0 || (numvouts=tx.vout.size())<=0)
            CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cant find bindtxid " << txid.GetHex());
        if (DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tmptokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2,wiftype)!='B')
            CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "invalid bindtxid " << txid.GetHex());
    }
	
    if ( AddNormalinputs(mtx,mypk,amount,64,pk.IsValid()) >= txfee + deposit )
    {
        for (int i=0; i<100; i++) mtx.vout.push_back(MakeCC1vout(EVAL_PEGS,(amount-txfee)/100,pegspk));
        return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodePegsCreateOpRet(bindtxids)));
    }
	
    CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");*/
}

/*
UniValue AgreementCreate();
	creates new proposals, creator becomes seller
UniValue AgreementAccept();
	accepts proposals and turns them into contracts, only done by designated client

//Requests and related
UniValue AgreementRequestUpdate();
	creates an update request, only needed in contract phase
UniValue AgreementUpdate();
	updates an agreement, if in contract phase you also need request from other party
UniValue AgreementRequestCancel();
UniValue AgreementCancel();


UniValue AgreementInfo(agreementtxid);
	lists info about specific agreement (TODO: what info?)
UniValue AgreementList();
	lists all agreements/proposals/contracts (canceled ones may be omitted?)
(?)UniValue AgreementInventory();

*/
