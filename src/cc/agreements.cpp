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

#include "CCagreements.h"
#include "CCPawnshop.h"

//===========================================================================
// Opret encoders/decoders
//===========================================================================
uint8_t DecodeAgreementOpRet(const CScript scriptPubKey)
{
	std::vector<uint8_t> vopret, dummypk;
	int64_t dummyamount;
	uint256 dummyhash;
	std::string dummystr;
	uint8_t evalcode, funcid, *script, dummytype;
	GetOpReturnData(scriptPubKey, vopret);
	script = (uint8_t *)vopret.data();
	if(script != NULL && vopret.size() > 2)
	{
		evalcode = script[0];
		if (evalcode != EVAL_AGREEMENTS)
		{
			LOGSTREAM("agreementscc", CCLOG_DEBUG1, stream << "script[0] " << script[0] << " != EVAL_AGREEMENTS" << std::endl);
			return (uint8_t)0;
		}
		funcid = script[1];
		LOGSTREAM((char *)"agreementscc", CCLOG_DEBUG2, stream << "DecodeAgreementOpRet() decoded funcId=" << (char)(funcid ? funcid : ' ') << std::endl);
		switch (funcid)
		{
			case 'p':
				return DecodeAgreementProposalOpRet(scriptPubKey, dummytype, dummytype, dummypk, dummypk, dummypk, dummyamount, dummyamount, dummyamount, dummyhash, dummyhash, dummyhash, dummystr);
			case 't':
				return DecodeAgreementProposalCloseOpRet(scriptPubKey, dummytype, dummyhash, dummypk);
			case 'c':
				return DecodeAgreementSigningOpRet(scriptPubKey, dummytype, dummyhash);
			case 'u':
				return DecodeAgreementUpdateOpRet(scriptPubKey, dummytype, dummyhash, dummyhash);
			case 's':
				return DecodeAgreementCloseOpRet(scriptPubKey, dummytype, dummyhash, dummyhash);
			case 'd':
				return DecodeAgreementDisputeOpRet(scriptPubKey, dummytype, dummyhash, dummypk, dummyhash);
			case 'r':
				return DecodeAgreementDisputeResolveOpRet(scriptPubKey, dummytype, dummyhash, dummypk);
			case 'n':
				return DecodeAgreementUnlockOpRet(scriptPubKey, dummytype, dummyhash, dummyhash);
			default:
				LOGSTREAM((char *)"agreementscc", CCLOG_DEBUG1, stream << "DecodeAgreementOpRet() illegal funcid=" << (int)funcid << std::endl);
				return (uint8_t)0;
		}
	}
	else
		LOGSTREAM("agreementscc",CCLOG_DEBUG1, stream << "not enough opret.[" << vopret.size() << "]" << std::endl);
	return (uint8_t)0;
}
CScript EncodeAgreementProposalOpRet(uint8_t version, uint8_t proposaltype, std::vector<uint8_t> srcpub, std::vector<uint8_t> destpub, std::vector<uint8_t> arbitratorpk, int64_t payment, int64_t arbitratorfee, int64_t depositval, uint256 datahash, uint256 agreementtxid, uint256 prevproposaltxid, std::string name)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'p';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << proposaltype << srcpub << destpub << arbitratorpk << payment << arbitratorfee << depositval << datahash << agreementtxid << prevproposaltxid << name);
	return(opret);
}
uint8_t DecodeAgreementProposalOpRet(CScript scriptPubKey, uint8_t &version, uint8_t &proposaltype, std::vector<uint8_t> &srcpub, std::vector<uint8_t> &destpub, std::vector<uint8_t> &arbitratorpk, int64_t &payment, int64_t &arbitratorfee, int64_t &depositval, uint256 &datahash, uint256 &agreementtxid, uint256 &prevproposaltxid, std::string &name)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> proposaltype; ss >> srcpub; ss >> destpub; ss >> arbitratorpk; ss >> payment; ss >> arbitratorfee; ss >> depositval; ss >> datahash; ss >> agreementtxid; ss >> prevproposaltxid; ss >> name) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}
CScript EncodeAgreementProposalCloseOpRet(uint8_t version, uint256 proposaltxid, std::vector<uint8_t> srcpub)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 't';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << proposaltxid << srcpub);
	return(opret);
}
uint8_t DecodeAgreementProposalCloseOpRet(CScript scriptPubKey, uint8_t &version, uint256 &proposaltxid, std::vector<uint8_t> &srcpub)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> proposaltxid; ss >> srcpub) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}
CScript EncodeAgreementSigningOpRet(uint8_t version, uint256 proposaltxid)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'c';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << proposaltxid);
	return(opret);
}
uint8_t DecodeAgreementSigningOpRet(CScript scriptPubKey, uint8_t &version, uint256 &proposaltxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> proposaltxid) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}
CScript EncodeAgreementUpdateOpRet(uint8_t version, uint256 agreementtxid, uint256 proposaltxid)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'u';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << agreementtxid << proposaltxid);
	return(opret);
}
uint8_t DecodeAgreementUpdateOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &proposaltxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> agreementtxid; ss >> proposaltxid) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}
CScript EncodeAgreementCloseOpRet(uint8_t version, uint256 agreementtxid, uint256 proposaltxid)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 's';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << agreementtxid << proposaltxid);
	return(opret);
}
uint8_t DecodeAgreementCloseOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &proposaltxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> agreementtxid; ss >> proposaltxid) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}
CScript EncodeAgreementDisputeOpRet(uint8_t version, uint256 agreementtxid, std::vector<uint8_t> srcpub, uint256 datahash)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'd';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << agreementtxid << srcpub << datahash);
	return(opret);
}
uint8_t DecodeAgreementDisputeOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, std::vector<uint8_t> &srcpub, uint256 &datahash)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> agreementtxid; ss >> srcpub; ss >> datahash) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}
CScript EncodeAgreementDisputeResolveOpRet(uint8_t version, uint256 agreementtxid, std::vector<uint8_t> rewardedpubkey)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'r';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << agreementtxid << rewardedpubkey);
	return(opret);
}
uint8_t DecodeAgreementDisputeResolveOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, std::vector<uint8_t> &rewardedpubkey)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> agreementtxid; ss >> rewardedpubkey) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}
CScript EncodeAgreementUnlockOpRet(uint8_t version, uint256 agreementtxid, uint256 pawnshoptxid)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'n';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << agreementtxid << pawnshoptxid);
	return(opret);
}
uint8_t DecodeAgreementUnlockOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &pawnshoptxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> agreementtxid; ss >> pawnshoptxid) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}
//===========================================================================
// Validation
//===========================================================================
bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	CTransaction proposaltx, pawnshoptx;
	CScript proposalopret;
	int32_t numvins, numvouts;
	uint256 hashBlock, datahash, agreementtxid, proposaltxid, prevproposaltxid, dummytxid, pawnshoptxid, spendingtxid, latesttxid, borrowtxid, updatetxid, refagreementtxid;
	std::vector<uint8_t> srcpub, destpub, signpub, sellerpk, clientpk, arbitratorpk, rewardedpubkey;
	int64_t payment, arbitratorfee, depositval, totaldeposit, dummyamount, numtokens, numcoins, tokenbalance, coinbalance, refund;
	std::string name, CCerror = "", pawnshopname;
	bool bHasReceiver, bHasArbitrator;
	uint8_t proposaltype, version, spendingfuncid, funcid, updatefuncid;
	uint32_t pawnshopflags;
	char globaladdr[65], srcaddr[65], destaddr[65], arbitratoraddr[65], pawnshopaddr[65];
	CPubKey CPK_src, CPK_dest, CPK_arbitrator, CPK_signer, CPK_rewarded, tokensupplier, coinsupplier;
	struct CCcontract_info *cpPawnshop, CPawnshop;
	cpPawnshop = CCinit(&CPawnshop, EVAL_PAWNSHOP);
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	numvins = tx.vin.size();
	numvouts = tx.vout.size();
	if (numvouts < 1)
		return eval->Invalid("no vouts");
	CCOpretCheck(eval,tx,true,true,true);
	ExactAmounts(eval,tx,ASSETCHAINS_CCZEROTXFEE[EVAL_AGREEMENTS]?0:CC_TXFEE);
	if ((funcid = DecodeAgreementOpRet(tx.vout[numvouts-1].scriptPubKey)) != 0)
	{
		GetCCaddress(cp, globaladdr, GetUnspendable(cp, NULL));
		switch (funcid)
		{
			case 'p':
				/*
				agreement proposal:
				vin.0 normal input
				vin.1 previous proposal marker
				vin.2 previous proposal baton
				vout.0 marker
				vout.1 response hook
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_AGREEMENTS 'p' version proposaltype srcpub destpub arbitratorpk payment arbitratorfee depositvalue datahash agreementtxid prevproposaltxid name
				*/
				// Proposal data validation.
				if (!ValidateProposalOpRet(tx.vout[numvouts-1].scriptPubKey, CCerror))
					return eval->Invalid(CCerror);
				DecodeAgreementProposalOpRet(tx.vout[numvouts-1].scriptPubKey, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, dummytxid, agreementtxid, prevproposaltxid, name);
				CPK_src = pubkey2pk(srcpub);
				CPK_dest = pubkey2pk(destpub);
				bHasReceiver = CPK_dest.IsValid();
				if (TotalPubkeyNormalInputs(tx, CPK_src) == 0 && TotalPubkeyCCInputs(tx, CPK_src) == 0)
					return eval->Invalid("found no normal or cc inputs signed by source pubkey!");
				if (prevproposaltxid != zeroid)
				{
					// Checking if selected proposal was already spent.
					if (IsProposalSpent(prevproposaltxid, spendingtxid, spendingfuncid))
						return eval->Invalid("prevproposal has already been spent!");
					// Comparing proposal data to previous proposal.
					if (!CompareProposals(tx.vout[numvouts-1].scriptPubKey, prevproposaltxid, CCerror))
						return eval->Invalid(CCerror);
				}
				else
					return eval->Invalid("unexpected proposal with no prevproposaltxid in AgreementsValidate!");
				if (bHasReceiver)
					GetCCaddress1of2(cp, destaddr, CPK_src, CPK_dest);
				else
					GetCCaddress(cp, destaddr, CPK_src);
				// Checking if vins/vouts are correct.
				if (numvouts < 3)
					return eval->Invalid("not enough vouts for 'p' tx!");
				else if (ConstrainVout(tx.vout[0], 1, globaladdr, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.0 must be CC marker to agreements global address!");
				else if (ConstrainVout(tx.vout[1], 1, destaddr, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.1 must be CC baton to mutual or srcpub CC address!");
				if (numvins < 3)
					return eval->Invalid("not enough vins for 'p' tx in AgreementsValidate!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal for agreementcreate!");
				else if ((*cp->ismyvin)(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for agreementcreate!");
				// does vin1 and vin2 point to the previous proposal's vout0 and vout1? (if it doesn't, the tx might have no previous proposal, in that case it shouldn't have CC inputs)
				else if (tx.vin[1].prevout.hash != prevproposaltxid || tx.vin[1].prevout.n != 0)
					return eval->Invalid("vin.1 tx hash doesn't match prevproposaltxid!");
				else if ((*cp->ismyvin)(tx.vin[2].scriptSig) == 0)
					return eval->Invalid("vin.2 must be CC for agreementcreate!");
				else if (tx.vin[2].prevout.hash != prevproposaltxid || tx.vin[2].prevout.n != 1)
					return eval->Invalid("vin.2 tx hash doesn't match prevproposaltxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 3; i > numvins; i++)
				{
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
				vout.n-1 OP_RETURN EVAL_AGREEMENTS 't' version proposaltxid signpub
				*/
				// Getting the transaction data.
				DecodeAgreementProposalCloseOpRet(tx.vout[numvouts-1].scriptPubKey, version, proposaltxid, signpub);
				if (myGetTransaction(proposaltxid, proposaltx, hashBlock) == 0 || proposaltx.vout.size() <= 0)
					return eval->Invalid("couldn't find proposaltx for 't' tx!");
				if (IsProposalSpent(proposaltxid, spendingtxid, spendingfuncid))
					return eval->Invalid("prevproposal has already been spent!");
				// Retrieving the proposal data tied to this transaction.
				DecodeAgreementProposalOpRet(proposaltx.vout[proposaltx.vout.size()-1].scriptPubKey, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, dummytxid, agreementtxid, prevproposaltxid, name);
				CPK_src = pubkey2pk(srcpub);
				CPK_signer = pubkey2pk(signpub);
				CPK_dest = pubkey2pk(destpub);
				bHasReceiver = CPK_dest.IsValid();
				if (TotalPubkeyNormalInputs(tx, CPK_signer) == 0 && TotalPubkeyCCInputs(tx, CPK_signer) == 0)
					return eval->Invalid("found no normal or cc inputs signed by signer pubkey!");
				// We won't be using ValidateProposalOpRet here, since we only care about sender and receiver, and even if the other data in the proposal is invalid the proposal will be closed anyway
				// Instead we'll just check if the source pubkey of this tx is allowed to close the proposal.
				switch (proposaltype)
				{
					case 'p':
						if (bHasReceiver && CPK_signer != CPK_src && CPK_signer != CPK_dest)
							return eval->Invalid("signpub is not the source or receiver of specified proposal!");
						else if (!bHasReceiver && CPK_signer != CPK_src)
							return eval->Invalid("signpub is not the source of specified proposal!");
						break;
					case 'u':
					case 't':
						if (agreementtxid == zeroid)
							return eval->Invalid("proposal has no defined agreement, unable to verify membership!");
						if (!GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, arbitratorfee, depositval, dummytxid, dummytxid, name))
							return eval->Invalid("couldn't get proposal's agreement name successfully!");
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
					return eval->Invalid("vin.0 must be normal for agreementstopproposal!");
				else if ((*cp->ismyvin)(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for agreementstopproposal!");
				// does vin1 point to the previous proposal's vout1?
				else if (tx.vin[1].prevout.hash != proposaltxid || tx.vin[1].prevout.n != 1)
					return eval->Invalid("vin.1 tx hash doesn't match proposaltxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 2; i > numvins; i++)
				{
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
				vout.2 deposit / agreement completion baton
				vout.3 payment (optional)
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_AGREEMENTS 'c' version proposaltxid
				*/
				// Getting the transaction data.
				if (!GetAcceptedProposalOpRet(tx, proposaltxid, proposalopret))
					return eval->Invalid("couldn't find proposal tx opret for 'c' tx!");
				// Retrieving the proposal data tied to this transaction.
				DecodeAgreementProposalOpRet(proposalopret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, dummytxid, agreementtxid, prevproposaltxid, name);
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
				// Check pubkeys.
				myGetTransaction(proposaltxid, proposaltx, hashBlock);
				if (TotalPubkeyNormalInputs(proposaltx, CPK_src) == 0 && TotalPubkeyCCInputs(proposaltx, CPK_src) == 0)
					return eval->Invalid("found no normal or cc inputs signed by proposal source pubkey!");
				if (TotalPubkeyCCInputs(tx, CPK_dest) == 0)
					return eval->Invalid("found no cc inputs signed by proposal receiver pubkey!");
				Getscriptaddress(srcaddr, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG);
				GetCCaddress1of2(cp, destaddr, CPK_src, CPK_dest);
				// Checking if vins/vouts are correct.
				if (numvouts < 4)
					return eval->Invalid("not enough vouts for 'c' tx!");
				else if (ConstrainVout(tx.vout[0], 1, globaladdr, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.0 must be CC marker to agreements global address!");
				else if (ConstrainVout(tx.vout[1], 1, destaddr, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.1 must be CC baton to mutual CC address!");
				else if (ConstrainVout(tx.vout[2], 1, globaladdr, depositval) == 0)
					return eval->Invalid("vout.2 must be deposit to global CC address!");
				else if (payment > 0 && ConstrainVout(tx.vout[3], 0, srcaddr, payment) == 0)
					return eval->Invalid("vout.3 must be normal payment to srcaddr when payment defined!");
				if (numvins < 3)
					return eval->Invalid("not enough vins for 'c' tx!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal for 'c' tx!");
				else if ((*cp->ismyvin)(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for 'c' tx!");
				// does vin1 and vin2 point to the proposal's vout0 and vout1?
				else if (tx.vin[1].prevout.hash != proposaltxid || tx.vin[1].prevout.n != 0)
					return eval->Invalid("vin.1 tx hash doesn't match proposaltxid!");
				else if ((*cp->ismyvin)(tx.vin[2].scriptSig) == 0)
					return eval->Invalid("vin.2 must be CC for 'c' tx!");
				else if (tx.vin[2].prevout.hash != proposaltxid || tx.vin[2].prevout.n != 1)
					return eval->Invalid("vin.2 tx hash doesn't match proposaltxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 3; i > numvins; i++)
				{
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
				vout.n-1 OP_RETURN EVAL_AGREEMENTS 'u' version agreementtxid proposaltxid
				*/
				// Getting the transaction data.
				if (!GetAcceptedProposalOpRet(tx, proposaltxid, proposalopret))
					return eval->Invalid("couldn't find proposal tx opret for 'u' tx!");
				// Retrieving the proposal data tied to this transaction.
				DecodeAgreementProposalOpRet(proposalopret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, dummytxid, agreementtxid, prevproposaltxid, name);
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
				// Check pubkeys.
				myGetTransaction(proposaltxid, proposaltx, hashBlock);
				if (TotalPubkeyNormalInputs(proposaltx, CPK_src) == 0 && TotalPubkeyCCInputs(proposaltx, CPK_src) == 0)
					return eval->Invalid("found no normal or cc inputs signed by proposal source pubkey!");
				if (TotalPubkeyCCInputs(tx, CPK_dest) == 0)
					return eval->Invalid("found no cc inputs signed by proposal receiver pubkey!");
				// Getting the latest update txid.
				GetLatestAgreementUpdate(agreementtxid, latesttxid, updatefuncid);
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
				else if ((*cp->ismyvin)(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for 'u' tx!");
				else if (latesttxid == agreementtxid)
				{
					if (tx.vin[1].prevout.hash != agreementtxid || tx.vin[1].prevout.n != 1)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				else if (latesttxid != agreementtxid)
				{
					if (tx.vin[1].prevout.hash != latesttxid || tx.vin[1].prevout.n != 0)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				else if ((*cp->ismyvin)(tx.vin[2].scriptSig) == 0)
					return eval->Invalid("vin.2 must be CC for 'u' tx!");
				// does vin2 and vin3 point to the proposal's vout0 and vout1?
				else if (tx.vin[2].prevout.hash != proposaltxid || tx.vin[2].prevout.n != 0)
					return eval->Invalid("vin.2 tx hash doesn't match proposaltxid!");
				else if ((*cp->ismyvin)(tx.vin[3].scriptSig) == 0)
					return eval->Invalid("vin.3 must be CC for 'u' tx!");
				else if (tx.vin[3].prevout.hash != proposaltxid || tx.vin[3].prevout.n != 1)
					return eval->Invalid("vin.3 tx hash doesn't match proposaltxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 4; i > numvins; i++)
				{
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
				vout.n-1 OP_RETURN EVAL_AGREEMENTS 's' version agreementtxid proposaltxid
				*/
				// Getting the transaction data.
				if (!GetAcceptedProposalOpRet(tx, proposaltxid, proposalopret))
					return eval->Invalid("couldn't find proposal tx opret for 's' tx!");
				// Retrieving the proposal data tied to this transaction.
				DecodeAgreementProposalOpRet(proposalopret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, dummytxid, agreementtxid, prevproposaltxid, name);
				GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, arbitratorfee, totaldeposit, dummytxid, dummytxid, name);
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
				// Check pubkeys.
				myGetTransaction(proposaltxid, proposaltx, hashBlock);
				if (TotalPubkeyNormalInputs(proposaltx, CPK_src) == 0 && TotalPubkeyCCInputs(proposaltx, CPK_src) == 0)
					return eval->Invalid("found no normal or cc inputs signed by proposal source pubkey!");
				if (TotalPubkeyCCInputs(tx, CPK_dest) == 0)
					return eval->Invalid("found no cc inputs signed by proposal receiver pubkey!");
				// Getting the latest update txid.
				GetLatestAgreementUpdate(agreementtxid, latesttxid, updatefuncid);
				Getscriptaddress(srcaddr, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG);
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
				else if ((*cp->ismyvin)(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for 's' tx!");
				else if (latesttxid == agreementtxid)
				{
					if (tx.vin[1].prevout.hash != agreementtxid || tx.vin[1].prevout.n != 1)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				else if (latesttxid != agreementtxid)
				{
					if (tx.vin[1].prevout.hash != latesttxid || tx.vin[1].prevout.n != 0)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				else if ((*cp->ismyvin)(tx.vin[2].scriptSig) == 0)
					return eval->Invalid("vin.2 must be CC for 's' tx!");
				// does vin2 and vin3 point to the proposal's vout0 and vout1?
				else if (tx.vin[2].prevout.hash != proposaltxid || tx.vin[2].prevout.n != 0)
					return eval->Invalid("vin.2 tx hash doesn't match proposaltxid!");
				else if ((*cp->ismyvin)(tx.vin[3].scriptSig) == 0)
					return eval->Invalid("vin.3 must be CC for 's' tx!");
				else if (tx.vin[3].prevout.hash != proposaltxid || tx.vin[3].prevout.n != 1)
					return eval->Invalid("vin.3 tx hash doesn't match proposaltxid!");
				else if ((*cp->ismyvin)(tx.vin[4].scriptSig) == 0)
					return eval->Invalid("vin.4 must be CC for 's' tx!");
				else if (tx.vin[4].prevout.hash != agreementtxid || tx.vin[4].prevout.n != 2)
					return eval->Invalid("vin.4 tx hash doesn't match agreementtxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 5; i > numvins; i++)
				{
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
				vout.n-1 OP_RETURN EVAL_AGREEMENTS 'd' version agreementtxid srcpub datahash
				*/
				// Getting the transaction data.
				DecodeAgreementDisputeOpRet(tx.vout[numvouts-1].scriptPubKey, version, agreementtxid, signpub, datahash);
				if (datahash == zeroid)
					return eval->Invalid("datahash empty or invalid for 'd' tx!");
				if (!GetAgreementInitialData(agreementtxid, dummytxid, srcpub, destpub, arbitratorpk, arbitratorfee, totaldeposit, dummytxid, dummytxid, name))
					return eval->Invalid("couldn't find agreement tx for 'd' tx!");
				GetLatestAgreementUpdate(agreementtxid, latesttxid, updatefuncid);
				if (updatefuncid != 'c' && updatefuncid != 'u')
					return eval->Invalid("agreement inactive or already in dispute!");
				CPK_src = pubkey2pk(srcpub);
				CPK_dest = pubkey2pk(destpub);
				CPK_signer = pubkey2pk(signpub);
				if (TotalPubkeyCCInputs(tx, CPK_signer) == 0)
					return eval->Invalid("found no cc inputs signed by signer pubkey!");
				if (CPK_signer != CPK_src && CPK_signer != CPK_dest)
					return eval->Invalid("signer pubkey is not a member of the agreement!");
				// Some arbitrator shenanigans.
				CPK_arbitrator = pubkey2pk(arbitratorpk);
				if (!(CPK_arbitrator.IsValid()))
					return eval->Invalid("no valid arbitrator found in agreement!");
				GetAgreementUpdateData(latesttxid, name, dummytxid, arbitratorfee, depositval, dummyamount);
				GetCCaddress(cp, arbitratoraddr, CPK_arbitrator);
				// Checking if vins/vouts are correct.
				if (numvouts < 2)
					return eval->Invalid("not enough vouts for 'd' tx!");
				else if (ConstrainVout(tx.vout[0], 1, arbitratoraddr, arbitratorfee) == 0)
					return eval->Invalid("vout.0 must be CC fee to agreements arbitrator's address!");
				if (numvins < 2)
					return eval->Invalid("not enough vins for 'd' tx!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal for agreementdispute!");
				else if ((*cp->ismyvin)(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for agreementdispute!");
				// does vin1 point to latesttxid baton?
				else if (latesttxid == agreementtxid)
				{
					if (tx.vin[1].prevout.hash != agreementtxid || tx.vin[1].prevout.n != 1)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				else if (latesttxid != agreementtxid)
				{
					if (tx.vin[1].prevout.hash != latesttxid || tx.vin[1].prevout.n != 0)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				// Do not allow any additional CC vins.
				for (int32_t i = 2; i > numvins; i++)
				{
					if (IsCCInput(tx.vin[i].scriptSig) != 0)
						return eval->Invalid("tx exceeds allowed amount of CC vins!");
				}
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
				vout.n-1 OP_RETURN EVAL_AGREEMENTS 'r' version agreementtxid rewardedpubkey
				*/
				// Getting the transaction data.
				DecodeAgreementDisputeResolveOpRet(tx.vout[numvouts-1].scriptPubKey, version, agreementtxid, rewardedpubkey);
				if (!GetAgreementInitialData(agreementtxid, dummytxid, srcpub, destpub, arbitratorpk, arbitratorfee, depositval, dummytxid, dummytxid, name))
					return eval->Invalid("couldn't find agreement tx for 'r' tx!");
				GetLatestAgreementUpdate(agreementtxid, latesttxid, updatefuncid);
				if (updatefuncid != 'd')
					return eval->Invalid("agreement not in dispute!");
				CPK_src = pubkey2pk(srcpub);
				CPK_dest = pubkey2pk(destpub);
				CPK_rewarded = pubkey2pk(rewardedpubkey);
				if (CPK_rewarded != CPK_src && CPK_rewarded != CPK_dest)
					return eval->Invalid("rewarded pubkey is not a member of the agreement!");
				// Some arbitrator shenanigans.
				CPK_arbitrator = pubkey2pk(arbitratorpk);
				if (!(CPK_arbitrator.IsValid()))
					return eval->Invalid("no valid arbitrator found in agreement!");
				if (TotalPubkeyCCInputs(tx, CPK_arbitrator) == 0)
					return eval->Invalid("found no cc inputs signed by arbitrator pubkey!");
				Getscriptaddress(destaddr, CScript() << ParseHex(HexStr(CPK_rewarded)) << OP_CHECKSIG);
				// Checking if vins/vouts are correct.
				if (numvouts < 2)
					return eval->Invalid("not enough vouts for 'r' tx!");
				else if (ConstrainVout(tx.vout[0], 0, destaddr, depositval) == 0)
					return eval->Invalid("vout.0 must be normal deposit payout to rewarded pubkey address!");
				if (numvins < 3)
					return eval->Invalid("not enough vins for 'r' tx!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal for agreementresolve!");
				else if ((*cp->ismyvin)(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for agreementresolve!");
				// does vin1 point to latesttxid baton?
				else if (latesttxid == agreementtxid)
				{
					if (tx.vin[1].prevout.hash != agreementtxid || tx.vin[1].prevout.n != 1)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				else if (latesttxid != agreementtxid)
				{
					if (tx.vin[1].prevout.hash != latesttxid || tx.vin[1].prevout.n != 0)
						return eval->Invalid("vin.1 tx hash doesn't match latesttxid!");
				}
				// does vin2 point to deposit?
				else if ((*cp->ismyvin)(tx.vin[2].scriptSig) == 0)
					return eval->Invalid("vin.2 must be CC for agreementresolve!");
				else if (tx.vin[2].prevout.hash != agreementtxid || tx.vin[2].prevout.n != 2)
					return eval->Invalid("vin.2 tx hash doesn't match agreementtxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 3; i > numvins; i++)
				{
					if (IsCCInput(tx.vin[i].scriptSig) != 0)
						return eval->Invalid("tx exceeds allowed amount of CC vins!");
				}
				break;
			case 'n':
				/*
				unlock contract deposit:
				vin.0 normal input
				vin.1 previous proposal baton
				vin.2 deposit
				vout.0 payout to pawnshop CC 1of2 address
				vout.1 deposit refund to client (optional)
				vout.2 arbitrator fee payout to arbitrator (optional)
				vout.n-2 change
				vout.n-1 OP_RETURN EVAL_AGREEMENTS 'n' agreementtxid pawnshoptxid
				*/
				
				// Getting the transaction data.
				DecodeAgreementUnlockOpRet(tx.vout[numvouts-1].scriptPubKey, version, agreementtxid, pawnshoptxid);
				if (pawnshoptxid == zeroid)
					return eval->Invalid("pawnshoptxid invalid or empty!");
				if (!GetAgreementInitialData(agreementtxid, dummytxid, srcpub, destpub, arbitratorpk, arbitratorfee, depositval, dummytxid, dummytxid, name))
					return eval->Invalid("couldn't find agreement tx for 'n' tx!");
				GetLatestAgreementUpdate(agreementtxid, updatetxid, updatefuncid);
				if (updatefuncid != 'c' && updatefuncid != 'u')
					return eval->Invalid("agreement inactive or suspended!");
				CPK_src = pubkey2pk(srcpub);
				CPK_dest = pubkey2pk(destpub);
				if (TotalPubkeyCCInputs(tx, CPK_src) == 0 && TotalPubkeyCCInputs(tx, CPK_dest) == 0)
					return eval->Invalid("found no cc inputs signed by agreement member pubkey!");
				Getscriptaddress(destaddr, CScript() << ParseHex(HexStr(CPK_dest)) << OP_CHECKSIG);
				// Checking pawnshop.
				if (myGetTransaction(pawnshoptxid, pawnshoptx, hashBlock) == 0 || pawnshoptx.vout.size() <= 0)
					return eval->Invalid("cant find pawnshop tx!");
				if (DecodePawnshopCreateOpRet(pawnshoptx.vout[pawnshoptx.vout.size() - 1].scriptPubKey,version,pawnshopname,tokensupplier,coinsupplier,pawnshopflags,dummytxid,numtokens,numcoins,refagreementtxid) == 0)
					return eval->Invalid("invalid pawnshop open opret!");
				if (TotalPubkeyCCInputs(tx, coinsupplier) == 0)
					return eval->Invalid("found no cc inputs signed by excahnge coinsupplier pubkey!");
				GetCCaddress1of2(cpPawnshop, pawnshopaddr, tokensupplier, coinsupplier);
				if (refagreementtxid != agreementtxid)
					return eval->Invalid("agreement txid in pawnshop is different from agreement txid specified!");
				if (!(pawnshopflags & PTF_REQUIREUNLOCK))
					return eval->Invalid("deposit unlock is disabled for this pawnshop!");
				if (!ValidatePawnshopCreateTx(pawnshoptx,CCerror))
					return eval->Invalid(CCerror);
				if (!GetLatestPawnshopTxid(pawnshoptxid, latesttxid, updatefuncid) || updatefuncid == 'e' || updatefuncid == 'x')
					return eval->Invalid("pawnshop tx closed!");
				if (CheckDepositUnlockCond(pawnshoptxid) >= 0)
					return eval->Invalid("deposit unlock already sent to pawnshop!");
				coinbalance = GetPawnshopInputs(cpPawnshop,pawnshoptx,PIF_COINS,unspentOutputs);
				tokenbalance = GetPawnshopInputs(cpPawnshop,pawnshoptx,PIF_TOKENS,unspentOutputs);
				if (tokenbalance < numtokens)
					return eval->Invalid("not enough tokens in pawnshop!");
				if (coinbalance + depositval < numcoins)
					return eval->Invalid("not enough coins in pawnshop!");
				else
					refund = coinbalance + depositval - numcoins;
				// Checking if vins/vouts are correct.
				if (numvouts < 2)
					return eval->Invalid("not enough vouts for 'n' tx!");
				else if (coinbalance < numcoins && refund > 0) // contains deposit payout & refund
				{
					if (numvouts < 3)
						return eval->Invalid("not enough vouts for 'n' tx!");
					else if (ConstrainVout(tx.vout[0], 1, pawnshopaddr, depositval - refund) == 0)
						return eval->Invalid("vout.0 must be CC to pawnshop mutual 1of2 address!");
					else if (ConstrainVout(tx.vout[1], 0, destaddr, refund) == 0)
						return eval->Invalid("vout.1 must be normal deposit refund payout to destpub!");
				}
				else if (coinbalance < numcoins && refund == 0) // contains deposit payout only
				{
					if (ConstrainVout(tx.vout[0], 1, pawnshopaddr, depositval) == 0)
						return eval->Invalid("vout.0 must be CC to pawnshop mutual 1of2 address!");
				}
				else if (coinbalance >= numcoins && refund > 0) // contains deposit refund only
				{
					if (ConstrainVout(tx.vout[0], 0, destaddr, refund) == 0)
						return eval->Invalid("vout.0 must be normal deposit refund payout to destpub!");
				}
				if (numvins < 3)
					return eval->Invalid("not enough vins for 'n' tx!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal for agreementunlock!");
				else if ((*cp->ismyvin)(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC for agreementunlock!");
				// does vin1 point to latest update baton?
				else if (updatetxid == agreementtxid)
				{
					if (tx.vin[1].prevout.hash != agreementtxid || tx.vin[1].prevout.n != 1)
						return eval->Invalid("vin.1 tx hash doesn't match updatetxid!");
				}
				else if (updatetxid != agreementtxid)
				{
					if (tx.vin[1].prevout.hash != updatetxid || tx.vin[1].prevout.n != 0)
						return eval->Invalid("vin.1 tx hash doesn't match updatetxid!");
				}
				// does vin2 point to deposit?
				else if ((*cp->ismyvin)(tx.vin[2].scriptSig) == 0)
					return eval->Invalid("vin.2 must be CC for agreementunlock!");
				else if (tx.vin[2].prevout.hash != agreementtxid || tx.vin[2].prevout.n != 2)
					return eval->Invalid("vin.2 tx hash doesn't match agreementtxid!");
				// Do not allow any additional CC vins.
				for (int32_t i = 3; i > numvins; i++)
				{
					if (IsCCInput(tx.vin[i].scriptSig) != 0)
						return eval->Invalid("tx exceeds allowed amount of CC vins!");
				}
				break;
			default:
				fprintf(stderr,"unexpected agreements funcid (%c)\n",funcid);
				return eval->Invalid("unexpected agreements funcid!");
		}
	}
	else
		return eval->Invalid("must be valid agreements funcid!");
	LOGSTREAM("agreements", CCLOG_INFO, stream << "Agreements tx validated" << std::endl);
	return true;
}
//===========================================================================
// Helper functions
//===========================================================================

bool GetAcceptedProposalOpRet(CTransaction tx, uint256 &proposaltxid, CScript &opret)
{
	CTransaction proposaltx;
	uint8_t version, funcid;
	uint256 agreementtxid, hashBlock;
	int64_t deposit_send, deposit_keep;
	if (tx.vout.size() <= 0)
	{
		std::cerr << "GetAcceptedProposalOpRet: given tx has no vouts" << std::endl;
		return false;
	}
	if ((funcid = DecodeAgreementOpRet(tx.vout[tx.vout.size() - 1].scriptPubKey)) != 'c' && funcid != 'u' && funcid != 's')
	{
		std::cerr << "GetAcceptedProposalOpRet: given tx doesn't have a correct funcid" << std::endl;
		return false;
	}
	switch (funcid)
	{
		case 'c':
			DecodeAgreementSigningOpRet(tx.vout[tx.vout.size() - 1].scriptPubKey, version, proposaltxid);
			break;
		case 'u':
			DecodeAgreementUpdateOpRet(tx.vout[tx.vout.size() - 1].scriptPubKey, version, agreementtxid, proposaltxid);
			break;
		case 's':
			DecodeAgreementCloseOpRet(tx.vout[tx.vout.size() - 1].scriptPubKey, version, agreementtxid, proposaltxid);
			break;
	}
	if (myGetTransaction(proposaltxid, proposaltx, hashBlock) == 0 || proposaltx.vout.size() <= 0)
	{
		std::cerr << "GetAcceptedProposalOpRet: couldn't find agreement accepted proposal tx" << std::endl;
		return false;
	}
	opret = proposaltx.vout[proposaltx.vout.size() - 1].scriptPubKey;
	return true;
}

bool ValidateProposalOpRet(CScript opret, std::string &CCerror)
{
	CTransaction agreementtx;
	uint256 proposaltxid, datahash, agreementtxid, refagreementtxid, prevproposaltxid, spendingtxid, hashBlock;
	uint8_t version, proposaltype, spendingfuncid;
	std::vector<uint8_t> srcpub, destpub, sellerpk, clientpk, arbitratorpk, ref_arbitratorpk;
	int64_t payment, arbitratorfee, ref_arbitratorfee, depositval, ref_depositval;
	std::string name;
	bool bHasReceiver, bHasArbitrator;
	CPubKey CPK_src, CPK_dest, CPK_arbitrator;
	CCerror = "";
	LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: decoding opret" << std::endl);
	if (DecodeAgreementProposalOpRet(opret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, depositval, datahash, agreementtxid, prevproposaltxid, name) != 'p')
	{
		CCerror = "proposal tx opret invalid or not a proposal tx!";
		return false;
	}
	LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: check if name meets requirements (not empty, <= 64 chars)" << std::endl);
	if (name.empty() || name.size() > 64)
	{
		CCerror = "proposal name empty or exceeds 64 chars!";
		return false;
	}
	LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: check if datahash meets requirements (not empty)" << std::endl);
	if (datahash == zeroid)
	{
		CCerror = "proposal datahash empty!";
		return false;
	}
	LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: check if payment is positive" << std::endl);
	if (payment < 0)
	{
		CCerror = "proposal has payment < 0!";
		return false;
	}
	CPK_src = pubkey2pk(srcpub);
	CPK_dest = pubkey2pk(destpub);
	CPK_arbitrator = pubkey2pk(arbitratorpk);
	bHasReceiver = CPK_dest.IsValid();
	bHasArbitrator = CPK_arbitrator.IsValid();
	LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: making sure srcpub != destpub != arbitratorpk" << std::endl);
	if (bHasReceiver && CPK_src == CPK_dest)
	{
		CCerror = "proposal srcpub cannot be the same as destpub!";
		return false;
	}
	if (bHasArbitrator && CPK_src == CPK_arbitrator)
	{
		CCerror = "proposal srcpub cannot be the same as arbitrator pubkey!";
		return false;
	}
	if (bHasReceiver && bHasArbitrator && CPK_dest == CPK_arbitrator)
	{
		CCerror = "proposal destpub cannot be the same as arbitrator pubkey!";
		return false;
	}
	switch (proposaltype)
	{
		case 'p':
			LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: checking deposit value" << std::endl);
			if (depositval < CC_MARKER_VALUE)
			{
				CCerror = "proposal doesn't have minimum required deposit!";
				return false;
			}
			LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: checking arbitrator fee" << std::endl);
			if (arbitratorfee < 0 || bHasArbitrator && arbitratorfee < CC_MARKER_VALUE)
			{
				CCerror = "proposal has invalid arbitrator fee value!";
				return false;
			}
			if (agreementtxid != zeroid)
			{
				LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: refagreement was defined, check if it's a correct tx" << std::endl);
				if (myGetTransaction(agreementtxid, agreementtx, hashBlock) == 0 || agreementtx.vout.size() <= 0)
				{
					CCerror = "proposal's refagreementtxid has nonexistent tx!";
					return false;
				}
				if (DecodeAgreementSigningOpRet(agreementtx.vout[agreementtx.vout.size() - 1].scriptPubKey, version, proposaltxid) != 'c')
				{
					CCerror = "proposal refagreement tx is not a contract signing tx!";
					return false;
				}
				LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: checking if subcontract's srcpub and destpub are members of the refagreement" << std::endl);
				if (!GetAgreementInitialData(agreementtxid, proposaltxid, sellerpk, clientpk, ref_arbitratorpk, ref_arbitratorfee, depositval, datahash, refagreementtxid, name))
				{
					CCerror = "refagreement tx has invalid agreement member pubkeys!";
					return false;
				}
				LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: checking arbitrator fee" << std::endl);
				if (arbitratorfee < 0 || bHasArbitrator && arbitratorfee < CC_MARKER_VALUE)
				{
					CCerror = "proposal has invalid arbitrator fee value!";
					return false;
				}
				if (!bHasReceiver || CPK_src != pubkey2pk(sellerpk) && CPK_src != pubkey2pk(clientpk) && CPK_dest != pubkey2pk(sellerpk) && CPK_dest != pubkey2pk(clientpk))
				{
					CCerror = "subcontracts must have at least one party that's a member in the refagreementtxid!";
					return false;
				}
			}
			break;
		case 'u':
			LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: checking deposit value for 'u' tx" << std::endl);
			if (depositval != 0)
			{
				CCerror = "proposal has invalid deposit value for update!";
				return false;
			}
		// intentional fall-through
		case 't':
			LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: checking if update/termination proposal has destpub" << std::endl);
			if (!bHasReceiver)
			{
				CCerror = "proposal has no defined receiver on update/termination proposal!";
				return false;
			}
			LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: checking if agreementtxid defined" << std::endl);
			if (agreementtxid == zeroid)
			{
				CCerror = "proposal has no agreementtxid defined for update/termination proposal!";
				return false;
			}
			LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: agreementtxid status check" << std::endl);
			if (GetLatestAgreementUpdate(agreementtxid, spendingtxid, spendingfuncid))
			{
				if (spendingfuncid == 'd')
				{
					CCerror = "proposal's specified agreement is in dispute!";
					return false;
				}
				else if (spendingfuncid != 'c' && spendingfuncid != 'u')
				{
					CCerror = "proposal's specified agreement is no longer active!";
					return false;
				}
			}
			else
			{
				CCerror = "proposal's agreementtxid name not found!";
				return false;
			}
			if (!GetAgreementInitialData(agreementtxid, proposaltxid, sellerpk, clientpk, ref_arbitratorpk, arbitratorfee, ref_depositval, datahash, refagreementtxid, name))
			{
				CCerror = "proposal agreement tx has invalid agreement data!";
				return false;
			}
			LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: checking deposit value" << std::endl);
			if (depositval < 0 || depositval > ref_depositval)
			{
				CCerror = "proposal has invalid deposit value!";
				return false;
			}
			LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: checking if srcpub and destpub are members of the agreement" << std::endl);
			if (CPK_src != pubkey2pk(sellerpk) && CPK_src != pubkey2pk(clientpk) || CPK_dest != pubkey2pk(sellerpk) && CPK_dest != pubkey2pk(clientpk))
			{
				CCerror = "proposal srcpub or destpub is not a member of the specified agreement!";
				return false;
			}
			LOGSTREAM("agreements", CCLOG_INFO, stream << "ValidateProposalOpRet: checking arbitrator" << std::endl);
			if (bHasArbitrator && CPK_arbitrator != pubkey2pk(ref_arbitratorpk))
			{
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
bool CompareProposals(CScript proposalopret, uint256 refproposaltxid, std::string &CCerror)
{
	CTransaction refproposaltx;
	uint256 hashBlock, datahash, agreementtxid, ref_agreementtxid, prevproposaltxid, ref_prevproposaltxid;
	std::vector<uint8_t> srcpub, ref_srcpub, destpub, ref_destpub, arbitratorpk, ref_arbitratorpk;
	int64_t payment, arbitratorfee, deposit;
	std::string name;
	uint8_t proposaltype, ref_proposaltype, version, ref_version;
	CCerror = "";
	LOGSTREAM("agreements", CCLOG_INFO, stream << "CompareProposals: decoding opret" << std::endl);
	if (DecodeAgreementProposalOpRet(proposalopret, version, proposaltype, srcpub, destpub, arbitratorpk, payment, arbitratorfee, deposit, datahash, agreementtxid, prevproposaltxid, name) != 'p')
	{
		CCerror = "proposal tx opret invalid or not a proposal tx!";
		return false;
	}
	LOGSTREAM("agreements", CCLOG_INFO, stream << "CompareProposals: fetching refproposal tx" << std::endl);
	if (myGetTransaction(refproposaltxid, refproposaltx, hashBlock) == 0 || refproposaltx.vout.size() <= 0)
	{
		CCerror = "couldn't find previous proposal tx!";;
		return false;
	}
	LOGSTREAM("agreements", CCLOG_INFO, stream << "CompareProposals: decoding refproposaltx opret" << std::endl);
	if (DecodeAgreementProposalOpRet(refproposaltx.vout[refproposaltx.vout.size()-1].scriptPubKey, ref_version, ref_proposaltype, ref_srcpub, ref_destpub, ref_arbitratorpk, payment, arbitratorfee, deposit, datahash, ref_agreementtxid, ref_prevproposaltxid, name) != 'p')
	{
		CCerror = "previous proposal tx opret invalid or not a proposal tx!";
		return false;
	}
	LOGSTREAM("agreements", CCLOG_INFO, stream << "CompareProposals: checking if refproposaltxid = prevproposaltxid" << std::endl);
	if (refproposaltxid != prevproposaltxid)
	{
		CCerror = "current proposal doesn't correctly refer to the previous proposal!";
		return false;
	}
	LOGSTREAM("agreements", CCLOG_INFO, stream << "CompareProposals: checking if proposal types match" << std::endl);
	if (proposaltype != ref_proposaltype)
	{
		CCerror = "current and previous proposal types don't match!";
		return false;
	}
	switch (proposaltype)
	{
		case 't':
		case 'u':
			LOGSTREAM("agreements", CCLOG_INFO, stream << "CompareProposals: checking if dest pubkeys match" << std::endl);
			if (destpub != ref_destpub)
			{
				CCerror = "current and previous proposal destination pubkeys don't match!";
				return false;
			}
			LOGSTREAM("agreements", CCLOG_INFO, stream << "CompareProposals: checking if agreementtxid matches" << std::endl);
			if (agreementtxid != ref_agreementtxid)
			{
				CCerror = "current and previous proposal agreement id doesn't match!";
				return false;
			}
		// intentional fall-through
		case 'p':
			LOGSTREAM("agreements", CCLOG_INFO, stream << "CompareProposals: checking if src pubkeys match" << std::endl);
			if (srcpub != ref_srcpub)
			{
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
	if ((retcode = CCgetspenttxid(spendingtxid, vini, height, proposaltxid, 1)) == 0)
	{
		if (myGetTransaction(spendingtxid, spendingtx, hashBlock) != 0 && spendingtx.vout.size() > 0)
			spendingfuncid = DecodeAgreementOpRet(spendingtx.vout[spendingtx.vout.size() - 1].scriptPubKey);
			// if 'c' or 'u' or 's', proposal was accepted
			// if 'p', proposal was amended with another proposal
			// if 't', proposal was closed
		else
			spendingfuncid = 0;
		return true;
	}
	return false;
}

// gets the data from the accepted proposal for the specified agreement txid
// this is for "static" data like seller, client & arbitrator pubkeys. gathering "updateable" data like name, datahash etc are handled by different functions
bool GetAgreementInitialData(uint256 agreementtxid, uint256 &proposaltxid, std::vector<uint8_t> &sellerpk, std::vector<uint8_t> &clientpk, std::vector<uint8_t> &arbitratorpk, int64_t &firstarbitratorfee, int64_t &deposit, uint256 &firstdatahash, uint256 &refagreementtxid, std::string &firstinfo)
{
	CScript proposalopret;
	CTransaction agreementtx;
	uint256 prevproposaltxid, hashBlock;
	uint8_t version, proposaltype;
	int64_t payment;
	if (myGetTransaction(agreementtxid, agreementtx, hashBlock) == 0 || agreementtx.vout.size() <= 0)
	{
		std::cerr << "GetAgreementInitialData: couldn't find agreement tx" << std::endl;
		return false;
	}
	if (!GetAcceptedProposalOpRet(agreementtx, proposaltxid, proposalopret))
	{
		std::cerr << "GetAgreementInitialData: couldn't get accepted proposal tx opret" << std::endl;
		return false;
	}
	if (DecodeAgreementProposalOpRet(proposalopret, version, proposaltype, sellerpk, clientpk, arbitratorpk, payment, firstarbitratorfee, deposit, firstdatahash, refagreementtxid, prevproposaltxid, firstinfo) != 'p' || proposaltype != 'p')
	{
		std::cerr << "GetAgreementInitialData: agreement accepted proposal tx opret invalid" << std::endl;
		return false;
	}
	return true;
}
// gets the latest update baton txid of a agreement
// can be also used to check status of a agreement by looking at its funcid
bool GetLatestAgreementUpdate(uint256 agreementtxid, uint256 &latesttxid, uint8_t &funcid)
{
	int32_t vini, height, retcode;
	uint256 batontxid, sourcetxid, hashBlock;
	CTransaction agreementtx, batontx;
	// special handling for agreement tx - baton vout is vout1
	if (myGetTransaction(agreementtxid, agreementtx, hashBlock) == 0 || agreementtx.vout.size() <= 0)
	{
		std::cerr << "GetLatestAgreementUpdate: couldn't find agreement tx" << std::endl;
		return false;
	}
	if (DecodeAgreementOpRet(agreementtx.vout[agreementtx.vout.size() - 1].scriptPubKey) != 'c')
	{
		std::cerr << "GetLatestAgreementUpdate: agreement tx is not a contract signing tx" << std::endl;
		return false;
	}
	if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) != 0)
	{
		latesttxid = agreementtxid; // no updates, return agreementtxid
		funcid = 'c';
		return true;
	}
	else if (!(myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 &&
	((funcid = DecodeAgreementOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey)) == 'u' || funcid == 's' || funcid == 'd' || funcid == 'n'))/* ||
	batontx.vout[1].nValue != CC_MARKER_VALUE*/)
	{
		std::cerr << "GetLatestAgreementUpdate: found first update, but it has incorrect funcid" << std::endl;
		return false;
	}
	sourcetxid = batontxid;
	// baton vout should be vout0 from now on
	while ((retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 0)) == 0 && myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 &&
	DecodeAgreementOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey) != 0)
	{
		funcid = DecodeAgreementOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey);
		switch (funcid)
		{
			case 'u':
			case 'd':
				sourcetxid = batontxid;
				continue;
			case 'n':
			case 's':
			case 'r':
				sourcetxid = batontxid;
				break;
			default:
				std::cerr << "GetLatestAgreementUpdate: found an update, but it has incorrect funcid " << funcid << std::endl;
				return false;
		}
		break;
	}
	latesttxid = sourcetxid;
	return true;
}
// gets the data from the accepted proposal for the specified update txid
// this is for "updateable" data like name, arbitrator fee etc.
void GetAgreementUpdateData(uint256 updatetxid, std::string &name, uint256 &datahash, int64_t &arbitratorfee, int64_t &depositsplit, int64_t &revision)
{
	CScript proposalopret;
	CTransaction updatetx, agreementtx, batontx;
	std::vector<uint8_t> dummypk;
	uint256 proposaltxid, agreementtxid, sourcetxid, batontxid, dummytxid, hashBlock;
	uint8_t version, funcid, dummychar;
	int64_t dummyamount;
	int32_t vini, height, retcode;
	while (myGetTransaction(updatetxid, updatetx, hashBlock) && updatetx.vout.size() > 0 &&
	(funcid = DecodeAgreementOpRet(updatetx.vout[updatetx.vout.size() - 1].scriptPubKey)) != 0)
	{
		switch (funcid)
		{
			case 'u':
			case 's':
				GetAcceptedProposalOpRet(updatetx, proposaltxid, proposalopret);
				DecodeAgreementProposalOpRet(proposalopret,version,dummychar,dummypk,dummypk,dummypk,dummyamount,arbitratorfee,depositsplit,datahash,agreementtxid,dummytxid,name);
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
	if (myGetTransaction(agreementtxid, agreementtx, hashBlock) && agreementtx.vout.size() > 0 &&
	(funcid = DecodeAgreementOpRet(agreementtx.vout[agreementtx.vout.size() - 1].scriptPubKey)) == 'c' &&
	(retcode = CCgetspenttxid(sourcetxid, vini, height, agreementtxid, 1)) == 0 &&
	myGetTransaction(sourcetxid, batontx, hashBlock) && batontx.vout.size() > 0)
	{
		revision++;
		while (sourcetxid != updatetxid && (retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 0)) == 0 &&
		myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 &&
		DecodeAgreementOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey) != 0)
		{
			revision++;
			sourcetxid = batontxid;
		}
	}
	return;
}
//===========================================================================
// RPCs - tx creation
//===========================================================================
// agreementcreate - constructs a 'p' transaction, with the 'p' proposal type
UniValue AgreementCreate(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> destpub, std::vector<uint8_t> arbitrator, int64_t payment, int64_t arbitratorfee, int64_t deposit, uint256 prevproposaltxid, uint256 refagreementtxid)
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
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	CPK_dest = pubkey2pk(destpub);
	CPK_arbitrator = pubkey2pk(arbitrator);
	bHasReceiver = CPK_dest.IsFullyValid();
	bHasArbitrator = CPK_arbitrator.IsFullyValid();
	// check if destpub & arbitrator pubkeys exist and are valid
	/*if (!destpub.empty() && !bHasReceiver)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Receiver pubkey invalid");
	if (!arbitrator.empty() && !bHasArbitrator)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Arbitrator pubkey invalid");*/
	// if arbitrator exists, check if arbitrator fee & deposit are sufficient
	if (bHasArbitrator)
	{
		if (arbitratorfee == 0)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Arbitrator fee must be specified if valid arbitrator exists");
		else if (arbitratorfee < CC_MARKER_VALUE)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Arbitrator fee is too low");
	}
	else
	{
		arbitratorfee = 0;
	}
	if (deposit < CC_MARKER_VALUE)
		deposit = CC_MARKER_VALUE;
	// additional checks are done using ValidateProposalOpRet
	CScript opret = EncodeAgreementProposalOpRet(AGREEMENTCC_VERSION,'p',std::vector<uint8_t>(mypk.begin(),mypk.end()),destpub,arbitrator,payment,arbitratorfee,deposit,datahash,refagreementtxid,prevproposaltxid,name);
	if (!ValidateProposalOpRet(opret, CCerror))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << CCerror);
	// check prevproposaltxid if specified
	if (prevproposaltxid != zeroid)
	{
		if (myGetTransaction(prevproposaltxid,prevproposaltx,hashBlock)==0 || (numvouts=prevproposaltx.vout.size())<=0)
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cant find specified previous proposal txid " << prevproposaltxid.GetHex());
		DecodeAgreementProposalOpRet(prevproposaltx.vout[numvouts-1].scriptPubKey,dummychar,dummychar,ref_srcpub,ref_destpub,dummypk,dummyamount,dummyamount,dummyamount,dummytxid,dummytxid,dummytxid,dummystr);
		if (IsProposalSpent(prevproposaltxid, spendingtxid, spendingfuncid))
		{
			switch (spendingfuncid)
			{
				case 'p':
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been amended by txid " << spendingtxid.GetHex());
				case 'c':
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been accepted by txid " << spendingtxid.GetHex());
				case 't':
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been closed by txid " << spendingtxid.GetHex());
				default:
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been spent by txid " << spendingtxid.GetHex());
			}
		}
		if (!CompareProposals(opret, prevproposaltxid, CCerror))
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << CCerror << " txid: " << prevproposaltxid.GetHex());
	}
	if (AddNormalinputs2(mtx, txfee + CC_MARKER_VALUE * 2, 8) > 0)
	{
		if (prevproposaltxid != zeroid)
		{
			mtx.vin.push_back(CTxIn(prevproposaltxid,0,CScript())); // vin.n-2 previous proposal marker (optional, will trigger validation)
			GetCCaddress1of2(cp, mutualaddr, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub));
			mtx.vin.push_back(CTxIn(prevproposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (optional, will trigger validation)
			Myprivkey(mypriv);
			CCaddr1of2set(cp, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub), mypriv, mutualaddr);
		}
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
		if (bHasReceiver)
			mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, mypk, CPK_dest)); // vout.1 response hook (with destpub)
		else
			mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, mypk)); // vout.1 response hook (no destpub)
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}
	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "error adding normal inputs");
}
// agreementupdate - constructs a 'p' transaction, with the 'u' proposal type
// This transaction will only be validated if agreementtxid is specified. prevproposaltxid can be used to amend previous update requests.
UniValue AgreementUpdate(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, std::string name, uint256 datahash, int64_t payment, uint256 prevproposaltxid, int64_t newarbitratorfee)
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
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (!GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, arbitratorfee, dummyamount, dummytxid, dummytxid, dummystr))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "couldn't get specified agreement name successfully, probably invalid agreement txid");
	GetLatestAgreementUpdate(agreementtxid, latesttxid, dummychar);
	GetAgreementUpdateData(latesttxid, dummystr, dummytxid, arbitratorfee, dummyamount, dummyamount);
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
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "you are not a valid member of this agreement");
	CPK_dest = pubkey2pk(destpub);
	// additional checks are done using ValidateProposalOpRet
	CScript opret = EncodeAgreementProposalOpRet(AGREEMENTCC_VERSION,'u',std::vector<uint8_t>(mypk.begin(),mypk.end()),destpub,arbitratorpk,payment,newarbitratorfee,0,datahash,agreementtxid,prevproposaltxid,name);
	if (!ValidateProposalOpRet(opret, CCerror))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << CCerror);
	// check prevproposaltxid if specified
	if (prevproposaltxid != zeroid)
	{
		if (myGetTransaction(prevproposaltxid,prevproposaltx,hashBlock)==0 || (numvouts=prevproposaltx.vout.size())<=0)
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cant find specified previous proposal txid " << prevproposaltxid.GetHex());
		DecodeAgreementProposalOpRet(prevproposaltx.vout[numvouts-1].scriptPubKey,dummychar,dummychar,ref_srcpub,ref_destpub,dummypk,dummyamount,dummyamount,dummyamount,dummytxid,dummytxid,dummytxid,dummystr);
		if (IsProposalSpent(prevproposaltxid, spendingtxid, spendingfuncid))
		{
			switch (spendingfuncid)
			{
				case 'p':
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been amended by txid " << spendingtxid.GetHex());
				case 'u':
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been accepted by txid " << spendingtxid.GetHex());
				case 't':
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been closed by txid " << spendingtxid.GetHex());
				default:
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been spent by txid " << spendingtxid.GetHex());
			}
		}
		if (!CompareProposals(opret, prevproposaltxid, CCerror))
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << CCerror << " txid: " << prevproposaltxid.GetHex());
	}
	if (AddNormalinputs2(mtx, txfee + CC_MARKER_VALUE * 2, 8) > 0)
	{
		if (prevproposaltxid != zeroid)
		{
			mtx.vin.push_back(CTxIn(prevproposaltxid,0,CScript())); // vin.n-2 previous proposal marker (optional, will trigger validation)
			GetCCaddress1of2(cp, mutualaddr, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub));
			mtx.vin.push_back(CTxIn(prevproposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (optional, will trigger validation)
			Myprivkey(mypriv);
			CCaddr1of2set(cp, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub), mypriv, mutualaddr);
		}
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
		mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, mypk, pubkey2pk(destpub))); // vout.1 response hook
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}
	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "error adding normal inputs");
}
// agreementclose - constructs a 'p' transaction, with the 't' proposal type
// This transaction will only be validated if agreementtxid is specified. prevproposaltxid can be used to amend previous cancel requests.
UniValue AgreementClose(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, std::string name, uint256 datahash, int64_t depositcut, int64_t payment, uint256 prevproposaltxid)
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
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (!GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, dummyamount, deposit, dummytxid, dummytxid, dummystr))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "couldn't get specified agreement name successfully, probably invalid agreement txid");
	// setting destination pubkey
	if (mypk == pubkey2pk(sellerpk))
		destpub = clientpk;
	else if (mypk == pubkey2pk(clientpk))
		destpub = sellerpk;
	else
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "you are not a valid member of this agreement");
	CPK_dest = pubkey2pk(destpub);
	// checking deposit cut to prevent vouts with dust value
	if (depositcut != 0 && depositcut < CC_MARKER_VALUE)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Deposit cut is too low");
	else if (depositcut > deposit)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Deposit cut exceeds total deposit value");
	else if ((deposit - depositcut) != 0 && (deposit - depositcut) < CC_MARKER_VALUE)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Remainder of deposit is too low");
	// additional checks are done using ValidateProposalOpRet
	CScript opret = EncodeAgreementProposalOpRet(AGREEMENTCC_VERSION,'t',std::vector<uint8_t>(mypk.begin(),mypk.end()),destpub,arbitratorpk,payment,CC_MARKER_VALUE,depositcut,datahash,agreementtxid,prevproposaltxid,name);
	if (!ValidateProposalOpRet(opret, CCerror))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << CCerror);
	// check prevproposaltxid if specified
	if (prevproposaltxid != zeroid)
	{
		if (myGetTransaction(prevproposaltxid,prevproposaltx,hashBlock)==0 || (numvouts=prevproposaltx.vout.size())<=0)
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cant find specified previous proposal txid " << prevproposaltxid.GetHex());
		DecodeAgreementProposalOpRet(prevproposaltx.vout[numvouts-1].scriptPubKey,dummychar,dummychar,ref_srcpub,ref_destpub,dummypk,dummyamount,dummyamount,dummyamount,dummytxid,dummytxid,dummytxid,dummystr);
		if (IsProposalSpent(prevproposaltxid, spendingtxid, spendingfuncid))
		{
			switch (spendingfuncid)
			{
				case 'p':
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been amended by txid " << spendingtxid.GetHex());
				case 'u':
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been accepted by txid " << spendingtxid.GetHex());
				case 't':
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been closed by txid " << spendingtxid.GetHex());
				default:
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been spent by txid " << spendingtxid.GetHex());
			}
		}
		if (!CompareProposals(opret, prevproposaltxid, CCerror))
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << CCerror << " txid: " << prevproposaltxid.GetHex());
	}
	if (AddNormalinputs2(mtx, txfee + CC_MARKER_VALUE * 2, 8) > 0)
	{
		if (prevproposaltxid != zeroid)
		{
			mtx.vin.push_back(CTxIn(prevproposaltxid,0,CScript())); // vin.n-2 previous proposal marker (optional, will trigger validation)
			GetCCaddress1of2(cp, mutualaddr, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub));
			mtx.vin.push_back(CTxIn(prevproposaltxid,1,CScript())); // vin.n-1 previous proposal response hook (optional, will trigger validation)
			Myprivkey(mypriv);
			CCaddr1of2set(cp, pubkey2pk(ref_srcpub), pubkey2pk(ref_destpub), mypriv, mutualaddr);
		}
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
		mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, mypk, pubkey2pk(destpub))); // vout.1 response hook
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}
	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "error adding normal inputs");
}
// agreementstopproposal - constructs a 't' transaction and spends the specified 'p' transaction
// can be done by the proposal initiator, as well as the receiver if they are able to accept the proposal.
UniValue AgreementStopProposal(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid)
{
	CPubKey mypk, CPK_src, CPK_dest;
	CTransaction proposaltx;
	uint256 hashBlock, refagreementtxid, spendingtxid, dummytxid;
	std::vector<uint8_t> srcpub, destpub, sellerpk, clientpk, dummypk;
	int32_t numvouts;
	int64_t dummyamount;
	std::string dummystr;
	bool bHasReceiver, bHasArbitrator;
	uint8_t proposaltype, version, spendingfuncid, mypriv[32];
	char mutualaddr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (myGetTransaction(proposaltxid, proposaltx, hashBlock) == 0 || (numvouts = proposaltx.vout.size()) <= 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "cant find specified proposal txid " << proposaltxid.GetHex());
	if (DecodeAgreementProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,version,proposaltype,srcpub,destpub,dummypk,dummyamount,dummyamount,dummyamount,dummytxid,refagreementtxid,dummytxid,dummystr) != 'p')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified txid has incorrect proposal opret");
	if (IsProposalSpent(proposaltxid, spendingtxid, spendingfuncid))
	{
		switch (spendingfuncid)
		{
			case 'p':
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been amended by txid " << spendingtxid.GetHex());
			case 'c':
			case 'u':
			case 's':
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been accepted by txid " << spendingtxid.GetHex());
			case 't':
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been closed by txid " << spendingtxid.GetHex());
			default:
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been spent by txid " << spendingtxid.GetHex());
		}
	}
	CPK_src = pubkey2pk(srcpub);
	CPK_dest = pubkey2pk(destpub);
	bHasReceiver = CPK_dest.IsFullyValid();
	switch (proposaltype)
	{
		case 'p':
			if (bHasReceiver && mypk != CPK_src && mypk != CPK_dest)
				CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "you are not the source or receiver of specified proposal");
			else if (!bHasReceiver && mypk != CPK_src)
				CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "you are not the source of specified proposal");
			break;
		case 'u':
		case 't':
			if (refagreementtxid == zeroid)
				CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "proposal has no defined agreement, unable to verify membership");
			if (!GetAgreementInitialData(refagreementtxid, dummytxid, sellerpk, clientpk, dummypk, dummyamount, dummyamount, dummytxid, dummytxid, dummystr))
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "couldn't get proposal's agreement name successfully");
			if (mypk != CPK_src && mypk != CPK_dest && mypk != pubkey2pk(sellerpk) && mypk != pubkey2pk(clientpk))
				CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "you are not the source or receiver of specified proposal");
			break;
		default:
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "invalid proposaltype in proposal tx opret");
	}
	if (AddNormalinputs2(mtx, txfee, 5) > 0)
	{
		if (bHasReceiver)
		{
			GetCCaddress1of2(cp, mutualaddr, CPK_src, CPK_dest);
			mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.1 previous proposal CC response hook (1of2 addr version)
			Myprivkey(mypriv);
			CCaddr1of2set(cp, CPK_src, CPK_dest, mypriv, mutualaddr);
		}
		else
			mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.1 previous proposal CC response hook (1of1 addr version)
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementProposalCloseOpRet(AGREEMENTCC_VERSION,proposaltxid,std::vector<uint8_t>(mypk.begin(),mypk.end())));
	}
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");
}
// agreementaccept - spend a 'p' transaction that was submitted by the other party.
// this function is context aware and does different things dependent on the proposal type:
// if txid opret has the 'p' proposal type, will create a 'c' transaction (create contract)
// if txid opret has the 'u' proposal type, will create a 'u' transaction (update contract)
// if txid opret has the 't' proposal type, will create a 's' transaction (close contract)
UniValue AgreementAccept(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid)
{
	CPubKey mypk, CPK_src, CPK_dest, CPK_arbitrator;
	CTransaction proposaltx;
	uint256 hashBlock, datahash, agreementtxid, prevproposaltxid, spendingtxid, latesttxid;
	std::vector<uint8_t> srcpub, destpub, arbitrator;
	int32_t numvouts;
	int64_t payment, arbitratorfee, deposit;
	std::string name, CCerror;
	bool bHasReceiver, bHasArbitrator;
	uint8_t proposaltype, version, spendingfuncid, updatefuncid, mypriv[32];
	char mutualaddr[65], depositaddr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (myGetTransaction(proposaltxid, proposaltx, hashBlock) == 0 || (numvouts = proposaltx.vout.size()) <= 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "cant find specified proposal txid " << proposaltxid.GetHex());
	if (!ValidateProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey, CCerror))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << CCerror);
	if (DecodeAgreementProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,version,proposaltype,srcpub,destpub,arbitrator,payment,arbitratorfee,deposit,datahash,agreementtxid,prevproposaltxid,name) != 'p')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified txid has incorrect proposal opret");
	if (IsProposalSpent(proposaltxid, spendingtxid, spendingfuncid))
	{
		switch (spendingfuncid)
		{
			case 'p':
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been amended by txid " << spendingtxid.GetHex());
			case 'c':
			case 'u':
			case 's':
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been accepted by txid " << spendingtxid.GetHex());
			case 't':
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been closed by txid " << spendingtxid.GetHex());
			default:
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "specified proposal has been spent by txid " << spendingtxid.GetHex());
		}
	}
	CPK_src = pubkey2pk(srcpub);
	CPK_dest = pubkey2pk(destpub);
	CPK_arbitrator = pubkey2pk(arbitrator);
	bHasReceiver = CPK_dest.IsFullyValid();
	bHasArbitrator = CPK_arbitrator.IsFullyValid();
	if (!bHasReceiver)
		CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "specified proposal has no receiver, can't accept");
	else if (mypk != CPK_dest)
		CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "you are not the receiver of specified proposal");
	switch (proposaltype)
	{
		case 'p':
			// constructing a 'c' transaction
			if (AddNormalinputs2(mtx, txfee + payment + deposit, 64) > 0)
			{
				mtx.vin.push_back(CTxIn(proposaltxid,0,CScript())); // vin.1 previous proposal CC marker
				GetCCaddress1of2(cp, mutualaddr, CPK_src, CPK_dest);
				mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.2 previous proposal CC response hook (must have designated receiver!)
				Myprivkey(mypriv);
				CCaddr1of2set(cp, CPK_src, CPK_dest, mypriv, mutualaddr);
				mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
				mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, CPK_src, mypk)); // vout.1 baton / update log
				mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, deposit, GetUnspendable(cp, NULL))); // vout.2 deposit / contract completion marker
				if (payment > 0)
					mtx.vout.push_back(CTxOut(payment, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG)); // vout.3 payment (optional)
				return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementSigningOpRet(AGREEMENTCC_VERSION, proposaltxid));
			}
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "error adding normal inputs");
		case 'u':
			GetLatestAgreementUpdate(agreementtxid, latesttxid, updatefuncid);
			// constructing a 'u' transaction
			if (AddNormalinputs2(mtx, txfee + payment, 64) > 0)
			{
				GetCCaddress1of2(cp, mutualaddr, CPK_src, CPK_dest);
				if (latesttxid == agreementtxid)
					mtx.vin.push_back(CTxIn(agreementtxid,1,CScript())); // vin.1 last update baton (no previous updates)
				else
					mtx.vin.push_back(CTxIn(latesttxid,0,CScript())); // vin.1 last update baton (with previous updates)
				mtx.vin.push_back(CTxIn(proposaltxid,0,CScript())); // vin.2 previous proposal CC marker
				mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.3 previous proposal CC response hook
				Myprivkey(mypriv);
				CCaddr1of2set(cp, CPK_src, CPK_dest, mypriv, mutualaddr);
				mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, CPK_src, mypk)); // vout.0 next update baton
				if (payment > 0)
					mtx.vout.push_back(CTxOut(payment, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG)); // vout.1 payment (optional)
				return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementUpdateOpRet(AGREEMENTCC_VERSION, agreementtxid, proposaltxid));
			}
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");
		case 't':
			GetLatestAgreementUpdate(agreementtxid, latesttxid, updatefuncid);
			// constructing a 's' transaction
			if (AddNormalinputs2(mtx, txfee + payment, 64) > 0)
			{
				GetCCaddress1of2(cp, mutualaddr, CPK_src, CPK_dest);
				if (latesttxid == agreementtxid)
					mtx.vin.push_back(CTxIn(agreementtxid,1,CScript())); // vin.1 last update baton (no previous updates)
				else
					mtx.vin.push_back(CTxIn(latesttxid,0,CScript())); // vin.1 last update baton (with previous updates)
				mtx.vin.push_back(CTxIn(proposaltxid,0,CScript())); // vin.2 previous proposal CC marker
				mtx.vin.push_back(CTxIn(proposaltxid,1,CScript())); // vin.3 previous proposal CC response hook
				Myprivkey(mypriv);
				CCaddr1of2set(cp, CPK_src, CPK_dest, mypriv, mutualaddr);
				mtx.vin.push_back(CTxIn(agreementtxid,2,CScript())); // vin.4 deposit
				mtx.vout.push_back(CTxOut(deposit, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG)); // vout.0 deposit cut to proposal creator
				if (payment > 0)
					mtx.vout.push_back(CTxOut(payment, CScript() << ParseHex(HexStr(CPK_src)) << OP_CHECKSIG)); // vout.1 payment (optional)
				return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementCloseOpRet(AGREEMENTCC_VERSION, agreementtxid, proposaltxid));
			}
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");
		default:
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "invalid proposal type for proposal txid " << proposaltxid.GetHex());
	}
}
// agreementdispute - constructs a 'd' transaction and spends the latest update baton of agreementtxid
// only allowed/validated if agreement has an arbitrator
UniValue AgreementDispute(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash)
{
	CPubKey mypk, CPK_seller, CPK_client, CPK_arbitrator;
	uint256 latesttxid, dummytxid;
	std::vector<uint8_t> destpub, sellerpk, clientpk, arbitratorpk, ref_srcpub, ref_destpub, dummypk;
	int64_t arbitratorfee, dummyamount;
	std::string dummystr;
	uint8_t version, updatefuncid, mypriv[32];
	char mutualaddr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (datahash == zeroid)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Data hash must not be empty");
	if (!GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, arbitratorfee, dummyamount, dummytxid, dummytxid, dummystr))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "couldn't get specified agreement name successfully, probably invalid agreement txid");
	CPK_seller = pubkey2pk(sellerpk);
	CPK_client = pubkey2pk(clientpk);
	CPK_arbitrator = pubkey2pk(arbitratorpk);
	// check sender pubkey
	if (mypk != CPK_seller && mypk != CPK_client)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "you are not a valid member of this agreement");
	// check if arbitrator exists
	if (!(CPK_arbitrator.IsFullyValid()))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "agreement has no arbitrator, disputes are disabled");
	// check if agreement is active
	GetLatestAgreementUpdate(agreementtxid, latesttxid, updatefuncid);
	if (updatefuncid != 'c' && updatefuncid != 'u')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "agreement is no longer active or is already suspended");
	GetAgreementUpdateData(latesttxid, dummystr, dummytxid, arbitratorfee, dummyamount, dummyamount);
	if (AddNormalinputs2(mtx, txfee + arbitratorfee, 64) > 0)
	{
		GetCCaddress1of2(cp, mutualaddr, CPK_seller, CPK_client);
		if (latesttxid == agreementtxid)
			mtx.vin.push_back(CTxIn(agreementtxid,1,CScript())); // vin.1 last update baton (no previous updates)
		else
			mtx.vin.push_back(CTxIn(latesttxid,0,CScript())); // vin.1 last update baton (with previous updates)
		Myprivkey(mypriv);
		CCaddr1of2set(cp, CPK_seller, CPK_client, mypriv, mutualaddr);
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, arbitratorfee, CPK_arbitrator)); // vout.0 arbitrator fee
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementDisputeOpRet(AGREEMENTCC_VERSION,agreementtxid,std::vector<uint8_t>(mypk.begin(),mypk.end()),datahash));
	}
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");
}
// agreementresolve - constructs a 'r' transaction and spends the latest update baton of agreementtxid
// only available to arbitrators
UniValue AgreementResolve(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, std::vector<uint8_t> rewardedpubkey)
{
	CPubKey mypk, CPK_seller, CPK_client, CPK_rewarded, CPK_arbitrator;
	uint256 latesttxid, dummytxid;
	std::vector<uint8_t> destpub, sellerpk, clientpk, arbitratorpk, ref_srcpub, ref_destpub, dummypk;
	int64_t deposit, dummyamount;
	std::string dummystr;
	uint8_t version, updatefuncid, mypriv[32];
	char mutualaddr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (!GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, dummyamount, deposit, dummytxid, dummytxid, dummystr))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "couldn't get specified agreement name successfully, probably invalid agreement txid");
	CPK_seller = pubkey2pk(sellerpk);
	CPK_client = pubkey2pk(clientpk);
	CPK_arbitrator = pubkey2pk(arbitratorpk);
	CPK_rewarded = pubkey2pk(rewardedpubkey);
	// check if arbitrator exists
	if (!(CPK_arbitrator.IsFullyValid()))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "agreement has no arbitrator, disputes are disabled");
	// check sender pubkey
	if (mypk != CPK_arbitrator)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "you are not the arbitrator of this agreement");
	// check rewarded pubkey
	if (!(CPK_rewarded.IsFullyValid()))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Invalid rewarded pubkey");
	else if (CPK_rewarded != CPK_seller && CPK_rewarded != CPK_client)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "rewarded pubkey is not a valid member of this agreement");
	// check if agreement is suspended
	GetLatestAgreementUpdate(agreementtxid, latesttxid, updatefuncid);
	if (updatefuncid != 'd')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "agreement is not in dispute");
	if (AddNormalinputs2(mtx, txfee, 5) > 0)
	{
		if (latesttxid == agreementtxid)
			mtx.vin.push_back(CTxIn(agreementtxid,1,CScript())); // vin.1 last update baton (no previous updates)
		else
			mtx.vin.push_back(CTxIn(latesttxid,0,CScript())); // vin.1 last update baton (with previous updates)
		mtx.vin.push_back(CTxIn(agreementtxid,2,CScript())); // vin.2 deposit
		mtx.vout.push_back(CTxOut(deposit, CScript() << ParseHex(HexStr(CPK_rewarded)) << OP_CHECKSIG)); // vout.0 deposit payout
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementDisputeResolveOpRet(AGREEMENTCC_VERSION,agreementtxid,rewardedpubkey));
	}
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");
}
// agreementunlock - constructs a 'n' transaction and spends the latest update baton of agreementtxid
// sends amount required to fill numcoins amount to 1of2 CC pawnshopaddr from deposit, and refunds the rest to client
UniValue AgreementUnlock(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 pawnshoptxid)
{
	CPubKey mypk, CPK_seller, CPK_client, tokensupplier, coinsupplier;
	uint256 hashBlock,borrowtxid, updatetxid, latesttxid, dummytxid, refagreementtxid;
	std::vector<uint8_t> sellerpk, clientpk, arbitratorpk;
	int64_t arbitratorfee, deposit, numtokens, numcoins, tokenbalance, coinbalance, refund;
	int32_t numvouts;
	std::string dummystr, CCerror = "", pawnshopname;
	uint8_t version, updatefuncid, mypriv[32];
	uint32_t pawnshopflags;
	char mutualaddr[65];
	CTransaction pawnshoptx;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp, C, *cpPawnshop, CPawnshop;
	cp = CCinit(&C, EVAL_AGREEMENTS);
	cpPawnshop = CCinit(&CPawnshop, EVAL_PAWNSHOP);
	if (txfee == 0) txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (!GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, arbitratorfee, deposit, dummytxid, dummytxid, dummystr))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "couldn't get specified agreement name successfully, probably invalid agreement txid");
	CPK_seller = pubkey2pk(sellerpk);
	CPK_client = pubkey2pk(clientpk);
	// check sender pubkey
	if (mypk != CPK_seller && mypk != CPK_client)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "you are not a valid member of this agreement");
	// check if agreement is active
	GetLatestAgreementUpdate(agreementtxid, updatetxid, updatefuncid);
	if (updatefuncid == 'n')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "deposit is already unlocked for this agreement");
	else if (updatefuncid != 'c' && updatefuncid != 'u')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "agreement is no longer active");
	if (pawnshoptxid != zeroid)
	{
		if (myGetTransaction(pawnshoptxid, pawnshoptx, hashBlock) == 0 || (numvouts = pawnshoptx.vout.size()) <= 0)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "cant find specified pawnshop txid " << pawnshoptxid.GetHex());
		if (DecodePawnshopCreateOpRet(pawnshoptx.vout[numvouts - 1].scriptPubKey,version,pawnshopname,tokensupplier,coinsupplier,pawnshopflags,dummytxid,numtokens,numcoins,refagreementtxid) == 0)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "invalid pawnshop create opret " << pawnshoptxid.GetHex());
		if (mypk != coinsupplier)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "you are not the coin supplier of this pawnshop");
		if (refagreementtxid != agreementtxid)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "agreement txid in pawnshop is different from agreement txid specified");
		if (!(pawnshopflags & PTF_REQUIREUNLOCK))
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "deposit unlock is disabled for this pawnshop");
		if (!ValidatePawnshopCreateTx(pawnshoptx,CCerror))
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << CCerror);
		if (!GetLatestPawnshopTxid(pawnshoptxid, latesttxid, updatefuncid) || updatefuncid == 'e' || updatefuncid == 'x')
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "pawnshop " << pawnshoptxid.GetHex() << " closed");
		coinbalance = GetPawnshopInputs(cpPawnshop,pawnshoptx,PIF_COINS,unspentOutputs);
		tokenbalance = GetPawnshopInputs(cpPawnshop,pawnshoptx,PIF_TOKENS,unspentOutputs);
		if (tokenbalance < numtokens)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "pawnshop must have all required tokens for deposit unlock");
		if (coinbalance + deposit < numcoins)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "pawnshop must have enough coins + deposit to match required amount for unlock");
		else
			refund = coinbalance + deposit - numcoins;
	}
	else
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Invalid pawnshoptxid");
	if (AddNormalinputs2(mtx, txfee, 5) > 0)
	{
		GetCCaddress1of2(cp, mutualaddr, CPK_seller, CPK_client);
		if (updatetxid == agreementtxid)
			mtx.vin.push_back(CTxIn(agreementtxid,1,CScript())); // vin.1 last update baton (no previous updates)
		else
			mtx.vin.push_back(CTxIn(updatetxid,0,CScript())); // vin.1 last update baton (with previous updates)
		Myprivkey(mypriv);
		CCaddr1of2set(cp, CPK_seller, CPK_client, mypriv, mutualaddr);
		mtx.vin.push_back(CTxIn(agreementtxid,2,CScript())); // vin.2 deposit
		if (coinbalance < numcoins)
		{
			mtx.vout.push_back(MakeCC1of2vout(EVAL_PAWNSHOP, deposit - refund, tokensupplier, coinsupplier)); // vout.0 payout to pawnshop CC 1of2 address
		}
		if (refund > 0)
		{
			mtx.vout.push_back(CTxOut(refund, CScript() << ParseHex(HexStr(CPK_client)) << OP_CHECKSIG)); // vout.1 deposit refund to client (optional)
		}
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementUnlockOpRet(AGREEMENTCC_VERSION, agreementtxid, pawnshoptxid));
	}
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");
}
//===========================================================================
// RPCs - informational
//===========================================================================
UniValue AgreementInfo(uint256 txid)
{
	UniValue result(UniValue::VOBJ), members(UniValue::VOBJ), data(UniValue::VOBJ);
	CPubKey CPK_src, CPK_dest, CPK_arbitrator;
	CTransaction tx, proposaltx;
	uint256 hashBlock, datahash, proposaltxid, prevproposaltxid, agreementtxid, latesttxid, spendingtxid, dummytxid, pawnshoptxid;
	std::vector<uint8_t> srcpub, destpub, arbitrator, rewardedpubkey;
	int32_t numvouts;
	int64_t payment, arbitratorfee, deposit, totaldeposit, revision;
	std::string name, CCerror;
	bool bHasReceiver, bHasArbitrator;
	uint8_t funcid, version, proposaltype, updatefuncid, spendingfuncid, mypriv[32];
	char mutualaddr[65];
	if (myGetTransaction(txid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
	(funcid = DecodeAgreementOpRet(tx.vout[numvouts-1].scriptPubKey)) != 0)
	{
		result.push_back(Pair("result", "success"));
		result.push_back(Pair("txid", txid.GetHex()));
		switch (funcid)
		{
			case 'p':
				result.push_back(Pair("type","proposal"));
				DecodeAgreementProposalOpRet(tx.vout[numvouts-1].scriptPubKey,version,proposaltype,srcpub,destpub,arbitrator,payment,arbitratorfee,deposit,datahash,agreementtxid,prevproposaltxid,name);
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
				data.push_back(Pair("name",name));
				data.push_back(Pair("data_hash",datahash.GetHex()));
				switch (proposaltype)
				{
					case 'p':
						result.push_back(Pair("proposal_type","contract_create"));
						if (bHasArbitrator)
						{
							members.push_back(Pair("arbitrator",HexStr(arbitrator)));
							data.push_back(Pair("arbitrator_fee",arbitratorfee));
						}
						data.push_back(Pair("deposit",deposit));
						if (agreementtxid != zeroid)
							data.push_back(Pair("master_contract_txid",agreementtxid.GetHex()));
						break;
					case 'u':
						result.push_back(Pair("proposal_type","contract_update"));
						result.push_back(Pair("contract_txid",agreementtxid.GetHex()));
						if (bHasArbitrator)
						{
							data.push_back(Pair("new_arbitrator_fee", arbitratorfee));
							GetAgreementInitialData(agreementtxid, proposaltxid, srcpub, destpub, arbitrator, arbitratorfee, deposit, datahash, dummytxid, name);
							GetLatestAgreementUpdate(agreementtxid, latesttxid, updatefuncid);
							GetAgreementUpdateData(latesttxid, name, datahash, arbitratorfee, deposit, revision);
							data.push_back(Pair("current_arbitrator_fee", arbitratorfee));
						}
						break;
					case 't':
						result.push_back(Pair("proposal_type","contract_close"));
						result.push_back(Pair("contract_txid",agreementtxid.GetHex()));
						GetAgreementInitialData(agreementtxid, proposaltxid, srcpub, destpub, arbitrator, arbitratorfee, totaldeposit, datahash, dummytxid, name);
						data.push_back(Pair("deposit_for_sender", deposit));
						data.push_back(Pair("deposit_for_receiver", totaldeposit-deposit));
						data.push_back(Pair("total_deposit", totaldeposit));
						break;
				}
				result.push_back(Pair("members",members));
				if (IsProposalSpent(txid, spendingtxid, spendingfuncid))
				{
					switch (spendingfuncid)
					{
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
				else
				{
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
				DecodeAgreementProposalCloseOpRet(tx.vout[numvouts-1].scriptPubKey, version, proposaltxid, srcpub);
				result.push_back(Pair("source_pubkey",HexStr(srcpub)));
				result.push_back(Pair("proposal_txid",proposaltxid.GetHex()));
				break;
			case 'c':
				result.push_back(Pair("type","contract"));
				DecodeAgreementSigningOpRet(tx.vout[numvouts-1].scriptPubKey, version, proposaltxid);
				GetAgreementInitialData(txid, proposaltxid, srcpub, destpub, arbitrator, arbitratorfee, deposit, datahash, agreementtxid, name);
				CPK_arbitrator = pubkey2pk(arbitrator);
				bHasArbitrator = CPK_arbitrator.IsFullyValid();
				result.push_back(Pair("accepted_txid",proposaltxid.GetHex()));
				members.push_back(Pair("seller",HexStr(srcpub)));
				members.push_back(Pair("client",HexStr(destpub)));
				result.push_back(Pair("deposit",deposit));
				if (bHasArbitrator)
				{	
					members.push_back(Pair("arbitrator",HexStr(arbitrator)));
				}
				result.push_back(Pair("members",members));
				if (agreementtxid != zeroid)
					data.push_back(Pair("master_contract_txid",agreementtxid.GetHex()));
				GetLatestAgreementUpdate(txid, latesttxid, updatefuncid);
				if (latesttxid != txid)
				{
					switch (updatefuncid)
					{
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
							result.push_back(Pair("status","in settlement"));
							break;
					}
					result.push_back(Pair("last_txid",latesttxid.GetHex()));
				}
				else
					result.push_back(Pair("status","active"));
				GetAgreementUpdateData(latesttxid, name, datahash, arbitratorfee, deposit, revision);
				data.push_back(Pair("revisions",revision));
				if (bHasArbitrator)
					data.push_back(Pair("arbitrator_fee",arbitratorfee));
				data.push_back(Pair("latest_name",name));
				data.push_back(Pair("latest_data_hash",datahash.GetHex()));
				result.push_back(Pair("data",data));
				break;
			case 'u':
				result.push_back(Pair("type","contract update"));
				DecodeAgreementUpdateOpRet(tx.vout[numvouts-1].scriptPubKey, version, agreementtxid, proposaltxid);
				result.push_back(Pair("contract_txid",agreementtxid.GetHex()));
				result.push_back(Pair("proposal_txid",proposaltxid.GetHex()));
				GetAgreementInitialData(agreementtxid, proposaltxid, srcpub, destpub, arbitrator, arbitratorfee, totaldeposit, datahash, dummytxid, name);
				CPK_arbitrator = pubkey2pk(arbitrator);
				GetAgreementUpdateData(txid, name, datahash, arbitratorfee, deposit, revision);
				data.push_back(Pair("revision",revision));
				if (CPK_arbitrator.IsFullyValid())
					data.push_back(Pair("arbitrator_fee",arbitratorfee));
				data.push_back(Pair("name",name));
				data.push_back(Pair("data_hash",datahash.GetHex()));
				result.push_back(Pair("data",data));
				break;
			case 's':
				result.push_back(Pair("type","contract close"));
				DecodeAgreementCloseOpRet(tx.vout[numvouts-1].scriptPubKey, version, agreementtxid, proposaltxid);
				result.push_back(Pair("contract_txid",agreementtxid.GetHex()));
				result.push_back(Pair("proposal_txid",proposaltxid.GetHex()));
				GetAgreementInitialData(agreementtxid, proposaltxid, srcpub, destpub, arbitrator, arbitratorfee, totaldeposit, datahash, dummytxid, name);
				CPK_arbitrator = pubkey2pk(arbitrator);
				GetAgreementUpdateData(txid, name, datahash, arbitratorfee, deposit, revision);
				data.push_back(Pair("revision",revision));
				data.push_back(Pair("name",name));
				data.push_back(Pair("data_hash",datahash.GetHex()));
				data.push_back(Pair("deposit_for_sender", deposit));
				data.push_back(Pair("deposit_for_receiver", totaldeposit-deposit));
				data.push_back(Pair("total_deposit", totaldeposit));
				result.push_back(Pair("data",data));
				break;
			case 'd':
				result.push_back(Pair("type","dispute"));
				DecodeAgreementDisputeOpRet(tx.vout[numvouts-1].scriptPubKey, version, agreementtxid, srcpub, datahash);
				result.push_back(Pair("contract_txid",agreementtxid.GetHex()));
				result.push_back(Pair("source_pubkey",HexStr(srcpub)));
				result.push_back(Pair("data_hash",datahash.GetHex()));
				break;
			case 'r':
				result.push_back(Pair("type","dispute resolution"));
				DecodeAgreementDisputeResolveOpRet(tx.vout[numvouts-1].scriptPubKey, version, agreementtxid, rewardedpubkey);
				result.push_back(Pair("contract_txid",agreementtxid.GetHex()));
				result.push_back(Pair("rewarded_pubkey",HexStr(rewardedpubkey)));
				break;
			case 'n':
				result.push_back(Pair("type","agreement unlock"));
				DecodeAgreementUnlockOpRet(tx.vout[numvouts-1].scriptPubKey, version, agreementtxid, pawnshoptxid);
				result.push_back(Pair("contract_txid",agreementtxid.GetHex()));
				result.push_back(Pair("dest_pawnshop_txid",pawnshoptxid.GetHex()));
				GetAgreementInitialData(agreementtxid, proposaltxid, srcpub, destpub, arbitrator, arbitratorfee, totaldeposit, datahash, dummytxid, name);
				deposit = CheckDepositUnlockCond(pawnshoptxid);
				if (deposit > -1)
				{
					result.push_back(Pair("deposit_sent", deposit));
					result.push_back(Pair("deposit_refunded", totaldeposit-deposit));
				}
				result.push_back(Pair("total_deposit", totaldeposit));
				break;
		}
		return(result);
	}
	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "invalid Agreements transaction id");
}
UniValue AgreementUpdateLog(uint256 agreementtxid, int64_t samplenum, bool backwards)
{
    UniValue result(UniValue::VARR);
    int64_t total = 0LL;
	CTransaction agreementtx, latesttx, batontx;
    int32_t numvouts, vini, height, retcode;
    uint256 batontxid, sourcetxid, hashBlock, latesttxid;
    uint8_t funcid;
	if (myGetTransaction(agreementtxid,agreementtx,hashBlock) != 0 && (numvouts = agreementtx.vout.size()) > 0 &&
	(funcid = DecodeAgreementOpRet(agreementtx.vout[numvouts-1].scriptPubKey)) == 'c')
	{
		GetLatestAgreementUpdate(agreementtxid, latesttxid, funcid);
		if (latesttxid != agreementtxid)
		{
			if (backwards)
			{
				total++;
				result.push_back(latesttxid.GetHex());
				myGetTransaction(latesttxid, latesttx, hashBlock);
				batontxid = latesttx.vin[1].prevout.hash;
				while ((total < samplenum || samplenum == 0) && 
				myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 &&
				(funcid = DecodeAgreementOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey)) != 0)
				{
					switch (funcid)
					{
						case 'u':
						case 'd':
							total++;
							result.push_back(batontxid.GetHex());
							batontxid = batontx.vin[1].prevout.hash;
							continue;
						default:
							break;
					}
					break;
				}
			}
			else
			{
				if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) == 0 && 
					myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 &&
					((funcid = DecodeAgreementOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey)) == 'u' || funcid == 's' || funcid == 'd'))
				{
					total++;
					result.push_back(batontxid.GetHex());
					sourcetxid = batontxid;
				}
				while ((total < samplenum || samplenum == 0) &&
					(retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 0)) == 0 && 
					myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0)
				{
					funcid = DecodeAgreementOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey);
					switch (funcid)
					{
						case 'u':
						case 'd':
							total++;
							result.push_back(batontxid.GetHex());
							if (batontxid == latesttxid)
								break;
							else
							{
								sourcetxid = batontxid;
								continue;
							}
						case 'n':
						case 's':
						case 'r':
							result.push_back(batontxid.GetHex());
							break;
						default:
							break;
					}
					break;
				}
			}
		}
		return (result);
	}
	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "invalid Agreements transaction id");
}
// agreementproposals - returns every unspent proposal that pk is referenced in. agreementtxid can be specified to filter out proposals unrelated to this agreement
UniValue AgreementProposals(CPubKey pk, uint256 agreementtxid)
{
	UniValue result(UniValue::VOBJ), senderlist(UniValue::VARR), receiverlist(UniValue::VARR), arbitratorlist(UniValue::VARR);
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressIndexCCMarker;
	std::vector<uint256> foundtxids;
	struct CCcontract_info *cp, C;
	uint256 txid, hashBlock, dummytxid, refagreementtxid;
	std::vector<uint8_t> srcpub, destpub, arbitrator;
	uint8_t version, proposaltype, dummychar;
	int64_t dummyamount;
	std::string dummystr;
	CTransaction vintx;
	cp = CCinit(&C, EVAL_AGREEMENTS);
	CPubKey mypk;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	auto AddProposalWithPk = [&](uint256 txid, uint256 agreementtxid)
	{
		if (myGetTransaction(txid, vintx, hashBlock) != 0 && vintx.vout.size() > 0 && 
		DecodeAgreementProposalOpRet(vintx.vout[vintx.vout.size() - 1].scriptPubKey,version,proposaltype,srcpub,destpub,arbitrator,dummyamount,dummyamount,dummyamount,dummytxid,refagreementtxid,dummytxid,dummystr) == 'p' &&
		!IsProposalSpent(txid, dummytxid, dummychar) &&
		(agreementtxid == zeroid || (proposaltype != 'p' && agreementtxid == refagreementtxid)) &&
		std::find(foundtxids.begin(), foundtxids.end(), txid) == foundtxids.end())
		{
			if (mypk == pubkey2pk(srcpub))
			{
				senderlist.push_back(txid.GetHex());
				foundtxids.push_back(txid);
			}
			if (pubkey2pk(destpub).IsValid() && mypk == pubkey2pk(destpub))
			{
				receiverlist.push_back(txid.GetHex());
				foundtxids.push_back(txid);
			}
			if (pubkey2pk(arbitrator).IsValid() && mypk == pubkey2pk(arbitrator))
			{
				arbitratorlist.push_back(txid.GetHex());
				foundtxids.push_back(txid);
			}
		}
	};
	SetCCunspents(addressIndexCCMarker,cp->unspendableCCaddr,true);
	for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressIndexCCMarker.begin(); it != addressIndexCCMarker.end(); it++)
		AddProposalWithPk(it->first.txhash, agreementtxid);
	result.push_back(Pair("sender",senderlist));
	result.push_back(Pair("receiver",receiverlist));
	result.push_back(Pair("arbitrator",arbitratorlist));
	return (result);
}
// agreementsubcontracts - returns every contract that has agreementtxid defined as its master contract
UniValue AgreementSubcontracts(uint256 agreementtxid)
{
	UniValue result(UniValue::VARR);
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressIndexCCMarker;
	std::vector<uint256> foundtxids;
	struct CCcontract_info *cp, C;
	std::vector<uint8_t> dummypk;
	int64_t dummyamount;
	std::string dummystr;
	uint256 txid, hashBlock, dummytxid, refagreementtxid;
	CTransaction vintx;
	cp = CCinit(&C, EVAL_AGREEMENTS);
	auto AddAgreementWithRef = [&](uint256 txid)
	{
		if (myGetTransaction(txid, vintx, hashBlock) != 0 && vintx.vout.size() > 0 && 
		DecodeAgreementOpRet(vintx.vout[vintx.vout.size() - 1].scriptPubKey) == 'c' &&
		GetAgreementInitialData(txid, dummytxid, dummypk, dummypk, dummypk, dummyamount, dummyamount, dummytxid, refagreementtxid, dummystr) &&
		agreementtxid == refagreementtxid &&
		std::find(foundtxids.begin(), foundtxids.end(), txid) == foundtxids.end())
		{
			result.push_back(txid.GetHex());
			foundtxids.push_back(txid);
		}
	};
	SetCCunspents(addressIndexCCMarker,cp->unspendableCCaddr,true);
	for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressIndexCCMarker.begin(); it != addressIndexCCMarker.end(); it++)
		AddAgreementWithRef(it->first.txhash);
	return (result);
}
// agreementinventory - returns every agreement pk is a member of
UniValue AgreementInventory(CPubKey pk)
{
	UniValue result(UniValue::VOBJ), sellerlist(UniValue::VARR), clientlist(UniValue::VARR), arbitratorlist(UniValue::VARR);
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressIndexCCMarker;
	std::vector<uint256> foundtxids;
	struct CCcontract_info *cp, C;
	uint256 txid, hashBlock, dummytxid;
	std::vector<uint8_t> seller, client, arbitrator;
	int64_t dummyamount;
	std::string dummystr;
	CTransaction vintx;
	cp = CCinit(&C, EVAL_AGREEMENTS);
	auto AddAgreementWithPk = [&](uint256 txid)
	{
		if (myGetTransaction(txid, vintx, hashBlock) != 0 && vintx.vout.size() > 0 && 
		DecodeAgreementOpRet(vintx.vout[vintx.vout.size() - 1].scriptPubKey) == 'c' &&
		GetAgreementInitialData(txid, dummytxid, seller, client, arbitrator, dummyamount, dummyamount, dummytxid, dummytxid, dummystr) &&
		std::find(foundtxids.begin(), foundtxids.end(), txid) == foundtxids.end())
		{
			if (pk == pubkey2pk(seller))
			{
				sellerlist.push_back(txid.GetHex());
				foundtxids.push_back(txid);
			}
			if (pk == pubkey2pk(client))
			{
				clientlist.push_back(txid.GetHex());
				foundtxids.push_back(txid);
			}
			if (pk == pubkey2pk(arbitrator))
			{
				arbitratorlist.push_back(txid.GetHex());
				foundtxids.push_back(txid);
			}
		}
	};
	SetCCunspents(addressIndexCCMarker,cp->unspendableCCaddr,true);
	for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressIndexCCMarker.begin(); it != addressIndexCCMarker.end(); it++)
		AddAgreementWithPk(it->first.txhash);
	result.push_back(Pair("seller",sellerlist));
	result.push_back(Pair("client",clientlist));
	result.push_back(Pair("arbitrator",arbitratorlist));
	return (result);
}
// agreementsettlements - returns every pawnshop that has been designated to this agreement
// bActiveOnly can be used to filter out inactive/closed pawnshop
UniValue AgreementSettlements(const CPubKey& pk, uint256 agreementtxid, bool bActiveOnly)
{
	UniValue result(UniValue::VARR);
	CPubKey mypk, CPK_seller, CPK_client, dummypk;
	CTransaction agreementtx, tx;
	uint256 txid, hashBlock, refagreementtxid, dummytxid;
	std::vector<uint8_t> sellerpk, clientpk, arbitratorpk;
	int32_t numvouts;
	int64_t dummyamount;
	std::string dummystr, pawnshopname;
	uint8_t version, lastfuncid;
	uint32_t pawnshopflags;
	char myCCaddr[65];
	std::vector<uint256> txids;
	struct CCcontract_info *cpPawnshop, CPawnshop;
	cpPawnshop = CCinit(&CPawnshop, EVAL_PAWNSHOP);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (myGetTransaction(agreementtxid,agreementtx,hashBlock) != 0 && agreementtx.vout.size() > 0 &&
	DecodeAgreementOpRet(agreementtx.vout[agreementtx.vout.size()-1].scriptPubKey) == 'c' &&
	GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, dummyamount, dummyamount, dummytxid, dummytxid, dummystr))
	{
		CPK_seller = pubkey2pk(sellerpk);
		CPK_client = pubkey2pk(clientpk);
		if (mypk != CPK_seller && mypk != CPK_client)
			return (result);
		GetCCaddress(cpPawnshop,myCCaddr,mypk);
		SetCCtxids(txids,myCCaddr,true,EVAL_PAWNSHOP,CC_MARKER_VALUE,zeroid,'c');
		for (std::vector<uint256>::const_iterator it=txids.begin(); it!=txids.end(); it++)
		{
			txid = *it;
			if (myGetTransaction(txid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
			DecodePawnshopCreateOpRet(tx.vout[numvouts-1].scriptPubKey,version,pawnshopname,dummypk,dummypk,pawnshopflags,dummytxid,dummyamount,dummyamount,refagreementtxid) != 0 &&
			refagreementtxid == agreementtxid && GetLatestPawnshopTxid(txid, dummytxid, lastfuncid))
			{
				if (bActiveOnly)
				{
					if (lastfuncid == 'c'/* || lastfuncid == 'l' || lastfuncid == 'b'*/)
						result.push_back(txid.GetHex());
				}
				else
				{
					result.push_back(txid.GetHex());
				}
			}
		}
	}
	return (result);
}
UniValue AgreementList()
{
	UniValue result(UniValue::VARR);
	std::vector<uint256> foundtxids;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressIndexCCMarker;
	struct CCcontract_info *cp, C; uint256 txid, hashBlock;
	CTransaction vintx;
	cp = CCinit(&C, EVAL_AGREEMENTS);
	auto addAgreementTxid = [&](uint256 txid)
	{
		if (myGetTransaction(txid, vintx, hashBlock) != 0)
		{
			if (vintx.vout.size() > 0 && DecodeAgreementOpRet(vintx.vout[vintx.vout.size() - 1].scriptPubKey) != 0)
			{
				if (std::find(foundtxids.begin(), foundtxids.end(), txid) == foundtxids.end())
				{
					result.push_back(txid.GetHex());
					foundtxids.push_back(txid);
				}
			}
		}
	};
	SetCCunspents(addressIndexCCMarker,cp->unspendableCCaddr,true);
	for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressIndexCCMarker.begin(); it != addressIndexCCMarker.end(); it++)
		addAgreementTxid(it->first.txhash);
	return(result);
}
