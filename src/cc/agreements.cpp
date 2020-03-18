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
if deposit exists but there is no mediator, it acts as immediate payment upon acceptance (deposit)
when contracts expire, raising disputes should still be allowed
IMPORTANT!!! - deposit is locked under EVAL_AGREEMENTS, but it may also be spent by a specific Settlements transaction. Make sure this code is able to check for this.
don't allow swapping between no mediator <-> mediator
only 1 update/cancel request per party, per agreement
version numbers should be reset after contract acceptance

Agreements transaction types:
	
	case 'p':
		agreement proposal:
		vin.0 normal input
		vin.1 previous proposal marker
		vin.2 previous proposal baton
		vout.0 marker
		vout.1 response hook
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 'p' proposaltype initiator receiver mediator mediatorfee deposit depositcut datahash agreementtxid prevproposaltxid name
	
	case 't':
		proposal cancel:
		vin.0 normal input
		vin.1 previous proposal marker
		vin.2 previous proposal baton
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 't' proposaltxid initiator [message]
	
	case 'c':
		contract creation:
		vin.0 normal input
		vin.1 latest proposal by seller
		vout.0 marker
		vout.1 update baton
		vout.2 dispute baton
		vout.3 deposit / agreement completion marker
		vout.4 prepayment
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 'c' proposaltxid
	
	case 'u':
		contract update:
		vin.0 normal input
		vin.1 latest proposal by other party
		vout.0 next update baton
		vout.1 deposit split to party 1
		vout.2 deposit split to party 2
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 'u' [initiator] confirmer [lastupdatetxid] updateproposaltxid type
	
	case 'd':
		contract dispute:
		vin.0 normal input
		vin.1 previous dispute by disputer
		vout.0 next dispute baton
		vout.1 response hook / mediator fee
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 'd' initiator [receiver] [lastdisputetxid] disputetype disputehash
	
	case 'r':
		contract dispute resolve:
		vin.0 normal input
		vin.1 dispute resolved
		vout.0 mediator fee OR change
		vout.1 deposit redeem
		vout.n-2 change
		vout.n-1 OP_RETURN EVAL_AGREEMENTS 'r' disputetxid verdict rewardedpubkey message
	
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

	DONE agreementpropose (name datahash buyer mediator [mediatorfee][deposit][prevproposaltxid][refagreementtxid])
	agreementrequestupdate(agreementtxid name datahash [newmediator][prevproposaltxid])
	agreementrequestcancel(agreementtxid name datahash [depositsplit][prevproposaltxid])
	Proposal creation
	
	DONE agreementcloseproposal(proposaltxid message)
	agreementaccept(proposaltxid)
	Proposal response
	
	agreementdispute(agreementtxid disputetype [disputehash])
	agreementresolve(agreementtxid disputetxid verdict [rewardedpubkey][message])
	Disputes
	
	DONE agreementaddress
	DONE agreementlist
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
	std::vector<uint8_t> vopret, dummyInitiator, dummyReceiver, dummyMediator;
	int64_t dummyPrepayment, dummyMediatorFee, dummyDeposit, dummyDepositCut;
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
			return DecodeAgreementProposalOpRet(scriptPubKey, dummyProposalType, dummyInitiator, dummyReceiver, dummyMediator, dummyPrepayment, dummyMediatorFee, dummyDeposit, dummyDepositCut, dummyHash, dummyAgreementTxid, dummyPrevProposalTxid, dummyName);
		case 't':
			return DecodeAgreementProposalCloseOpRet(scriptPubKey, dummyPrevProposalTxid, dummyInitiator, dummyName);
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

CScript EncodeAgreementProposalOpRet(uint8_t proposaltype, std::vector<uint8_t> initiator, std::vector<uint8_t> receiver, std::vector<uint8_t> mediator, int64_t prepayment, int64_t mediatorfee, int64_t deposit, int64_t depositcut, uint256 datahash, uint256 agreementtxid, uint256 prevproposaltxid, std::string name)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'p';
	proposaltype = 'p'; //temporary
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << proposaltype << initiator << receiver << mediator << prepayment << mediatorfee << deposit << depositcut << datahash << agreementtxid << prevproposaltxid << name);
	return(opret);
}
uint8_t DecodeAgreementProposalOpRet(CScript scriptPubKey, uint8_t &proposaltype, std::vector<uint8_t> &initiator, std::vector<uint8_t> &receiver, std::vector<uint8_t> &mediator, int64_t &prepayment, int64_t &mediatorfee, int64_t &deposit, int64_t &depositcut, uint256 &datahash, uint256 &agreementtxid, uint256 &prevproposaltxid, std::string &name)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> proposaltype; ss >> initiator; ss >> receiver; ss >> mediator; ss >> prepayment; ss >> mediatorfee; ss >> deposit; ss >> depositcut; ss >> datahash; ss >> agreementtxid; ss >> prevproposaltxid; ss >> name) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

CScript EncodeAgreementProposalCloseOpRet(uint256 proposaltxid, std::vector<uint8_t> initiator, std::string message)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 't';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << proposaltxid << initiator << message);
	return(opret);
}
uint8_t DecodeAgreementProposalCloseOpRet(CScript scriptPubKey, uint256 &proposaltxid, std::vector<uint8_t> &initiator, std::string &message)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> proposaltxid; ss >> initiator; ss >> message) != 0 && evalcode == EVAL_AGREEMENTS)
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

//===========================================================================
// Validation
//===========================================================================

bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	/*
	
	- Fetch transaction
	- Generic checks (measure boundaries, etc. Check other modules for inspiration)
	- Get transaction funcid
	- Switch case:
		if funcid == 'c':
			- Fetch the opret and make sure all data is included. If opret is malformed or data is missing, invalidate
			- Get sellerpubkey, buyerpubkey, mediator and refagreementtxid
				- Buyerpubkey: confirm that it isn't null and that current tx came from this pubkey
			- Check if vout0 marker exists and is sent to global addr. If not, invalidate
			- Check if vout1 update baton exists and is sent to seller/buyer 1of2 address. Confirm that 1of2 address can only be spent by the specified seller/buyer pubkeys.
			- Check if vout2 seller dispute baton exists and is sent to seller CC address. Confirm that the address can only be spent by the specified seller pubkey.
			- Check if vout3 buyer dispute baton exists and is sent to buyer CC address. Confirm that the address can only be spent by the specified buyer pubkey.
			- Check if vout4 exists and its nValue is >= 10000 sats.
				TODO: what to do with this? Which eval code to use for the deposit?
			- Check if vin.n-1 exists and is a 'p' type transaction. If it is not correct or non-existant, invalidate, otherwise:
				- Check if vout0 marker exists and is sent to global addr. If not, invalidate
				- Check if vin.n-1 prevout is vout1 in 'p' tx
				- Fetch the opret from 'p' tx and make sure all data is included. If opret is malformed or data is missing, invalidate
				- Get proposaltype, initiatorpubkey, receiverpubkey, mediatorpubkey, agreementtxid, deposit, mediatorfee, depositsplit, datahash, description
					- Proposaltype: must be "p" (proposal). If it is "u" (contract update) or "t" (contract terminate), invalidate
					- Initiatorpubkey: confirm that 'p' tx came from this pubkey. Make sure that initiatorpubkey == sellerpubkey
					- Receiverpubkey: confirm that the current tx came from this pubkey. Make sure that receiverpubkey == buyerpubkey
					- Mediatorpubkey: if mediator == false, must be null (or failing that, just ignored). If mediator == true, must be a valid pubkey
					- Agreementtxid: can be either null or valid agreement txid, however it must be the same as the refagreementtxid in current tx
					- Prepayment: if mediator == false, must be 0. Otherwise, check if the vout4 nValue is the same as this value
					- Mediatorfee: if mediator == false, must be 0. Otherwise, must be more than 10000 satoshis
					- Prepaymentsplit: must be 0 (or failing that, just ignored)
					- Datahash and description doesn't need to be validated, can be w/e
				- Check if vin.n-1 exists in the 'p' type transaction. If it does, get the prevout txid, then run the Løøp. If it returns false, invalidate
			- Misc Business rules
				- Make sure that sellerpubkey != buyerpubkey != mediatorpubkey
			return true?
		if funcid == 'p': 
				- Check if marker exists and is sent to global addr. If not, invalidate
				- Check if response hook exists and is sent to 1of2 CC addr (we'll confirm if this address is correct later)
				- Fetch the opret from currenttx and make sure all data is included. If opret is malformed or data is missing, invalidate
				- Get initiatorpubkey, receiverpubkey, mediatorpubkey, proposaltype, agreementtxid, deposit, mediatorfee, depositsplit, datahash, description
					- Initiatorpubkey: confirm that this tx came from this pubkey
					- Receiverpubkey: check if pubkey is valid
					- Mediatorpubkey: can be either null or valid pubkey
					- Proposaltype: can be 'a'(proposal amend),'u'(contract update) or 't' (contract terminate). If it is 'c'(proposal create), invalidate as 'c' types shouldn't be able to trigger validation
					- Agreementtxid: if proposaltype is 'a', agreementtxid must be zeroid.
						Otherwise, check if agreementtxid is valid (optionally, check if its 'c' type and that initiator and receiver match)
					- Prepayment: only relevant if proposaltype is 'a' and mediatorpubkey is valid. Is otherwise ignored for now
					- Mediatorfee: only relevant if proposaltype is 'a' and mediatorpubkey is valid, in which case it must be at least 10000 satoshis. Is otherwise ignored for now
					- Prepaymentsplit: only relevant if proposaltype is 't', and the deposit is non-zero. The deposit must be evenly divisible so that there are no remaining coins left
					- Datahash and description doesn't need to be validated, can be w/e
				- Save all this data somewhere
				- If a 'p' type is being validated, that means it probably spent a previous 'p' type, therefore check vin.n-1 prevout and get the prevouttxid. if it doesn't exist or is incorrect, invalidate
				Run the Løøp(currenttxid, currentfuncid, prevouttxid):
					- Get transaction(prevouttxid)
					- Check if marker exists and is sent to global addr. If not, invalidate
					- Check if response hook exists and is spent by currenttxid. If not, invalidate
					- Fetch the opret from prevouttx and make sure all data is included. If opret is malformed or data is missing, invalidate
					- Get initiatorpubkey, receiverpubkey, mediatorpubkey, proposaltype, agreementtxid, deposit, mediatorfee, depositsplit, datahash, description
						- Initiatorpubkey: confirm that this tx came from this pubkey
						- Receiverpubkey: check if pubkey is valid. 
						- Mediatorpubkey: can be either null or valid pubkey
		
		
	RecursiveProposalLøøp(proposaltxid,)
	- Get proposal tx
	- Check if marker exists and is sent to global addr. If not, invalidate
	- Check if response hook exists and is sent to 1of2 CC addr (we'll confirm if this address is correct later)
	*/
	//return(eval->Invalid("no validation yet"));
	
	int32_t numvins = tx.vin.size(), numvouts = tx.vout.size();
	//CPubKey globalpubkey;
	
	// check boundaries:
    if (numvouts < 1)
        return eval->Invalid("no vouts\n");
	
	/*globalpubkey = check_signing_pubkey(tx.vin[numvins-2].scriptSig);
	std::cerr << "globalpubkey=" << HexStr(std::vector<uint8_t>(globalpubkey.begin(),globalpubkey.end())) << std::endl;*/
			
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

//===========================================================================
// RPCs - tx creation
//===========================================================================

// agreementpropose - constructs a 'p' transaction, with the 'p' proposal type
UniValue AgreementPropose(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> buyer, std::vector<uint8_t> mediator, int64_t prepayment, int64_t mediatorfee, int64_t deposit, uint256 prevproposaltxid, uint256 refagreementtxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	CTransaction prevproposaltx, refagreementtx, spenttx;
	int32_t numvouts, vini, height, retcode;
	uint256 hashBlock, refHash, refAgreementTxid, refPrevProposalTxid, spenttxid;
	std::vector<uint8_t> refInitiator, refReceiver, refMediator;
	int64_t refPrepayment, refMediatorFee, refDeposit, refDepositCut;
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
	
	// check if mediator pubkey exists and is valid
	if(!mediator.empty() && !(pubkey2pk(mediator).IsValid()))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Mediator pubkey invalid");
	
	// checking if mypk != buyerpubkey != mediatorpubkey
	if(pubkey2pk(buyer).IsValid() && pubkey2pk(buyer) == mypk)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Seller pubkey cannot be the same as buyer pubkey");
	if(pubkey2pk(mediator).IsValid() && pubkey2pk(mediator) == mypk)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Seller pubkey cannot be the same as mediator pubkey");
	if(pubkey2pk(buyer).IsValid() && pubkey2pk(mediator).IsValid() && pubkey2pk(mediator) == pubkey2pk(buyer))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Buyer pubkey cannot be the same as mediator pubkey");
	
	// if mediator exists, check if mediator fee is sufficient
	if(pubkey2pk(mediator).IsValid()) {
		if(mediatorfee == 0)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Mediator fee must be specified if valid mediator exists");
		else if(mediatorfee < CC_MEDIATORFEE_MIN)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Mediator fee is too low");
	}
	else
		mediatorfee = 0;
	
	// if mediator exists, check if deposit is sufficient
	if(pubkey2pk(mediator).IsValid()) {
		if(deposit == 0)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Deposit must be specified if valid mediator exists");
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
		else if(DecodeAgreementProposalOpRet(prevproposaltx.vout[numvouts-1].scriptPubKey,refProposalType,refInitiator,refReceiver,refMediator,refPrepayment,refMediatorFee,refDeposit,refDepositCut,refHash,refAgreementTxid,refPrevProposalTxid,refName) != 'p')
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
		return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementProposalOpRet('p',std::vector<uint8_t>(mypk.begin(),mypk.end()),buyer,mediator,prepayment,mediatorfee,deposit,0,datahash,refagreementtxid,prevproposaltxid,name)));
	}
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");
}

// agreementrequestupdate - constructs a 'p' transaction, with the 'u' proposal type
// This transaction will only be validated if agreementtxid is specified. Optionally, prevproposaltxid can be used to amend previous update requests.
UniValue AgreementRequestUpdate(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash, std::vector<uint8_t> newmediator, uint256 prevproposaltxid)
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
	// if newmediator is 0, maintain current mediator status
	// don't allow swapping between no mediator <-> mediator
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
UniValue AgreementCloseProposal(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid, std::string message)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	
	CTransaction proposaltx, spenttx;
	int32_t numvouts, vini, height, retcode;
	uint256 hashBlock, refHash, refAgreementTxid, refPrevProposalTxid, spenttxid;
	std::vector<uint8_t> refInitiator, refReceiver, refMediator;
	int64_t refPrepayment, refMediatorFee, refDeposit, refDepositCut;
	std::string refName;
	uint8_t refProposalType;
	
	struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	if(txfee == 0)
		txfee = 10000;
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());

	// check message, if it exists
	if(!(message.empty()) && message.size() > 1024)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Optional message cannot exceed 1024 characters");

	// check proposaltxid
	if(proposaltxid != zeroid) {
		if(myGetTransaction(proposaltxid,proposaltx,hashBlock)==0 || (numvouts=proposaltx.vout.size())<=0)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "cant find specified proposal txid " << proposaltxid.GetHex());
		if(DecodeAgreementOpRet(proposaltx.vout[numvouts - 1].scriptPubKey) == 'c')
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "specified txid is a contract id, needs to be proposal id");
		else if(DecodeAgreementProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,refProposalType,refInitiator,refReceiver,refMediator,refPrepayment,refMediatorFee,refDeposit,refDepositCut,refHash,refAgreementTxid,refPrevProposalTxid,refName) != 'p')
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
		return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementProposalCloseOpRet(proposaltxid,std::vector<uint8_t>(mypk.begin(),mypk.end()),message)));
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
	std::vector<uint8_t> refInitiator, refReceiver, refMediator;
	int64_t refPrepayment, refMediatorFee, refDeposit, refDepositCut;
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
		else if(DecodeAgreementProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,refProposalType,refInitiator,refReceiver,refMediator,refPrepayment,refMediatorFee,refDeposit,refDepositCut,refHash,refAgreementTxid,refPrevProposalTxid,refName) != 'p')
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
			
			// check if mediator pubkey exists and is valid
			if(!refMediator.empty() && !(pubkey2pk(refMediator).IsValid()))
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Mediator pubkey in proposal invalid");
			
			// checking if mypk != initiatorpubkey != mediatorpubkey
			if(pubkey2pk(refInitiator).IsValid() && pubkey2pk(refInitiator) == mypk)
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Initiator pubkey cannot accept their own agreement");
			if(pubkey2pk(refMediator).IsValid() && pubkey2pk(refMediator) == mypk)
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Receiver pubkey cannot be the same as mediator pubkey");
			if(pubkey2pk(refInitiator).IsValid() && pubkey2pk(refMediator).IsValid() && pubkey2pk(refMediator) == pubkey2pk(refInitiator))
				CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Buyer pubkey cannot be the same as mediator pubkey");
			
			// if mediator exists, check if deposit and mediator fee are sufficient
			if(!(refMediator.empty())) {
				if(refMediatorFee < CC_MEDIATORFEE_MIN)
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Mediator is defined in proposal, but mediator fee is too low");
				if(refDeposit < CC_DEPOSIT_MIN)
					CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Mediator is defined in proposal, but deposit is too low");
			}
			else {
				refMediatorFee = 0;
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
				mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, refDeposit, mypk, pubkey2pk(refMediator))); // vout.3 deposit / agreement completion marker
				//mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, refPrepayment, pubkey2pk(refInitiator)));
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
				mediator: pubkey3 (or none);
				deposit: number (or none);
				mediatorfee: number (or none);
			- If "u":
				receiver: pubkey2;
				mediator: pubkey3 (or none);
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
			- Else if deposit taken by mediator:
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
			- If mediator = true:
				mediator: pubkey3 (or none);
				deposit: number;
				mediatorfee: number;
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
