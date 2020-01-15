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
Agreements transaction types:
	
	'p' - agreement proposal (possibly doesn't have CC inputs):
	vins.* normal input
	vin.n-1 previous proposal baton (optional)
		CC input - unlocks validation
	vout.0 marker
		can't be spent
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
		can't be spent
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
			can be spent by a Settlements Payment transaction or 'u' transaction
		if mediator exists:
			can be spent by a Settlements Payment transaction (if buyer), 'u' transaction, or a 'r' transaction (if mediator)
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'c'
		sellerpubkey
		buyerpubkey
		mediator (true/false)
		[refagreementtxid]
	
	'u' - contract update:
	vins.* normal input
	vin.n-1 latest proposal by other party
	vout.0 next update baton
		can be spent by 'u' transactions, provided they also spend the appropriate 'p' transaction
		sent to seller/buyer 1of2 address
	vout.1 deposit split to party 1
		sent to party 1 normal address
	vout.2 deposit split to party 2
		sent to party 2 normal address
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'u'
		initiatorpubkey
		confirmerpubkey
		lastupdatetxid
		updateproposaltxid
		type (contract update, contract cancel)
	
	'd' - contract dispute:
	vins.* normal input
	vin.n-1 previous dispute by disputer
	vout.0 next dispute baton
		can be spent by 'd' transactions
		sent to disputer CC address
	vout.1 response hook (can also be mediator fee)
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
	vout.0 mediator fee OR change
		if no mediator:
			sent to disputer CC address
		if mediator exists:
			sent to mediator CC address
	vout.1 deposit redeem
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
	agreementpropose(name [receiverpubkey][datahash][description][mediatorpubkey][deposit][mediatorfee][prevproposaltxid])
		
	agreementupdate(agreementtxid datahash [description][mediatorpubkey][prevproposaltxid])
		Creates a contract update type transaction, which will need to be confirmed by the other party.
	agreementterminate(agreementtxid datahash [description][depositsplit][prevproposaltxid])
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
	agreementviewupdates(agreementtxid [samplenum][recursive])
		Retrieves a list of updates for the specified agreement.
	agreementviewdisputes(agreementtxid [samplenum][recursive])
		Retrieves a list of disputes for the specified agreement.
	agreementinventory([pubkey])
		Retrieves every agreement wherein the specified pubkey is the buyer, seller or mediator.
		Can look up both current and past agreements.

*/

// start of consensus code

//===========================================================================
// Opret encoders/decoders
//===========================================================================

//uint8_t DecodeAgreementOpRet()
// just returns funcid of whatever agreement tx opret is fed into it
uint8_t DecodeAgreementOpRet(const CScript scriptPubKey, uint8_t &proposaltype)
{
	std::vector<uint8_t> vopret, dummyInitiator, dummyReceiver, dummyMediator;
	int64_t dummyMediatorFee, dummyDeposit, dummyDepositCut;
	uint256 dummyHash, dummyAgreementTxid, dummyPrevProposalTxid;
	std::string dummyName;
	uint8_t evalcode, funcid, *script;
	proposaltype = 0;
	
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
			return DecodeAgreementProposalOpRet(scriptPubKey, proposaltype, dummyInitiator, dummyReceiver, dummyMediator, dummyMediatorFee, dummyDeposit, dummyDepositCut, dummyHash, dummyAgreementTxid, dummyPrevProposalTxid, dummyName);
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

CScript EncodeAgreementProposalOpRet(uint8_t proposaltype, std::vector<uint8_t> initiator, std::vector<uint8_t> receiver, std::vector<uint8_t> mediator, int64_t mediatorfee, int64_t deposit, int64_t depositcut, uint256 datahash, uint256 agreementtxid, uint256 prevproposaltxid, std::string name)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'p';
	proposaltype = 'p'; //temporary
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << proposaltype << initiator << receiver << mediator << mediatorfee << deposit << depositcut << datahash << agreementtxid << prevproposaltxid << name);
	return(opret);
}

uint8_t DecodeAgreementProposalOpRet(CScript scriptPubKey, uint8_t &proposaltype, std::vector<uint8_t> &initiator, std::vector<uint8_t> &receiver, std::vector<uint8_t> &mediator, int64_t &mediatorfee, int64_t &deposit, int64_t &depositcut, uint256 &datahash, uint256 &agreementtxid, uint256 &prevproposaltxid, std::string &name)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> proposaltype; ss >> initiator; ss >> receiver; ss >> mediator; ss >> mediatorfee; ss >> deposit; ss >> depositcut; ss >> datahash; ss >> agreementtxid; ss >> prevproposaltxid; ss >> name) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

//===========================================================================
// Validation
//===========================================================================

bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	/*
	IMPORTANT!!! - deposit is locked under EVAL_AGREEMENTS, but it may also be spent by a specific Settlements transaction. Make sure this code is able to check for this.
	
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
					- Deposit: if mediator == false, must be 0. Otherwise, check if the vout4 nValue is the same as this value
					- Mediatorfee: if mediator == false, must be 0. Otherwise, must be more than 10000 satoshis
					- Depositsplit: must be 0 (or failing that, just ignored)
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
					- Deposit: only relevant if proposaltype is 'a' and mediatorpubkey is valid. Is otherwise ignored for now
					- Mediatorfee: only relevant if proposaltype is 'a' and mediatorpubkey is valid, in which case it must be at least 10000 satoshis. Is otherwise ignored for now
					- Depositsplit: only relevant if proposaltype is 't', and the deposit is non-zero. The deposit must be evenly divisible so that there are no remaining coins left
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
	std::cerr << "AgreementsValidate triggered, passing through" << std::endl;
	return true;
}

//===========================================================================
// Helper functions
//===========================================================================

int64_t IsAgreementsvout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v)
{
	char destaddr[64];
	if( tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0 )
	{
		if( Getscriptaddress(destaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(destaddr,cp->unspendableCCaddr) == 0 )
			return(tx.vout[v].nValue);
	}
	return(0);
}

//===========================================================================
// RPCs - tx creation
//===========================================================================

// agreementpropose - constructs a 'p' transaction, with the 'p' proposal type
UniValue AgreementPropose(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> buyer, std::vector<uint8_t> mediator, int64_t mediatorfee, int64_t deposit, uint256 prevproposaltxid, uint256 refagreementtxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
	CTransaction prevproposaltx, refagreementtx;
	int32_t numvouts, vini, height, retcode;
	uint256 hashBlock, refHash, refAgreementTxid, refPrevProposalTxid, spenttxid;
	std::vector<uint8_t> refInitiator, refReceiver, refMediator;
	int64_t refMediatorFee, refDeposit, refDepositCut;
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
		if(DecodeAgreementProposalOpRet(prevproposaltx.vout[numvouts - 1].scriptPubKey, refProposalType, refInitiator, refReceiver, refMediator, refMediatorFee, refDeposit, refDepositCut, refHash, refAgreementTxid, refPrevProposalTxid, refName) != 'p')
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "invalid agreement proposal txid " << prevproposaltxid.GetHex());
		if(refProposalType != 'p') {
			if(refProposalType == 't')
				CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "specified proposal terminated by creator, txid " << prevproposaltxid.GetHex());
			else
				CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "specified proposal has incorrect proposal type, txid " << prevproposaltxid.GetHex());
		}
		if(retcode = CCgetspenttxid(spenttxid, vini, height, prevproposaltxid, 1) == 0)
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "specified proposal has already been updated by txid " << spenttxid.GetHex());
		if(mypk != pubkey2pk(refInitiator))
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "-pubkey doesn't match creator of previous proposal txid " << prevproposaltxid.GetHex());
		if(pubkey2pk(buyer).IsValid() && !(refReceiver.empty()) && buyer != refReceiver)
				CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "buyer must be the same as specified in previous proposal txid " << prevproposaltxid.GetHex());
		if(!(pubkey2pk(buyer).IsValid()) && !(refReceiver.empty()))
				CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cannot remove buyer when one exists in previous proposal txid " << prevproposaltxid.GetHex());
	}
	
	// check refagreementtxid if specified
	if(refagreementtxid != zeroid) {
		if(myGetTransaction(refagreementtxid,refagreementtx,hashBlock)==0 || (numvouts=refagreementtx.vout.size())<=0)
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cant find specified reference agreement txid " << refagreementtxid.GetHex());
		if(DecodeAgreementOpRet(refagreementtx.vout[numvouts - 1].scriptPubKey, refProposalType) != 'c')
			CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "invalid reference agreement txid " << refagreementtxid.GetHex());
	}

	std::cerr << "seller: " << HexStr(std::vector<uint8_t>(mypk.begin(),mypk.end())) << std::endl;
	std::cerr << "buyer: " << HexStr(buyer) << std::endl;
	std::cerr << "buyerisValid: " << pubkey2pk(buyer).IsValid() << std::endl;
	std::cerr << "mediator: " << HexStr(mediator) << std::endl;
	std::cerr << "mediatorisValid: " << pubkey2pk(mediator).IsValid() << std::endl;
	std::cerr << "mediatorfee: " << mediatorfee << std::endl;
	std::cerr << "deposit: " << deposit << std::endl;
	std::cerr << "refagreementtxid: " << refagreementtxid.GetHex() << std::endl;
	std::cerr << "prevproposaltxid: " << prevproposaltxid.GetHex() << std::endl;

	if(AddNormalinputs(mtx,mypk,txfee+CC_MARKER_VALUE+CC_RESPONSE_VALUE,64,pk.IsValid()) >= txfee+CC_MARKER_VALUE+CC_RESPONSE_VALUE) {
		if(prevproposaltxid != zeroid)
			mtx.vin.push_back(CTxIn(prevproposaltxid,1,CScript())); // vin.n-1 previous proposal (optional, will trigger validation)
			// could also spend marker here, maybe?
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, GetUnspendable(cp, NULL))); // vout.0 marker
		if(pubkey2pk(buyer).IsValid())
			mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_RESPONSE_VALUE, mypk, pubkey2pk(buyer))); // vout.1 response hook (with buyer)
		else
			mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_RESPONSE_VALUE, mypk)); // vout.1 response hook (no buyer)
		return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeAgreementProposalOpRet('p',std::vector<uint8_t>(mypk.begin(),mypk.end()),buyer,mediator,mediatorfee,deposit,0,datahash,refagreementtxid,prevproposaltxid,name)));
	}
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");
}

// agreementupdate - constructs a 'p' transaction, with the 'u' proposal type
// This transaction will only be validated if agreementtxid is specified. Optionally, prevproposaltxid can be used to amend previous update requests.
UniValue AgreementUpdate(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash, std::vector<uint8_t> newmediator, uint256 prevproposaltxid)
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
	
	CCERR_RESULT("agreementscc",CCLOG_INFO,stream << "incomplete");
}

// agreementterminate - constructs a 'p' transaction, with the 't' proposal type
// This transaction will only be validated if agreementtxid OR prevproposaltxid are specified (can also have both).
// if prevproposaltxid is specified but agreementtxid isn't: if prevproposaltxid has the 'p' proposal type, cancels the specified proposal.
// if agreementtxid is specified but prevproposaltxid isn't: creates a new request for cancelling the specified agreement.
// if both agreementtxid and prevproposaltxid are specified: updates the specified request for cancelling the agreement.
UniValue AgreementTerminate(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash, uint64_t depositcut, uint256 prevproposaltxid)
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
	
	CCERR_RESULT("agreementscc",CCLOG_INFO,stream << "incomplete");
}

// agreementaccept - spend a 'p' transaction that was submitted by the other party.
// this function is context aware and does different things dependent on the proposal type:
// if txid opret has the 'p' proposal type, will create a 'c' transaction (create contract)
// if txid opret has the 'u' or 't' proposal type, will create a 'u' transaction (update contract)
UniValue AgreementAccept(const CPubKey& pk, uint64_t txfee, uint256 reftxid, uint256 datahash, uint256 agreementtxid)
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
	
	//get reftxid
	//confirm that tx is 'p' type
	//check proposal type
	//verify datahashes
	//switch(proposaltype)
	// 	stuff like addnormalinputs etc.
	
	CCERR_RESULT("agreementscc",CCLOG_INFO,stream << "incomplete");
}

// agreementdispute
// agreementresolve

//===========================================================================
// RPCs - informational
//===========================================================================

// agreementinfo
// 	Note: version numbers should be reset after contract acceptance
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
			if (vintx.vout.size() > 0 && DecodeAgreementOpRet(vintx.vout[vintx.vout.size() - 1].scriptPubKey, proposaltype) != 0) {
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