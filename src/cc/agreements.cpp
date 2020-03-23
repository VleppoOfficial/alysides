/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.							 *
 *																			*
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at				  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *																			*
 * Unless otherwise agreed in a custom licensing agreement, no part of the	*
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *																			*
 * Removal or modification of this copyright notice is prohibited.			*
 *																			*
 ******************************************************************************/

#include "CCagreements.h"

/*

Put all notes here!


don't allow swapping between no arbitrator <-> arbitrator
only 1 update/cancel request per party, per agreement
version numbers should be reset after contract acceptance
fix agreementcloseproposal not detecting outdated proposals
merge deposit and depositcut into depositvalue?
merge update and dispute batons into one?
keep closed proposal markers on!
Note: limit amount of CC inputs for each type of transaction

Agreements transaction types:
	
	case 'p':
		agreement proposal:
		vin.0 normal input
		vin.1 previous proposal marker
		vin.2 previous proposal baton
		vout.0 marker
		vout.1 response hook
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 'p' proposaltype initiator receiver arbitrator prepayment arbitratorfee depositvalue datahash agreementtxid prevproposaltxid name

	case 't':
		proposal cancel:
		vin.0 normal input
		vin.1 previous proposal baton
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 't' proposaltxid initiator
	
	case 'c':
		contract creation:
		vin.0 normal input
		vin.1 latest proposal by seller
		vout.0 marker
		vout.1 update/dispute baton
		vout.2 deposit / agreement completion baton
		vout.3 prepayment
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 'c' proposaltxid
	
	case 'u':
		contract update:
		vin.0 normal input
		vin.1 last update baton
		vin.2 latest proposal by other party
		vout.0 next update baton
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 'u' agreementtxid proposaltxid
	
	case 's':
		contract close:
		vin.0 normal input
		vin.1 last update baton
		vin.2 latest termination proposal by other party
		vin.3 deposit
		vout.0 deposit split to party 1
		vout.1 deposit split to party 2
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 's' agreementtxid proposaltxid
	
	case 'd':
		contract dispute:
		vin.0 normal input
		vin.1 last update baton
		vout.0 response hook / arbitrator fee
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 'd' agreementtxid initiator disputehash
	
	case 'r':
		contract dispute resolve:
		vin.0 normal input
		vin.1 dispute resolved w/ fee
		vin.2 deposit
		vout.0 deposit redeem
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 'r' disputetxid rewardedpubkey
	
Agreements statuses:
	
	Proposal status:
	draft
	pending
	approved
	closed
	updated
	
	Contract status:
	active
		[revised/expanded]
	request pending
	terminated/cancelled
	completed
		[pending payment]
		[pending asset transfer]
		[pending asset collection]
	suspended/in dispute
	expired (priority over 'suspended')
	
Agreements RPCs:

	agreementpropose (name datahash buyer arbitrator [arbitratorfee][deposit][prevproposaltxid][refagreementtxid])
	agreementrequestupdate(agreementtxid name datahash [newarbitrator][prevproposaltxid])
	agreementrequestcancel(agreementtxid name datahash [depositsplit][prevproposaltxid])
	Proposal creation
	
	agreementcloseproposal(proposaltxid message)
	agreementaccept(proposaltxid)
	Proposal response
	
	agreementdispute(agreementtxid disputetype [disputehash])
	agreementresolve(agreementtxid disputetxid verdict [rewardedpubkey])
	Disputes
	
	agreementaddress
	agreementlist
	agreementinfo(txid)
	agreementviewupdates(agreementtxid [samplenum][recursive])
	agreementviewdisputes(agreementtxid [samplenum][recursive])
	agreementinventory([pubkey])

*/

// start of consensus code

//===========================================================================
// Opret encoders/decoders
//===========================================================================

//uint8_t DecodeAgreementOpRet()
// just returns funcid of whatever agreement tx opret is fed into it
uint8_t DecodeAgreementOpRet(const CScript scriptPubKey)
{
	std::vector<uint8_t> vopret, dummyInitiator, dummyReceiver, dummyArbitrator;
	int64_t dummyPrepayment, dummyArbitratorFee, dummyDeposit, dummyDepositCut;
	uint256 dummyHash, dummyAgreementTxid, dummyPrevProposalTxid;
	std::string dummyName;
	uint8_t evalcode, funcid, *script, dummyProposalType;
	GetOpReturnData(scriptPubKey, vopret);
	script = (uint8_t *)vopret.data();
	if(script != NULL && vopret.size() > 2) {
		evalcode = script[0];
		if (evalcode != EVAL_AGREEMENTS) {
			LOGSTREAM("agreementscc", CCLOG_DEBUG1, stream << "script[0] " << script[0] << " != EVAL_AGREEMENTS" << std::endl);
			return (uint8_t)0;
		}
		funcid = script[1];
		LOGSTREAM((char *)"agreementscc", CCLOG_DEBUG2, stream << "DecodeAgreementOpRet() decoded funcId=" << (char)(funcid ? funcid : ' ') << std::endl);
		switch (funcid) {
		case 'p':
			return DecodeAgreementProposalOpRet(scriptPubKey, dummyProposalType, dummyInitiator, dummyReceiver, dummyArbitrator, dummyPrepayment, dummyArbitratorFee, dummyDeposit, dummyDepositCut, dummyHash, dummyAgreementTxid, dummyPrevProposalTxid, dummyName);
		case 't':
			return DecodeAgreementProposalCloseOpRet(scriptPubKey, dummyPrevProposalTxid, dummyInitiator);
		case 'c':
			return DecodeAgreementSigningOpRet(scriptPubKey, dummyPrevProposalTxid);
		//case 'whatever':
			//insert new cases here
		default:
			LOGSTREAM((char *)"agreementscc", CCLOG_DEBUG1, stream << "DecodeAgreementOpRet() illegal funcid=" << (int)funcid << std::endl);
			return (uint8_t)0;
		}
	}
	else
		LOGSTREAM("agreementscc",CCLOG_DEBUG1, stream << "not enough opret.[" << vopret.size() << "]" << std::endl);
	return (uint8_t)0;
}

CScript EncodeAgreementProposalOpRet(uint8_t proposaltype, std::vector<uint8_t> initiator, std::vector<uint8_t> receiver, std::vector<uint8_t> arbitrator, int64_t prepayment, int64_t arbitratorfee, int64_t deposit, int64_t depositcut, uint256 datahash, uint256 agreementtxid, uint256 prevproposaltxid, std::string name)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'p';
	proposaltype = 'p'; //temporary
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << proposaltype << initiator << receiver << arbitrator << prepayment << arbitratorfee << deposit << depositcut << datahash << agreementtxid << prevproposaltxid << name);
	return(opret);
}
uint8_t DecodeAgreementProposalOpRet(CScript scriptPubKey, uint8_t &proposaltype, std::vector<uint8_t> &initiator, std::vector<uint8_t> &receiver, std::vector<uint8_t> &arbitrator, int64_t &prepayment, int64_t &arbitratorfee, int64_t &deposit, int64_t &depositcut, uint256 &datahash, uint256 &agreementtxid, uint256 &prevproposaltxid, std::string &name)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> proposaltype; ss >> initiator; ss >> receiver; ss >> arbitrator; ss >> prepayment; ss >> arbitratorfee; ss >> deposit; ss >> depositcut; ss >> datahash; ss >> agreementtxid; ss >> prevproposaltxid; ss >> name) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

CScript EncodeAgreementProposalCloseOpRet(uint256 proposaltxid, std::vector<uint8_t> initiator)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 't';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << proposaltxid << initiator);
	return(opret);
}
uint8_t DecodeAgreementProposalCloseOpRet(CScript scriptPubKey, uint256 &proposaltxid, std::vector<uint8_t> &initiator)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> proposaltxid; ss >> initiator) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

CScript EncodeAgreementSigningOpRet(uint256 proposaltxid)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'c';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << proposaltxid);
	return(opret);
}
uint8_t DecodeAgreementSigningOpRet(CScript scriptPubKey, uint256 &proposaltxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> proposaltxid) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

CScript EncodeAgreementUpdateOpRet(uint256 agreementtxid, uint256 proposaltxid)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'u';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << agreementtxid << proposaltxid);
	return(opret);
}
uint8_t DecodeAgreementUpdateOpRet(CScript scriptPubKey, uint256 &agreementtxid, uint256 &proposaltxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> agreementtxid; ss >> proposaltxid) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

CScript EncodeAgreementDisputeOpRet(uint256 agreementtxid, uint256 disputehash)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'd';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << agreementtxid << disputehash);
	return(opret);
}
uint8_t DecodeAgreementDisputeOpRet(CScript scriptPubKey, uint256 &agreementtxid, uint256 &disputehash)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> agreementtxid; ss >> disputehash) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

CScript EncodeAgreementDisputeResolveOpRet(uint256 agreementtxid, uint256 disputetxid, std::vector<uint8_t> rewardedpubkey)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'r';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << agreementtxid << disputetxid << rewardedpubkey);
	return(opret);
}
uint8_t DecodeAgreementDisputeResolveOpRet(CScript scriptPubKey, uint256 &agreementtxid, uint256 &disputetxid, std::vector<uint8_t> &rewardedpubkey)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> agreementtxid; ss >> disputetxid; ss >> rewardedpubkey) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

//===========================================================================
// Validation
//===========================================================================

bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	
/// How to validate agreements transactions:

/// generic:
/*
		check boundaries
		get funcid (DecodeAgreementOpRet). If funcid unavailable, invalidate until Settlements is finished.
*/
/// 	- IMPORTANT!!! - deposit is locked under EVAL_AGREEMENTS, but it may also be spent by a specific Settlements transaction. Make sure this code is able to check for this when Settlements is ready.

/// case 'p':
///		- Proposal data validation.
/*	
		use CheckProposalData(tx)
		use DecodeAgreementProposalOpRet, get proposaltype, initiator, receiver, prevproposaltxid
*/
///		- Checking if selected proposal was already spent.
/*
		if prevproposaltxid != zeroid:
			CheckIfProposalSpent(prevproposaltxid)
		else
			if we're in validation code, invalidate
*/
///		- Comparing proposal data to previous proposals.
/*
		while prevproposaltxid != zeroid:
			get prevproposal tx
			CheckProposalData(prevproposaltx)
			use DecodeAgreementProposalOpRet, get proposaltype, initiator, receiver, prevx2proposaltxid
			check if proposal types match. else, invalidate
			check if initiator pubkeys match. else, invalidate
			prevproposaltxid = prevx2proposaltxid
			Loop
*/	
///		- Checking if our inputs/outputs are correct.
/*
		check vout0:
			does it point to the agreements global address?
			is it the correct value?
		check vout1:
			if receiver undefined:
				does vout1 point to the initiator's address?
					does initiatorpubkey point to the same privkey as the address?
				is the value correct?
			if receiver defined:
				does vout1 point to a 1of2 address between initiator and receiver?
					does initiatorpubkey point to the same privkey as the initiator's address?
					does receiverpubkey point to the same privkey as the receiver's address?
				is the value correct?
		check vin1:
			does it point to the previous proposal's marker? (if it doesn't, it's probably an original proposal, which isn't validated here)
		check vin2:
			does it point to the previous proposal's response? (same as above)
		Do not allow any additional CC vins.
		break;
*/	
/// case 't':
///		- Getting the transaction data.
/*
		use DecodeAgreementProposalCloseOpRet, get initiator, proposaltxid
		use TotalPubkeyNormalInputs to verify that initiator == pubkey that submitted tx
*/
///		- Checking if selected proposal was already spent.
/*
		if proposaltxid != zeroid:
			CheckIfProposalSpent(proposaltxid)
		else
			if we're in validation code, invalidate
*/	
///		- Retrieving the proposal data tied to this transaction.
/*
		get ref proposal tx
*/	
///		- We won't be using CheckProposalData here, since we only care about initiator and receiver, and even if the other data in the proposal is invalid the proposal will be closed anyway
///		- Instead we'll just check if the initiator of this tx is allowed to close the proposal.
/*	
		use DecodeAgreementProposalOpRet on ref proposal tx, get proposaltype, refinitiator, refreceiver, refagreementtxid
		if proposaltype = 'p' 
			get status of receiver - is it valid or null?
			if receiver is null:
				check if initiator pubkey matches with ref initiator. else, invalidate
			else
				check if initiator pubkey matches with either ref initiator or receiver. else, invalidate
		else if proposaltype = 'u' or 't'
			if receiver doesn't exist, invalidate.
			check if refagreementtxid is defined. If not, invalidate
			CheckIfInitiatorValid(initiator, agreementtxid)
		else
			invalidate
*/	
///		- Checking if our inputs/outputs are correct.
/*
		check vout0:
			is it a normal output (change)?
		check vout1:
			is it the last vout? if not, invalidate
		check vin1:
			does it point to the proposal's response?
		Do not allow any additional CC vins.
		break;
*/	
/// case 'c':
///		- Getting the transaction data.
/*
		use DecodeAgreementProposalSigningOpRet, get proposaltxid
*/
///		- Proposal data validation.
/*	
		get proposal tx
		use CheckProposalData(proposaltx)
		use DecodeAgreementProposalOpRet, get proposaltype, initiator, receiver, prevproposaltxid, prepayment, depositvalue
		if proposaltype != 'p', invalidate
		if refreceiver == null, invalidate
		else, use TotalPubkeyNormalInputs to verify that receiver == pubkey that submitted tx
*/
///		- Checking if selected proposal was already spent.
/*
		CheckIfProposalSpent(proposaltxid)
*/
///		- Checking if our inputs/outputs are correct.
/*
		check vout0:
			does it point to the agreements global address?
			is it the correct value?
		check vout1:
			does vout1 point to a 1of2 address between initiator and receiver?
				does initiatorpubkey point to the same privkey as the initiator's address?
				does receiverpubkey point to the same privkey as the receiver's address?
			is the value correct?
		check vout2:
			does vout2 point to a 1of2 address between receiver and arbitrator?
				does receiverpubkey point to the same privkey as the receiver's address?
				does arbitratorpubkey point to the same privkey as the arbitrator's address?
			is the value equal to the deposit specified in the proposal opret?
		check vout3:
			is it a normal output?
			does vout3 point to the initiator's address?
			is the vout3 value equal to the prepayment specified in the proposal opret?
		check vin1:
			does vin1 point to the proposal's marker?
		check vin2:
			does vin2 point to the proposal's response?
		Do not allow any additional CC vins.
		break;
*/	
/// case 'u':
///		- Getting the transaction data.
/*
		use DecodeAgreementUpdateOpRet, get proposaltxid, agreementtxid
		use GetLatestUpdateType(agreementtxid)
		if updatefuncid != 'c' or 'u', invalidate
		use GetLatestAgreementUpdate(agreementtxid) to get the latest update txid
*/
///		- Proposal data validation.
/*	
		get proposal tx
		use CheckProposalData(proposaltx)
		use DecodeAgreementProposalOpRet, get proposaltype, initiator, receiver, prepayment
		if proposaltype != 'u', invalidate
		use TotalPubkeyNormalInputs to verify that receiver == pubkey that submitted tx
		CheckIfInitiatorValid(initiator, agreementtxid)
		CheckIfInitiatorValid(receiver, agreementtxid)
*/
///		- Checking if selected proposal was already spent.
/*
		CheckIfProposalSpent(proposaltxid)
*/
///		- Checking if the arbitrator is set up correctly.
/*
		use GetAgreementArbitrator(agreementtxid) to find out if current agreement has arbitrator or not
		if agreement has no arbitrator, and if update has arbitrator set to anything but null, invalidate
		if agreement has arbitrator, and if update has arbitrator set to null:
		if we're in validation, invalidate (if arbitrator was unchanged, it should be left the same)
		else, set arbitrator to current agreement arbitrator
*/
///		- Checking if our inputs/outputs are correct.
/*
		check vout0:
			does it point to a 1of2 address between initiator and receiver?
				does initiatorpubkey point to the same privkey as the initiator's address?
				does receiverpubkey point to the same privkey as the receiver's address?
			is the value correct?
		check vin1:
			does it point to the previous update baton?
		check vin2:
			does it point to the proposal's marker?
		check vin3:
			does it point to the proposal's response?
		Do not allow any additional CC vins.
		break;
*/
/// case 's':
///		- Getting the transaction data.
/*
		use DecodeAgreementCloseOpRet, get proposaltxid, agreementtxid
		use GetLatestUpdateType(agreementtxid)
		if updatefuncid != 'c' or 'u', invalidate
		use GetLatestAgreementUpdate(agreementtxid) to get the latest update txid
		totaldeposit = GetDepositValue(agreementtxid)
*/
///		- Proposal data validation.
/*	
		get proposal tx
		use CheckProposalData(proposaltx)
		use DecodeAgreementProposalOpRet, get proposaltype, initiator, receiver, prepayment, deposit amount
		if proposaltype != 't', invalidate
		use TotalPubkeyNormalInputs to verify that receiver == pubkey that submitted tx
		CheckIfInitiatorValid(initiator, agreementtxid)
		CheckIfInitiatorValid(receiver, agreementtxid)
*/
///		- Checking if selected proposal was already spent.
/*
		CheckIfProposalSpent(proposaltxid)
*/
///		- Checking if our inputs/outputs are correct.
/*
		do the values of vout0+vout1 equal the deposit amount?
		check vout0:
			does vout0 point to the initiator's pubkey and is normal input?
				is it equal to depositcut?
		check vout1:
			does vout1 point to the receiver's pubkey and is normal input?
				is it equal to totaldeposit - depositcut?
		check vin1:
			does it point to the previous update baton?
		check vin2:
			does it point to the proposal's marker?
		check vin3:
			does it point to the proposal's response?
		check vin4:
			does it point to the agreement deposit?
		Do not allow any additional CC vins.
		break;
*/
/// case 'd':
///		- Getting and verifying the transaction data.
/*
		use DecodeAgreementDisputeOpRet, get initiator, agreementtxid, disputehash
		use GetLatestUpdateType(agreementtxid)
		if updatefuncid != 'c' or 'u', invalidate
		use GetLatestAgreementUpdate(agreementtxid) to get the latest update txid
		check if disputehash is empty. Invalidate if it is.
		use TotalPubkeyNormalInputs to verify that initiator == pubkey that submitted tx
		CheckIfInitiatorValid(initiator, agreementtxid)
*/
///		- Some arbitrator shenanigans.
/*	
		use GetAgreementArbitrator(agreementtxid) to get the current arbitrator
		if one doesn't exist, invalidate (disputes are disabled when there are no arbitrators)
		arbitratorfee = GetArbitratorFee(agreementtxid)
*/
///		- Checking if our inputs/outputs are correct.
/*
		check vout0:
			does it point to the arbitrator's address?
				does arbitratorpubkey point to the same privkey as the arbitrator's address?
			is the value == arbitratorfee?
		check vin1:
			does it point to the previous update baton?
		Do not allow any additional CC vins.
		break;
*/
/// case 'r':
///		- Getting and verifying the transaction data.
/*
		use DecodeAgreementDisputeResolveOpRet, get disputetxid, rewardedpubkey
		use DecodeAgreementDisputeOpRet, get agreementtxid, initiator
		use GetLatestUpdateType(agreementtxid)
		if updatefuncid != 'c' or 'u', invalidate
		use GetLatestAgreementUpdate(agreementtxid) to get the latest update txid
		CheckIfInitiatorValid(rewardedpubkey, agreementtxid)
		deposit = GetDepositValue(agreementtxid)
*/
///		- Some arbitrator shenanigans.
/*	
		use GetAgreementArbitrator(agreementtxid) to get the current arbitrator
		if one doesn't exist, invalidate (disputes are disabled when there are no arbitrators)
		arbitratorfee = GetArbitratorFee(agreementtxid)
		use TotalPubkeyNormalInputs to verify that arbitrator == pubkey that submitted tx
*/
///		- Checking if our inputs/outputs are correct.
/*
		check vout0:
			does it point to the rewardedpubkey's address?
				does rewardedpubkey point to the same privkey as the rewardedpubkey's address?
			is the value == deposit?
		check vin1:
			does it point to the disputetxid / marker?
			is the value == arbitratorfee?
		check vin2:
			does it point to the agreement deposit?
		Do not allow any additional CC vins.
		break;
*/
/// default:
/*
		invalidate
		
	return true;
*/

	// check boundaries:
    //if (numvouts < 1)
    //    return eval->Invalid("no vouts\n");
	
	//return(eval->Invalid("no validation yet"));

	std::cerr << "AgreementsValidate triggered, passing through" << std::endl;
	return true;
}

//===========================================================================
// Helper functions
//===========================================================================

int64_t IsAgreementsVout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v)
{
	char destaddr[64];
	if( tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0 )
	{
		if( Getscriptaddress(destaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(destaddr,cp->unspendableCCaddr) == 0 )
			return(tx.vout[v].nValue);
	}
	return(0);
}
// GetLatestExpirationDate
// GetAgreementData
// etc.
/*
GetLatestDataHash(agreementtxid)
GetAgreementArbitrator(agreementtxid)
GetAgreementName(agreementtxid)
GetProposalVersion(proposaltxid)
GetAgreementUpdateVersion(updatetxid)
char GetLatestUpdateType(agreementtxid) - use this to check updates, if contract was closed or failed due to dispute
GetLatestAgreementUpdate(agreementtxid) 

GetArbitratorFee(agreementtxid)
	get refagreementtx
	use DecodeAgreementSigningOpRet on refagreementtx, get proposaltxid
	get accepted proposal tx
	use DecodeAgreementProposalOpRet on accepted proposal tx, get arbitratorfee
	return arbitratorfee

GetDepositValue(agreementtxid)
	get refagreementtx
	use DecodeAgreementSigningOpRet on refagreementtx, get proposaltxid
	get accepted proposal tx
	use DecodeAgreementProposalOpRet on accepted proposal tx, get deposit
	check agreement vout for deposit value as well?
	return deposit

CheckIfInitiatorValid(initiatorpubkey, agreementtxid)
	get refagreementtx
	use DecodeAgreementSigningOpRet on refagreementtx, get proposaltxid
	get accepted proposal tx
	use DecodeAgreementProposalOpRet on accepted proposal tx, get refinitiator, refreceiver
	check if initiator pubkey matches either refInitiator or refReceiver. If it doesn't, return false.
	return true;
	
CheckProposalData (CTransaction proposaltx):
	decode proposal opret (DecodeAgreementProposalOpRet) to get the data
	check if name meets reqs (not empty, <= 64 chars). Invalidate if it doesn't.
	check if datahash is empty. Invalidate if it is.
	check if prepayment is positive
	use TotalPubkeyNormalInputs to verify that initiator == pubkey that submitted tx
	get status of receiver - is it valid or null?
	get status of arbitrator - is it valid or null?
	if receiver exists:
		make sure initiator != receiver
	if arbitrator exists:
		make sure initiator != arbitrator
	if receiver exists & if arbitrator exists:
		make sure receiver != arbitrator
	check proposaltype
	case 'p':
		check if deposit >= default minimum value
		check if fee >= default minimum value
		check if agreementtxid is defined. If it is:
			get agreement tx and its funcid. If it isn't 'c', invalidate.
	case 'u':
		if receiver doesn't exist, invalidate.
		check if agreementtxid is defined. If not, invalidate
		check if agreement is still active
		CheckIfInitiatorValid(initiator, agreementtxid)
		in this type arbitratorfee and deposit value must be -1. If they aren't, invalidate.
	case 't':
		if receiver doesn't exist, invalidate.
		check if agreementtxid is defined. If not, invalidate
		check if agreement is still active
		CheckIfInitiatorValid(initiator, agreementtxid)
		in this type arbitrator must be null, and arbitratorfee must be -1. If it isn't, invalidate.
		check deposit value - must be between 0 and ref deposit value

CheckIfProposalSpent (proposaltxid):
	use CCgetspenttxid on prevproposaltxid
	if prevproposaltxid was spent:
		if DecodeAgreementOpRet = 'c'
			invalidate with errormsg indicating proposal was accepted
		else if DecodeAgreementOpRet = 'p'
			invalidate with errormsg indicating proposal was amended
		else if DecodeAgreementOpRet = 't'
			invalidate with errormsg indicating proposal was closed
		else
			invalidate with generic errormsg
	return true

*/

//===========================================================================
// RPCs - tx creation
//===========================================================================

// agreementpropose - constructs a 'p' transaction, with the 'p' proposal type
UniValue AgreementPropose(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> buyer, std::vector<uint8_t> arbitrator, int64_t prepayment, int64_t arbitratorfee, int64_t deposit, uint256 prevproposaltxid, uint256 refagreementtxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	CTransaction prevproposaltx, refagreementtx, spenttx;
	int32_t numvouts, vini, height, retcode;
	uint256 hashBlock, refHash, refAgreementTxid, refPrevProposalTxid, spenttxid;
	std::vector<uint8_t> refInitiator, refReceiver, refArbitrator;
	int64_t refPrepayment, refArbitratorFee, refDeposit, refDepositCut;
	std::string refName;
	uint8_t refProposalType;
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if(txfee == 0)
		txfee = 10000;
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	// check name & datahash
	if(name.empty() || name.size() > 64)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement name must not be empty and up to 64 characters");
	if(datahash == zeroid)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Data hash empty or invalid");
	
	// check prepayment
	if (prepayment < 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Prepayment must be positive");
	
	// check if buyer pubkey exists and is valid
	if(!buyer.empty() && !(pubkey2pk(buyer).IsValid()))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Buyer pubkey invalid");
	
	// check if arbitrator pubkey exists and is valid
	if(!arbitrator.empty() && !(pubkey2pk(arbitrator).IsValid()))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Arbitrator pubkey invalid");
	
	// checking if mypk != buyerpubkey != arbitratorpubkey
	if(pubkey2pk(buyer).IsValid() && pubkey2pk(buyer) == mypk)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Seller pubkey cannot be the same as buyer pubkey");
	if(pubkey2pk(arbitrator).IsValid() && pubkey2pk(arbitrator) == mypk)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Seller pubkey cannot be the same as arbitrator pubkey");
	if(pubkey2pk(buyer).IsValid() && pubkey2pk(arbitrator).IsValid() && pubkey2pk(arbitrator) == pubkey2pk(buyer))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Buyer pubkey cannot be the same as arbitrator pubkey");
	
	// if arbitrator exists, check if arbitrator fee is sufficient
	if(pubkey2pk(arbitrator).IsValid()) {
		if(arbitratorfee == 0)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Arbitrator fee must be specified if valid arbitrator exists");
		else if(arbitratorfee < CC_MEDIATORFEE_MIN)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Arbitrator fee is too low");
	}
	else
		arbitratorfee = 0;
	
	// if arbitrator exists, check if deposit is sufficient
	if(pubkey2pk(arbitrator).IsValid()) {
		if(deposit == 0)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Deposit must be specified if valid arbitrator exists");
		else if(deposit < CC_DEPOSIT_MIN)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Deposit is too low");
	}
	else
		deposit = 0;
	
	// check prevproposaltxid if specified
	if(prevproposaltxid != zeroid) {
		if(myGetTransaction(prevproposaltxid,prevproposaltx,hashBlock)==0 || (numvouts=prevproposaltx.vout.size())<=0)
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cant find specified previous proposal txid " << prevproposaltxid.GetHex());
		if(DecodeAgreementOpRet(prevproposaltx.vout[numvouts - 1].scriptPubKey) == 'c')
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "specified txid is a contract id, needs to be proposal id");
		else if(DecodeAgreementProposalOpRet(prevproposaltx.vout[numvouts-1].scriptPubKey,refProposalType,refInitiator,refReceiver,refArbitrator,refPrepayment,refArbitratorFee,refDeposit,refDepositCut,refHash,refAgreementTxid,refPrevProposalTxid,refName) != 'p')
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "invalid proposal txid " << prevproposaltxid.GetHex());
		if(retcode = CCgetspenttxid(spenttxid, vini, height, prevproposaltxid, 1) == 0) {
			if(myGetTransaction(spenttxid,spenttx,hashBlock)==0 || (numvouts=spenttx.vout.size())<=0)
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has already been amended by txid " << spenttxid.GetHex());
			else {
				if(DecodeAgreementOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 't')
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has already been closed by txid " << spenttxid.GetHex());
				else if(DecodeAgreementOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 'c')
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has already been accepted by txid " << spenttxid.GetHex());
			}
		}
		if(mypk != pubkey2pk(refInitiator))
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "-pubkey doesn't match creator of previous proposal txid " << prevproposaltxid.GetHex());
		if(pubkey2pk(buyer).IsValid() && !(refReceiver.empty()) && buyer != refReceiver)
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "buyer must be the same as specified in previous proposal txid " << prevproposaltxid.GetHex());
		if(!(pubkey2pk(buyer).IsValid()) && !(refReceiver.empty()))
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cannot remove buyer when one exists in previous proposal txid " << prevproposaltxid.GetHex());
	}
	
	// check refagreementtxid if specified
	if(refagreementtxid != zeroid) {
		if(myGetTransaction(refagreementtxid,refagreementtx,hashBlock) == 0 || (numvouts=refagreementtx.vout.size()) <= 0)
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cant find specified reference agreement txid " << refagreementtxid.GetHex());
		if(DecodeAgreementOpRet(refagreementtx.vout[numvouts - 1].scriptPubKey) != 'c')
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "invalid reference agreement txid " << refagreementtxid.GetHex());
	}
	
	if(AddNormalinputs(mtx,mypk,txfee+CC_MARKER_VALUE+CC_RESPONSE_VALUE,64,pk.IsValid()) >= txfee+CC_MARKER_VALUE+CC_RESPONSE_VALUE) {
		if(prevproposaltxid != zeroid) {
			mtx.vin.push_back(CTxIn(prevproposaltxid,0,CScript())); // vin.n-2 previous proposal marker (optional, will trigger validation)
			char mutualaddr[64];
			GetCCaddress1of2(cp, mutualaddr, pubkey2pk(refInitiator), pubkey2pk(refReceiver));
			mtx.vin.push_back(CTxIn(prevproposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (optional, will trigger validation)
			uint8_t mypriv[32];
			Myprivkey(mypriv);
			CCaddr1of2set(cp, pubkey2pk(refInitiator), pubkey2pk(refReceiver), mypriv, mutualaddr);
		}
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
		if(pubkey2pk(buyer).IsValid())
			mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_RESPONSE_VALUE, mypk, pubkey2pk(buyer))); // vout.1 response hook (with buyer)
		else
			mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_RESPONSE_VALUE, mypk)); // vout.1 response hook (no buyer)
		return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementProposalOpRet('p',std::vector<uint8_t>(mypk.begin(),mypk.end()),buyer,arbitrator,prepayment,arbitratorfee,deposit,0,datahash,refagreementtxid,prevproposaltxid,name)));
	}
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");
}

// agreementrequestupdate - constructs a 'p' transaction, with the 'u' proposal type
// This transaction will only be validated if agreementtxid is specified. Optionally, prevproposaltxid can be used to amend previous update requests.
UniValue AgreementRequestUpdate(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash, std::vector<uint8_t> newarbitrator, uint256 prevproposaltxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	
	CTransaction prevproposaltx, refagreementtx;
	int32_t numvouts;
	uint256 hashBlock;
	
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if(txfee == 0)
		txfee = 10000;
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	//stuff
	// if newarbitrator is 0, maintain current arbitrator status
	// don't allow swapping between no arbitrator <-> arbitrator
	// only 1 update/cancel request per party, per agreement
	
	CCERR_RESULT("agreementscc",CCLOG_INFO,stream << "incomplete");
}

// agreementrequestcancel - constructs a 'p' transaction, with the 't' proposal type
// This transaction will only be validated if agreementtxid is specified. Optionally, prevproposaltxid can be used to amend previous cancel requests.
UniValue AgreementRequestCancel(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash, uint64_t depositcut, uint256 prevproposaltxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	
	CTransaction prevproposaltx, refagreementtx;
	int32_t numvouts;
	uint256 hashBlock;
	
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if(txfee == 0)
		txfee = 10000;
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	//stuff
	// only 1 update/cancel request per party, per agreement
	
	CCERR_RESULT("agreementscc",CCLOG_INFO,stream << "incomplete");
}

// agreementcloseproposal - constructs a 't' transaction and spends the specified 'p' transaction
// can always be done by the proposal initiator, as well as the receiver if they are able to accept the proposal.
UniValue AgreementCloseProposal(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	
	CTransaction proposaltx, spenttx;
	int32_t numvouts, vini, height, retcode;
	uint256 hashBlock, refHash, refAgreementTxid, refPrevProposalTxid, spenttxid;
	std::vector<uint8_t> refInitiator, refReceiver, refArbitrator;
	int64_t refPrepayment, refArbitratorFee, refDeposit, refDepositCut;
	std::string refName;
	uint8_t refProposalType;
	
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if(txfee == 0)
		txfee = 10000;
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());

	// check proposaltxid
	if(proposaltxid != zeroid) {
		if(myGetTransaction(proposaltxid,proposaltx,hashBlock)==0 || (numvouts=proposaltx.vout.size())<=0)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "cant find specified proposal txid " << proposaltxid.GetHex());
		if(DecodeAgreementOpRet(proposaltx.vout[numvouts - 1].scriptPubKey) == 'c')
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "specified txid is a contract id, needs to be proposal id");
		else if(DecodeAgreementProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,refProposalType,refInitiator,refReceiver,refArbitrator,refPrepayment,refArbitratorFee,refDeposit,refDepositCut,refHash,refAgreementTxid,refPrevProposalTxid,refName) != 'p')
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "invalid proposal txid " << proposaltxid.GetHex());
		if(retcode = CCgetspenttxid(spenttxid, vini, height, proposaltxid, 1) == 0) {
			std::cerr << "Found a spenttxid" << std::endl;
			if(myGetTransaction(spenttxid,spenttx,hashBlock)==0 || (numvouts=spenttx.vout.size())<=0)
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has already been amended by txid " << spenttxid.GetHex());
			std::cerr << "Found the spenttx as well" << std::endl;
			else {
				if(DecodeAgreementOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 't')
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has already been closed by txid " << spenttxid.GetHex());
				else if(DecodeAgreementOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 'c')
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has already been accepted by txid " << spenttxid.GetHex());
			}
		}
		if(mypk != pubkey2pk(refInitiator) && (!(refReceiver.empty()) && mypk != pubkey2pk(refReceiver)))
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "-pubkey must be either initiator or receiver of specified proposal txid " << proposaltxid.GetHex());
	}
	else
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Proposal transaction id must be specified");
	
	if(AddNormalinputs(mtx,mypk,txfee,64,pk.IsValid()) >= txfee) {
		mtx.vin.push_back(CTxIn(proposaltxid,0,CScript())); // vin.n-2 previous proposal marker (optional, will trigger validation)
		char mutualaddr[64];
		GetCCaddress1of2(cp, mutualaddr, pubkey2pk(refInitiator), pubkey2pk(refReceiver));
		mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (optional, will trigger validation)
		uint8_t mypriv[32];
		Myprivkey(mypriv);
		CCaddr1of2set(cp, pubkey2pk(refInitiator), pubkey2pk(refReceiver), mypriv, mutualaddr);
		return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementProposalCloseOpRet(proposaltxid,std::vector<uint8_t>(mypk.begin(),mypk.end()))));
	}
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");
}

// agreementaccept - spend a 'p' transaction that was submitted by the other party.
// this function is context aware and does different things dependent on the proposal type:
// if txid opret has the 'p' proposal type, will create a 'c' transaction (create contract)
// if txid opret has the 'u' or 't' proposal type, will create a 'u' transaction (update contract)
UniValue AgreementAccept(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	
	CTransaction proposaltx, spenttx;
	int32_t numvouts, vini, height, retcode;
	uint256 hashBlock, refHash, refAgreementTxid, refPrevProposalTxid, spenttxid;
	std::vector<uint8_t> refInitiator, refReceiver, refArbitrator;
	int64_t refPrepayment, refArbitratorFee, refDeposit, refDepositCut;
	std::string refName;
	uint8_t refProposalType;
	
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if(txfee == 0)
		txfee = 10000;
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());

	// check if the proposal itself is valid for acceptance (not cancelled, etc.)
	if(proposaltxid != zeroid) {
		if(myGetTransaction(proposaltxid,proposaltx,hashBlock)==0 || (numvouts=proposaltx.vout.size())<=0)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "cant find specified proposal txid " << proposaltxid.GetHex());
		if(DecodeAgreementOpRet(proposaltx.vout[numvouts - 1].scriptPubKey) == 'c')
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "specified txid is a contract id, needs to be proposal id");
		else if(DecodeAgreementProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,refProposalType,refInitiator,refReceiver,refArbitrator,refPrepayment,refArbitratorFee,refDeposit,refDepositCut,refHash,refAgreementTxid,refPrevProposalTxid,refName) != 'p')
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "invalid proposal txid " << proposaltxid.GetHex());
		if(retcode = CCgetspenttxid(spenttxid, vini, height, proposaltxid, 1) == 0) {
			if(myGetTransaction(spenttxid,spenttx,hashBlock)==0 || (numvouts=spenttx.vout.size())<=0)
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has already been amended by txid " << spenttxid.GetHex());
			else {
				if(DecodeAgreementOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 't')
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has already been closed by txid " << spenttxid.GetHex());
				else if(DecodeAgreementOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 'c')
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has already been accepted by txid " << spenttxid.GetHex());
			}
		}
		if(refReceiver.empty())
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "proposal has no designated receiver");
		else if(mypk != pubkey2pk(refReceiver))
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "-pubkey must be receiver of specified proposal txid " << proposaltxid.GetHex());
	}
	else
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Proposal transaction id must be specified");

	// check if the data in the proposal is correct
	// stuff might be put here for general proposal checks
	switch(refProposalType)
	{
		case 'p':
			// check datahash
			if(refHash == zeroid)
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Data hash in proposal empty or invalid");
			
			// check prepayment
			if (refPrepayment < 0)
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Prepayment must be positive");
			
			// check if arbitrator pubkey exists and is valid
			if(!refArbitrator.empty() && !(pubkey2pk(refArbitrator).IsValid()))
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Arbitrator pubkey in proposal invalid");
			
			// checking if mypk != initiatorpubkey != arbitratorpubkey
			if(pubkey2pk(refInitiator).IsValid() && pubkey2pk(refInitiator) == mypk)
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Initiator pubkey cannot accept their own agreement");
			if(pubkey2pk(refArbitrator).IsValid() && pubkey2pk(refArbitrator) == mypk)
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Receiver pubkey cannot be the same as arbitrator pubkey");
			if(pubkey2pk(refInitiator).IsValid() && pubkey2pk(refArbitrator).IsValid() && pubkey2pk(refArbitrator) == pubkey2pk(refInitiator))
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Buyer pubkey cannot be the same as arbitrator pubkey");
			
			// if arbitrator exists, check if deposit and arbitrator fee are sufficient
			if(!(refArbitrator.empty())) {
				if(refArbitratorFee < CC_MEDIATORFEE_MIN)
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Arbitrator is defined in proposal, but arbitrator fee is too low");
				if(refDeposit < CC_DEPOSIT_MIN)
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Arbitrator is defined in proposal, but deposit is too low");
			}
			else {
				refArbitratorFee = 0;
				refDeposit = CC_BATON_VALUE;
			}
			
			if(AddNormalinputs(mtx,mypk,txfee+CC_MARKER_VALUE+CC_BATON_VALUE*2+refDeposit+refPrepayment,64,pk.IsValid()) >= txfee+CC_MARKER_VALUE+CC_BATON_VALUE*2+refDeposit+refPrepayment) {
				mtx.vin.push_back(CTxIn(proposaltxid,0,CScript())); // vin.n-2 previous proposal marker (will trigger validation)
				char mutualaddr[64];
				GetCCaddress1of2(cp, mutualaddr, pubkey2pk(refInitiator), pubkey2pk(refReceiver));
				mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (will trigger validation)
				uint8_t mypriv[32];
				Myprivkey(mypriv);
				CCaddr1of2set(cp, pubkey2pk(refInitiator), pubkey2pk(refReceiver), mypriv, mutualaddr);
				mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
				mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_BATON_VALUE, mypk, pubkey2pk(refInitiator))); // vout.1 update baton
				mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_BATON_VALUE, mypk, pubkey2pk(refInitiator))); // vout.2 dispute baton
				if(pubkey2pk(refArbitrator).IsValid())
					mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, refDeposit, mypk, pubkey2pk(refArbitrator))); // vout.3 deposit / agreement completion marker (with arbitrator)
				else
					mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, refDeposit, mypk)); // vout.3 deposit / agreement completion marker (without arbitrator)
				mtx.vout.push_back(CTxOut(refPrepayment,CScript() << ParseHex(HexStr(refInitiator)) << OP_CHECKSIG)); // vout.4 prepayment
				return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementSigningOpRet(proposaltxid)));
			}
			else
				CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");

		case 'u':
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "update of contract not supported yet");
			
		case 't':
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "termination of contract not supported yet");
		
		default:
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "invalid proposal type for proposal txid " << proposaltxid.GetHex());
	}
}

// agreementdispute
// agreementresolve

//===========================================================================
// RPCs - informational
//===========================================================================

// agreementinfo
/*
	Retrieves info about the specified Agreements transaction.
		- Check funcid of transaction.
		- If 'p':
			- Check proposaltype and data.
			result: success;
			name: name;
			type: funcid desc;
			initiator: pubkey1;
			- If "p":
				receiver: pubkey2 (or none);
				arbitrator: pubkey3 (or none);
				deposit: number (or none);
				arbitratorfee: number (or none);
			- If "u":
				receiver: pubkey2;
				arbitrator: pubkey3 (or none);
			- If "t":
				receiver: pubkey2;
				depositsplit: percentage; (how much the receiver will get)
			iteration: iteration;
			datahash: datahash;
			description: description (or none);
			prevproposaltxid: txid (or none);
		- If 'c':
			- Check accepted proposal and verify if its 'p' type.
			result: success;
			txid: txid;
			name: name;
			type: funcid desc;
			seller: pubkey1;
			buyer: pubkey2;
			- Check for any 'u' transactions
			- If canceled:
				status: canceled;
			- Else if deposit taken by arbitrator:
				status: failed;
			- Else if deposit spent by Settlements Payment:
				- If payment has been withdrawn by buyer:
					status: failed;
				- Else if payment has been withdrawn by seller:
					status: completed;
			- Else:
				status: active;
			iteration: iteration;
			datahash: datahash;
			description: description (or none);
			- Check for any 'd' transactions in both vouts
				sellerdisputes: number;
				buyerdisputes: number;
			- If arbitrator = true:
				arbitrator: pubkey3 (or none);
				deposit: number;
				arbitratorfee: number;
			proposaltxid: proposaltxid; <- which proposal was accepted
			refagreementtxid: txid (or none);
		- If 'u':
			result: success;
			txid: txid;
			- Get update type
			- If "u":
				type: contract update;
			- If "c":
				type: contract cancel;
			- Check accepted proposal and verify if its 'p' type.
	TODO: Finish this later
*/
/*
UniValue AgreementInfo(uint256 txid)
{
    UniValue result(UniValue::VOBJ);
    uint256 hashBlock, latesttxid, datahash;
    CTransaction tokenbaseTx, latestUpdateTx;
	CScript batonopret;
    std::vector<uint8_t> origpubkey;
    std::vector<std::pair<uint8_t, vscript_t>> oprets;
    vscript_t vopretNonfungible;
    std::string name, description, ccode;
    double ownerPerc;
    struct CCcontract_info *cpTokens, tokensCCinfo;
    int64_t supply = 0, value;
    int32_t numblocks, licensetype;

    cpTokens = CCinit(&tokensCCinfo, EVAL_TOKENS);

	if( !myGetTransaction(tokenid, tokenbaseTx, hashBlock) ) {
		//fprintf(stderr, "TokenInfo() cant find tokenid\n");
		result.push_back(Pair("result", "error"));
		result.push_back(Pair("error", "cant find tokenid"));
		return(result);
	}
    if ( KOMODO_NSPV_FULLNODE && hashBlock.IsNull()) {
        result.push_back(Pair("result", "error"));
        result.push_back(Pair("error", "the transaction is still in mempool"));
        return (result);
    }

    if (tokenbaseTx.vout.size() > 0 && DecodeTokenCreateOpRet(tokenbaseTx.vout[tokenbaseTx.vout.size() - 1].scriptPubKey, origpubkey, name, description, ownerPerc, oprets) != 'c') {
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "TokenInfo() passed tokenid isnt token creation txid" << std::endl);
        result.push_back(Pair("result", "error"));
        result.push_back(Pair("error", "tokenid isnt token creation txid"));
        return result;
    }

    result.push_back(Pair("result", "success"));
    result.push_back(Pair("tokenid", tokenid.GetHex()));
    result.push_back(Pair("owner", HexStr(origpubkey)));
    result.push_back(Pair("name", name));

    supply = CCfullsupply(tokenid);

    result.push_back(Pair("supply", supply));
    result.push_back(Pair("description", description));
    result.push_back(Pair("ownerpercentage", ownerPerc));
	
	if (GetLatestTokenUpdate(tokenid, latesttxid)) {
		if (tokenid == latesttxid) {
			if (tokenbaseTx.vout[2].nValue == 10000 &&
			getCCopret(tokenbaseTx.vout[2].scriptPubKey, batonopret) &&
			DecodeTokenUpdateCCOpRet(batonopret, datahash, value, ccode, licensetype) == 'u') {
				result.push_back(Pair("latesthash", datahash.GetHex()));
				result.push_back(Pair("latestvalue", (double)value/COIN));
				result.push_back(Pair("latestccode", ccode));
				result.push_back(Pair("license", licensetype));
			}
		}
		else {
			if (myGetTransaction(latesttxid, latestUpdateTx, hashBlock) &&
			latestUpdateTx.vout.size() > 0 &&
			latestUpdateTx.vout[0].nValue == 10000 &&
			getCCopret(latestUpdateTx.vout[0].scriptPubKey, batonopret) &&
			DecodeTokenUpdateCCOpRet(batonopret, datahash, value, ccode, licensetype) == 'u')
				result.push_back(Pair("latesthash", datahash.GetHex()));
				result.push_back(Pair("latestvalue", (double)value/COIN));
				result.push_back(Pair("latestccode", ccode));
				result.push_back(Pair("license", licensetype));
		}
	}

    GetOpretBlob(oprets, OPRETID_NONFUNGIBLEDATA, vopretNonfungible);
    if (!vopretNonfungible.empty())
        result.push_back(Pair("data", HexStr(vopretNonfungible)));
    
    if (tokenbaseTx.IsCoinImport()) { // if imported token
        ImportProof proof;
        CTransaction burnTx;
        std::vector<CTxOut> payouts;
        CTxDestination importaddress;

        std::string sourceSymbol = "can't decode";
        std::string sourceTokenId = "can't decode";

        if (UnmarshalImportTx(tokenbaseTx, proof, burnTx, payouts)) {
            // extract op_return to get burn source chain.
            std::vector<uint8_t> burnOpret;
            std::string targetSymbol;
            uint32_t targetCCid;
            uint256 payoutsHash;
            std::vector<uint8_t> rawproof;
            if (UnmarshalBurnTx(burnTx, targetSymbol, &targetCCid, payoutsHash, rawproof)) {
                if (rawproof.size() > 0) {
                    CTransaction tokenbasetx;
                    E_UNMARSHAL(rawproof, ss >> sourceSymbol;
                                if (!ss.eof())
                                    ss >>
                                tokenbasetx);

                    if (!tokenbasetx.IsNull())
                        sourceTokenId = tokenbasetx.GetHash().GetHex();
                }
            }
        }
        result.push_back(Pair("IsImported", "yes"));
        result.push_back(Pair("sourceChain", sourceSymbol));
        result.push_back(Pair("sourceTokenId", sourceTokenId));
    }

    return result;
}
*/

// agreementviewupdates [list]
// agreementviewdisputes [list]
// agreementinventory([pubkey])

UniValue AgreementList()
{
	UniValue result(UniValue::VARR);
	std::vector<uint256> txids, foundtxids;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressIndexCCMarker;

	struct CCcontract_info *cp, C; uint256 txid, hashBlock;
	CTransaction vintx; std::vector<uint8_t> origpubkey;
	std::string name, description;
	uint8_t proposaltype;

	cp = CCinit(&C, EVAL_AGREEMENTS);

	auto addAgreementTxid = [&](uint256 txid) {
		if (myGetTransaction(txid, vintx, hashBlock) != 0) {
			if (vintx.vout.size() > 0 && DecodeAgreementOpRet(vintx.vout[vintx.vout.size() - 1].scriptPubKey) != 0) {
				if (std::find(foundtxids.begin(), foundtxids.end(), txid) == foundtxids.end()) {
					result.push_back(txid.GetHex());
					foundtxids.push_back(txid);
				}
			}
		}
	};

	SetCCunspents(addressIndexCCMarker,cp->unspendableCCaddr,true);
	for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressIndexCCMarker.begin(); it != addressIndexCCMarker.end(); it++) {
		addAgreementTxid(it->first.txhash);
	}

	return(result);
}
