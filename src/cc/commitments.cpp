/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.							  *
 *																			  *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at				  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *																			  *
 * Unless otherwise agreed in a custom licensing agreement, no part of the	  *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *																			  *
 * Removal or modification of this copyright notice is prohibited.			  *
 *																			  *
 ******************************************************************************/

#include "CCcommitments.h"

/*
Put all notes here!

don't allow swapping between no arbitrator <-> arbitrator
only 1 update/cancel request per party, per commitment
version numbers should be reset after contract acceptance
fix commitmentcloseproposal not detecting outdated proposals

keep closed proposal markers on!
allow renaming commitments through updates
	
Commitments RPCs:

	commitmentpropose (info datahash destpub arbitrator [arbitratorfee][deposit][prevproposaltxid][refcommitmenttxid])
	commitmentrequestupdate(commitmenttxid info datahash [newarbitrator][prevproposaltxid])
	commitmentrequestcancel(commitmenttxid info datahash [depositsplit][prevproposaltxid])
	Proposal creation
	
	commitmentaccept(proposaltxid)
	commitmentcloseproposal(proposaltxid message)
	Proposal response
	
	commitmentdispute(commitmenttxid disputetype [disputehash])
	commitmentresolve(commitmenttxid disputetxid verdict [rewardedpubkey])
	Disputes
	
	commitmentaddress
	commitmentlist
	commitmentinfo(txid)
	commitmentviewupdates(commitmenttxid [samplenum][recursive])
	commitmentviewdisputes(commitmenttxid [samplenum][recursive])
	commitmentinventory([pubkey])

*/

//===========================================================================
// Opret encoders/decoders
//===========================================================================

//uint8_t DecodeCommitmentOpRet()
// just returns funcid of whatever commitment tx opret is fed into it
uint8_t DecodeCommitmentOpRet(const CScript scriptPubKey)
{
	std::vector<uint8_t> vopret, dummypk;
	int64_t dummyamount;
	uint256 dummyhash;
	std::string dummyinfo;
	uint8_t evalcode, funcid, *script, dummytype;
	GetOpReturnData(scriptPubKey, vopret);
	script = (uint8_t *)vopret.data();
	if(script != NULL && vopret.size() > 2) {
		evalcode = script[0];
		if (evalcode != EVAL_COMMITMENTS) {
			LOGSTREAM("commitmentscc", CCLOG_DEBUG1, stream << "script[0] " << script[0] << " != EVAL_COMMITMENTS" << std::endl);
			return (uint8_t)0;
		}
		funcid = script[1];
		LOGSTREAM((char *)"commitmentscc", CCLOG_DEBUG2, stream << "DecodeCommitmentOpRet() decoded funcId=" << (char)(funcid ? funcid : ' ') << std::endl);
		switch (funcid) {
		case 'p':
			return DecodeCommitmentProposalOpRet(scriptPubKey, dummytype, dummytype, dummypk, dummypk, dummypk, dummyamount, dummyamount, dummyamount, dummyhash, dummyhash, dummyhash, dummyinfo);
		case 't':
			return DecodeCommitmentProposalCloseOpRet(scriptPubKey, dummytype, dummyhash, dummypk);
		case 'c':
			return DecodeCommitmentSigningOpRet(scriptPubKey, dummytype, dummyhash);
		//case 'whatever':
			//insert new cases here
		default:
			LOGSTREAM((char *)"commitmentscc", CCLOG_DEBUG1, stream << "DecodeCommitmentOpRet() illegal funcid=" << (int)funcid << std::endl);
			return (uint8_t)0;
		}
	}
	else
		LOGSTREAM("commitmentscc",CCLOG_DEBUG1, stream << "not enough opret.[" << vopret.size() << "]" << std::endl);
	return (uint8_t)0;
}

CScript EncodeCommitmentProposalOpRet(uint8_t version, uint8_t proposaltype, std::vector<uint8_t> srcpub, std::vector<uint8_t> destpub, std::vector<uint8_t> arbitratorpk, int64_t payment, int64_t arbitratorfee, int64_t depositval, uint256 datahash, uint256 commitmenttxid, uint256 prevproposaltxid, std::string info)
{
	CScript opret; uint8_t evalcode = EVAL_COMMITMENTS, funcid = 'p';
	proposaltype = 'p'; //temporary
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << proposaltype << srcpub << destpub << arbitratorpk << payment << arbitratorfee << depositval << datahash << commitmenttxid << prevproposaltxid << info);
	return(opret);
}
uint8_t DecodeCommitmentProposalOpRet(CScript scriptPubKey, uint8_t &version, uint8_t &proposaltype, std::vector<uint8_t> &srcpub, std::vector<uint8_t> &destpub, std::vector<uint8_t> &arbitratorpk, int64_t &payment, int64_t &arbitratorfee, int64_t &depositval, uint256 &datahash, uint256 &commitmenttxid, uint256 &prevproposaltxid, std::string &info)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> proposaltype; ss >> srcpub; ss >> destpub; ss >> arbitratorpk; ss >> payment; ss >> arbitratorfee; ss >> depositval; ss >> datahash; ss >> commitmenttxid; ss >> prevproposaltxid; ss >> info) != 0 && evalcode == EVAL_COMMITMENTS)
		return(funcid);
	return(0);
}

CScript EncodeCommitmentProposalCloseOpRet(uint8_t version, uint256 proposaltxid, std::vector<uint8_t> srcpub)
{
	CScript opret; uint8_t evalcode = EVAL_COMMITMENTS, funcid = 't';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << proposaltxid << srcpub);
	return(opret);
}
uint8_t DecodeCommitmentProposalCloseOpRet(CScript scriptPubKey, uint8_t &version, uint256 &proposaltxid, std::vector<uint8_t> &srcpub)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> proposaltxid; ss >> srcpub) != 0 && evalcode == EVAL_COMMITMENTS)
		return(funcid);
	return(0);
}

CScript EncodeCommitmentSigningOpRet(uint8_t version, uint256 proposaltxid)
{
	CScript opret; uint8_t evalcode = EVAL_COMMITMENTS, funcid = 'c';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << proposaltxid);
	return(opret);
}
uint8_t DecodeCommitmentSigningOpRet(CScript scriptPubKey, uint8_t &version, uint256 &proposaltxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> proposaltxid) != 0 && evalcode == EVAL_COMMITMENTS)
		return(funcid);
	return(0);
}

CScript EncodeCommitmentUpdateOpRet(uint8_t version, uint256 commitmenttxid, uint256 proposaltxid)
{
	CScript opret; uint8_t evalcode = EVAL_COMMITMENTS, funcid = 'u';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << commitmenttxid << proposaltxid);
	return(opret);
}
uint8_t DecodeCommitmentUpdateOpRet(CScript scriptPubKey, uint8_t &version, uint256 &commitmenttxid, uint256 &proposaltxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> commitmenttxid; ss >> proposaltxid) != 0 && evalcode == EVAL_COMMITMENTS)
		return(funcid);
	return(0);
}

CScript EncodeCommitmentDisputeOpRet(uint8_t version, uint256 commitmenttxid, uint256 disputehash)
{
	CScript opret; uint8_t evalcode = EVAL_COMMITMENTS, funcid = 'd';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << commitmenttxid << disputehash);
	return(opret);
}
uint8_t DecodeCommitmentDisputeOpRet(CScript scriptPubKey, uint8_t &version, uint256 &commitmenttxid, uint256 &disputehash)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> commitmenttxid; ss >> disputehash) != 0 && evalcode == EVAL_COMMITMENTS)
		return(funcid);
	return(0);
}

CScript EncodeCommitmentDisputeResolveOpRet(uint8_t version, uint256 commitmenttxid, uint256 disputetxid, std::vector<uint8_t> rewardedpubkey)
{
	CScript opret; uint8_t evalcode = EVAL_COMMITMENTS, funcid = 'r';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << commitmenttxid << disputetxid << rewardedpubkey);
	return(opret);
}
uint8_t DecodeCommitmentDisputeResolveOpRet(CScript scriptPubKey, uint8_t &version, uint256 &commitmenttxid, uint256 &disputetxid, std::vector<uint8_t> &rewardedpubkey)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> commitmenttxid; ss >> disputetxid; ss >> rewardedpubkey) != 0 && evalcode == EVAL_COMMITMENTS)
		return(funcid);
	return(0);
}

//===========================================================================
// Validation
//===========================================================================

bool CommitmentsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	int32_t numvins,numvouts,preventCCvins,preventCCvouts;
	bool retval;
    uint256 hashBlock;
    uint8_t funcid;

	numvins = tx.vin.size();
    numvouts = tx.vout.size();
    preventCCvins = preventCCvouts = -1;
	
    if (numvouts < 1)
        return eval->Invalid("no vouts");
	
	CCOpretCheck(eval,tx,true,true,true);
    CCExactAmounts(eval,tx,CC_TXFEE);
    //if (ChannelsExactAmounts(cp,eval,tx) == false )
    //{
    //    return eval->Invalid("invalid channel inputs vs. outputs!");            
    //}
	
	if ((funcid = DecodeCommitmentOpRet(tx.vout[numvouts-1].scriptPubKey)) != 0) {
		switch (funcid)
        {
			case 'p':
				/*
				commitment proposal:
				vin.0 normal input
				vin.1 previous proposal marker
				vin.2 previous proposal baton
				vout.0 marker
				vout.1 response hook
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 'p' version proposaltype srcpub destpub arbitratorpk payment arbitratorfee depositvalue datahash commitmenttxid prevproposaltxid info
				*/
				std::cerr << "funcid - 'p'" << std::endl;
				///		- Proposal data validation.
				/*	
						use CheckProposalData(tx)
						use DecodeCommitmentProposalOpRet, get proposaltype, initiator, receiver, prevproposaltxid
						use TotalPubkeyNormalInputs to verify that initiator == pubkey that submitted tx
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
							use DecodeCommitmentProposalOpRet, get proposaltype, initiator, receiver, prevx2proposaltxid
							check if proposal types match. else, invalidate
							check if initiator pubkeys match. else, invalidate
							prevproposaltxid = prevx2proposaltxid
							Loop
				*/	
				///		- Checking if our inputs/outputs are correct.
				/*
						check vout0:
							does it point to the commitments global address?
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
				break;
			case 't':
				/*
				proposal cancel:
				vin.0 normal input
				vin.1 previous proposal baton
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 't' proposaltxid initiator
				*/
				return eval->Invalid("funcid - 't'");
				///		- Getting the transaction data.
				/*
						use DecodeCommitmentProposalCloseOpRet, get initiator, proposaltxid
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
						use DecodeCommitmentProposalOpRet on ref proposal tx, get proposaltype, refinitiator, refreceiver, refcommitmenttxid
						if proposaltype = 'p' 
							get status of receiver - is it valid or null?
							if receiver is null:
								check if initiator pubkey matches with ref initiator. else, invalidate
							else
								check if initiator pubkey matches with either ref initiator or receiver. else, invalidate
						else if proposaltype = 'u' or 't'
							if receiver doesn't exist, invalidate.
							check if refcommitmenttxid is defined. If not, invalidate
							CheckIfInitiatorValid(initiator, commitmenttxid)
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
				break;
			case 'c':
				/*
				contract creation:
				vin.0 normal input
				vin.1 latest proposal by seller
				vout.0 marker
				vout.1 update/dispute baton
				vout.2 deposit / commitment completion baton
				vout.3 payment (optional)
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 'c' proposaltxid
				*/
				return eval->Invalid("funcid - 'c'");
				///		- Getting the transaction data.
				/*
						use DecodeCommitmentProposalSigningOpRet, get proposaltxid
				*/
				///		- Proposal data validation.
				/*	
						get proposal tx
						use CheckProposalData(proposaltx)
						use DecodeCommitmentProposalOpRet, get proposaltype, initiator, receiver, prevproposaltxid, payment, depositvalue
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
							does it point to the commitments global address?
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
							is the vout3 value equal to the payment specified in the proposal opret?
						check vin1:
							does vin1 point to the proposal's marker?
						check vin2:
							does vin2 point to the proposal's response?
						Do not allow any additional CC vins.
						break;
				*/	
				break;
			case 'u':
				/*
				contract update:
				vin.0 normal input
				vin.1 last update baton
				vin.2 latest proposal by other party
				vout.0 next update baton
				vout.1 payment (optional)
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 'u' commitmenttxid proposaltxid
				*/
				return eval->Invalid("funcid - 'u'");
				///		- Getting the transaction data.
				/*
						use DecodeCommitmentUpdateOpRet, get proposaltxid, commitmenttxid
						use GetLatestCommitmentUpdateType(commitmenttxid)
						if updatefuncid != 'c' or 'u' or 'r', invalidate
						use GetLatestCommitmentUpdate(commitmenttxid) to get the latest update txid
				*/
				///		- Proposal data validation.
				/*	
						get proposal tx
						use CheckProposalData(proposaltx)
						use DecodeCommitmentProposalOpRet, get proposaltype, initiator, receiver, payment
						if proposaltype != 'u', invalidate
						use TotalPubkeyNormalInputs to verify that receiver == pubkey that submitted tx
						CheckIfInitiatorValid(initiator, commitmenttxid)
						CheckIfInitiatorValid(receiver, commitmenttxid)
				*/
				///		- Checking if selected proposal was already spent.
				/*
						CheckIfProposalSpent(proposaltxid)
				*/
				///		- Checking if the arbitrator is set up correctly.
				/*
						use GetCommitmentArbitrator(commitmenttxid) to find out if current commitment has arbitrator or not
						if commitment has no arbitrator, and if update has arbitrator set to anything but null, invalidate
						if commitment has arbitrator, and if update has arbitrator set to null:
						if we're in validation, invalidate (if arbitrator was unchanged, it should be left the same)
						else, set arbitrator to current commitment arbitrator
				*/
				///		- Checking if our inputs/outputs are correct.
				/*
						check vout0:
							does it point to a 1of2 address between initiator and receiver?
								does initiatorpubkey point to the same privkey as the initiator's address?
								does receiverpubkey point to the same privkey as the receiver's address?
							is the value correct?
						check vout1:
							is it a normal output?
							does vout1 point to the initiator's address?
							is the vout1 value equal to the payment specified in the proposal opret?
						check vin1:
							does it point to the previous update baton?
						check vin2:
							does it point to the proposal's marker?
						check vin3:
							does it point to the proposal's response?
						Do not allow any additional CC vins.
						break;
				*/
				break;
			case 's':
				/*
				contract close:
				vin.0 normal input
				vin.1 last update baton
				vin.2 latest termination proposal by other party
				vin.3 deposit
				vout.0 deposit split to party 1
				vout.1 deposit split to party 2
				vout.2 payment
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 's' commitmenttxid proposaltxid
				*/
				return eval->Invalid("funcid - 's'");
				///		- Getting the transaction data.
				/*
						use DecodeCommitmentCloseOpRet, get proposaltxid, commitmenttxid
						use GetLatestCommitmentUpdateType(commitmenttxid)
						if updatefuncid != 'c' or 'u' or 'r', invalidate
						use GetLatestCommitmentUpdate(commitmenttxid) to get the latest update txid
						totaldeposit = GetDepositValue(commitmenttxid)
				*/
				///		- Proposal data validation.
				/*	
						get proposal tx
						use CheckProposalData(proposaltx)
						use DecodeCommitmentProposalOpRet, get proposaltype, initiator, receiver, payment, deposit amount
						if proposaltype != 't', invalidate
						use TotalPubkeyNormalInputs to verify that receiver == pubkey that submitted tx
						CheckIfInitiatorValid(initiator, commitmenttxid)
						CheckIfInitiatorValid(receiver, commitmenttxid)
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
						check vout2:
							is it a normal output?
							does vout2 point to the initiator's address?
							is the vout2 value equal to the payment specified in the proposal opret?
						check vin1:
							does it point to the previous update baton?
						check vin2:
							does it point to the proposal's marker?
						check vin3:
							does it point to the proposal's response?
						check vin4:
							does it point to the commitment deposit?
						Do not allow any additional CC vins.
						break;
				*/
				break;
			case 'd':
				/*
				contract dispute:
				vin.0 normal input
				vin.1 last update baton
				vout.0 response hook / arbitrator fee
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 'd' commitmenttxid initiator disputehash
				*/
				return eval->Invalid("funcid - 'd'");
				///		- Getting and verifying the transaction data.
				/*
						use DecodeCommitmentDisputeOpRet, get initiator, commitmenttxid, disputehash
						use GetLatestCommitmentUpdateType(commitmenttxid)
						if updatefuncid != 'c' or 'u' or 'r', invalidate
						use GetLatestCommitmentUpdate(commitmenttxid) to get the latest update txid
						check if disputehash is empty. Invalidate if it is.
						use TotalPubkeyNormalInputs to verify that initiator == pubkey that submitted tx
						CheckIfInitiatorValid(initiator, commitmenttxid)
				*/
				///		- Some arbitrator shenanigans.
				/*
						use GetCommitmentArbitrator(commitmenttxid) to get the current arbitrator
						if one doesn't exist, invalidate (disputes are disabled when there are no arbitrators)
						arbitratorfee = GetArbitratorFee(commitmenttxid)
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
				break;
			case 'r':
				/*
				contract dispute resolve:
				vin.0 normal input
				vin.1 dispute resolved w/ fee
				vin.2 deposit
				vout.0 next update baton
				vout.1 deposit redeem
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 'r' disputetxid rewardedpubkey
				*/
				return eval->Invalid("funcid - 'r'");
				///		- Getting and verifying the transaction data.
				/*
						use DecodeCommitmentDisputeResolveOpRet, get disputetxid, rewardedpubkey
						use DecodeCommitmentDisputeOpRet, get commitmenttxid, initiator
						use GetLatestCommitmentUpdateType(commitmenttxid)
						if updatefuncid != 'd', invalidate
						use GetLatestCommitmentUpdate(commitmenttxid) to get the latest update txid
				*/
				///		- Depending on what rewardedpubkey is set to, the dispute may or not be cancelled by the arbitrator
				/*
						CheckIfInitiatorValid(rewardedpubkey, commitmenttxid)
						deposit = GetDepositValue(commitmenttxid)
				*/
				///		- Some arbitrator shenanigans.
				/*	
						use GetCommitmentArbitrator(commitmenttxid) to get the current arbitrator
						if one doesn't exist, invalidate (disputes are disabled when there are no arbitrators)
						arbitratorfee = GetArbitratorFee(commitmenttxid)
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
							does it point to the commitment deposit?
						Do not allow any additional CC vins.
						break;
				*/
				break;
			default:
                fprintf(stderr,"unexpected commitments funcid (%c)\n",funcid);
                return eval->Invalid("unexpected commitments funcid!");
		}
    }
	//IMPORTANT!!! - deposit is locked under EVAL_COMMITMENTS, but it may also be spent by a specific Exchanges transaction.
	else {
		// this part will be needed to validate Exchanges that withdraw the deposit. for now, this is invalid
		return eval->Invalid("must be valid commitments funcid!"); 
	}

	retval = PreventCC(eval,tx,preventCCvins,numvins,preventCCvouts,numvouts);
    if (retval != 0)
        LOGSTREAM("commitments",CCLOG_INFO, stream << "Commitments tx validated" << std::endl);
    else
		fprintf(stderr,"Commitments tx invalid\n");
	std::cerr << "CommitmentsValidate triggered" << std::endl;
    return(retval);
}

//===========================================================================
// Helper functions
//===========================================================================

int64_t IsCommitmentsVout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v)
{
	char destaddr[64];
	if( tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0 )
	{
		if( Getscriptaddress(destaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(destaddr,cp->unspendableCCaddr) == 0 )
			return(tx.vout[v].nValue);
	}
	return(0);
}

/*
GetAcceptedProposalOpRet (just gets the opret from a accepted commitment's proposal)

validation:
	ValidateRefProposalOpRet
	
might not do this one
CompareProposalPubkeys (proposal1, proposal2)
	gets two proposal txids, checks opret
	switch(proposaltype)
		if 'p':
			srcpub must be the same
		if 'u' or 't':
			srcpub, destpub must be the same

IsProposalSpent
IsDepositSpent (if it is spent, retrieves spendingtxid and spendingfuncid)

static data:
	GetCommitmentMembers (gets seller, client and firstarbitrator if they exist)
	GetCommitmentDeposit (gets deposit from opret, checks if the vout value matches it)
	

updateable data:
	GetLatestCommitmentUpdate(commitmenttxid)
		pretty much same thing as GetLatestTokenUpdate, gets latest update txid
	GetCommitmentUpdateData(updatetxid)
		takes a 'u' transaction, finds its 'p' transaction and gets the data from there:
		info
		datahash
		arbitrator pk
		arbitrator fee

dynamic data (non-user):
	GetProposalTxRevisions(proposaltxid)
		starts from beginning, counts up revisions up until proposaltxid
	GetCommitmentTxRevisions(commitmenttxid)
		counts number of updates to the commitment ('u' txs)
	GetCommitmentStatus(commitmenttxid, bool includeExchanges)
		updatetxid = GetLatestCommitmentUpdate(commitmenttxid)
		if updatetxid = commitmenttxid // no updates were made
			return "active"
		get update tx
		DecodeCommitmentOpRet on update tx opret, get funcid
		--
		check if deposit was spent or not
		if it was:
			if funcid = 's' // the commitment was cancelled
				return "closed"
			if funcid = 'r' // the arbitrator spent the deposit as part of dispute resolution
				return "arbitrated"
			if includeExchanges:
				TBD. for now, return "completed"
			return "completed"
		if it wasn't:
			if funcid = 'u'
				return "active"
			if funcid = 'd'
				return "suspended"
			return "unknown"
	--------------------------------------
		Proposal status:
		draft
		pending
		approved
		closed
		updated
	-------------------------------------	
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
		
how tf to get these???
	update/termination proposal list (?)
	settlement list (?)

*/
bool GetAcceptedProposalOpRet(CTransaction commitmenttx, CScript &opret)
{
	CTransaction proposaltx;
	uint8_t version;
	uint256 proposaltxid;
	
	if (commitmenttx.vout.size() <= 0) {
		std::cerr << "GetAcceptedProposalOpRet: commitment tx has no vouts" << std::endl;
		return false;
	}
	if (DecodeCommitmentSigningOpRet(commitmenttx.vout[commitmenttx.vout.size() - 1].scriptPubKey, version, proposaltxid) != 'c') {
		std::cerr << "GetAcceptedProposalOpRet: given tx is not a contract signing tx" << std::endl;
		return false;	
	}
	if (myGetTransaction(proposaltxid, proposaltx, hashBlock) == 0 || proposaltx.vout.size() <= 0) {
		std::cerr << "GetAcceptedProposalOpRet: couldn't find commitment accepted proposal tx" << std::endl;
		return false;
	}
	opret = proposaltx.vout[proposaltx.vout.size() - 1].scriptPubKey;
	return true;
}

bool ValidateRefProposalOpRet(CScript opret, std::string &CCerror)
{
	CTransaction commitmenttx;
	uint256 proposaltxid, datahash, commitmenttxid, prevproposaltxid, hashBlock;
	uint8_t version, proposaltype;
	std::vector<uint8_t> srcpub, destpub, sellerpk, clientpk, arbitratorpk;
	int64_t payment, arbitratorfee, depositval;
	std::string info;
	bool bHasReceiver, bHasArbitrator;
	CPubKey CPK_src, CPK_dest, CPK_arbitrator;
	
	CCerror = "";
	
	std::cerr << "ValidateRefProposalOpRet: decoding opret" << std::endl;
	if (DecodeCommitmentProposalOpRet(opret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, datahash, commitmenttxid, prevproposaltxid, info) != 'p') {
		CCerror = "proposal tx opret invalid or not a proposal tx!";
		return false;
	}
	std::cerr << "ValidateRefProposalOpRet: check if info meets requirements (not empty, <= 2048 chars)" << std::endl;
	if (info.empty() || info.size() > 2048) {
		CCerror = "proposal info empty or exceeds 2048 chars!";
		return false;
	}
	std::cerr << "ValidateRefProposalOpRet: check if datahash meets requirements (not empty)" << std::endl;
	if (datahash == zeroid) {
		CCerror = "proposal datahash empty!";
		return false;
	}
	std::cerr << "ValidateRefProposalOpRet: check if payment is positive" << std::endl;
	if (payment < 0) {
		CCerror = "proposal has payment < 0!";
		return false;
	}
	
	CPK_src = pubkey2pk(srcpub);
	CPK_dest = pubkey2pk(destpub);
	CPK_arbitrator = pubkey2pk(arbitratorpk);
	bHasReceiver = CPK_dest.IsValid();
	bHasArbitrator = CPK_arbitrator.IsValid();
	
	std::cerr << "ValidateRefProposalOpRet: making sure srcpub != destpub != arbitratorpk" << std::endl;
	if (bHasReceiver && CPK_src == CPK_dest) {
		CCerror = "proposal srcpub cannot be the same as destpub!";
		return false;
	}
	if (bHasArbitrator && CPK_src == CPK_arbitrator) {
		CCerror = "proposal srcpub cannot be the same as arbitrator pubkey!";
		return false;
	}
	if (bHasReceiver && bHasArbitrator && CPK_dest == CPK_arbitrator) {
		CCerror = "proposal destpub cannot be the same as arbitrator pubkey!";
		return false;
	}
	
	switch (proposaltype) {
		case 'p':
			std::cerr << "ValidateRefProposalOpRet: checking deposit value" << std::endl;
			if (depositval < 0 || bHasArbitrator && depositval < CC_MARKER_VALUE) {
				CCerror = "proposal has invalid deposit value!";
				return false;
			}
			std::cerr << "ValidateRefProposalOpRet: checking arbitrator fee" << std::endl;
			if (arbitratorfee < 0 || bHasArbitrator && arbitratorfee < CC_MARKER_VALUE) {
				CCerror = "proposal has invalid arbitrator fee value!";
				return false;
			}
			if (commitmenttxid != zeroid) {
				std::cerr << "ValidateRefProposalOpRet: refcommitment was defined, check if it's a correct tx" << std::endl;
				if (myGetTransaction(commitmenttxid, commitmenttx, hashBlock) == 0 || commitmenttx.vout.size() <= 0) {
					CCerror = "proposal has refcommitment txid that does not have a tx!";
					return false;
				}
				if (DecodeCommitmentSigningOpRet(commitmenttx.vout[commitmenttx.vout.size() - 1].scriptPubKey, version, proposaltxid) != 'c') {
					CCerror = "proposal refcommitment tx is not a contract signing tx!";
					return false;	
				}
			}
			break;
			
		case 'u':
			std::cerr << "ValidateRefProposalOpRet: checking deposit value" << std::endl;
			if (depositval != 0) {
				CCerror = "proposal has invalid deposit value for update!";
				return false;
			}
		// intentional fall-through
		
		case 't':
			std::cerr << "ValidateRefProposalOpRet: checking if update/termination proposal has destpub" << std::endl;
			if (!bHasReceiver) {
				CCerror = "proposal has no defined receiver on update/termination proposal!";
				return false;
			}
			std::cerr << "ValidateRefProposalOpRet: checking arbitrator fee" << std::endl;
			if (arbitratorfee < 0 || bHasArbitrator && arbitratorfee < CC_MARKER_VALUE) {
				CCerror = "proposal has invalid arbitrator fee value!";
				return false;
			}
			std::cerr << "ValidateRefProposalOpRet: checking if commitmenttxid defined" << std::endl;
			if (commitmenttxid == zeroid) {
				CCerror = "proposal has no commitmenttxid defined for update/termination proposal!";
				return false;
			}
			std::cerr << "ValidateRefProposalOpRet: checking if srcpub and destpub are members of the commitment" << std::endl;
			if (!GetCommitmentMembers(commitmenttxid, sellerpk, clientpk)) {
				CCerror = "proposal commitment tx has invalid commitment member pubkeys!";
				return false;
			}
			if (CPK_src != pubkey2pk(sellerpk) && CPK_src != pubkey2pk(clientpk)) {
				CCerror = "proposal srcpub is not a member of the specified commitment!";
				return false;
			}
			if (CPK_dest != pubkey2pk(sellerpk) && CPK_dest != pubkey2pk(clientpk)) {
				CCerror = "proposal destpub is not a member of the specified commitment!";
				return false;
			}
			
			// put status check here
			
			//would be nice if arbitrator was null here
			// put deposit check here - must be between 0 and ref deposit value
			
			break;
			
		default:
            CCerror = "proposal has invalid proposaltype!";
			return false;
	}
}

bool IsProposalSpent(uint256 proposaltxid, uint256 &spendingtxid, uint8_t &spendingfuncid)
{
	int32_t vini, height, retcode;
	uint256 hashBlock;
	CTransaction spendingtx;
	
	if ((retcode = CCgetspenttxid(spendingtxid, vini, height, proposaltxid, 1)) == 0) {
		if (myGetTransaction(spendingtxid, spendingtx, hashBlock) != 0 && spendingtx.vout.size() > 0)
			spendingfuncid = DecodeCommitmentOpRet(spendingtx.vout[spendingtx.vout.size() - 1].scriptPubKey);
			// if 'c' or 'u' or 's', proposal was accepted
			// if 'p', proposal was amended with another proposal
			// if 't', proposal was closed
		else
			spendingfuncid = 0;
		return true;
	}
	return false;
}

bool GetCommitmentMembers(uint256 commitmenttxid, std::vector<uint8_t> &sellerpk, std::vector<uint8_t> &clientpk)
{
	CScript proposalopret;
	CTransaction commitmenttx, proposaltx;
	uint256 proposaltxid, datahash, prevproposaltxid, hashBlock;
	uint8_t version, proposaltype;
	std::vector<uint8_t> firstarbitratorpk;
	int64_t payment, arbitratorfee, depositval;
	std::string info;
	
	if (myGetTransaction(commitmenttxid, commitmenttx, hashBlock) == 0 || commitmenttx.vout.size() <= 0) {
		std::cerr << "GetCommitmentMembers: couldn't find commitment tx" << std::endl;
		return false;
	}
	
	/*if (DecodeCommitmentSigningOpRet(commitmenttx.vout[commitmenttx.vout.size() - 1].scriptPubKey, version, proposaltxid) != 'c') {
		std::cerr << "GetCommitmentMembers: given tx is not a contract signing tx" << std::endl;
		return false;	
	}
	if (myGetTransaction(proposaltxid, proposaltx, hashBlock) == 0 || proposaltx.vout.size() <= 0) {
		std::cerr << "GetCommitmentMembers: couldn't find commitment accepted proposal tx" << std::endl;
		return false;
	}*/
	
	if (!GetAcceptedProposalOpRet(commitmenttx, proposalopret)) {
		std::cerr << "GetCommitmentMembers: couldn't get accepted proposal tx opret" << std::endl;
		return false;	
	}
	if (DecodeCommitmentProposalOpRet(proposaltx.vout[proposaltx.vout.size() - 1].scriptPubKey, version, proposaltype, sellerpk, clientpk, firstarbitratorpk, payment, arbitratorfee, depositval, datahash, commitmenttxid, prevproposaltxid, info) != 'p' || proposaltype != 'p') {
		std::cerr << "GetCommitmentMembers: commitment accepted proposal tx opret invalid" << std::endl;
		return false;	
	}

	return true;
}

//===========================================================================
// RPCs - tx creation
//===========================================================================

// commitmentpropose - constructs a 'p' transaction, with the 'p' proposal type
UniValue CommitmentPropose(const CPubKey& pk, uint64_t txfee, std::string info, uint256 datahash, std::vector<uint8_t> destpub, std::vector<uint8_t> arbitrator, int64_t payment, int64_t arbitratorfee, int64_t deposit, uint256 prevproposaltxid, uint256 refcommitmenttxid)
{
	CPubKey mypk;
	CTransaction prevproposaltx, refcommitmenttx;
	int32_t numvouts, vini, height, retcode;
	uint256 hashBlock, refHash, refCommitmentTxid, refPrevProposalTxid, spendingtxid;
	std::vector<uint8_t> refInitiator, refReceiver, refArbitrator;
	int64_t refPrepayment, refArbitratorFee, refDeposit;
	std::string refName, CCerror;
	uint8_t refProposalType, version, spendingfuncid;
	
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	
	///		- Proposal data validation.
	/*	
			use CheckProposalData(tx)
			use DecodeCommitmentProposalOpRet, get proposaltype, initiator, receiver, prevproposaltxid
			use TotalPubkeyNormalInputs to verify that initiator == pubkey that submitted tx
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
				use DecodeCommitmentProposalOpRet, get proposaltype, initiator, receiver, prevx2proposaltxid
				check if proposal types match. else, invalidate
				check if initiator pubkeys match. else, invalidate
				prevproposaltxid = prevx2proposaltxid
				Loop
	*/	
	
	// check info & datahash
	/*if(info.empty() || info.size() > 2048)
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Commitment info must not be empty and up to 2048 characters");
	if(datahash == zeroid)
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Data hash empty or invalid");
	
	// check payment
	if (payment < 0)
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Prepayment must be positive");*/
	
	// check if destpub pubkey exists and is valid
	if(!destpub.empty() && !(pubkey2pk(destpub).IsValid()))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Buyer pubkey invalid");
	
	// check if arbitrator pubkey exists and is valid
	if(!arbitrator.empty() && !(pubkey2pk(arbitrator).IsValid()))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Arbitrator pubkey invalid");
	
	/*// checking if mypk != destpubpubkey != arbitratorpubkey
	if(pubkey2pk(destpub).IsValid() && pubkey2pk(destpub) == mypk)
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Seller pubkey cannot be the same as destpub pubkey");
	if(pubkey2pk(arbitrator).IsValid() && pubkey2pk(arbitrator) == mypk)
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Seller pubkey cannot be the same as arbitrator pubkey");
	if(pubkey2pk(destpub).IsValid() && pubkey2pk(arbitrator).IsValid() && pubkey2pk(arbitrator) == pubkey2pk(destpub))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Buyer pubkey cannot be the same as arbitrator pubkey");*/
	
	// if arbitrator exists, check if arbitrator fee is sufficient
	if(pubkey2pk(arbitrator).IsValid()) {
		if(arbitratorfee == 0)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Arbitrator fee must be specified if valid arbitrator exists");
		else if(arbitratorfee < CC_MARKER_VALUE)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Arbitrator fee is too low");
	}
	else
		arbitratorfee = 0;
	
	// if arbitrator exists, check if deposit is sufficient
	if(pubkey2pk(arbitrator).IsValid()) {
		if(deposit == 0)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Deposit must be specified if valid arbitrator exists");
		else if(deposit < CC_MARKER_VALUE)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Deposit is too low");
	}
	else
		deposit = 0;
	
	// additional checks are done using ValidateRefProposalOpRet
	CScript opret = EncodeCommitmentProposalOpRet(COMMITMENTCC_VERSION,'p',std::vector<uint8_t>(mypk.begin(),mypk.end()),destpub,arbitrator,payment,arbitratorfee,deposit,datahash,refcommitmenttxid,prevproposaltxid,info);
	if (!ValidateRefProposalOpRet(opret, CCerror))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << CCerror);
	
	// check prevproposaltxid if specified
	if(prevproposaltxid != zeroid) {
		if(myGetTransaction(prevproposaltxid,prevproposaltx,hashBlock)==0 || (numvouts=prevproposaltx.vout.size())<=0)
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "cant find specified previous proposal txid " << prevproposaltxid.GetHex());
		if(DecodeCommitmentOpRet(prevproposaltx.vout[numvouts - 1].scriptPubKey) == 'c')
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "specified txid is a contract id, needs to be proposal id");
		
		if (!ValidateRefProposalOpRet(prevproposaltx.vout[numvouts-1].scriptPubKey, CCerror))
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "previous " << CCerror << " txid: " << prevproposaltxid.GetHex());
		
		DecodeCommitmentProposalOpRet(prevproposaltx.vout[numvouts-1].scriptPubKey,version,refProposalType,refInitiator,refReceiver,refArbitrator,refPrepayment,refArbitratorFee,refDeposit,refHash,refCommitmentTxid,refPrevProposalTxid,refName);
		
		if (IsProposalSpent(prevproposaltxid, spendingtxid, spendingfuncid)) {
			switch (spendingfuncid) {
				case 'p':
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been amended by txid " << spendingtxid.GetHex());
				case 'c':
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been accepted by txid " << spendingtxid.GetHex());
				case 't':
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been closed by txid " << spendingtxid.GetHex());
				default:
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been spent by txid " << spendingtxid.GetHex());
			}
		}
		
		/*if(retcode = CCgetspenttxid(spenttxid, vini, height, prevproposaltxid, 1) == 0) {
			if(myGetTransaction(spenttxid,spenttx,hashBlock)==0 || (numvouts=spenttx.vout.size())<=0)
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has already been amended by txid " << spenttxid.GetHex());
			else {
				if(DecodeCommitmentOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 't')
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has already been closed by txid " << spenttxid.GetHex());
				else if(DecodeCommitmentOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 'c')
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has already been accepted by txid " << spenttxid.GetHex());
			}
		}*/
		
		if(mypk != pubkey2pk(refInitiator))
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "-pubkey doesn't match creator of previous proposal txid " << prevproposaltxid.GetHex());
		if(pubkey2pk(destpub).IsValid() && !(refReceiver.empty()) && destpub != refReceiver)
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "destpub must be the same as specified in previous proposal txid " << prevproposaltxid.GetHex());
		if(!(pubkey2pk(destpub).IsValid()) && !(refReceiver.empty()))
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "cannot remove destpub when one exists in previous proposal txid " << prevproposaltxid.GetHex());
	}
	
	// check refcommitmenttxid if specified
	if(refcommitmenttxid != zeroid) {
		if(myGetTransaction(refcommitmenttxid,refcommitmenttx,hashBlock) == 0 || (numvouts=refcommitmenttx.vout.size()) <= 0)
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "cant find specified reference commitment txid " << refcommitmenttxid.GetHex());
		if(DecodeCommitmentOpRet(refcommitmenttx.vout[numvouts - 1].scriptPubKey) != 'c')
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "invalid reference commitment txid " << refcommitmenttxid.GetHex());
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
		mtx.vout.push_back(MakeCC1vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
		if(pubkey2pk(destpub).IsValid())
			mtx.vout.push_back(MakeCC1of2vout(EVAL_COMMITMENTS, CC_RESPONSE_VALUE, mypk, pubkey2pk(destpub))); // vout.1 response hook (with destpub)
		else
			mtx.vout.push_back(MakeCC1vout(EVAL_COMMITMENTS, CC_RESPONSE_VALUE, mypk)); // vout.1 response hook (no destpub)
		return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret));
	}
	CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "error adding normal inputs");
}

// commitmentrequestupdate - constructs a 'p' transaction, with the 'u' proposal type
// This transaction will only be validated if commitmenttxid is specified. Optionally, prevproposaltxid can be used to amend previous update requests.
UniValue CommitmentRequestUpdate(const CPubKey& pk, uint64_t txfee, uint256 commitmenttxid, uint256 datahash, std::vector<uint8_t> newarbitrator, uint256 prevproposaltxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	
	CTransaction prevproposaltx, refcommitmenttx;
	int32_t numvouts;
	uint256 hashBlock;
	
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	if(txfee == 0)
		txfee = 10000;
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	//stuff
	// if newarbitrator is 0, maintain current arbitrator status
	// don't allow swapping between no arbitrator <-> arbitrator
	// only 1 update/cancel request per party, per commitment
	
	CCERR_RESULT("commitmentscc",CCLOG_INFO,stream << "incomplete");
}

// commitmentrequestcancel - constructs a 'p' transaction, with the 't' proposal type
// This transaction will only be validated if commitmenttxid is specified. Optionally, prevproposaltxid can be used to amend previous cancel requests.
UniValue CommitmentRequestCancel(const CPubKey& pk, uint64_t txfee, uint256 commitmenttxid, uint256 datahash, uint64_t depositcut, uint256 prevproposaltxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	
	CTransaction prevproposaltx, refcommitmenttx;
	int32_t numvouts;
	uint256 hashBlock;
	
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	if(txfee == 0)
		txfee = 10000;
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	//stuff
	// only 1 update/cancel request per party, per commitment
	
	CCERR_RESULT("commitmentscc",CCLOG_INFO,stream << "incomplete");
}

// commitmentcloseproposal - constructs a 't' transaction and spends the specified 'p' transaction
// can always be done by the proposal initiator, as well as the receiver if they are able to accept the proposal.
UniValue CommitmentCloseProposal(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	
	CTransaction proposaltx, spenttx;
	int32_t numvouts, vini, height, retcode;
	uint256 hashBlock, refHash, refCommitmentTxid, refPrevProposalTxid, spenttxid;
	std::vector<uint8_t> refInitiator, refReceiver, refArbitrator;
	int64_t refPrepayment, refArbitratorFee, refDeposit;
	std::string refName;
	uint8_t refProposalType;
	
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	if(txfee == 0)
		txfee = 10000;
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());

	CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "not done yet");
	
	// check proposaltxid
	/*if(proposaltxid != zeroid) {
		if(myGetTransaction(proposaltxid,proposaltx,hashBlock)==0 || (numvouts=proposaltx.vout.size())<=0)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "cant find specified proposal txid " << proposaltxid.GetHex());
		if(DecodeCommitmentOpRet(proposaltx.vout[numvouts - 1].scriptPubKey) == 'c')
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "specified txid is a contract id, needs to be proposal id");
		else if(DecodeCommitmentProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,refProposalType,refInitiator,refReceiver,refArbitrator,refPrepayment,refArbitratorFee,refDeposit,refDepositCut,refHash,refCommitmentTxid,refPrevProposalTxid,refName) != 'p')
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "invalid proposal txid " << proposaltxid.GetHex());
		if(retcode = CCgetspenttxid(spenttxid, vini, height, proposaltxid, 1) == 0) {
			std::cerr << "Found a spenttxid" << std::endl;
			if(myGetTransaction(spenttxid,spenttx,hashBlock)==0 || (numvouts=spenttx.vout.size())<=0)
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has already been amended by txid " << spenttxid.GetHex());
			std::cerr << "Found the spenttx as well" << std::endl;
			else {
				if(DecodeCommitmentOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 't')
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has already been closed by txid " << spenttxid.GetHex());
				if(DecodeCommitmentOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 'c')
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has already been accepted by txid " << spenttxid.GetHex());
			}
		}
		if(mypk != pubkey2pk(refInitiator) && (!(refReceiver.empty()) && mypk != pubkey2pk(refReceiver)))
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "-pubkey must be either initiator or receiver of specified proposal txid " << proposaltxid.GetHex());
	}
	else
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Proposal transaction id must be specified");
	
	if(AddNormalinputs(mtx,mypk,txfee,64,pk.IsValid()) >= txfee) {
		mtx.vin.push_back(CTxIn(proposaltxid,0,CScript())); // vin.n-2 previous proposal marker (optional, will trigger validation)
		char mutualaddr[64];
		GetCCaddress1of2(cp, mutualaddr, pubkey2pk(refInitiator), pubkey2pk(refReceiver));
		mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (optional, will trigger validation)
		uint8_t mypriv[32];
		Myprivkey(mypriv);
		CCaddr1of2set(cp, pubkey2pk(refInitiator), pubkey2pk(refReceiver), mypriv, mutualaddr);
		return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeCommitmentProposalCloseOpRet(proposaltxid,std::vector<uint8_t>(mypk.begin(),mypk.end()))));
	}
	CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "error adding normal inputs");*/
}

// commitmentaccept - spend a 'p' transaction that was submitted by the other party.
// this function is context aware and does different things dependent on the proposal type:
// if txid opret has the 'p' proposal type, will create a 'c' transaction (create contract)
// if txid opret has the 'u' or 't' proposal type, will create a 'u' transaction (update contract)
UniValue CommitmentAccept(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	
	CTransaction proposaltx, spenttx;
	int32_t numvouts, vini, height, retcode;
	uint256 hashBlock, refHash, refCommitmentTxid, refPrevProposalTxid, spenttxid;
	std::vector<uint8_t> refInitiator, refReceiver, refArbitrator;
	int64_t refPrepayment, refArbitratorFee, refDeposit;
	std::string refName;
	uint8_t refProposalType, version;
	
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	if(txfee == 0)
		txfee = 10000;
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());

	// check if the proposal itself is valid for acceptance (not cancelled, etc.)
	if(proposaltxid != zeroid) {
		if(myGetTransaction(proposaltxid,proposaltx,hashBlock)==0 || (numvouts=proposaltx.vout.size())<=0)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "cant find specified proposal txid " << proposaltxid.GetHex());
		if(DecodeCommitmentOpRet(proposaltx.vout[numvouts - 1].scriptPubKey) == 'c')
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "specified txid is a contract id, needs to be proposal id");
		else if(DecodeCommitmentProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,version,refProposalType,refInitiator,refReceiver,refArbitrator,refPrepayment,refArbitratorFee,refDeposit,refHash,refCommitmentTxid,refPrevProposalTxid,refName) != 'p')
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "invalid proposal txid " << proposaltxid.GetHex());
		
		
		
		if(retcode = CCgetspenttxid(spenttxid, vini, height, proposaltxid, 1) == 0) {
			if(myGetTransaction(spenttxid,spenttx,hashBlock)==0 || (numvouts=spenttx.vout.size())<=0)
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has already been amended by txid " << spenttxid.GetHex());
			else {
				if(DecodeCommitmentOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 't')
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has already been closed by txid " << spenttxid.GetHex());
				else if(DecodeCommitmentOpRet(spenttx.vout[numvouts - 1].scriptPubKey) == 'c')
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has already been accepted by txid " << spenttxid.GetHex());
			}
		}
		
		if(refReceiver.empty())
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "proposal has no designated receiver");
		else if(mypk != pubkey2pk(refReceiver))
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "-pubkey must be receiver of specified proposal txid " << proposaltxid.GetHex());
	}
	else
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Proposal transaction id must be specified");

	// check if the data in the proposal is correct
	// stuff might be put here for general proposal checks
	switch(refProposalType)
	{
		case 'p':
			// check datahash
			if(refHash == zeroid)
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Data hash in proposal empty or invalid");
			
			// check payment
			if (refPrepayment < 0)
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Prepayment must be positive");
			
			// check if arbitrator pubkey exists and is valid
			if(!refArbitrator.empty() && !(pubkey2pk(refArbitrator).IsValid()))
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Arbitrator pubkey in proposal invalid");
			
			// checking if mypk != initiatorpubkey != arbitratorpubkey
			if(pubkey2pk(refInitiator).IsValid() && pubkey2pk(refInitiator) == mypk)
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Initiator pubkey cannot accept their own commitment");
			if(pubkey2pk(refArbitrator).IsValid() && pubkey2pk(refArbitrator) == mypk)
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Receiver pubkey cannot be the same as arbitrator pubkey");
			if(pubkey2pk(refInitiator).IsValid() && pubkey2pk(refArbitrator).IsValid() && pubkey2pk(refArbitrator) == pubkey2pk(refInitiator))
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Buyer pubkey cannot be the same as arbitrator pubkey");
			
			// if arbitrator exists, check if deposit and arbitrator fee are sufficient
			if(!(refArbitrator.empty())) {
				if(refArbitratorFee < CC_MARKER_VALUE)
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Arbitrator is defined in proposal, but arbitrator fee is too low");
				if(refDeposit < CC_MARKER_VALUE)
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Arbitrator is defined in proposal, but deposit is too low");
			}
			else {
				refArbitratorFee = 0;
				refDeposit = CC_MARKER_VALUE;
			}
			
			if(AddNormalinputs(mtx,mypk,txfee+CC_MARKER_VALUE+CC_MARKER_VALUE*2+refDeposit+refPrepayment,64,pk.IsValid()) >= txfee+CC_MARKER_VALUE+CC_MARKER_VALUE*2+refDeposit+refPrepayment) {
				mtx.vin.push_back(CTxIn(proposaltxid,0,CScript())); // vin.n-2 previous proposal marker (will trigger validation)
				char mutualaddr[64];
				GetCCaddress1of2(cp, mutualaddr, pubkey2pk(refInitiator), pubkey2pk(refReceiver));
				mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (will trigger validation)
				uint8_t mypriv[32];
				Myprivkey(mypriv);
				CCaddr1of2set(cp, pubkey2pk(refInitiator), pubkey2pk(refReceiver), mypriv, mutualaddr);
				mtx.vout.push_back(MakeCC1vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
				mtx.vout.push_back(MakeCC1of2vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, mypk, pubkey2pk(refInitiator))); // vout.1 update baton
				mtx.vout.push_back(MakeCC1of2vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, mypk, pubkey2pk(refInitiator))); // vout.2 dispute baton
				if(pubkey2pk(refArbitrator).IsValid())
					mtx.vout.push_back(MakeCC1of2vout(EVAL_COMMITMENTS, refDeposit, mypk, pubkey2pk(refArbitrator))); // vout.3 deposit / commitment completion marker (with arbitrator)
				else
					mtx.vout.push_back(MakeCC1vout(EVAL_COMMITMENTS, refDeposit, mypk)); // vout.3 deposit / commitment completion marker (without arbitrator)
				mtx.vout.push_back(CTxOut(refPrepayment,CScript() << ParseHex(HexStr(refInitiator)) << OP_CHECKSIG)); // vout.4 payment
				return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeCommitmentSigningOpRet(COMMITMENTCC_VERSION,proposaltxid)));
			}
			else
				CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "error adding normal inputs");

		case 'u':
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "update of contract not supported yet");
			
		case 't':
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "termination of contract not supported yet");
		
		default:
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "invalid proposal type for proposal txid " << proposaltxid.GetHex());
	}
}

// commitmentdispute
// commitmentresolve

//===========================================================================
// RPCs - informational
//===========================================================================

// commitmentinfo

/*
UniValue CommitmentInfo(uint256 txid)
{
    UniValue result(UniValue::VOBJ);
    uint256 hashBlock, latesttxid, datahash;
    CTransaction tokenbaseTx, latestUpdateTx;
	CScript batonopret;
    std::vector<uint8_t> origpubkey;
    std::vector<std::pair<uint8_t, vscript_t>> oprets;
    vscript_t vopretNonfungible;
    std::string info, description, ccode;
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

    if (tokenbaseTx.vout.size() > 0 && DecodeTokenCreateOpRet(tokenbaseTx.vout[tokenbaseTx.vout.size() - 1].scriptPubKey, origpubkey, info, description, ownerPerc, oprets) != 'c') {
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "TokenInfo() passed tokenid isnt token creation txid" << std::endl);
        result.push_back(Pair("result", "error"));
        result.push_back(Pair("error", "tokenid isnt token creation txid"));
        return result;
    }

    result.push_back(Pair("result", "success"));
    result.push_back(Pair("tokenid", tokenid.GetHex()));
    result.push_back(Pair("owner", HexStr(origpubkey)));
    result.push_back(Pair("info", info));

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

// commitmentviewupdates [list]
// commitmentviewdisputes [list]
// commitmentinventory([pubkey])

UniValue CommitmentList()
{
	UniValue result(UniValue::VARR);
	std::vector<uint256> txids, foundtxids;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressIndexCCMarker;

	struct CCcontract_info *cp, C; uint256 txid, hashBlock;
	CTransaction vintx; std::vector<uint8_t> origpubkey;
	std::string info, description;
	uint8_t proposaltype;

	cp = CCinit(&C, EVAL_COMMITMENTS);

	auto addCommitmentTxid = [&](uint256 txid) {
		if (myGetTransaction(txid, vintx, hashBlock) != 0) {
			if (vintx.vout.size() > 0 && DecodeCommitmentOpRet(vintx.vout[vintx.vout.size() - 1].scriptPubKey) != 0) {
				if (std::find(foundtxids.begin(), foundtxids.end(), txid) == foundtxids.end()) {
					result.push_back(txid.GetHex());
					foundtxids.push_back(txid);
				}
			}
		}
	};

	SetCCunspents(addressIndexCCMarker,cp->unspendableCCaddr,true);
	for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressIndexCCMarker.begin(); it != addressIndexCCMarker.end(); it++) {
		addCommitmentTxid(it->first.txhash);
	}

	return(result);
}
