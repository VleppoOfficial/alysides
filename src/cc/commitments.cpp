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
Things to potentially descope:
"Draft" proposals (no receiver)
Dynamic arbitrator fees
the 'info' field

Commitments RPCs:

	commitmentcreate (info datahash destpub arbitrator [arbitratorfee][deposit][prevproposaltxid][refcommitmenttxid])
	commitmentupdate(commitmenttxid info datahash [newarbitrator][prevproposaltxid])
	commitmentclose(commitmenttxid info datahash [depositsplit][prevproposaltxid])
	Proposal creation
	
	commitmentaccept(proposaltxid)
	commitmentstopproposal(proposaltxid message)
	Proposal response
	
	commitmentdispute(commitmenttxid disputetype [disputehash])
	commitmentresolve(commitmenttxid disputetxid verdict [rewardedpubkey])
	Disputes
	
	commitmentunlock(commitmenttxid exchangetxid)
		case 'n'
	commitmentaddress
	commitmentlist
	commitmentinfo(txid)
	commitmentviewupdates(commitmenttxid [samplenum][recursive])
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
	std::string dummystr;
	uint8_t evalcode, funcid, *script, dummytype;
	bool dummybool;
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
			return DecodeCommitmentProposalOpRet(scriptPubKey, dummytype, dummytype, dummypk, dummypk, dummypk, dummyamount, dummyamount, dummyamount, dummyhash, dummyhash, dummyhash, dummystr);
		case 't':
			return DecodeCommitmentProposalCloseOpRet(scriptPubKey, dummytype, dummyhash, dummypk);
		case 'c':
			return DecodeCommitmentSigningOpRet(scriptPubKey, dummytype, dummyhash);
		case 'u':
			return DecodeCommitmentUpdateOpRet(scriptPubKey, dummytype, dummyhash, dummyhash);
		case 's':
			return DecodeCommitmentCloseOpRet(scriptPubKey, dummytype, dummyhash, dummyhash);
		case 'd':
			return DecodeCommitmentDisputeOpRet(scriptPubKey, dummytype, dummyhash, dummyhash);
		case 'r':
			return DecodeCommitmentDisputeResolveOpRet(scriptPubKey, dummytype, dummyhash, dummyhash, dummybool);
		case 'n':
			return DecodeCommitmentUnlockOpRet(scriptPubKey, dummytype, dummyhash, dummyhash);
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
CScript EncodeCommitmentCloseOpRet(uint8_t version, uint256 commitmenttxid, uint256 proposaltxid)
{
	CScript opret; uint8_t evalcode = EVAL_COMMITMENTS, funcid = 's';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << commitmenttxid << proposaltxid);
	return(opret);
}
uint8_t DecodeCommitmentCloseOpRet(CScript scriptPubKey, uint8_t &version, uint256 &commitmenttxid, uint256 &proposaltxid)
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
CScript EncodeCommitmentDisputeResolveOpRet(uint8_t version, uint256 commitmenttxid, uint256 disputetxid, bool rewardsender)
{
	CScript opret; uint8_t evalcode = EVAL_COMMITMENTS, funcid = 'r';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << commitmenttxid << disputetxid << rewardsender);
	return(opret);
}
uint8_t DecodeCommitmentDisputeResolveOpRet(CScript scriptPubKey, uint8_t &version, uint256 &commitmenttxid, uint256 &disputetxid, bool &rewardsender)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> commitmenttxid; ss >> disputetxid; ss >> rewardsender) != 0 && evalcode == EVAL_COMMITMENTS)
		return(funcid);
	return(0);
}
CScript EncodeCommitmentUnlockOpRet(uint8_t version, uint256 commitmenttxid, uint256 exchangetxid)
{
	CScript opret; uint8_t evalcode = EVAL_COMMITMENTS, funcid = 'n';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << commitmenttxid << exchangetxid);
	return(opret);
}
uint8_t DecodeCommitmentUnlockOpRet(CScript scriptPubKey, uint8_t &version, uint256 &commitmenttxid, uint256 &exchangetxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> commitmenttxid; ss >> exchangetxid) != 0 && evalcode == EVAL_COMMITMENTS)
		return(funcid);
	return(0);
}

//===========================================================================
// Validation
//===========================================================================

bool CommitmentsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	CTransaction proposaltx;
	CScript proposalopret;
	int32_t numvins, numvouts;
	uint256 hashBlock, commitmenttxid, proposaltxid, prevproposaltxid, dummytxid, spendingtxid, latesttxid;
	std::vector<uint8_t> srcpub, destpub, signpub, sellerpk, clientpk, arbitratorpk;
	int64_t payment, arbitratorfee, depositval, totaldeposit;
	std::string info, CCerror = "";
	bool bHasReceiver, bHasArbitrator;
	uint8_t proposaltype, version, spendingfuncid, funcid, updatefuncid;
	char markeraddr[65], srcaddr[65], destaddr[65], depositaddr[65];
	CPubKey CPK_src, CPK_dest, CPK_arbitrator, CPK_signer;

	numvins = tx.vin.size();
    numvouts = tx.vout.size();
    if (numvouts < 1)
        return eval->Invalid("no vouts");
	CCOpretCheck(eval,tx,true,true,true);
    CCExactAmounts(eval,tx,CC_TXFEE);
	if ((funcid = DecodeCommitmentOpRet(tx.vout[numvouts-1].scriptPubKey)) != 0) {
		GetCCaddress(cp, markeraddr, GetUnspendable(cp, NULL));
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
				// Proposal data validation.
				if (!ValidateProposalOpRet(tx.vout[numvouts-1].scriptPubKey, CCerror))
					return eval->Invalid(CCerror);
				DecodeCommitmentProposalOpRet(tx.vout[numvouts-1].scriptPubKey, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, dummytxid, commitmenttxid, prevproposaltxid, info);
				CPK_src = pubkey2pk(srcpub);
				CPK_dest = pubkey2pk(destpub);
				bHasReceiver = CPK_dest.IsValid();
				if (TotalPubkeyNormalInputs(tx, CPK_src) == 0 && TotalPubkeyCCInputs(tx, CPK_src) == 0)
					return eval->Invalid("found no normal or cc inputs signed by source pubkey!");
				if (prevproposaltxid != zeroid) {
					// Checking if selected proposal was already spent.
					if (IsProposalSpent(prevproposaltxid, spendingtxid, spendingfuncid))
						return eval->Invalid("prevproposal has already been spent!");
					// Comparing proposal data to previous proposal.
					if (!CompareProposals(tx.vout[numvouts-1].scriptPubKey, prevproposaltxid, CCerror))
						return eval->Invalid(CCerror);
				}
				else
					return eval->Invalid("unexpected proposal with no prevproposaltxid in CommitmentsValidate!");
				if (bHasReceiver)
					GetCCaddress1of2(cp, destaddr, CPK_src, CPK_dest);
				else
					GetCCaddress(cp, destaddr, CPK_src);
				// Checking if vins/vouts are correct.
				if (numvouts < 3)
					return eval->Invalid("not enough vouts for 'p' tx!");
				else if (ConstrainVout(tx.vout[0], 1, markeraddr, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.0 must be CC marker to commitments global address!");
				else if (ConstrainVout(tx.vout[1], 1, destaddr, CC_RESPONSE_VALUE) == 0)
					return eval->Invalid("vout.1 must be CC baton to mutual or srcpub CC address!");
				if (numvins < 3)
					return eval->Invalid("not enough vins for 'p' tx in CommitmentsValidate!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal for commitmentcreate!");
				else if (IsCCInput(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for commitmentcreate!");
				// does vin1 and vin2 point to the previous proposal's vout0 and vout1? (if it doesn't, the tx might have no previous proposal, in that case it shouldn't have CC inputs)
				else if (tx.vin[1].prevout.hash != prevproposaltxid || tx.vin[1].prevout.n != 0)
					return eval->Invalid("vin.1 tx hash doesn't match prevproposaltxid!");
				else if (IsCCInput(tx.vin[2].scriptSig) == 0)
					return eval->Invalid("vin.2 must be CC for commitmentcreate!");
				else if (tx.vin[2].prevout.hash != prevproposaltxid || tx.vin[2].prevout.n != 1)
					return eval->Invalid("vin.2 tx hash doesn't match prevproposaltxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 3; i > numvins; i++) {
					if (IsCCInput(tx.vin[i].scriptSig) != 0)
						return eval->Invalid("tx exceeds allowed amount of CC vins!");
				}
				break;
			case 't':
				/*
				proposal cancel:
				vin.0 normal input
				vin.1 previous proposal baton
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 't' proposaltxid signpub
				*/
				// Getting the transaction data.
				DecodeCommitmentProposalCloseOpRet(tx.vout[numvouts-1].scriptPubKey, version, proposaltxid, signpub);
				if (myGetTransaction(proposaltxid, proposaltx, hashBlock) == 0 || proposaltx.vout.size() <= 0)
					return eval->Invalid("couldn't find proposaltx for 't' tx!");
				if (IsProposalSpent(proposaltxid, spendingtxid, spendingfuncid))
					return eval->Invalid("prevproposal has already been spent!");
				// Retrieving the proposal data tied to this transaction.
				DecodeCommitmentProposalOpRet(proposaltx.vout[proposaltx.vout.size()-1].scriptPubKey, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, dummytxid, commitmenttxid, prevproposaltxid, info);
				CPK_src = pubkey2pk(srcpub);
				CPK_signer = pubkey2pk(signpub);
				CPK_dest = pubkey2pk(destpub);
				bHasReceiver = CPK_dest.IsValid();
				if (TotalPubkeyNormalInputs(tx, CPK_signer) == 0 && TotalPubkeyCCInputs(tx, CPK_signer) == 0)
					return eval->Invalid("found no normal or cc inputs signed by signer pubkey!");
				// We won't be using ValidateProposalOpRet here, since we only care about initiator and receiver, and even if the other data in the proposal is invalid the proposal will be closed anyway
				// Instead we'll just check if the initiator of this tx is allowed to close the proposal.
				switch (proposaltype) {
					case 'p':
						if (bHasReceiver && CPK_signer != CPK_src && CPK_signer != CPK_dest)
							return eval->Invalid("signpub is not the source or receiver of specified proposal!");
						else if (!bHasReceiver && CPK_signer != CPK_src)
							return eval->Invalid("signpub is not the source of specified proposal!");
						break;
					case 'u':
					case 't':
						if (commitmenttxid == zeroid)
							return eval->Invalid("proposal has no defined commitment, unable to verify membership!");
						if (!GetCommitmentInitialData(commitmenttxid, dummytxid, sellerpk, clientpk, arbitratorpk, arbitratorfee, depositval, dummytxid, dummytxid, info))
							return eval->Invalid("couldn't get proposal's commitment info successfully!");
						if (CPK_signer != CPK_src && CPK_signer != CPK_dest && CPK_signer != pubkey2pk(sellerpk) && CPK_signer != pubkey2pk(clientpk))
							return eval->Invalid("signpub is not the source or receiver of specified proposal!");
						break;
					default:
						return eval->Invalid("invalid proposaltype!");
				}
				// Checking if vins are correct. (we don't care about the vouts in this case)
				if (numvins < 2)
					return eval->Invalid("not enough vins for 't' tx!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal for commitmentstopproposal!");
				else if (IsCCInput(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for commitmentstopproposal!");
				// does vin1 point to the previous proposal's vout1?
				else if (tx.vin[1].prevout.hash != proposaltxid || tx.vin[1].prevout.n != 1)
					return eval->Invalid("vin.1 tx hash doesn't match proposaltxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 2; i > numvins; i++) {
					if (IsCCInput(tx.vin[i].scriptSig) != 0)
						return eval->Invalid("tx exceeds allowed amount of CC vins!");
				}
				break;
			case 'c':
				/*
				contract creation:
				vin.0 normal input
				vin.1 proposal marker
				vin.2 proposal baton
				vout.0 marker
				vout.1 update/dispute baton
				vout.2 deposit / commitment completion baton
				vout.3 payment (optional)
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 'c' proposaltxid
				*/
				// Getting the transaction data.
				if (!GetAcceptedProposalOpRet(tx, proposaltxid, proposalopret))
					return eval->Invalid("couldn't find proposal tx opret for 'c' tx!");
				// Retrieving the proposal data tied to this transaction.
				DecodeCommitmentProposalOpRet(proposalopret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, dummytxid, commitmenttxid, prevproposaltxid, info);
				CPK_src = pubkey2pk(srcpub);
				CPK_dest = pubkey2pk(destpub);
				CPK_arbitrator = pubkey2pk(arbitratorpk);
				bHasReceiver = CPK_dest.IsValid();
				bHasArbitrator = CPK_arbitrator.IsValid();
				// Proposal data validation.
				if (!ValidateProposalOpRet(proposalopret, CCerror))
					return eval->Invalid(CCerror);
				if (proposaltype != 'p')
					return eval->Invalid("attempting to create 'c' tx for non-'p' proposal type!");
				if (!bHasReceiver)
					return eval->Invalid("proposal doesn't have valid destpub!");
				if (IsProposalSpent(proposaltxid, spendingtxid, spendingfuncid))
					return eval->Invalid("prevproposal has already been spent!");
				if (TotalPubkeyNormalInputs(tx, CPK_dest) == 0 && TotalPubkeyCCInputs(tx, CPK_dest) == 0)
					return eval->Invalid("found no normal or cc inputs signed by proposal receiver pubkey!");
				Getscriptaddress(srcaddr, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG);
				GetCCaddress1of2(cp, destaddr, CPK_src, CPK_dest);
				if (bHasArbitrator)
					GetCCaddress1of2(cp, depositaddr, CPK_dest, CPK_arbitrator);
				else
					GetCCaddress(cp, depositaddr, CPK_dest);
				// Checking if vins/vouts are correct.
				if (numvouts < 4)
					return eval->Invalid("not enough vouts for 'c' tx!");
				else if (ConstrainVout(tx.vout[0], 1, markeraddr, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.0 must be CC marker to commitments global address!");
				else if (ConstrainVout(tx.vout[1], 1, destaddr, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.1 must be CC baton to mutual CC address!");
				else if (ConstrainVout(tx.vout[2], 1, depositaddr, depositval) == 0)
					return eval->Invalid("vout.2 must be deposit to CC address!");
				else if (payment > 0 && ConstrainVout(tx.vout[3], 0, srcaddr, payment) == 0)
					return eval->Invalid("vout.3 must be normal payment to srcaddr when payment defined!");
				if (numvins < 3)
					return eval->Invalid("not enough vins for 'c' tx!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal for 'c' tx!");
				else if (IsCCInput(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for 'c' tx!");
				// does vin1 and vin2 point to the proposal's vout0 and vout1?
				else if (tx.vin[1].prevout.hash != proposaltxid || tx.vin[1].prevout.n != 0)
					return eval->Invalid("vin.1 tx hash doesn't match proposaltxid!");
				else if (IsCCInput(tx.vin[2].scriptSig) == 0)
					return eval->Invalid("vin.2 must be CC for 'c' tx!");
				else if (tx.vin[2].prevout.hash != proposaltxid || tx.vin[2].prevout.n != 1)
					return eval->Invalid("vin.2 tx hash doesn't match proposaltxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 3; i > numvins; i++) {
					if (IsCCInput(tx.vin[i].scriptSig) != 0)
						return eval->Invalid("tx exceeds allowed amount of CC vins!");
				}
				break;
			case 'u':
				/*
				contract update:
				vin.0 normal input
				vin.1 last update baton
				vin.2 update proposal marker
				vin.3 update proposal response
				vout.0 next update baton
				vout.1 payment (optional)
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 'u' commitmenttxid proposaltxid
				*/
				// Getting the transaction data.
				if (!GetAcceptedProposalOpRet(tx, proposaltxid, proposalopret))
					return eval->Invalid("couldn't find proposal tx opret for 'u' tx!");
				// Retrieving the proposal data tied to this transaction.
				DecodeCommitmentProposalOpRet(proposalopret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, dummytxid, commitmenttxid, prevproposaltxid, info);
				CPK_src = pubkey2pk(srcpub);
				CPK_dest = pubkey2pk(destpub);
				bHasReceiver = CPK_dest.IsValid();
				// Proposal data validation.
				if (!ValidateProposalOpRet(proposalopret, CCerror))
					return eval->Invalid(CCerror);
				if (proposaltype != 'u')
					return eval->Invalid("attempting to create 'u' tx for non-'u' proposal type!");
				if (!bHasReceiver)
					return eval->Invalid("proposal doesn't have valid destpub!");
				if (IsProposalSpent(proposaltxid, spendingtxid, spendingfuncid))
					return eval->Invalid("prevproposal has already been spent!");
				if (TotalPubkeyNormalInputs(tx, CPK_dest) == 0 && TotalPubkeyCCInputs(tx, CPK_dest) == 0)
					return eval->Invalid("found no normal or cc inputs signed by proposal receiver pubkey!");
				// Getting the latest update txid.
				GetLatestCommitmentUpdate(commitmenttxid, latesttxid, updatefuncid);
				Getscriptaddress(srcaddr, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG);
				GetCCaddress1of2(cp, destaddr, CPK_src, CPK_dest);
				// Checking if vins/vouts are correct.
				if (numvouts < 3)
					return eval->Invalid("not enough vouts for 'u' tx!");
				else if (ConstrainVout(tx.vout[0], 1, destaddr, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.0 must be CC baton to mutual CC address!");
				else if (payment > 0 && ConstrainVout(tx.vout[1], 0, srcaddr, payment) == 0)
					return eval->Invalid("vout.1 must be normal payment to srcaddr when payment defined!");
				if (numvins < 4)
					return eval->Invalid("not enough vins for 'u' tx!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal for 'u' tx!");
				// does vin1 point to latesttxid baton?
				else if (IsCCInput(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for 'u' tx!");
				else if (latesttxid == commitmenttxid) {
					if (tx.vin[1].prevout.hash != commitmenttxid || tx.vin[1].prevout.n != 1)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				else if (latesttxid != commitmenttxid) {
					if (tx.vin[1].prevout.hash != latesttxid || tx.vin[1].prevout.n != 0)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				else if (IsCCInput(tx.vin[2].scriptSig) == 0)
					return eval->Invalid("vin.2 must be CC for 'u' tx!");
				// does vin2 and vin3 point to the proposal's vout0 and vout1?
				else if (tx.vin[2].prevout.hash != proposaltxid || tx.vin[2].prevout.n != 0)
					return eval->Invalid("vin.2 tx hash doesn't match proposaltxid!");
				else if (IsCCInput(tx.vin[3].scriptSig) == 0)
					return eval->Invalid("vin.3 must be CC for 'u' tx!");
				else if (tx.vin[3].prevout.hash != proposaltxid || tx.vin[3].prevout.n != 1)
					return eval->Invalid("vin.3 tx hash doesn't match proposaltxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 4; i > numvins; i++) {
					if (IsCCInput(tx.vin[i].scriptSig) != 0)
						return eval->Invalid("tx exceeds allowed amount of CC vins!");
				}
				break;
			case 's':
				/*
				contract close:
				vin.0 normal input
				vin.1 last update baton
				vin.2 update proposal marker
				vin.3 update proposal response
				vin.4 deposit
				vout.0 deposit cut to proposal creator
				vout.1 payment (optional)
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 's' commitmenttxid proposaltxid
				*/
				// Getting the transaction data.
				if (!GetAcceptedProposalOpRet(tx, proposaltxid, proposalopret))
					return eval->Invalid("couldn't find proposal tx opret for 's' tx!");
				// Retrieving the proposal data tied to this transaction.
				DecodeCommitmentProposalOpRet(proposalopret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, dummytxid, commitmenttxid, prevproposaltxid, info);
				GetCommitmentInitialData(commitmenttxid, dummytxid, sellerpk, clientpk, arbitratorpk, arbitratorfee, totaldeposit, dummytxid, dummytxid, info);
				CPK_src = pubkey2pk(srcpub);
				CPK_dest = pubkey2pk(destpub);
				CPK_arbitrator = pubkey2pk(arbitratorpk);
				bHasReceiver = CPK_dest.IsValid();
				bHasArbitrator = CPK_arbitrator.IsValid();
				// Proposal data validation.
				if (!ValidateProposalOpRet(proposalopret, CCerror))
					return eval->Invalid(CCerror);
				if (proposaltype != 't')
					return eval->Invalid("attempting to create 's' tx for non-'t' proposal type!");
				if (!bHasReceiver)
					return eval->Invalid("proposal doesn't have valid destpub!");
				if (IsProposalSpent(proposaltxid, spendingtxid, spendingfuncid))
					return eval->Invalid("prevproposal has already been spent!");
				if (TotalPubkeyNormalInputs(tx, CPK_dest) == 0 && TotalPubkeyCCInputs(tx, CPK_dest) == 0)
					return eval->Invalid("found no normal or cc inputs signed by proposal receiver pubkey!");
				// Getting the latest update txid.
				GetLatestCommitmentUpdate(commitmenttxid, latesttxid, updatefuncid);
				Getscriptaddress(srcaddr, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG);
				/*if (bHasArbitrator)
					GetCCaddress1of2(cp, depositaddr, CPK_dest, CPK_arbitrator);
				else
					GetCCaddress(cp, depositaddr, CPK_dest);*/
				// Checking if vins/vouts are correct.
				if (numvouts < 3)
					return eval->Invalid("not enough vouts for 's' tx!");
				else if (ConstrainVout(tx.vout[0], 0, srcaddr, depositval) == 0)
					return eval->Invalid("vout.0 must be normal deposit cut to srcaddr!");
				else if (payment > 0 && ConstrainVout(tx.vout[1], 0, srcaddr, payment) == 0)
					return eval->Invalid("vout.1 must be normal payment to srcaddr when payment defined!");
				
				if (numvins < 5)
					return eval->Invalid("not enough vins for 's' tx!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal for 's' tx!");
				// does vin1 point to latesttxid baton?
				else if (IsCCInput(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for 's' tx!");
				else if (latesttxid == commitmenttxid) {
					if (tx.vin[1].prevout.hash != commitmenttxid || tx.vin[1].prevout.n != 1)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				else if (latesttxid != commitmenttxid) {
					if (tx.vin[1].prevout.hash != latesttxid || tx.vin[1].prevout.n != 0)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				else if (IsCCInput(tx.vin[2].scriptSig) == 0)
					return eval->Invalid("vin.2 must be CC for 's' tx!");
				// does vin2 and vin3 point to the proposal's vout0 and vout1?
				else if (tx.vin[2].prevout.hash != proposaltxid || tx.vin[2].prevout.n != 0)
					return eval->Invalid("vin.2 tx hash doesn't match proposaltxid!");
				else if (IsCCInput(tx.vin[3].scriptSig) == 0)
					return eval->Invalid("vin.3 must be CC for 's' tx!");
				else if (tx.vin[3].prevout.hash != proposaltxid || tx.vin[3].prevout.n != 1)
					return eval->Invalid("vin.3 tx hash doesn't match proposaltxid!");
				else if (IsCCInput(tx.vin[4].scriptSig) == 0)
					return eval->Invalid("vin.4 must be CC for 's' tx!");
				else if (tx.vin[4].prevout.hash != commitmenttxid || tx.vin[4].prevout.n != 2)
					return eval->Invalid("vin.4 tx hash doesn't match commitmenttxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 5; i > numvins; i++) {
					if (IsCCInput(tx.vin[i].scriptSig) != 0)
						return eval->Invalid("tx exceeds allowed amount of CC vins!");
				}
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
			case 'n':
				/*
				unlock contract deposit:
				vin.0 normal input
				vin.1 deposit
				vout.0 payout to exchange CC 1of2 address
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_COMMITMENTS 'n' commitmenttxid exchangetxid
				*/
				// this part will be needed to validate Exchanges that withdraw the deposit. 
				return eval->Invalid("no validation for 'n' tx yet!");
			default:
                fprintf(stderr,"unexpected commitments funcid (%c)\n",funcid);
                return eval->Invalid("unexpected commitments funcid!");
		}
    }
	else 
		return eval->Invalid("must be valid commitments funcid!"); 
	LOGSTREAM("commitments", CCLOG_INFO, stream << "Commitments tx validated" << std::endl);
	std::cerr << "Commitments tx validated" << std::endl;
    return true;
}

//===========================================================================
// Helper functions
//===========================================================================

// gets the opret object of a proposal transaction that was or is being accepted by its receiver. 
// use this to extract data for validating "accept" txes like 'c', 'u' or 's'
// takes a CTransaction as input, returns true if its proposal opret was returned succesfully
bool GetAcceptedProposalOpRet(CTransaction tx, uint256 &proposaltxid, CScript &opret)
{
	CTransaction proposaltx;
	uint8_t version, funcid;
	uint256 commitmenttxid, hashBlock;
	int64_t deposit_send, deposit_keep;
	
	if (tx.vout.size() <= 0) {
		std::cerr << "GetAcceptedProposalOpRet: given tx has no vouts" << std::endl;
		return false;
	}
	if ((funcid = DecodeCommitmentOpRet(tx.vout[tx.vout.size() - 1].scriptPubKey)) != 'c' && funcid != 'u' && funcid != 's') {
		std::cerr << "GetAcceptedProposalOpRet: given tx doesn't have a correct funcid" << std::endl;
		return false;
	}
	switch (funcid) {
		case 'c':
			DecodeCommitmentSigningOpRet(tx.vout[tx.vout.size() - 1].scriptPubKey, version, proposaltxid);
			break;
		case 'u':
			DecodeCommitmentUpdateOpRet(tx.vout[tx.vout.size() - 1].scriptPubKey, version, commitmenttxid, proposaltxid);
			break;
		case 's':
			DecodeCommitmentCloseOpRet(tx.vout[tx.vout.size() - 1].scriptPubKey, version, commitmenttxid, proposaltxid);
			break;
	}
	if (myGetTransaction(proposaltxid, proposaltx, hashBlock) == 0 || proposaltx.vout.size() <= 0) {
		std::cerr << "GetAcceptedProposalOpRet: couldn't find commitment accepted proposal tx" << std::endl;
		return false;
	}
	opret = proposaltx.vout[proposaltx.vout.size() - 1].scriptPubKey;
	return true;
}

// checks if the data in the given proposal opret is valid.
// used in validation and some RPCs
bool ValidateProposalOpRet(CScript opret, std::string &CCerror)
{
	CTransaction commitmenttx;
	uint256 proposaltxid, datahash, commitmenttxid, refcommitmenttxid, prevproposaltxid, spendingtxid, hashBlock;
	uint8_t version, proposaltype, spendingfuncid;
	std::vector<uint8_t> srcpub, destpub, sellerpk, clientpk, arbitratorpk, ref_arbitratorpk;
	int64_t payment, arbitratorfee, depositval, ref_depositval;
	std::string info;
	bool bHasReceiver, bHasArbitrator;
	CPubKey CPK_src, CPK_dest, CPK_arbitrator;
	CCerror = "";
	
	LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: decoding opret" << std::endl);
	if (DecodeCommitmentProposalOpRet(opret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, datahash, commitmenttxid, prevproposaltxid, info) != 'p') {
		CCerror = "proposal tx opret invalid or not a proposal tx!";
		return false;
	}
	LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: check if info meets requirements (not empty, <= 2048 chars)" << std::endl);
	if (info.empty() || info.size() > 2048) {
		CCerror = "proposal info empty or exceeds 2048 chars!";
		return false;
	}
	LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: check if datahash meets requirements (not empty)" << std::endl);
	if (datahash == zeroid) {
		CCerror = "proposal datahash empty!";
		return false;
	}
	LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: check if payment is positive" << std::endl);
	if (payment < 0) {
		CCerror = "proposal has payment < 0!";
		return false;
	}
	CPK_src = pubkey2pk(srcpub);
	CPK_dest = pubkey2pk(destpub);
	CPK_arbitrator = pubkey2pk(arbitratorpk);
	bHasReceiver = CPK_dest.IsValid();
	bHasArbitrator = CPK_arbitrator.IsValid();
	LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: making sure srcpub != destpub != arbitratorpk" << std::endl);
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
			LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: checking deposit value" << std::endl);
			if (depositval < CC_MARKER_VALUE) {
				CCerror = "proposal doesn't have minimum required deposit!";
				return false;
			}
			LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: checking arbitrator fee" << std::endl);
			if (arbitratorfee < 0 || bHasArbitrator && arbitratorfee < CC_MARKER_VALUE) {
				CCerror = "proposal has invalid arbitrator fee value!";
				return false;
			}
			if (commitmenttxid != zeroid) {
				LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: refcommitment was defined, check if it's a correct tx" << std::endl);
				if (myGetTransaction(commitmenttxid, commitmenttx, hashBlock) == 0 || commitmenttx.vout.size() <= 0) {
					CCerror = "proposal's refcommitmenttxid has nonexistent tx!";
					return false;
				}
				if (DecodeCommitmentSigningOpRet(commitmenttx.vout[commitmenttx.vout.size() - 1].scriptPubKey, version, proposaltxid) != 'c') {
					CCerror = "proposal refcommitment tx is not a contract signing tx!";
					return false;	
				}
				LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: checking if subcontract's srcpub and destpub are members of the refcommitment" << std::endl);
				if (!GetCommitmentInitialData(commitmenttxid, proposaltxid, sellerpk, clientpk, arbitratorpk, arbitratorfee, depositval, datahash, refcommitmenttxid, info)) {
					CCerror = "refcommitment tx has invalid commitment member pubkeys!";
					return false;
				}
				LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: checking arbitrator fee" << std::endl);
				if (arbitratorfee < 0 || bHasArbitrator && arbitratorfee < CC_MARKER_VALUE) {
					CCerror = "proposal has invalid arbitrator fee value!";
					return false;
				}
				if (!bHasReceiver || CPK_src != pubkey2pk(sellerpk) && CPK_src != pubkey2pk(clientpk) && CPK_dest != pubkey2pk(sellerpk) && CPK_dest != pubkey2pk(clientpk)) {
					CCerror = "subcontracts must have at least one party that's a member in the refcommitmenttxid!";
					return false;
				}
			}
			break;
		case 'u':
			LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: checking deposit value for 'u' tx" << std::endl);
			if (depositval != 0) {
				CCerror = "proposal has invalid deposit value for update!";
				return false;
			}
		// intentional fall-through
		case 't':
			LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: checking if update/termination proposal has destpub" << std::endl);
			if (!bHasReceiver) {
				CCerror = "proposal has no defined receiver on update/termination proposal!";
				return false;
			}
			LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: checking if commitmenttxid defined" << std::endl);
			if (commitmenttxid == zeroid) {
				CCerror = "proposal has no commitmenttxid defined for update/termination proposal!";
				return false;
			}
			LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: commitmenttxid status check" << std::endl);
			if (GetLatestCommitmentUpdate(commitmenttxid, spendingtxid, spendingfuncid)) {
				if (spendingfuncid != 'c' && spendingfuncid != 'u') {
					CCerror = "proposal's specified commitment is no longer active!";
					return false;
				}
			}
			else {
				CCerror = "proposal's commitmenttxid info not found!";
				return false;
			}
			if (!GetCommitmentInitialData(commitmenttxid, proposaltxid, sellerpk, clientpk, ref_arbitratorpk, arbitratorfee, ref_depositval, datahash, refcommitmenttxid, info)) {
				CCerror = "proposal commitment tx has invalid commitment data!";
				return false;
			}
			LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: checking deposit value" << std::endl);
			if (depositval < 0 || depositval > ref_depositval) {
				CCerror = "proposal has invalid deposit value!";
				return false;
			}
			LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: checking if srcpub and destpub are members of the commitment" << std::endl);
			if (proposaltype == 'u' && (CPK_src != pubkey2pk(sellerpk) && CPK_src != pubkey2pk(clientpk) || CPK_dest != pubkey2pk(sellerpk) && CPK_dest != pubkey2pk(clientpk))) {
				CCerror = "proposal srcpub or destpub is not a member of the specified commitment!";
				return false;
			}
			else if (proposaltype == 't' && (CPK_src != pubkey2pk(sellerpk) || CPK_dest != pubkey2pk(clientpk))) {
				CCerror = "proposal srcpub or destpub is invalid for close tx!";
				return false;
			}
			LOGSTREAM("commitments", CCLOG_INFO, stream << "ValidateProposalOpRet: checking arbitrator" << std::endl);
			if (bHasArbitrator && CPK_arbitrator != pubkey2pk(ref_arbitratorpk)) {
				CCerror = "proposal has incorrect arbitrator defined!";
				return false;
			}
			break;
		default:
            CCerror = "proposal has invalid proposaltype!";
			return false;
	}
	return true;
}

// compares two proposal txes and checks if their types and source/destination pubkeys match.
// used in validation and some RPCs
bool CompareProposals(CScript proposalopret, uint256 refproposaltxid, std::string &CCerror)
{
	CTransaction refproposaltx;
	uint256 hashBlock, datahash, commitmenttxid, ref_commitmenttxid, prevproposaltxid, ref_prevproposaltxid;
	std::vector<uint8_t> srcpub, ref_srcpub, destpub, ref_destpub, arbitratorpk, ref_arbitratorpk;
	int64_t payment, arbitratorfee, deposit;
	std::string info;
	uint8_t proposaltype, ref_proposaltype, version, ref_version;
	CCerror = "";

	LOGSTREAM("commitments", CCLOG_INFO, stream << "CompareProposals: decoding opret" << std::endl);
	if (DecodeCommitmentProposalOpRet(proposalopret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, deposit, datahash, commitmenttxid, prevproposaltxid, info) != 'p') {
		CCerror = "proposal tx opret invalid or not a proposal tx!";
		return false;
	}
	LOGSTREAM("commitments", CCLOG_INFO, stream << "CompareProposals: fetching refproposal tx" << std::endl);
	if (myGetTransaction(refproposaltxid, refproposaltx, hashBlock) == 0 || refproposaltx.vout.size() <= 0) {
		std::cerr << "GetCommitmentMembers: couldn't find previous proposal tx" << std::endl;
		return false;
	}
	LOGSTREAM("commitments", CCLOG_INFO, stream << "CompareProposals: decoding refproposaltx opret" << std::endl);
	if (DecodeCommitmentProposalOpRet(refproposaltx.vout[refproposaltx.vout.size()-1].scriptPubKey, ref_version, ref_proposaltype, ref_srcpub, ref_destpub, ref_arbitratorpk, payment, arbitratorfee, deposit, datahash, ref_commitmenttxid, ref_prevproposaltxid, info) != 'p') {
		CCerror = "previous proposal tx opret invalid or not a proposal tx!";
		return false;
	}
	LOGSTREAM("commitments", CCLOG_INFO, stream << "CompareProposals: checking if refproposaltxid = prevproposaltxid" << std::endl);
	if (refproposaltxid != prevproposaltxid) {
		CCerror = "current proposal doesn't correctly refer to the previous proposal!";
		return false;
	}
	LOGSTREAM("commitments", CCLOG_INFO, stream << "CompareProposals: checking if proposal types match" << std::endl);
	if (proposaltype != ref_proposaltype) {
		CCerror = "current and previous proposal types don't match!";
		return false;
	}
	switch (proposaltype) {
		case 't':
		case 'u':
			LOGSTREAM("commitments", CCLOG_INFO, stream << "CompareProposals: checking if dest pubkeys match" << std::endl);
			if (destpub != ref_destpub) {
				CCerror = "current and previous proposal destination pubkeys don't match!";
				return false;
			}
			LOGSTREAM("commitments", CCLOG_INFO, stream << "CompareProposals: checking if commitmenttxid matches" << std::endl);
			if (commitmenttxid != ref_commitmenttxid) {
				CCerror = "current and previous proposal commitment id doesn't match!";
				return false;
			}
		// intentional fall-through
		case 'p':
			LOGSTREAM("commitments", CCLOG_INFO, stream << "CompareProposals: checking if src pubkeys match" << std::endl);
			if (srcpub != ref_srcpub) {
				CCerror = "current and previous proposal source pubkeys don't match!";
				return false;
			}
			break;
		default:
            CCerror = "proposals have invalid proposaltype!";
			return false;
	}
	return true;
}

// returns txid and funcid of the tx that spent the specified proposal, if it exists
bool IsProposalSpent(uint256 proposaltxid, uint256 &spendingtxid, uint8_t &spendingfuncid)
{
	int32_t vini, height, retcode;
	uint256 hashBlock;
	CTransaction proposaltx, spendingtx;
	
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

// gets the data from the accepted proposal for the specified commitment txid
// this is for "static" data like seller, client & arbitrator pubkeys. gathering "updateable" data like info, datahash etc are handled by different functions
bool GetCommitmentInitialData(uint256 commitmenttxid, uint256 &proposaltxid, std::vector<uint8_t> &sellerpk, std::vector<uint8_t> &clientpk, std::vector<uint8_t> &arbitratorpk, int64_t &firstarbitratorfee, int64_t &deposit, uint256 &firstdatahash, uint256 &refcommitmenttxid, std::string &firstinfo)
{
	CScript proposalopret;
	CTransaction commitmenttx;
	uint256 prevproposaltxid, hashBlock;
	uint8_t version, proposaltype;
	int64_t payment;

	if (myGetTransaction(commitmenttxid, commitmenttx, hashBlock) == 0 || commitmenttx.vout.size() <= 0) {
		std::cerr << "GetCommitmentInitialData: couldn't find commitment tx" << std::endl;
		return false;
	}
	if (!GetAcceptedProposalOpRet(commitmenttx, proposaltxid, proposalopret)) {
		std::cerr << "GetCommitmentInitialData: couldn't get accepted proposal tx opret" << std::endl;
		return false;	
	}
	if (DecodeCommitmentProposalOpRet(proposalopret, version, proposaltype, sellerpk, clientpk, arbitratorpk, payment, firstarbitratorfee, deposit, firstdatahash, refcommitmenttxid, prevproposaltxid, firstinfo) != 'p' || proposaltype != 'p') {
		std::cerr << "GetCommitmentInitialData: commitment accepted proposal tx opret invalid" << std::endl;
		return false;	
	}
	return true;
}

// gets the latest update baton txid of a commitment
// can be also used to check status of a commitment by looking at its funcid
bool GetLatestCommitmentUpdate(uint256 commitmenttxid, uint256 &latesttxid, uint8_t &funcid)
{
    int32_t vini, height, retcode;
    uint256 batontxid, sourcetxid, hashBlock;
    CTransaction commitmenttx, batontx;
    uint8_t evalcode;

    // special handling for commitment tx - baton vout is vout1
	if (myGetTransaction(commitmenttxid, commitmenttx, hashBlock) == 0 || commitmenttx.vout.size() <= 0) {
		std::cerr << "GetLatestCommitmentUpdate: couldn't find commitment tx" << std::endl;
		return false;
	}
	if (DecodeCommitmentOpRet(commitmenttx.vout[commitmenttx.vout.size() - 1].scriptPubKey) != 'c') {
		std::cerr << "GetLatestCommitmentUpdate: commitment tx is not a contract signing tx" << std::endl;
		return false;	
	}
	if ((retcode = CCgetspenttxid(batontxid, vini, height, commitmenttxid, 1)) != 0) {
		latesttxid = commitmenttxid; // no updates, return commitmenttxid
		funcid = 'c';
		return true;	
	}
	else if (!(myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 && 
	((funcid = DecodeCommitmentOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey)) == 'u' || funcid == 's' || funcid == 'd')) &&
	batontx.vout[1].nValue == CC_MARKER_VALUE) {
		std::cerr << "GetLatestCommitmentUpdate: found first update, but it has incorrect funcid" << std::endl;
		return false;
	}
	sourcetxid = batontxid;
    // baton vout should be vout0 from now on
	while ((retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 0)) == 0 && myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0) {
		funcid = DecodeCommitmentOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey);
		switch (funcid) {
			case 'u':
			case 'd':
				sourcetxid = batontxid;
				continue;
			case 'n':
			case 's':
			case 'r':
				break;
			default:
				std::cerr << "GetLatestCommitmentUpdate: found an update, but it has incorrect funcid" << std::endl;
				return false;
		}
		break;
    }
	latesttxid = sourcetxid;
    return true;
}

// gets the data from the accepted proposal for the specified update txid
// this is for "updateable" data like info, arbitrator fee etc.
void GetCommitmentUpdateData(uint256 updatetxid, std::string &info, uint256 &datahash, int64_t &arbitratorfee, int64_t &depositsplit, int64_t &revision)
{
	CScript proposalopret;
	CTransaction updatetx, commitmenttx, batontx;
	std::vector<uint8_t> dummypk;
	uint256 proposaltxid, commitmenttxid, sourcetxid, batontxid, dummytxid, hashBlock;
	uint8_t version, funcid, dummychar;
	int64_t dummyamount;
	int32_t vini, height, retcode;
	
	while (myGetTransaction(updatetxid, updatetx, hashBlock) && updatetx.vout.size() > 0 &&
	(funcid = DecodeCommitmentOpRet(updatetx.vout[updatetx.vout.size() - 1].scriptPubKey)) != 0) {
		switch (funcid) {
			case 'u':
			case 's':
				GetAcceptedProposalOpRet(updatetx, proposaltxid, proposalopret);
				DecodeCommitmentProposalOpRet(proposalopret,version,dummychar,dummypk,dummypk,dummypk,dummyamount,arbitratorfee,depositsplit,datahash,commitmenttxid,dummytxid,info);
				break;
			case 'd':
			case 'n':
			case 'r':
				// get previous baton
				updatetxid = updatetx.vin[1].prevout.hash;
				continue;
			default:
				break;
		}
		break;
    }
	revision = 1;
	if (myGetTransaction(commitmenttxid, commitmenttx, hashBlock) && commitmenttx.vout.size() > 0 &&
	(funcid = DecodeCommitmentOpRet(commitmenttx.vout[commitmenttx.vout.size() - 1].scriptPubKey)) == 'c' &&
	(retcode = CCgetspenttxid(sourcetxid, vini, height, commitmenttxid, 1)) == 0 &&
	myGetTransaction(sourcetxid, batontx, hashBlock) && batontx.vout.size() > 0) {
		revision++;
		while (sourcetxid != updatetxid && (retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 0)) == 0 &&
		myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 &&
		DecodeCommitmentOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey) != 0) {
			revision++;
			sourcetxid = batontxid;
		}
	}
	return;
}

/*
dynamic data (non-user):
	GetProposalRevision(proposaltxid) gives revision number for proposal
		
how tf to get these???
	update/termination proposal list (?)
	settlement list (?)
*/

//===========================================================================
// RPCs - tx creation
//===========================================================================

// commitmentcreate - constructs a 'p' transaction, with the 'p' proposal type
UniValue CommitmentCreate(const CPubKey& pk, uint64_t txfee, std::string info, uint256 datahash, std::vector<uint8_t> destpub, std::vector<uint8_t> arbitrator, int64_t payment, int64_t arbitratorfee, int64_t deposit, uint256 prevproposaltxid, uint256 refcommitmenttxid)
{
	CPubKey mypk, CPK_src, CPK_dest, CPK_arbitrator;
	CTransaction prevproposaltx;
	uint256 hashBlock, spendingtxid, dummytxid;
	std::vector<uint8_t> ref_srcpub, ref_destpub, dummypk;
	int32_t numvouts;
	int64_t dummyamount;
	std::string dummystr, CCerror;
	bool bHasReceiver, bHasArbitrator;
	uint8_t dummychar, version, spendingfuncid, mypriv[32];
	char mutualaddr[65];
	
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	CPK_dest = pubkey2pk(destpub);
	CPK_arbitrator = pubkey2pk(arbitrator);
	bHasReceiver = CPK_dest.IsFullyValid();
	bHasArbitrator = CPK_arbitrator.IsFullyValid();
	// check if destpub & arbitrator pubkeys exist and are valid
	if (!destpub.empty() && !bHasReceiver)
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Buyer pubkey invalid");
	if (!arbitrator.empty() && !bHasArbitrator)
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Arbitrator pubkey invalid");
	// if arbitrator exists, check if arbitrator fee & deposit are sufficient
	if (bHasArbitrator) {
		if (arbitratorfee == 0)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Arbitrator fee must be specified if valid arbitrator exists");
		else if (arbitratorfee < CC_MARKER_VALUE)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Arbitrator fee is too low");
		if (deposit == 0)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Deposit must be specified if valid arbitrator exists");
		else if (deposit < CC_MARKER_VALUE)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Deposit is too low");
	}
	else {
		arbitratorfee = 0;
		deposit = CC_MARKER_VALUE;
	}
	// additional checks are done using ValidateProposalOpRet
	CScript opret = EncodeCommitmentProposalOpRet(COMMITMENTCC_VERSION,'p',std::vector<uint8_t>(mypk.begin(),mypk.end()),destpub,arbitrator,payment,arbitratorfee,deposit,datahash,refcommitmenttxid,prevproposaltxid,info);
	if (!ValidateProposalOpRet(opret, CCerror))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << CCerror);
	// check prevproposaltxid if specified
	if (prevproposaltxid != zeroid) {
		if (myGetTransaction(prevproposaltxid,prevproposaltx,hashBlock)==0 || (numvouts=prevproposaltx.vout.size())<=0)
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "cant find specified previous proposal txid " << prevproposaltxid.GetHex());
		DecodeCommitmentProposalOpRet(prevproposaltx.vout[numvouts-1].scriptPubKey,dummychar,dummychar,ref_srcpub,ref_destpub,dummypk,dummyamount,dummyamount,dummyamount,dummytxid,dummytxid,dummytxid,dummystr);
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
		if (!CompareProposals(opret, prevproposaltxid, CCerror))
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << CCerror << " txid: " << prevproposaltxid.GetHex());
	}
	if (AddNormalinputs2(mtx, txfee + CC_MARKER_VALUE + CC_RESPONSE_VALUE, 8) > 0) {
		if (prevproposaltxid != zeroid) {
			mtx.vin.push_back(CTxIn(prevproposaltxid,0,CScript())); // vin.n-2 previous proposal marker (optional, will trigger validation)
			GetCCaddress1of2(cp, mutualaddr, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub));
			mtx.vin.push_back(CTxIn(prevproposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (optional, will trigger validation)
			Myprivkey(mypriv);
			CCaddr1of2set(cp, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub), mypriv, mutualaddr);
		}
		mtx.vout.push_back(MakeCC1vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
		if (bHasReceiver)
			mtx.vout.push_back(MakeCC1of2vout(EVAL_COMMITMENTS, CC_RESPONSE_VALUE, mypk, CPK_dest)); // vout.1 response hook (with destpub)
		else
			mtx.vout.push_back(MakeCC1vout(EVAL_COMMITMENTS, CC_RESPONSE_VALUE, mypk)); // vout.1 response hook (no destpub)
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}
	CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "error adding normal inputs");
}

// commitmentupdate - constructs a 'p' transaction, with the 'u' proposal type
// This transaction will only be validated if commitmenttxid is specified. prevproposaltxid can be used to amend previous update requests.
UniValue CommitmentUpdate(const CPubKey& pk, uint64_t txfee, uint256 commitmenttxid, std::string info, uint256 datahash, int64_t payment, uint256 prevproposaltxid, int64_t newarbitratorfee)
{
	CPubKey mypk, CPK_src, CPK_dest;
	CTransaction prevproposaltx;
	uint256 hashBlock, spendingtxid, latesttxid, dummytxid;
	std::vector<uint8_t> destpub, sellerpk, clientpk, arbitratorpk, ref_srcpub, ref_destpub, dummypk;
	int32_t numvouts;
	int64_t arbitratorfee, dummyamount;
	std::string dummystr, CCerror;
	uint8_t version, spendingfuncid, dummychar, mypriv[32];
	char mutualaddr[65];
	
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (!GetCommitmentInitialData(commitmenttxid, dummytxid, sellerpk, clientpk, arbitratorpk, arbitratorfee, dummyamount, dummytxid, dummytxid, dummystr))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "couldn't get specified commitment info successfully, probably invalid commitment txid");
	GetLatestCommitmentUpdate(commitmenttxid, latesttxid, dummychar);
	GetCommitmentUpdateData(latesttxid, dummystr, dummytxid, arbitratorfee, dummyamount, dummyamount);
	if (pubkey2pk(arbitratorpk).IsFullyValid() && newarbitratorfee == 0)
		newarbitratorfee = arbitratorfee;
	else if (!(pubkey2pk(arbitratorpk).IsFullyValid()))
		newarbitratorfee = 0;
	// setting destination pubkey
	if (mypk == pubkey2pk(sellerpk))
		destpub = clientpk;
	else if (mypk == pubkey2pk(clientpk))
		destpub = sellerpk;
	else
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "you are not a valid member of this commitment");
	CPK_dest = pubkey2pk(destpub);
	// additional checks are done using ValidateProposalOpRet
	CScript opret = EncodeCommitmentProposalOpRet(COMMITMENTCC_VERSION,'u',std::vector<uint8_t>(mypk.begin(),mypk.end()),destpub,arbitratorpk,payment,newarbitratorfee,0,datahash,commitmenttxid,prevproposaltxid,info);
	if (!ValidateProposalOpRet(opret, CCerror))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << CCerror);
	// check prevproposaltxid if specified
	if (prevproposaltxid != zeroid) {
		if (myGetTransaction(prevproposaltxid,prevproposaltx,hashBlock)==0 || (numvouts=prevproposaltx.vout.size())<=0)
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "cant find specified previous proposal txid " << prevproposaltxid.GetHex());
		DecodeCommitmentProposalOpRet(prevproposaltx.vout[numvouts-1].scriptPubKey,dummychar,dummychar,ref_srcpub,ref_destpub,dummypk,dummyamount,dummyamount,dummyamount,dummytxid,dummytxid,dummytxid,dummystr);
		if (IsProposalSpent(prevproposaltxid, spendingtxid, spendingfuncid)) {
			switch (spendingfuncid) {
				case 'p':
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been amended by txid " << spendingtxid.GetHex());
				case 'u':
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been accepted by txid " << spendingtxid.GetHex());
				case 't':
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been closed by txid " << spendingtxid.GetHex());
				default:
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been spent by txid " << spendingtxid.GetHex());
			}
		}
		if (!CompareProposals(opret, prevproposaltxid, CCerror))
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << CCerror << " txid: " << prevproposaltxid.GetHex());
	}
	if (AddNormalinputs2(mtx, txfee + CC_MARKER_VALUE + CC_RESPONSE_VALUE, 8) > 0) {
		if (prevproposaltxid != zeroid) {
			mtx.vin.push_back(CTxIn(prevproposaltxid,0,CScript())); // vin.n-2 previous proposal marker (optional, will trigger validation)
			GetCCaddress1of2(cp, mutualaddr, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub));
			mtx.vin.push_back(CTxIn(prevproposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (optional, will trigger validation)
			Myprivkey(mypriv);
			CCaddr1of2set(cp, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub), mypriv, mutualaddr);
		}
		mtx.vout.push_back(MakeCC1vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
		mtx.vout.push_back(MakeCC1of2vout(EVAL_COMMITMENTS, CC_RESPONSE_VALUE, mypk, pubkey2pk(destpub))); // vout.1 response hook
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}
	CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "error adding normal inputs");
}

// commitmentclose - constructs a 'p' transaction, with the 't' proposal type
// This transaction will only be validated if commitmenttxid is specified. prevproposaltxid can be used to amend previous cancel requests.
UniValue CommitmentClose(const CPubKey& pk, uint64_t txfee, uint256 commitmenttxid, std::string info, uint256 datahash, int64_t depositcut, int64_t payment, uint256 prevproposaltxid)
{
	CPubKey mypk, CPK_src, CPK_dest;
	CTransaction prevproposaltx;
	uint256 hashBlock, spendingtxid, latesttxid, dummytxid;
	std::vector<uint8_t> destpub, sellerpk, clientpk, arbitratorpk, ref_srcpub, ref_destpub, dummypk;
	int32_t numvouts;
	int64_t deposit, dummyamount;
	std::string dummystr, CCerror;
	uint8_t version, spendingfuncid, dummychar, mypriv[32];
	char mutualaddr[65];
	
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (!GetCommitmentInitialData(commitmenttxid, dummytxid, sellerpk, clientpk, arbitratorpk, dummyamount, deposit, dummytxid, dummytxid, dummystr))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "couldn't get specified commitment info successfully, probably invalid commitment txid");
	// only sellers can create this proposal type
	if (mypk != pubkey2pk(sellerpk))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "you are not the seller of this commitment");
	// checking deposit cut to prevent vouts with dust value
	if (pubkey2pk(arbitratorpk).IsFullyValid()) {
		if (depositcut != 0 && depositcut < CC_MARKER_VALUE)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Deposit cut is too low");
		if (depositcut > deposit)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Deposit cut exceeds total deposit value");
		else if ((deposit - depositcut) != 0 && (deposit - depositcut) < CC_MARKER_VALUE)
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "Remainder of deposit is too low");
	}
	else
	if (depositcut == 0) depositcut = CC_MARKER_VALUE;
	// additional checks are done using ValidateProposalOpRet
	CScript opret = EncodeCommitmentProposalOpRet(COMMITMENTCC_VERSION,'t',std::vector<uint8_t>(mypk.begin(),mypk.end()),clientpk,arbitratorpk,payment,CC_MARKER_VALUE,depositcut,datahash,commitmenttxid,prevproposaltxid,info);
	if (!ValidateProposalOpRet(opret, CCerror))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << CCerror);
	// check prevproposaltxid if specified
	if (prevproposaltxid != zeroid) {
		if (myGetTransaction(prevproposaltxid,prevproposaltx,hashBlock)==0 || (numvouts=prevproposaltx.vout.size())<=0)
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "cant find specified previous proposal txid " << prevproposaltxid.GetHex());
		DecodeCommitmentProposalOpRet(prevproposaltx.vout[numvouts-1].scriptPubKey,dummychar,dummychar,ref_srcpub,ref_destpub,dummypk,dummyamount,dummyamount,dummyamount,dummytxid,dummytxid,dummytxid,dummystr);
		if (IsProposalSpent(prevproposaltxid, spendingtxid, spendingfuncid)) {
			switch (spendingfuncid) {
				case 'p':
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been amended by txid " << spendingtxid.GetHex());
				case 'u':
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been accepted by txid " << spendingtxid.GetHex());
				case 't':
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been closed by txid " << spendingtxid.GetHex());
				default:
					CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been spent by txid " << spendingtxid.GetHex());
			}
		}
		if (!CompareProposals(opret, prevproposaltxid, CCerror))
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << CCerror << " txid: " << prevproposaltxid.GetHex());
	}
	if (AddNormalinputs2(mtx, txfee + CC_MARKER_VALUE + CC_RESPONSE_VALUE, 8) > 0) {
		if (prevproposaltxid != zeroid) {
			mtx.vin.push_back(CTxIn(prevproposaltxid,0,CScript())); // vin.n-2 previous proposal marker (optional, will trigger validation)
			GetCCaddress1of2(cp, mutualaddr, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub));
			mtx.vin.push_back(CTxIn(prevproposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (optional, will trigger validation)
			Myprivkey(mypriv);
			CCaddr1of2set(cp, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub), mypriv, mutualaddr);
		}
		mtx.vout.push_back(MakeCC1vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
		mtx.vout.push_back(MakeCC1of2vout(EVAL_COMMITMENTS, CC_RESPONSE_VALUE, mypk, pubkey2pk(clientpk))); // vout.1 response hook
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}
	CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "error adding normal inputs");
}

// commitmentstopproposal - constructs a 't' transaction and spends the specified 'p' transaction
// can be done by the proposal initiator, as well as the receiver if they are able to accept the proposal.
UniValue CommitmentStopProposal(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid)
{
	CPubKey mypk, CPK_src, CPK_dest;
	CTransaction proposaltx;
	uint256 hashBlock, refcommitmenttxid, spendingtxid, dummytxid;
	std::vector<uint8_t> srcpub, destpub, sellerpk, clientpk, dummypk;
	int32_t numvouts;
	int64_t dummyamount;
	std::string dummystr;
	bool bHasReceiver, bHasArbitrator;
	uint8_t proposaltype, version, spendingfuncid, mypriv[32];
	char mutualaddr[65];
	
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (myGetTransaction(proposaltxid, proposaltx, hashBlock) == 0 || (numvouts = proposaltx.vout.size()) <= 0)
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "cant find specified proposal txid " << proposaltxid.GetHex());
	if (DecodeCommitmentProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,version,proposaltype,srcpub,destpub,dummypk,dummyamount,dummyamount,dummyamount,dummytxid,refcommitmenttxid,dummytxid,dummystr) != 'p')
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified txid has incorrect proposal opret");
	if (IsProposalSpent(proposaltxid, spendingtxid, spendingfuncid)) {
		switch (spendingfuncid) {
			case 'p':
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been amended by txid " << spendingtxid.GetHex());
			case 'c':
			case 'u':
			case 's':
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been accepted by txid " << spendingtxid.GetHex());
			case 't':
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been closed by txid " << spendingtxid.GetHex());
			default:
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been spent by txid " << spendingtxid.GetHex());
		}
	}
	CPK_src = pubkey2pk(srcpub);
	CPK_dest = pubkey2pk(destpub);
	bHasReceiver = CPK_dest.IsFullyValid();
	switch (proposaltype) {
		case 'p':
			if (bHasReceiver && mypk != CPK_src && mypk != CPK_dest)
				CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "you are not the source or receiver of specified proposal");
			else if (!bHasReceiver && mypk != CPK_src)
				CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "you are not the source of specified proposal");
			break;
		case 'u':
		case 't':
			if (refcommitmenttxid == zeroid)
				CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "proposal has no defined commitment, unable to verify membership");
			if (!GetCommitmentInitialData(refcommitmenttxid, dummytxid, sellerpk, clientpk, dummypk, dummyamount, dummyamount, dummytxid, dummytxid, dummystr))
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "couldn't get proposal's commitment info successfully");
			if (mypk != CPK_src && mypk != CPK_dest && mypk != pubkey2pk(sellerpk) && mypk != pubkey2pk(clientpk))
				CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "you are not the source or receiver of specified proposal");
			break;
		default:
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "invalid proposaltype in proposal tx opret");
	}
	if (AddNormalinputs2(mtx, txfee, 5) > 0) {
		if (bHasReceiver) {
			GetCCaddress1of2(cp, mutualaddr, CPK_src, CPK_dest);
			mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.1 previous proposal CC response hook (1of2 addr version)
			Myprivkey(mypriv);
			CCaddr1of2set(cp, CPK_src, CPK_dest, mypriv, mutualaddr);
		}
		else
			mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.1 previous proposal CC response hook (1of1 addr version)
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeCommitmentProposalCloseOpRet(COMMITMENTCC_VERSION,proposaltxid,std::vector<uint8_t>(mypk.begin(),mypk.end())));
	}
	CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "error adding normal inputs");
}

// commitmentaccept - spend a 'p' transaction that was submitted by the other party.
// this function is context aware and does different things dependent on the proposal type:
// if txid opret has the 'p' proposal type, will create a 'c' transaction (create contract)
// if txid opret has the 'u' or 't' proposal type, will create a 'u' transaction (update contract)
UniValue CommitmentAccept(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid)
{
	CPubKey mypk, CPK_src, CPK_dest, CPK_arbitrator;
	CTransaction proposaltx;
	uint256 hashBlock, datahash, commitmenttxid, prevproposaltxid, spendingtxid, latesttxid;
	std::vector<uint8_t> srcpub, destpub, arbitrator;
	int32_t numvouts;
	int64_t payment, arbitratorfee, deposit;
	std::string info, CCerror;
	bool bHasReceiver, bHasArbitrator;
	uint8_t proposaltype, version, spendingfuncid, updatefuncid, mypriv[32];
	char mutualaddr[65], depositaddr[65];
	
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (myGetTransaction(proposaltxid, proposaltx, hashBlock) == 0 || (numvouts = proposaltx.vout.size()) <= 0)
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "cant find specified proposal txid " << proposaltxid.GetHex());
	if (!ValidateProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey, CCerror))
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << CCerror);
	if (DecodeCommitmentProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,version,proposaltype,srcpub,destpub,arbitrator,payment,arbitratorfee,deposit,datahash,commitmenttxid,prevproposaltxid,info) != 'p')
		CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified txid has incorrect proposal opret");
	if (IsProposalSpent(proposaltxid, spendingtxid, spendingfuncid)) {
		switch (spendingfuncid) {
			case 'p':
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been amended by txid " << spendingtxid.GetHex());
			case 'c':
			case 'u':
			case 's':
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been accepted by txid " << spendingtxid.GetHex());
			case 't':
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been closed by txid " << spendingtxid.GetHex());
			default:
				CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "specified proposal has been spent by txid " << spendingtxid.GetHex());
		}
	}
	CPK_src = pubkey2pk(srcpub);
	CPK_dest = pubkey2pk(destpub);
	CPK_arbitrator = pubkey2pk(arbitrator);
	bHasReceiver = CPK_dest.IsFullyValid();
	bHasArbitrator = CPK_arbitrator.IsFullyValid();
	if (!bHasReceiver)
		CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "specified proposal has no receiver, can't accept");
	else if (mypk != CPK_dest)
		CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "you are not the receiver of specified proposal");
	switch (proposaltype) {
		case 'p':
			// constructing a 'c' transaction
			if (AddNormalinputs2(mtx, txfee + payment + deposit, 64) > 0) {
				mtx.vin.push_back(CTxIn(proposaltxid,0,CScript())); // vin.1 previous proposal CC marker
				GetCCaddress1of2(cp, mutualaddr, CPK_src, CPK_dest);
				mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.2 previous proposal CC response hook (must have designated receiver!)
				Myprivkey(mypriv);
				CCaddr1of2set(cp, CPK_src, CPK_dest, mypriv, mutualaddr);
				mtx.vout.push_back(MakeCC1vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
				mtx.vout.push_back(MakeCC1of2vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, CPK_src, mypk)); // vout.1 baton / update log
				if (bHasArbitrator) {
					mtx.vout.push_back(MakeCC1of2vout(EVAL_COMMITMENTS, deposit, mypk, CPK_arbitrator)); // vout.2 deposit (can be spent by client & arbitrator)
				}
				else
					mtx.vout.push_back(MakeCC1vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, mypk)); // vout.2 contract completion marker (no arbitrator)
				if (payment > 0)
					mtx.vout.push_back(CTxOut(payment, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG)); // vout.4 payment (optional)
				return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeCommitmentSigningOpRet(COMMITMENTCC_VERSION, proposaltxid));
			}
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "error adding normal inputs");
		case 'u':
			GetLatestCommitmentUpdate(commitmenttxid, latesttxid, updatefuncid);
			// constructing a 'u' transaction
			if (AddNormalinputs2(mtx, txfee + payment, 64) > 0) {
				GetCCaddress1of2(cp, mutualaddr, CPK_src, CPK_dest);
				if (latesttxid == commitmenttxid)
					mtx.vin.push_back(CTxIn(commitmenttxid,1,CScript())); // vin.1 last update baton (no previous updates)
				else
					mtx.vin.push_back(CTxIn(latesttxid,0,CScript())); // vin.1 last update baton (with previous updates)
				mtx.vin.push_back(CTxIn(proposaltxid,0,CScript())); // vin.2 previous proposal CC marker
				mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.3 previous proposal CC response hook
				Myprivkey(mypriv);
				CCaddr1of2set(cp, CPK_src, CPK_dest, mypriv, mutualaddr);
				mtx.vout.push_back(MakeCC1of2vout(EVAL_COMMITMENTS, CC_MARKER_VALUE, CPK_src, mypk)); // vout.0 next update baton
				if (payment > 0)
					mtx.vout.push_back(CTxOut(payment, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG)); // vout.1 payment (optional)
				return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeCommitmentUpdateOpRet(COMMITMENTCC_VERSION, commitmenttxid, proposaltxid)); 
			}
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "error adding normal inputs");
		case 't':
			GetLatestCommitmentUpdate(commitmenttxid, latesttxid, updatefuncid);
			// constructing a 's' transaction
			if (AddNormalinputs2(mtx, txfee + payment, 64) > 0) {
				GetCCaddress1of2(cp, mutualaddr, CPK_src, CPK_dest);
				if (latesttxid == commitmenttxid)
					mtx.vin.push_back(CTxIn(commitmenttxid,1,CScript())); // vin.1 last update baton (no previous updates)
				else
					mtx.vin.push_back(CTxIn(latesttxid,0,CScript())); // vin.1 last update baton (with previous updates)
				mtx.vin.push_back(CTxIn(proposaltxid,0,CScript())); // vin.2 previous proposal CC marker
				mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.3 previous proposal CC response hook
				Myprivkey(mypriv);
				CCaddr1of2set(cp, CPK_src, CPK_dest, mypriv, mutualaddr);
				if (bHasArbitrator) {
					GetCCaddress1of2(cp, depositaddr, CPK_dest, CPK_arbitrator);
					mtx.vin.push_back(CTxIn(commitmenttxid,2,CScript())); // vin.4 deposit (with arbitrator)
					CCaddr1of2set(cp, CPK_dest, CPK_arbitrator, mypriv, depositaddr);
				}
				else
					mtx.vin.push_back(CTxIn(commitmenttxid,2,CScript())); // vin.4 deposit (no arbitrator)
				mtx.vout.push_back(CTxOut(deposit, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG)); // vout.0 deposit cut to proposal creator
				if (payment > 0)
					mtx.vout.push_back(CTxOut(payment, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG)); // vout.1 payment (optional)
				
				return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeCommitmentCloseOpRet(COMMITMENTCC_VERSION, commitmenttxid, proposaltxid)); 
			}
			CCERR_RESULT("commitmentscc",CCLOG_INFO, stream << "error adding normal inputs");
		default:
			CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "invalid proposal type for proposal txid " << proposaltxid.GetHex());
	}
}

// commitmentdispute
// commitmentresolve

//===========================================================================
// RPCs - informational
//===========================================================================

UniValue CommitmentInfo(const CPubKey& pk, uint256 txid)
{
	UniValue result(UniValue::VOBJ), members(UniValue::VOBJ), data(UniValue::VOBJ);
	CPubKey mypk, CPK_src, CPK_dest, CPK_arbitrator;
	CTransaction tx, proposaltx;
	uint256 hashBlock, datahash, proposaltxid, prevproposaltxid, commitmenttxid, latesttxid, spendingtxid, dummytxid;
	std::vector<uint8_t> srcpub, destpub, arbitrator;
	int32_t numvouts;
	int64_t payment, arbitratorfee, deposit, totaldeposit, revision;
	std::string info, CCerror;
	bool bHasReceiver, bHasArbitrator;
	uint8_t funcid, version, proposaltype, updatefuncid, spendingfuncid, mypriv[32];
	char mutualaddr[65];
	
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_COMMITMENTS);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (myGetTransaction(txid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
    (funcid = DecodeCommitmentOpRet(tx.vout[numvouts-1].scriptPubKey)) != 0) {
		result.push_back(Pair("result", "success"));
        result.push_back(Pair("txid", txid.GetHex()));
		switch (funcid) {
			case 'p':
				result.push_back(Pair("type","proposal"));
				DecodeCommitmentProposalOpRet(tx.vout[numvouts-1].scriptPubKey,version,proposaltype,srcpub,destpub,arbitrator,payment,arbitratorfee,deposit,datahash,commitmenttxid,prevproposaltxid,info);
				CPK_src = pubkey2pk(srcpub);
				CPK_dest = pubkey2pk(destpub);
				CPK_arbitrator = pubkey2pk(arbitrator);
				bHasReceiver = CPK_dest.IsFullyValid();
				bHasArbitrator = CPK_arbitrator.IsFullyValid();
				members.push_back(Pair("sender",HexStr(srcpub)));
				if (bHasReceiver)
					members.push_back(Pair("receiver",HexStr(destpub)));
				if (payment > 0)
					data.push_back(Pair("required_payment", payment));
				
				// TODO: add revision numbers here (version numbers should be reset after contract acceptance)
				
				data.push_back(Pair("info",info));
				data.push_back(Pair("data_hash",datahash.GetHex()));
				switch (proposaltype) {
					case 'p':
						result.push_back(Pair("proposal_type","contract_create"));
						if (bHasArbitrator) {
							members.push_back(Pair("arbitrator",HexStr(arbitrator)));
							data.push_back(Pair("arbitrator_fee",arbitratorfee));
							data.push_back(Pair("deposit",deposit));
						}
						if (commitmenttxid != zeroid)
							data.push_back(Pair("master_contract_txid",commitmenttxid.GetHex()));
						break;
					case 'u':
						result.push_back(Pair("proposal_type","contract_update"));
						result.push_back(Pair("contract_txid",commitmenttxid.GetHex()));
						if (bHasArbitrator) {
							data.push_back(Pair("new_arbitrator_fee", arbitratorfee));
							GetCommitmentInitialData(commitmenttxid, proposaltxid, srcpub, destpub, arbitrator, arbitratorfee, deposit, datahash, dummytxid, info);
							GetLatestCommitmentUpdate(commitmenttxid, latesttxid, updatefuncid);
							GetCommitmentUpdateData(latesttxid, info, datahash, arbitratorfee, deposit, revision);
							data.push_back(Pair("current_arbitrator_fee", arbitratorfee));
						}
						break;
					case 't':
						result.push_back(Pair("proposal_type","contract_close"));
						result.push_back(Pair("contract_txid",commitmenttxid.GetHex()));
						if (bHasArbitrator) {
							GetCommitmentInitialData(commitmenttxid, proposaltxid, srcpub, destpub, arbitrator, arbitratorfee, totaldeposit, datahash, dummytxid, info);
							GetLatestCommitmentUpdate(commitmenttxid, latesttxid, updatefuncid);
							GetCommitmentUpdateData(latesttxid, info, datahash, arbitratorfee, deposit, revision);
							data.push_back(Pair("deposit_for_seller", deposit));
							data.push_back(Pair("deposit_for_client", totaldeposit-deposit));
							data.push_back(Pair("total_deposit", totaldeposit));
						}
						break;
				}
				result.push_back(Pair("members",members));
				if (IsProposalSpent(txid, spendingtxid, spendingfuncid)) {
					switch (spendingfuncid) {
						case 'p':
							result.push_back(Pair("status","updated"));
							break;
						case 'c':
						case 'u':
						case 's':
							result.push_back(Pair("status","accepted"));
							break;
						case 't':
							result.push_back(Pair("status","closed"));
							break;
					}
					result.push_back(Pair("next_txid",spendingtxid.GetHex()));
				}
				else {
					if (bHasReceiver)
						result.push_back(Pair("status","open"));
					else
						result.push_back(Pair("status","draft"));
				}
				if (prevproposaltxid != zeroid)
					result.push_back(Pair("previous_txid",prevproposaltxid.GetHex()));
				result.push_back(Pair("data",data));
				break;
			case 't':
				result.push_back(Pair("type","proposal cancel"));
				DecodeCommitmentProposalCloseOpRet(tx.vout[numvouts-1].scriptPubKey, version, proposaltxid, srcpub);
				result.push_back(Pair("source_pubkey",HexStr(srcpub)));
				result.push_back(Pair("proposal_txid",proposaltxid.GetHex()));
				break;
			case 'c':
				result.push_back(Pair("type","contract"));
				DecodeCommitmentSigningOpRet(tx.vout[numvouts-1].scriptPubKey, version, proposaltxid);
				GetCommitmentInitialData(txid, proposaltxid, srcpub, destpub, arbitrator, arbitratorfee, deposit, datahash, commitmenttxid, info);
				result.push_back(Pair("accepted_txid",proposaltxid.GetHex()));
				members.push_back(Pair("seller",HexStr(srcpub)));
				members.push_back(Pair("client",HexStr(destpub)));
				if (pubkey2pk(arbitrator).IsFullyValid()) {
					members.push_back(Pair("arbitrator",HexStr(arbitrator)));
					result.push_back(Pair("deposit",deposit));
				}
				result.push_back(Pair("members",members));
				if (commitmenttxid != zeroid)
					data.push_back(Pair("master_contract_txid",commitmenttxid.GetHex()));
				GetLatestCommitmentUpdate(txid, latesttxid, updatefuncid);
				if (latesttxid != txid) {
					switch (updatefuncid) {
						case 'u':
							result.push_back(Pair("status","updated"));
							break;
						case 's':
							result.push_back(Pair("status","closed"));
							break;
						case 'd':
							result.push_back(Pair("status","suspended"));
							break;
						case 'r':
							result.push_back(Pair("status","arbitrated"));
							break;
						case 'n':
							result.push_back(Pair("status","in exchange mode"));
							break;
					}
					result.push_back(Pair("last_txid",latesttxid.GetHex()));
				}
				else
					result.push_back(Pair("status","active"));
				GetCommitmentUpdateData(latesttxid, info, datahash, arbitratorfee, deposit, revision);
				data.push_back(Pair("revisions",revision));
				data.push_back(Pair("arbitrator_fee",arbitratorfee));
				data.push_back(Pair("latest_info",info));
				data.push_back(Pair("latest_data_hash",datahash.GetHex()));
				result.push_back(Pair("data",data));
				/*
				TODO: 
					proposals: [
					]
					subcontracts: [
					]
					settlements: [
					]
				*/
				break;	
			case 'u':
				result.push_back(Pair("type","contract update"));
				DecodeCommitmentUpdateOpRet(tx.vout[numvouts-1].scriptPubKey, version, commitmenttxid, proposaltxid);
				result.push_back(Pair("contract_txid",commitmenttxid.GetHex()));
				result.push_back(Pair("proposal_txid",proposaltxid.GetHex()));
				GetCommitmentUpdateData(txid, info, datahash, arbitratorfee, deposit, revision);
				data.push_back(Pair("revision",revision));
				data.push_back(Pair("arbitrator_fee",arbitratorfee));
				data.push_back(Pair("info",info));
				data.push_back(Pair("data_hash",datahash.GetHex()));
				result.push_back(Pair("data",data));
				break;
			case 's':
				result.push_back(Pair("type","contract close"));
				break;
			case 'd':
				result.push_back(Pair("type","dispute"));
				break;
			case 'r':
				result.push_back(Pair("type","dispute resolution"));
				break;
		}
		return(result);
	}
	CCERR_RESULT("commitmentscc", CCLOG_INFO, stream << "invalid Commitments transaction id");
}
/*
for 's' show:
	commitment_txid
	source_pubkey
	proposal_txid
	deposit_for_seller
	deposit_for_client
for 'd' show:
	commitment_txid
	source_pubkey
	arbitrator_pubkey
	data_hash
for 'r' show:
	commitment_txid
	dispute_txid
	source_pubkey
	rewarded_pubkey
*/

// commitmentviewupdates [list]
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
