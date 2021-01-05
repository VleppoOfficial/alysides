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

/*
The goal here is to create FSM-like on-chain agreements, which are created and their state updated by mutual assent of two separate pubkeys.
Each proposal transaction includes a space for a sha256 hash, which can be used to store a checksum for a legal contract document, point to an oracle/KV transaction, etc.
Agreements are created and their state updated only when the recipient pubkey explicitly accepts an offer/proposal with a separate transaction, somewhat like git pull requests or legally binding contracts.

A visual representation of a typical agreement under this CC would look something like this:

   o                             o - start here
   |                             p - proposal transaction
   V                             c - agreement creation transaction
 | p |  | p |  | p |  | p |      u - agreement update transaction
   |      |      |      |        t - agreement closure transaction
   V      V      V      V
 | c |->| u |->| u |->| t |

Each agreement has a small escrow for a deposit, which is funded by the recipient pubkey at the time of agreement creation.
The agreement can be updated and/or closed by any member (offeror or signer) pubkey creating a proposal for it, and having it accepted by the other member pubkey.
When the agreement is closed, the deposit amount held in the escrow can be split or sent in its entirety to a specific member.

In addition, an agreement can also be disputed if the initial proposal for creating an agreement contains a fixed arbitrator pubkey.
The arbitrator pubkey can be owned by a mutually agreed upon third party or an automatic arbitration system of some sort.

Each proposal can include a requirement for initial prepayments (can be used for mobilization fees, etc.).

Proposal creation:
vin.0: normal input
...
vin.n-1: normal input
vout.0: response baton output to srcpub/destpub CC 1of2 address
vout.1: marker to srcpub CC address
vout.2: marker to destpub CC address
vout.3 (optional): marker to arbitratorpub CC address
vout.n-2: normal change
vout.n-1: OP_RETURN EVAL_AGREEMENTS 'p' version srcpub destpub agreementname agreementhash deposit payment refagreementtxid bNewAgreement arbitratorpub disputefee

Proposal cancellation:
vin.0: normal input
vin.1: baton input from proposal
...
vin.n-1: normal input
vout.n-2: normal change
vout.n-1: OP_RETURN EVAL_AGREEMENTS 's' version proposaltxid srcpub cancelinfo

Agreement creation:
vin.0: normal input
vin.1: baton input from proposal
...
vin.n-1: normal input
vout.0: event log baton output to srcpub/destpub CC 1of2 address
vout.1: deposit / activity marker to global CC pubkey
vout.2: payment to proposal's srcpub normal address
vout.3: marker to proposal's srcpub CC address
vout.4: marker to proposal's destpub CC address
vout.5 (optional): marker to proposal's arbitratorpub CC address
vout.n-2: normal change
vout.n-1: OP_RETURN EVAL_AGREEMENTS 'c' version proposaltxid

Agreement update:
vin.0: normal input
vin.1: baton input from latest agreement event log entry
vin.2: baton input from proposal
...
vin.n-1: normal input
vout.0: event log baton output to srcpub/destpub CC 1of2 address
vout.1: payment to proposal's srcpub normal address
vout.n-2: normal change
vout.n-1: OP_RETURN EVAL_AGREEMENTS 'u' version agreementtxid proposaltxid

Agreement closure:
vin.0: normal input
vin.1: baton input from latest agreement event log entry
vin.2: baton input from proposal
vin.3: deposit from agreement
...
vin.n-1: normal input
vout.0: depositcut + payment to proposal's srcpub normal address
vout.n-2: normal change + (deposit - depositcut)
vout.n-1: OP_RETURN EVAL_AGREEMENTS 't' version agreementtxid proposaltxid depositcut

Agreement dispute:
vin.0: normal input
vin.1: baton input from latest agreement event log entry
...
vin.n-1: normal input
vout.0: event log baton output + disputefee to arbitratorpub CC address
vout.n-2: normal change
vout.n-1: OP_RETURN EVAL_AGREEMENTS 'd' version agreementtxid srcpub destpub disputeinfo bFinalDispute

Agreement dispute resolution:
vin.0: normal input
vin.1: baton input from latest agreement event log entry + disputefee
vin.2 (optional): deposit from agreement
...
vin.n-1: normal input
if deposit is taken:
	vout.0: depositcut to dispute's srcpub normal address, assuming depositcut != 0
	vout.1: (deposit - depositcut) to dispute's destpub normal address, assuming depositcut != deposit
if deposit is depositcut < 0:
	vout.0: event log baton output to srcpub/destpub CC 1of2 address
vout.n-2: normal change + disputefee
vout.n-1: OP_RETURN EVAL_AGREEMENTS 'r' version agreementtxid disputetxid depositcut resolutioninfo

Agreement deposit unlock:
TBD
OP_RETURN EVAL_AGREEMENTS 'n' version agreementtxid unlocktxid
*/

// --- Start of consensus code ---

int64_t IsAgreementsvout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v, char* destaddr)
{
	char tmpaddr[64];
	if ( tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0 )
	{
		if ( Getscriptaddress(tmpaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(tmpaddr,destaddr) == 0 )
			return(tx.vout[v].nValue);
	}
	return(0);
}

// OP_RETURN data encoders and decoders for all Agreements transactions.
CScript EncodeAgreementProposalOpRet(uint8_t version, CPubKey srcpub, CPubKey destpub, std::string agreementname, uint256 agreementhash, int64_t deposit, int64_t payment, uint256 refagreementtxid, bool bNewAgreement, CPubKey arbitratorpub, int64_t disputefee)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'p';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << srcpub << destpub << agreementname << agreementhash << deposit << payment << refagreementtxid << bNewAgreement << arbitratorpub << disputefee);
	return(opret);
}
uint8_t DecodeAgreementProposalOpRet(CScript scriptPubKey, uint8_t &version, CPubKey &srcpub, CPubKey &destpub, std::string &agreementname, uint256 &agreementhash, int64_t &deposit, int64_t &payment, uint256 &refagreementtxid, bool &bNewAgreement, CPubKey &arbitratorpub, int64_t &disputefee)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> srcpub; ss >> destpub; ss >> agreementname; ss >> agreementhash; ss >> deposit; ss >> payment; ss >> refagreementtxid; ss >> bNewAgreement; ss >> arbitratorpub; ss >> disputefee) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

CScript EncodeAgreementProposalCloseOpRet(uint8_t version, uint256 proposaltxid, CPubKey srcpub, std::string cancelinfo)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 's';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << proposaltxid << srcpub << cancelinfo);
	return(opret);
}
uint8_t DecodeAgreementProposalCloseOpRet(CScript scriptPubKey, uint8_t &version, uint256 &proposaltxid, CPubKey &srcpub, std::string &cancelinfo)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> proposaltxid; ss >> srcpub; ss >> cancelinfo) != 0 && evalcode == EVAL_AGREEMENTS)
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

CScript EncodeAgreementCloseOpRet(uint8_t version, uint256 agreementtxid, uint256 proposaltxid, int64_t depositcut)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 't';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << agreementtxid << proposaltxid << depositcut);
	return(opret);
}
uint8_t DecodeAgreementCloseOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &proposaltxid, int64_t &depositcut)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> agreementtxid; ss >> proposaltxid; ss >> depositcut) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

CScript EncodeAgreementDisputeOpRet(uint8_t version, uint256 agreementtxid, CPubKey srcpub, CPubKey destpub, std::string disputeinfo, bool bFinalDispute)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'd';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << agreementtxid << srcpub << destpub << disputeinfo << bFinalDispute);
	return(opret);
}
uint8_t DecodeAgreementDisputeOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, CPubKey &srcpub, CPubKey &destpub, std::string &disputeinfo, bool &bFinalDispute)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> agreementtxid; ss >> srcpub; ss >> destpub; ss >> disputeinfo; ss >> bFinalDispute) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

CScript EncodeAgreementDisputeResolveOpRet(uint8_t version, uint256 agreementtxid, uint256 disputetxid, int64_t depositcut, std::string resolutioninfo)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'r';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << agreementtxid << disputetxid << depositcut << resolutioninfo);
	return(opret);
}
uint8_t DecodeAgreementDisputeResolveOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &disputetxid, int64_t &depositcut, std::string &resolutioninfo)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> agreementtxid; ss >> disputetxid; ss >> depositcut; ss >> resolutioninfo) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

CScript EncodeAgreementUnlockOpRet(uint8_t version, uint256 agreementtxid, uint256 unlocktxid)
{
	CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'n';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << agreementtxid << unlocktxid);
	return(opret);
}
uint8_t DecodeAgreementUnlockOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &unlocktxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> agreementtxid; ss >> unlocktxid) != 0 && evalcode == EVAL_AGREEMENTS)
		return(funcid);
	return(0);
}

// Generic decoder for Agreements transactions, returns function id.
uint8_t DecodeAgreementOpRet(const CScript scriptPubKey)
{
	std::vector<uint8_t> vopret;
	CPubKey dummypubkey;
	int64_t dummyint64;
	bool dummybool;
	uint256 dummyuint256;
	std::string dummystring;
	uint8_t evalcode, funcid, *script, dummyuint8;
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
				return DecodeAgreementProposalOpRet(scriptPubKey, dummyuint8, dummypubkey, dummypubkey, dummystring, dummyuint256, dummyint64, dummyint64, dummyuint256, dummybool, dummypubkey, dummyint64);
			case 's':
				return DecodeAgreementProposalCloseOpRet(scriptPubKey, dummyuint8, dummyuint256, dummypubkey, dummystring);
			case 'c':
				return DecodeAgreementSigningOpRet(scriptPubKey, dummyuint8, dummyuint256);
			case 'u':
				return DecodeAgreementUpdateOpRet(scriptPubKey, dummyuint8, dummyuint256, dummyuint256);
			case 't':
				return DecodeAgreementCloseOpRet(scriptPubKey, dummyuint8, dummyuint256, dummyuint256, dummyint64);
			case 'd':
				return DecodeAgreementDisputeOpRet(scriptPubKey, dummyuint8, dummyuint256, dummypubkey, dummypubkey, dummystring, dummybool);
			case 'r':
				return DecodeAgreementDisputeResolveOpRet(scriptPubKey, dummyuint8, dummyuint256, dummyuint256, dummyint64, dummystring);
			//case 'n':
				// TBD
			default:
				LOGSTREAM((char *)"agreementscc", CCLOG_DEBUG1, stream << "DecodeAgreementOpRet() illegal funcid=" << (int)funcid << std::endl);
				return (uint8_t)0;
		}
	}
	else
		LOGSTREAM("agreementscc",CCLOG_DEBUG1, stream << "not enough opret.[" << vopret.size() << "]" << std::endl);
	return (uint8_t)0;
}

// Validator for normal inputs in Agreements transactions.
bool ValidateAgreementsNormalVins(Eval* eval, const CTransaction& tx, int32_t index)
{
	for (int i=index;i<tx.vin.size();i++)
		if (IsCCInput(tx.vin[i].scriptSig) != 0 )
			return eval->Invalid("vin."+std::to_string(i)+" is normal for agreement tx!");
	return (true);
}

// Validator for CC inputs found in Agreements transactions.
bool ValidateAgreementsCCVin(struct CCcontract_info *cp,Eval* eval,const CTransaction& tx,int32_t index,int32_t prevVout,uint256 prevtxid,char* fromaddr,int64_t amount)
{
	CTransaction prevTx;
	uint256 hashblock;
	int32_t numvouts;
	char tmpaddr[64];

	// Check if a vin is a Agreements CC vin.
	if ((*cp->ismyvin)(tx.vin[index].scriptSig) == 0)
		return eval->Invalid("vin."+std::to_string(index)+" is agreement CC for agreement tx!");

	// Verify previous transaction and its op_return.
	else if (myGetTransaction(tx.vin[index].prevout.hash,prevTx,hashblock) == 0)
		return eval->Invalid("vin."+std::to_string(index)+" tx does not exist!");
	else if ((numvouts=prevTx.vout.size()) > 0 && DecodeAgreementOpRet(prevTx.vout[numvouts-1].scriptPubKey) == 0) 
		return eval->Invalid("invalid vin."+std::to_string(index)+" tx OP_RETURN data!");
	
	// If fromaddr != 0, validate prevout dest address.
	else if (fromaddr!=0 && Getscriptaddress(tmpaddr,prevTx.vout[tx.vin[index].prevout.n].scriptPubKey) && strcmp(tmpaddr,fromaddr)!=0)
		return eval->Invalid("invalid vin."+std::to_string(index)+" address!");

	// if amount > 0, validate amount.
	else if (amount>0 && prevTx.vout[tx.vin[index].prevout.n].nValue!=amount)
		return eval->Invalid("vin."+std::to_string(index)+" invalid amount!");
	
	// if prevVout >= 0, validate spent vout number.
	else if (prevVout>=0 && tx.vin[index].prevout.n!=prevVout)
		return eval->Invalid("vin."+std::to_string(index)+" invalid prevout number, expected "+std::to_string(prevVout)+", got "+std::to_string(tx.vin[index].prevout.n)+"!");

	// Validate previous txid.
	else if (prevTx.GetHash()!=prevtxid) 
		return eval->Invalid("invalid vin."+std::to_string(index)+" tx, different txid than expected!");
	
	return (true);
}

// Finds information from the accepted proposal of the specified accept transaction id.
// accepttxid must refer to a transaction with function id 'c', 'u' or 't'.
// Returns proposaltxid if info is successfully retrieved, zeroid if no transaction is found with the specified txid or the info was not successfully retrieved.
uint256 GetAcceptedProposalData(uint256 accepttxid, CPubKey &offerorpub, CPubKey &signerpub, CPubKey &arbitratorpub, int64_t &deposit, int64_t &disputefee, uint256 &refagreementtxid)
{
	uint8_t version, funcid;
	int64_t payment, depositcut;
	CTransaction accepttx, proposaltx;
	uint256 hashBlock, dummyuint256, proposaltxid;
	std::string dummystring;
	bool dummybool;

	// Find specified transaction.
	if (myGetTransaction(accepttxid, accepttx, hashBlock) && accepttx.vout.size() > 0 && 

	// Decode op_return, check function id.
	(funcid = DecodeAgreementOpRet(accepttx.vout.back().scriptPubKey)) != 0)
	{
		switch (funcid)
		{
			// Finding the proposaltxid for various types of transactions.
			case 'c':
				DecodeAgreementSigningOpRet(accepttx.vout.back().scriptPubKey,version,proposaltxid);
				break;
			case 'u':
				DecodeAgreementUpdateOpRet(accepttx.vout.back().scriptPubKey,version,dummyuint256,proposaltxid);
				break;
			case 't':
				DecodeAgreementCloseOpRet(accepttx.vout.back().scriptPubKey,version,dummyuint256,proposaltxid,depositcut);
				break;
			default:
				std::cerr << "GetAcceptedProposalData: incorrect accepttxid funcid "+std::to_string(funcid)+"" << std::endl;
				return zeroid;
		}

		// Finding the accepted proposal transaction.
		if (myGetTransaction(proposaltxid, proposaltx, hashBlock) && proposaltx.vout.size() > 0 &&

		// Decoding the accepted proposal transaction op_return and getting the data.
		DecodeAgreementProposalOpRet(proposaltx.vout.back().scriptPubKey,version,offerorpub,signerpub,
		dummystring,dummyuint256,deposit,payment,refagreementtxid,dummybool,arbitratorpub,disputefee) == 'p')
		{
			return proposaltxid;
		}
	}

	std::cerr << "GetAcceptedProposalData: couldn't find accepttxid transaction or its data" << std::endl;
	return zeroid;
}

// Finds the txid of the latest valid transaction that spent the event log baton for the specified agreement.
// Returns agreementtxid if event log baton is unspent, or zeroid if agreement with the specified txid couldn't be found.
// For use in validation code, bCheckBlockHeight is also set to true so this function doesn't return txids newer than the tx being validated, which caused old transactions to fail validation.
uint256 FindLatestAgreementEventTx(uint256 agreementtxid, struct CCcontract_info *cp, bool bCheckBlockHeight)
{
	CTransaction sourcetx, batontx;
	uint256 hashBlock, batontxid, refagreementtxid;
	int32_t vini, height, retcode;
	uint8_t funcid;
	char mutualCCaddress[65], arbCCaddress[65];
	CPubKey offerorpub, signerpub, arbitratorpub;
	int64_t deposit, disputefee;

	// Get agreement transaction and its op_return.
	if (myGetTransaction(agreementtxid, sourcetx, hashBlock) && sourcetx.vout.size() > 0 &&
	DecodeAgreementOpRet(sourcetx.vout.back().scriptPubKey) == 'c')
	{
		// Get all member pubkeys of the agreement.
		GetAcceptedProposalData(agreementtxid,offerorpub,signerpub,arbitratorpub,deposit,disputefee,refagreementtxid);

		// There are two types of valid event log batons: vouts sent to the offerorpub/signerpub CC 1of2 address with CC_MARKER_VALUE nValue, and vouts sent to the arbitratorpub's CC address.
		// Both of these types are vout.0.
		// To check if a vout is valid, we'll need to get the relevant addresses first.
		GetCCaddress1of2(cp, mutualCCaddress, offerorpub, signerpub);
		GetCCaddress(cp, arbCCaddress, arbitratorpub);

		// Iterate through vout0 batons while we're finding valid Agreements transactions that spent the last baton.
		while ((IsAgreementsvout(cp,sourcetx,0,mutualCCaddress) == CC_MARKER_VALUE || IsAgreementsvout(cp,sourcetx,0,arbCCaddress) != 0) &&
		
		// Check if vout0 was spent.
		(retcode = CCgetspenttxid(batontxid, vini, height, sourcetx.GetHash(), 0)) == 0 &&

		// Get spending transaction and its op_return.
		myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 && 
		(funcid = DecodeAgreementOpRet(batontx.vout.back().scriptPubKey)) != 0 &&
		
		// If bCheckBlockHeight is true, check if the blockheight of the batontx is below or equal to current block height.
		(!bCheckBlockHeight || komodo_blockheight(hashBlock) <= chainActive.Height()))
		{
			sourcetx = batontx;
		}

		return sourcetx.GetHash();
	}

	return zeroid;
}

// Function for validating proposal ('p' type) transactions.
// Returns true if proposal is valid, otherwise returns eval->Invalid().
bool ValidateAgreementProposalTx(struct CCcontract_info *cp,Eval* eval,const CTransaction& proposaltx,bool bStrict)
{
	CTransaction refagreementtx;
	uint8_t version;
	int32_t numvins, numvouts;
	int64_t payment, deposit, disputefee, dummyint64;
	CPubKey srcpub, destpub, arbitratorpub, offerorpub, signerpub, refarbitratorpub;
	uint256 agreementhash, refagreementtxid, hashBlock, dummyuint256;
	char mutualCCaddress[65], srcmarker[65], destmarker[65], arbmarker[65];
	bool bNewAgreement;
	std::string agreementname;

	LOGSTREAM("agreementscc", CCLOG_INFO, stream << "ValidateProposalTx: validating proposal essentials" << std::endl);

	// Boundary checks.
	numvins = proposaltx.vin.size();
	numvouts = proposaltx.vout.size();

	CCOpretCheck(eval,proposaltx,true,true,true);
	ExactAmounts(eval,proposaltx,ASSETCHAINS_CCZEROTXFEE[EVAL_AGREEMENTS]?0:CC_TXFEE);

	// Decoding data from last vout.
	if (DecodeAgreementProposalOpRet(proposaltx.vout[numvouts-1].scriptPubKey,version,srcpub,destpub,agreementname,
	agreementhash,deposit,payment,refagreementtxid,bNewAgreement,arbitratorpub,disputefee) != 'p')
		return eval->Invalid("Referenced proposal transaction data invalid!");

	// Verify that vin0 is a normal input and was signed by srcpub.
	else if (ValidateAgreementsNormalVins(eval, proposaltx, 0) == 0)
		return (false);
	else if (TotalPubkeyNormalInputs(proposaltx,srcpub) == 0)
		return eval->Invalid("Found no normal inputs signed by proposal's source pubkey!");

	GetCCaddress1of2(cp, mutualCCaddress, srcpub, destpub);

	// Verify that vout0 is an Agreements vout and is sent to srcpub/destpub 1of2 CC address.
	if (ConstrainVout(proposaltx.vout[0],1,mutualCCaddress,0)==0)
		return eval->Invalid("vout.0 is agreement CC baton vout to srcpub for 'p' type transaction!");

	// If bStrict is set to true, we also check the proposal data and other vouts.
	// This is to make sure the resulting agreement/update/etc is valid.
	if (bStrict)
	{
		LOGSTREAM("agreementscc", CCLOG_INFO, stream << "ValidateProposalTx: essentials validated, checking proposal data and vouts" << std::endl);

		if (payment < 0)
			return eval->Invalid("Required payment in the proposal must not be negative value!");

		else if (agreementhash == zeroid)
			return eval->Invalid("agreementhash must be specified in the proposal!");

		// srcpub and destpub cannot be the same.
		else if (srcpub == destpub)
			return eval->Invalid("Source pubkey and destination pubkey cannot be the same in proposal!");
		
		// If arbitratorpub is defined, it can't be the same as srcpub or destpub.
		else if (arbitratorpub.IsValid() && (arbitratorpub == srcpub || arbitratorpub == destpub))
			return eval->Invalid("Arbitrator pubkey cannot be the same as srcpub or destpub in proposal!");

		// If refagreementtxid is specified, make sure it is of a valid agreement transaction.
		if (refagreementtxid != zeroid)
		{
			if (myGetTransaction(refagreementtxid, refagreementtx, hashBlock) == 0 || refagreementtx.vout.size() == 0 ||
			DecodeAgreementOpRet(refagreementtx.vout.back().scriptPubKey) != 'c')
				return eval->Invalid("Couldn't find valid transaction for ref agreement specified in proposal!");
			
			// Get the agreement's offerorpub/signerpub/refarbitratorpub.
			else if (GetAcceptedProposalData(refagreementtxid,offerorpub,signerpub,refarbitratorpub,dummyint64,dummyint64,dummyuint256) == zeroid)
				return eval->Invalid("Couldn't obtain member pubkeys of ref agreement specified in proposal!");

			// Check if any pubkey specified in the proposal relates to any of the pubkeys specified in the ref agreement.
			else if (srcpub != offerorpub && srcpub != signerpub && destpub != offerorpub && destpub != signerpub)
				return eval->Invalid("Member pubkeys of ref agreement don't match any of the pubkeys specified in proposal!");
		}
		
		// agreementname must be specified, and it must be <= 64 chars.
		if (agreementname.empty() || agreementname.size() > 64)
			return eval->Invalid("agreementname must be specified in the proposal, and it must be <= 64 chars!");

		// If a proposal has bNewAgreement set, it is treated as a proposal to create a new agreement.
		// Otherwise, it is treated as a proposal to update an existing agreement (defined in refagreementtxid)
		if (bNewAgreement)
		{
			// If arbitratorpub is defined, disputefee must be at least 10000 sats.
			if (arbitratorpub.IsValid() && disputefee < CC_MARKER_VALUE)
				return eval->Invalid("disputefee must be >="+std::to_string(CC_MARKER_VALUE)+" satoshis if arbitrator is specified in proposal!");
			
			// Deposit must be at least 10000 sats.
			else if (deposit < CC_MARKER_VALUE)
				return eval->Invalid("deposit must be >="+std::to_string(CC_MARKER_VALUE)+" satoshis in proposal!");
		}
		else
		{
			// refagreementtxid must be specified.
			if (refagreementtxid == zeroid)
				return eval->Invalid("No agreement specified in proposal for agreement update!");

			else if (!((srcpub == offerorpub && destpub == signerpub) || (destpub == offerorpub && srcpub == signerpub)))
				return eval->Invalid("Pubkey signing the proposal doesn't match any member pubkeys of the agreement!");
			
			// If the accepted proposal for the ref agreement has arbitratorpub defined, this proposal must also have the same pubkey defined.
			else if (refarbitratorpub.IsValid() && arbitratorpub != refarbitratorpub)
				return eval->Invalid("arbitratorpub must be the same as refarbitratorpub in proposal for agreement update!");
		}
		
		GetCCaddress(cp, srcmarker, srcpub);
		GetCCaddress(cp, destmarker, destpub);
		if (arbitratorpub.IsValid())
			GetCCaddress(cp, arbmarker, arbitratorpub);

		// Check vout0 for CC_RESPONSE_VALUE.
		if (ConstrainVout(proposaltx.vout[0],1,mutualCCaddress,CC_RESPONSE_VALUE)==0)
			return eval->Invalid("vout.0 must be at least "+std::to_string(CC_RESPONSE_VALUE)+" satoshis for 'p' type transaction!");
		
		// vout1 must be a marker sent to srcpub CC address.
		else if (ConstrainVout(proposaltx.vout[1],1,srcmarker,CC_MARKER_VALUE)==0)
			return eval->Invalid("vout.1 is marker sent to srcpub CC address for 'p' type transaction!");
		
		// vout2 must be a marker sent to destpub CC address.
		else if (ConstrainVout(proposaltx.vout[2],1,destmarker,CC_MARKER_VALUE)==0)
			return eval->Invalid("vout.2 is marker sent to destpub CC address for 'p' type transaction!");
		
		// If arbitratorpub is specified, vout3 must be a marker sent to arbitratorpub CC address.
		if (arbitratorpub.IsValid())
		{
			if (numvouts < 5 || numvouts > 6)
				return eval->Invalid("Invalid number of vouts for 'p' type transaction with arbitrator!");
			else if (ConstrainVout(proposaltx.vout[3],1,arbmarker,CC_MARKER_VALUE)==0)
				return eval->Invalid("vout.3 is marker sent to arbitratorpub CC address for 'p' type transaction!");
		}
		else
		{
			if (numvouts < 4 || numvouts > 5)
				return eval->Invalid("Invalid number of vouts for 'p' type transaction without arbitrator!");
		}
	}

	LOGSTREAM("agreementscc", CCLOG_INFO, stream << "ValidateProposalTx: proposal tx validated" << std::endl);

	return (true);
}

bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	CTransaction proposaltx,agreementtx,disputetx,latesttx;
	CPubKey srcpub,destpub,cancelpub,arbitratorpub,offerorpub,signerpub;
	uint256 proposaltxid,hashBlock,batontxid,agreementhash,agreementtxid,refagreementtxid,disputetxid,disputeagreementtxid;
	int64_t deposit,payment,disputefee,depositcut,proposeddepositcut;
	int32_t numvins,numvouts,vini,height,retcode;
	uint8_t funcid,version,latestfuncid;
	std::string cancelinfo,agreementname,disputeinfo,resolutioninfo;
	bool bNewAgreement,bFinalDispute;
	char globalCCaddress[65],mutualCCaddress[65],srcCCaddress[65],srcnormaladdress[65],destCCaddress[65],destnormaladdress[65],arbCCaddress[65];

	// TBD: define CCcontract_info and variables here for integration with other modules.

	// Check boundaries, and verify that input/output amounts are exact.
	numvins = tx.vin.size();
	numvouts = tx.vout.size();
	if (numvouts < 1)
		return eval->Invalid("No vouts!");

	CCOpretCheck(eval,tx,true,true,true);
	ExactAmounts(eval,tx,ASSETCHAINS_CCZEROTXFEE[EVAL_AGREEMENTS]?0:CC_TXFEE);

	// Check the op_return of the transaction and fetch its function id.
	if ((funcid = DecodeAgreementOpRet(tx.vout[numvouts-1].scriptPubKey)) != 0)
	{
		//fprintf(stderr,"validating Agreements transaction type (%c)\n",funcid);

		GetCCaddress(cp, globalCCaddress, GetUnspendable(cp, NULL));

		switch (funcid)
		{
			case 'p':
				// Proposal creation:
				// vin.0: normal input
				// ...
				// vin.n-1: normal input
				// vout.0: response baton output to srcpub/destpub CC 1of2 address
				// vout.1: marker to srcpub CC address
				// vout.2: marker to destpub CC address
				// vout.3 (optional): marker to arbitratorpub CC address
				// vout.n-2: normal change
				// vout.n-1: OP_RETURN EVAL_AGREEMENTS 'p' version srcpub destpub agreementname agreementhash deposit payment refagreementtxid bNewAgreement arbitratorpub disputefee

				// Proposal transactions shouldn't trigger validation directly as they shouldn't contain CC inputs. 
				return eval->Invalid("unexpected AgreementsValidate for 'p' type transaction!");

			case 's':
				// Proposal cancellation:
				// vin.0: normal input
				// vin.1: baton input from proposal
				// ...
				// vin.n-1: normal input
				// vout.n-2: normal change
				// vout.n-1: OP_RETURN EVAL_AGREEMENTS 's' version proposaltxid srcpub cancelinfo

				// Get the data from the transaction's op_return.
				DecodeAgreementProposalCloseOpRet(tx.vout[numvouts-1].scriptPubKey,version,proposaltxid,cancelpub,cancelinfo);

				// Check cancelinfo length (should be <= 256 chars).
				if (cancelinfo.size() > 256)
					return eval->Invalid("cancelinfo string over 256 chars!");

				// Get the proposaltxid transaction.
				else if (myGetTransaction(proposaltxid,proposaltx,hashBlock) == 0 || proposaltx.vout.size() == 0 ||
				DecodeAgreementOpRet(proposaltx.vout.back().scriptPubKey) != 'p')
					return eval->Invalid("Cancelled proposal transaction not found for 's' type transaction!");

				// Validate the proposal transaction. (non-strict)
				else if (ValidateAgreementProposalTx(cp,eval,proposaltx,false) == 0)
					return (false);

				// Verify that the proposal hasn't been spent already.
				//else if ((retcode = CCgetspenttxid(batontxid,vini,height,proposaltxid,0)) == 0 && height <= chainActive.Height())
				//	return eval->Invalid("Cancelled proposal transaction has already been spent!");

				// Get the data from the proposaltxid transaction's op_return.
				DecodeAgreementProposalOpRet(proposaltx.vout.back().scriptPubKey,version,srcpub,destpub,
				agreementname,agreementhash,deposit,payment,refagreementtxid,bNewAgreement,arbitratorpub,disputefee);

				// Verify that this transaction was signed by either srcpub or destpub of specified proposal.
				if (cancelpub != srcpub && cancelpub != destpub)
					return eval->Invalid("Signing pubkey of proposal cancel transaction is not sender or recipient pubkey!");
				
				GetCCaddress1of2(cp, mutualCCaddress, srcpub, destpub);

				// Check vout boundaries for proposal cancel transactions.
				if (numvouts > 2)
					return eval->Invalid("Too many vouts for 's' type transaction!");

				// Verify that vin.0 was signed by cancelpub.
				else if (IsCCInput(tx.vin[0].scriptSig) != 0 || TotalPubkeyNormalInputs(tx,cancelpub) == 0)
					return eval->Invalid("vin.0 must be normal input signed by transaction creator pubkey!");

				// vin.1 is baton input from proposal
				// Verify that vin.1 was signed correctly.
				else if (ValidateAgreementsCCVin(cp,eval,tx,1,0,proposaltxid,mutualCCaddress,0) == 0)
					return (false);

				// Proposal cancels shouldn't have any additional CC inputs.
				else if (ValidateAgreementsNormalVins(eval,tx,2) == 0)
					return (false);
				
				// Proposal cancels shouldn't have any CC outputs.
				if (tx.vout[0].scriptPubKey.IsPayToCryptoCondition() != 0 || tx.vout[1].scriptPubKey.IsPayToCryptoCondition() != 0)
					return eval->Invalid("Proposal cancels shouldn't have any CC vouts!");
					
				break;
			
			case 'c':
				// Agreement creation:
				// vin.0: normal input
				// vin.1: baton input from proposal
				// ...
				// vin.n-1: normal input
				// vout.0: event log baton output to srcpub/destpub CC 1of2 address
				// vout.1: deposit / activity marker to global CC pubkey
				// vout.2: payment to proposal's srcpub normal address
				// vout.3: marker to proposal's srcpub CC address
				// vout.4: marker to proposal's destpub CC address
				// vout.5 (optional): marker to proposal's arbitratorpub CC address
				// vout.n-2: normal change
				// vout.n-1: OP_RETURN EVAL_AGREEMENTS 'c' version proposaltxid

				// Get the data from the transaction's op_return.
				DecodeAgreementSigningOpRet(tx.vout[numvouts-1].scriptPubKey,version,proposaltxid);

				// Get the proposaltxid transaction.
				if (myGetTransaction(proposaltxid,proposaltx,hashBlock) == 0 || proposaltx.vout.size() == 0 ||
				DecodeAgreementOpRet(proposaltx.vout.back().scriptPubKey) != 'p')
					return eval->Invalid("Accepted proposal transaction not found for 'c' type transaction!");

				// Validate the proposal transaction. (strict)
				else if (ValidateAgreementProposalTx(cp,eval,proposaltx,true) == 0)
					return (false);
				
				// Get the data from the proposaltxid transaction's op_return.
				DecodeAgreementProposalOpRet(proposaltx.vout.back().scriptPubKey,version,srcpub,destpub,
				agreementname,agreementhash,deposit,payment,refagreementtxid,bNewAgreement,arbitratorpub,disputefee);

				// Verify that the proposal is for a new agreement, not for updating an existing agreement.
				if (!bNewAgreement)
					return eval->Invalid("Accepted proposal is not for creating new agreement!");
				
				// Verify that the proposal hasn't been spent already.
				//else if ((retcode = CCgetspenttxid(batontxid, vini, height, proposaltxid, 0)) == 0 && height <= chainActive.Height())
				//	return eval->Invalid("Accepted proposal transaction has already been spent!");
				
				GetCCaddress1of2(cp, mutualCCaddress, srcpub, destpub);
				Getscriptaddress(srcnormaladdress,CScript() << ParseHex(HexStr(srcpub)) << OP_CHECKSIG);
				GetCCaddress(cp, srcCCaddress, srcpub);
				GetCCaddress(cp, destCCaddress, destpub);
				if(arbitratorpub.IsValid())
					GetCCaddress(cp, arbCCaddress, arbitratorpub);

				// Check vout boundaries for agreement creation transactions.
				if (numvouts < (arbitratorpub.IsValid() ? 7 : 6) || numvouts > (arbitratorpub.IsValid() ? 8 : 7))
					return eval->Invalid("Invalid number of vouts for 'c' type transaction!");
				
				// Verify that vin.0 was signed by proposal's destpub.
				else if (IsCCInput(tx.vin[0].scriptSig) != 0 || TotalPubkeyNormalInputs(tx,destpub) == 0)
					return eval->Invalid("vin.0 must be normal input signed by proposal's destination pubkey!");
				
				// Verify that vin.1 was signed correctly.
				else if (ValidateAgreementsCCVin(cp,eval,tx,1,0,proposaltxid,mutualCCaddress,CC_RESPONSE_VALUE) == 0)
					return (false);
				
				// Agreement creations shouldn't have any additional CC inputs.
				else if (ValidateAgreementsNormalVins(eval,tx,2) == 0)
					return (false);

				else if (ConstrainVout(tx.vout[0], 1, mutualCCaddress, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.0 must be CC baton to proposal's srcpub and destpub CC 1of2 address!");

				// Check if vout1 has correct value assigned. (deposit value specified in proposal)
				else if (ConstrainVout(tx.vout[1], 1, globalCCaddress, deposit) == 0)
					return eval->Invalid("vout.1 must be deposit to agreements global CC address!");

				// Check if vout2 has correct value assigned. (10000 sats + payment specified in proposal)
				else if (ConstrainVout(tx.vout[2], 0, srcnormaladdress, CC_MARKER_VALUE + payment) == 0)
					return eval->Invalid("vout.2 must be "+std::to_string(CC_MARKER_VALUE)+" sats + payment to proposal's srcpub normal address!");

				else if (ConstrainVout(tx.vout[3], 1, srcCCaddress, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.3 must be marker to proposal's srcpub CC address!");

				else if (ConstrainVout(tx.vout[4], 1, destCCaddress, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.4 must be marker to proposal's destpub CC address!");

				// If arbitratorpub is specified in the proposal, vout5 must be a marker to proposal's arbitratorpub CC address.
				else if (arbitratorpub.IsValid() && ConstrainVout(tx.vout[5], 1, arbCCaddress, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.5 must be marker to proposal's arbitratorpub CC address!");
				
				break;
			
			case 'u':
				// Agreement update:
				// vin.0: normal input
				// vin.1: baton input from latest agreement event log entry
				// vin.2: baton input from proposal
				// ...
				// vin.n-1: normal input
				// vout.0: event log baton output to srcpub/destpub CC 1of2 address
				// vout.1: payment to proposal's srcpub normal address
				// vout.n-2: normal change
				// vout.n-1: OP_RETURN EVAL_AGREEMENTS 'u' version agreementtxid proposaltxid

				// Get the data from the transaction's op_return.
				DecodeAgreementUpdateOpRet(tx.vout[numvouts-1].scriptPubKey,version,agreementtxid,proposaltxid);

				// Get the agreementtxid transaction.
				if (myGetTransaction(agreementtxid,agreementtx,hashBlock) == 0 || agreementtx.vout.size() == 0 ||
				DecodeAgreementOpRet(agreementtx.vout.back().scriptPubKey) != 'c')
					return eval->Invalid("Specified agreement not found for 'u' type transaction!");

				// Verify that the agreement is still active by checking if its deposit has been spent or not.
				else if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) == 0 && height <= chainActive.Height())
					return eval->Invalid("Agreement specified in update transaction is no longer active!");

				// Verify that the agreement is not currently suspended.
				if (myGetTransaction(FindLatestAgreementEventTx(agreementtxid,cp,true),latesttx,hashBlock) != 0 &&
				latesttx.vout.size() > 0 &&
				(latestfuncid = DecodeAgreementOpRet(latesttx.vout.back().scriptPubKey)) != 0)
				{
					if (latestfuncid == 'd')
						return eval->Invalid("Specified agreement is currently suspended, can't validate 'u' type transaction!");
				}
				else
					return eval->Invalid("Couldn't find latest event for specified agreement!");

				// Get the proposaltxid transaction.
				if (myGetTransaction(proposaltxid,proposaltx,hashBlock) == 0 || proposaltx.vout.size() == 0 ||
				DecodeAgreementOpRet(proposaltx.vout.back().scriptPubKey) != 'p')
					return eval->Invalid("Accepted proposal transaction not found for 'u' type transaction!");

				// Validate the proposal transaction. (strict)
				else if (ValidateAgreementProposalTx(cp,eval,proposaltx,true) == 0)
					return (false);

				// Get the data from the proposaltxid transaction's op_return.
				DecodeAgreementProposalOpRet(proposaltx.vout.back().scriptPubKey,version,srcpub,destpub,
				agreementname,agreementhash,deposit,payment,refagreementtxid,bNewAgreement,arbitratorpub,disputefee);

				if (refagreementtxid != agreementtxid)
					return eval->Invalid("Accepted proposal and update transaction are not referring to the same agreement!");
				
				// Verify that the proposal is for updating an existing agreement, not for a new agreement.
				else if (bNewAgreement)
					return eval->Invalid("Accepted proposal is not for updating existing agreement!");
				
				// Verifying deposit. (should be < 0 for agreement updates)
				else if (deposit >= 0)
					return eval->Invalid("Invalid deposit amount for 'u' type transaction, should be -1 or less!");
				
				// Verify that the proposal hasn't been spent already.
				//else if ((retcode = CCgetspenttxid(batontxid, vini, height, proposaltxid, 0)) == 0 && height <= chainActive.Height())
				//	return eval->Invalid("Accepted proposal transaction has already been spent!");
				
				GetCCaddress1of2(cp, mutualCCaddress, srcpub, destpub);
				Getscriptaddress(srcnormaladdress,CScript() << ParseHex(HexStr(srcpub)) << OP_CHECKSIG);

				// Check vout boundaries for agreement update transactions.
				if (numvouts < 3 || numvouts > 4)
					return eval->Invalid("Invalid number of vouts for 'u' type transaction!");

				// Verify that vin.0 was signed by proposal's destpub.
				else if (IsCCInput(tx.vin[0].scriptSig) != 0 || TotalPubkeyNormalInputs(tx,destpub) == 0)
					return eval->Invalid("vin.0 must be normal input signed by proposal's destination pubkey!");

				// Verify that vin.1 is spending the event log baton from the latest agreement event.
				else if (ValidateAgreementsCCVin(cp,eval,tx,1,0,FindLatestAgreementEventTx(agreementtxid,cp,true),mutualCCaddress,CC_MARKER_VALUE) == 0)
					return (false);

				// Verify that vin.2 was signed correctly & is spending the proposal response baton.
				else if (ValidateAgreementsCCVin(cp,eval,tx,2,0,proposaltxid,mutualCCaddress,CC_RESPONSE_VALUE) == 0)
					return (false);

				// Agreement creations shouldn't have any additional CC inputs.
				else if (ValidateAgreementsNormalVins(eval,tx,3) == 0)
					return (false);

				else if (ConstrainVout(tx.vout[0], 1, mutualCCaddress, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.0 must be CC baton to proposal's srcpub and destpub CC 1of2 address!");
				
				// Check if vout1 has correct value assigned. (10000 sats + payment specified in proposal)
				else if (ConstrainVout(tx.vout[1], 0, srcnormaladdress, CC_MARKER_VALUE + payment) == 0)
					return eval->Invalid("vout.1 must be "+std::to_string(CC_MARKER_VALUE)+" sats + payment to proposal's srcpub normal address!");

				break;

			case 't':
				// Agreement closure:
				// vin.0: normal input
				// vin.1: baton input from latest agreement event log entry
				// vin.2: baton input from proposal
				// vin.3: deposit from agreement
				// ...
				// vin.n-1: normal input
				// vout.0: depositcut + payment to proposal's srcpub normal address
				// vout.n-2: normal change + (deposit - depositcut)
				// vout.n-1: OP_RETURN EVAL_AGREEMENTS 't' version agreementtxid proposaltxid depositcut

				// Get the data from the transaction's op_return.
				DecodeAgreementCloseOpRet(tx.vout[numvouts-1].scriptPubKey,version,agreementtxid,proposaltxid,depositcut);

				// Get the agreementtxid transaction.
				if (myGetTransaction(agreementtxid,agreementtx,hashBlock) == 0 || agreementtx.vout.size() == 0 ||
				DecodeAgreementOpRet(agreementtx.vout.back().scriptPubKey) != 'c')
					return eval->Invalid("Specified agreement not found for 't' type transaction!");

				// Verify that the agreement is still active by checking if its deposit has been spent or not.
				else if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) == 0 && height <= chainActive.Height())
					return eval->Invalid("Agreement specified in closure transaction is no longer active!");

				// Verify that the agreement is not currently suspended.
				if (myGetTransaction(FindLatestAgreementEventTx(agreementtxid,cp,true),latesttx,hashBlock) != 0 &&
				latesttx.vout.size() > 0 &&
				(latestfuncid = DecodeAgreementOpRet(latesttx.vout.back().scriptPubKey)) != 0)
				{
					if (latestfuncid == 'd')
						return eval->Invalid("Specified agreement is currently suspended, can't validate 't' type transaction!");
				}
				else
					return eval->Invalid("Couldn't find latest event for specified agreement!");
				
				// Check value of the deposit vout, for depositcut checks later on.
				deposit = agreementtx.vout[1].nValue;

				// Get the proposaltxid transaction.
				if (myGetTransaction(proposaltxid,proposaltx,hashBlock) == 0 || proposaltx.vout.size() == 0 ||
				DecodeAgreementOpRet(proposaltx.vout.back().scriptPubKey) != 'p')
					return eval->Invalid("Accepted proposal transaction not found for 't' type transaction!");

				// Validate the proposal transaction. (strict)
				else if (ValidateAgreementProposalTx(cp,eval,proposaltx,true) == 0)
					return (false);

				// Get the data from the proposaltxid transaction's op_return.
				DecodeAgreementProposalOpRet(proposaltx.vout.back().scriptPubKey,version,srcpub,destpub,agreementname,
				agreementhash,proposeddepositcut,payment,refagreementtxid,bNewAgreement,arbitratorpub,disputefee);

				if (refagreementtxid != agreementtxid)
					return eval->Invalid("Accepted proposal and closure transaction are not referring to the same agreement!");
				
				// Verify that the proposal is for updating an existing agreement, not for a new agreement.
				else if (bNewAgreement)
					return eval->Invalid("Accepted proposal is not for updating existing agreement!");

				// Verify that the proposal hasn't been spent already.
				//else if ((retcode = CCgetspenttxid(batontxid, vini, height, proposaltxid, 0)) == 0 && height <= chainActive.Height())
				//	return eval->Invalid("Accepted proposal transaction has already been spent!");
				
				// Verify that depositcut is <= deposit, and that it matches with what's in the proposal.
				else if (depositcut != proposeddepositcut)
					return eval->Invalid("Deposit cut in 't' type transaction does not match the proposed deposit cut!");

				// Verifying deposit cut. (should be >= 0 for agreement closures)
				else if (depositcut < 0 || depositcut > deposit)
					return eval->Invalid("Invalid depositcut amount for 't' type transaction, should be between 0 and total deposit!");
				
				GetCCaddress1of2(cp, mutualCCaddress, srcpub, destpub);
				Getscriptaddress(srcnormaladdress,CScript() << ParseHex(HexStr(srcpub)) << OP_CHECKSIG);

				// Check vout boundaries for agreement closure transactions.
				if (numvouts < 2 || numvouts > 3)
					return eval->Invalid("Invalid number of vouts for 't' type transaction!");

				// Verify that vin.0 was signed by proposal's destpub.
				else if (IsCCInput(tx.vin[0].scriptSig) != 0 || TotalPubkeyNormalInputs(tx,destpub) == 0)
					return eval->Invalid("vin.0 must be normal input signed by proposal's destination pubkey!");

				// Verify that vin.1 is spending the event log baton from the latest agreement event.
				else if (ValidateAgreementsCCVin(cp,eval,tx,1,0,FindLatestAgreementEventTx(agreementtxid,cp,true),mutualCCaddress,CC_MARKER_VALUE) == 0)
					return (false);

				// Verify that vin.2 was signed correctly & is spending the proposal response baton.
				else if (ValidateAgreementsCCVin(cp,eval,tx,2,0,proposaltxid,mutualCCaddress,CC_RESPONSE_VALUE) == 0)
					return (false);

				// Verify that vin.3 is spending the deposit from the specified agreement.
				else if (ValidateAgreementsCCVin(cp,eval,tx,3,1,agreementtxid,globalCCaddress,deposit) == 0)
					return (false);

				// Agreement closures shouldn't have any additional CC inputs.
				else if (ValidateAgreementsNormalVins(eval,tx,4) == 0)
					return (false);

				// Check if vout0 has correct value assigned. (10000 sats + depositcut + payment value specified in proposal)
				else if (ConstrainVout(tx.vout[0], 0, srcnormaladdress, CC_MARKER_VALUE + depositcut + payment) == 0)
					return eval->Invalid("vout.0 must be "+std::to_string(CC_MARKER_VALUE)+" sats + depositcut + payment to proposal's srcpub normal address!");

				// NOTE: should there be a check to make sure vout1 has enough nValue to be at least deposit-depositcut?

				break;
			
			case 'd':
				// Agreement dispute:
				// vin.0: normal input
				// vin.1: baton input from latest agreement event log entry
				// ...
				// vin.n-1: normal input
				// vout.0: event log baton output + disputefee to arbitratorpub CC address
				// vout.n-2: normal change
				// vout.n-1: OP_RETURN EVAL_AGREEMENTS 'd' version agreementtxid srcpub destpub disputeinfo bFinalDispute

				// Get the data from the transaction's op_return.
				DecodeAgreementDisputeOpRet(tx.vout[numvouts-1].scriptPubKey,version,agreementtxid,srcpub,destpub,disputeinfo,bFinalDispute);

				// Check disputeinfo length (should be <= 256 chars).
				if (disputeinfo.size() > 256)
					return eval->Invalid("disputeinfo string over 256 chars!");

				// Get the agreementtxid transaction.
				else if (myGetTransaction(agreementtxid,agreementtx,hashBlock) == 0 || agreementtx.vout.size() == 0 ||
				DecodeAgreementOpRet(agreementtx.vout.back().scriptPubKey) != 'c')
					return eval->Invalid("Specified agreement not found for 'd' type transaction!");

				// Verify that the agreement is still active by checking if its deposit has been spent or not.
				else if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) == 0 && height <= chainActive.Height())
					return eval->Invalid("Agreement specified in dispute transaction is no longer active!");
				
				// Verify that the agreement is not currently suspended.
				if (myGetTransaction(FindLatestAgreementEventTx(agreementtxid,cp,true),latesttx,hashBlock) != 0 &&
				latesttx.vout.size() > 0 &&
				(latestfuncid = DecodeAgreementOpRet(latesttx.vout.back().scriptPubKey)) != 0)
				{
					if (latestfuncid == 'd')
						return eval->Invalid("Specified agreement is currently suspended, can't validate 'd' type transaction!");
				}
				else
					return eval->Invalid("Couldn't find latest event for specified agreement!");

				// Check if it contains an arbitrator pubkey. (aka if disputes are enabled)
				GetAcceptedProposalData(agreementtxid,offerorpub,signerpub,arbitratorpub,deposit,disputefee,refagreementtxid);
				if (!(arbitratorpub.IsValid()))
					return eval->Invalid("Agreement arbitrator pubkey invalid or unspecified, disputes are disabled!");

				// Get the agreement's offeror and signer pubkeys, check if they match the srcpub/destpub pubkeys.
				else if (!((srcpub == offerorpub && destpub == signerpub) || (destpub == offerorpub && srcpub == signerpub)))
					return eval->Invalid("Pubkey signing the dispute doesn't match any member pubkeys of the agreement!");

				GetCCaddress1of2(cp, mutualCCaddress, srcpub, destpub);
				GetCCaddress(cp, arbCCaddress, arbitratorpub);

				// Check vout boundaries for agreement dispute transactions.
				if (numvouts < 2 || numvouts > 3)
					return eval->Invalid("Invalid number of vouts for 'd' type transaction!");

				// Verify that vin.0 was signed by srcpub.
				else if (IsCCInput(tx.vin[0].scriptSig) != 0 || TotalPubkeyNormalInputs(tx,srcpub) == 0)
					return eval->Invalid("vin.0 must be normal input signed by dispute source pubkey!");

				// Verify that vin.1 is spending the event log baton from the latest agreement event.
				else if (ValidateAgreementsCCVin(cp,eval,tx,1,0,FindLatestAgreementEventTx(agreementtxid,cp,true),mutualCCaddress,CC_MARKER_VALUE) == 0)
					return (false);
					
				// Agreement disputes shouldn't have any additional CC inputs.
				else if (ValidateAgreementsNormalVins(eval,tx,2) == 0)
					return (false);

				// Check if vout0 has correct value assigned. (10000 sats + disputefee value specified in dispute)
				else if (ConstrainVout(tx.vout[0], 1, arbCCaddress, CC_MARKER_VALUE + disputefee) == 0)
					return eval->Invalid("vout.0 must be "+std::to_string(CC_MARKER_VALUE)+" sats + disputefee to agreement's arbitratorpub CC address!");

				break;

			case 'r':
				// Agreement dispute resolution:
				// vin.0: normal input
				// vin.1: baton input from latest agreement event log entry + disputefee
				// vin.2 (optional): deposit from agreement
				// ...
				// vin.n-1: normal input
				// if deposit is taken:
				// 	vout.0: depositcut to dispute's srcpub normal address, assuming depositcut != 0
				// 	vout.1: (deposit - depositcut) to dispute's destpub normal address, assuming depositcut != deposit
				// if deposit is depositcut < 0:
				// 	vout.0: event log baton output to srcpub/destpub CC 1of2 address
				// vout.n-2: normal change + disputefee
				// vout.n-1: OP_RETURN EVAL_AGREEMENTS 'r' version agreementtxid disputetxid depositcut resolutioninfo

				// Get the data from the transaction's op_return.
				DecodeAgreementDisputeResolveOpRet(tx.vout[numvouts-1].scriptPubKey,version,agreementtxid,disputetxid,depositcut,resolutioninfo);

				// Check resolutioninfo length (should be <= 256 chars).
				if (resolutioninfo.size() > 256)
					return eval->Invalid("resolutioninfo string over 256 chars!");

				// Get the agreement transaction and check if it contains an arbitrator pubkey. (aka if disputes are enabled)
				else if (myGetTransaction(agreementtxid,agreementtx,hashBlock) == 0 || agreementtx.vout.size() == 0 ||
				DecodeAgreementOpRet(agreementtx.vout.back().scriptPubKey) != 'c')
					return eval->Invalid("Specified agreement not found for 'r' type transaction!");

				// Verify that the agreement is still active by checking if its deposit has been spent or not.
				else if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) == 0 && height <= chainActive.Height())
					return eval->Invalid("Agreement specified in dispute transaction is no longer active!");

				// Get the agreement deposit value.
				GetAcceptedProposalData(agreementtxid,offerorpub,signerpub,arbitratorpub,deposit,disputefee,refagreementtxid);
				if (!(arbitratorpub.IsValid()))
					return eval->Invalid("Agreement arbitrator pubkey invalid or unspecified, disputes are disabled!");
				
				// Verify that depositcut <= deposit.
				else if (depositcut > deposit)
					return eval->Invalid("Invalid depositcut amount for 'r' type transaction, should be between 0 and total deposit!");

				// Get the dispute transaction.
				else if (myGetTransaction(disputetxid,disputetx,hashBlock) == 0 || disputetx.vout.size() == 0 ||
				DecodeAgreementOpRet(disputetx.vout.back().scriptPubKey) != 'd')
					return eval->Invalid("Specified dispute not found for 'r' type transaction!");

				// Verify that the dispute is valid. (points to correct agreementtxid, srcpub/destpub is correct etc.)
				else if (FindLatestAgreementEventTx(agreementtxid,cp,true) != disputetxid)
					return eval->Invalid("Specified dispute txid is not latest event for this agreement!");

				// Get dispute srcpub and destpub pubkeys.
				DecodeAgreementDisputeOpRet(disputetx.vout.back().scriptPubKey,version,disputeagreementtxid,srcpub,destpub,disputeinfo,bFinalDispute);

				if (disputeagreementtxid != agreementtxid)
					return eval->Invalid("Dispute and dispute resolution are referring to different agreement txids!");

				else if (!((srcpub == offerorpub && destpub == signerpub) || (destpub == offerorpub && srcpub == signerpub)))
					return eval->Invalid("Pubkey signing the dispute doesn't match any member pubkeys of the agreement!");

				// If depositcut < 0, then the deposit vout from the agreement must not be spent.
				else if (depositcut < 0 && bFinalDispute)
					return eval->Invalid("Cannot cancel final dispute for 'r' type transaction!");

				GetCCaddress1of2(cp, mutualCCaddress, srcpub, destpub);
				Getscriptaddress(srcnormaladdress,CScript() << ParseHex(HexStr(srcpub)) << OP_CHECKSIG);
				Getscriptaddress(destnormaladdress,CScript() << ParseHex(HexStr(destpub)) << OP_CHECKSIG);
				GetCCaddress(cp, arbCCaddress, arbitratorpub);
				
				// Check boundaries for dispute resolution transactions.
				if (numvouts < 2 || numvouts > 4)
					return eval->Invalid("Invalid number of vouts for 'r' type transaction!");
				
				// Verify that vin.0 was signed by arbitratorpub.
				else if (IsCCInput(tx.vin[0].scriptSig) != 0 || TotalPubkeyNormalInputs(tx,arbitratorpub) == 0)
					return eval->Invalid("vin.0 must be normal input signed by agreement arbitrator pubkey!");

				// Verify that vin.1 is spending the event log baton from the dispute.
				else if (ValidateAgreementsCCVin(cp,eval,tx,1,0,disputetxid,arbCCaddress,CC_MARKER_VALUE+disputefee) == 0)
					return (false);
				
				// If depositcut >= 0, verify that vin.2 is spending the deposit from the agreement.
				if (depositcut >= 0)
				{
					if (ValidateAgreementsCCVin(cp,eval,tx,2,1,agreementtxid,globalCCaddress,deposit) == 0)
						return (false);
					else if (ValidateAgreementsNormalVins(eval,tx,3) == 0)
						return (false);
				}

				// If depositcut < 0, make sure there are no additional CC inputs.
				else
				{
					if (ValidateAgreementsNormalVins(eval,tx,2) == 0)
						return (false);
				} 

				// If depositcut < 0, verify that vout0 (event log) was sent as CC output to correct address.
				if (depositcut < 0)
				{
					if (ConstrainVout(tx.vout[0], 1, mutualCCaddress, CC_MARKER_VALUE) == 0)
						return eval->Invalid("vout.0 must be CC baton to proposal's srcpub and destpub CC 1of2 address if depositcut < 0!");
				}

				// If depositcut == 0, check if vout0 value is equal to deposit and if it was sent to dispute's destpub normal address.
				else if (depositcut == 0)
				{
					if (ConstrainVout(tx.vout[0], 0, destnormaladdress, deposit) == 0)
						return eval->Invalid("vout.0 must be deposit to dispute destpub normal address if depositcut == 0!");
				}

				// If depositcut > 0, check if vout0 value is equal to depositcut and is sent to dispute's srcpub normal address.
				else if (depositcut > 0)
				{
					if (ConstrainVout(tx.vout[0], 0, srcnormaladdress, depositcut) == 0)
						return eval->Invalid("vout.0 must be depositcut to dispute srcpub normal address if depositcut > 0!");

					// If depositcut != deposit, check if vout1 is equal to (deposit - depositcut) and if it was sent to dispute's destpub normal address.
					if (depositcut != deposit)
					{
						if (ConstrainVout(tx.vout[1], 0, destnormaladdress, deposit - depositcut) == 0)
							return eval->Invalid("vout.1 must be (deposit - depositcut) to dispute srcpub normal address if depositcut > 0 & depositcut != deposit!");
					}
				}

				// NOTE: do we check to make sure arbitrator gets >= disputefee to their normal address?

				break;
			
			case 'n':
				// Agreement deposit unlock:
				// TBD
				// OP_RETURN EVAL_AGREEMENTS 'n' version agreementtxid unlocktxid
				return eval->Invalid("Unexpected Agreements 'n' function id, not built yet!");

			default:
				fprintf(stderr,"unexpected agreements funcid (%c)\n",funcid);
				return eval->Invalid("Unexpected Agreements function id!");
		}
	}
	else
		return eval->Invalid("Invalid Agreements function id and/or data!");

	LOGSTREAM("agreementscc", CCLOG_INFO, stream << "Agreements transaction validated" << std::endl);
	return (true);
}

// --- End of consensus code ---

// --- Helper functions for RPC implementations ---

// Function for getting the latest accepted proposaltxid of an agreement. Useful for finding data like latest agreement name or hash.
// Using FindLatestAgreementEventTx on its own for this is insufficient since the latest transaction won't necessarily have a proposaltxid.
// Returns latest proposaltxid if successful, otherwise returns zeroid.
uint256 FindLatestAcceptedProposal(uint256 agreementtxid, struct CCcontract_info *cp, std::string &latestname, uint256 &latesthash)
{
	CPubKey srcpub,destpub,arbitratorpub;
	CTransaction latesttx,proposaltx;
	uint8_t funcid,version;
	int64_t depositcut,deposit,payment,disputefee;
	uint256 latesttxid,hashBlock,refagreementtxid,tempagreementtxid,tempproposaltxid,agreementhash,proposaltxid = zeroid;
	std::string agreementname;
	bool bNewAgreement;

	// Get the latest agreement event.
	latesttxid = FindLatestAgreementEventTx(agreementtxid, cp, false);

	// While we iterate through valid Agreements transactions...
	while (myGetTransaction(latesttxid, latesttx, hashBlock) != 0 && latesttx.vout.size() > 0 &&
	(funcid = DecodeAgreementOpRet(latesttx.vout.back().scriptPubKey)) != 0)
	{
		switch (funcid)
		{
			case 'c':
				if (latesttx.GetHash() == agreementtxid)
					proposaltxid = latesttx.vin[1].prevout.hash;
				break;
			case 'u':
				DecodeAgreementUpdateOpRet(latesttx.vout.back().scriptPubKey,version,tempagreementtxid,tempproposaltxid);
				if (tempagreementtxid == agreementtxid)
					proposaltxid = tempproposaltxid;
				break;
			case 't':
				DecodeAgreementCloseOpRet(latesttx.vout.back().scriptPubKey,version,tempagreementtxid,tempproposaltxid,depositcut);
				if (tempagreementtxid == agreementtxid)
					proposaltxid = tempproposaltxid;
				break;
			case 'd': case 'r': // these types don't contain an agreement name & hash, so skip them and move on to the previous baton
				latesttxid = latesttx.vin[1].prevout.hash;
				break;
			default:
				LOGSTREAM("agreementscc", CCLOG_INFO, stream << "FindLatestAcceptedProposal: found invalid funcid "+std::to_string(funcid)+"" << std::endl);
				return zeroid;
		}
		if (proposaltxid != zeroid)
			break;
	}

	// If we found a valid latest proposaltxid, check if it's valid and return it.
	if (proposaltxid != zeroid &&
	myGetTransaction(proposaltxid, proposaltx, hashBlock) != 0 && proposaltx.vout.size() > 0 &&
	DecodeAgreementProposalOpRet(proposaltx.vout.back().scriptPubKey,version,srcpub,destpub,latestname,
	latesthash,deposit,payment,refagreementtxid,bNewAgreement,arbitratorpub,disputefee) == 'p')
	{
		return proposaltxid;
	}
		
	LOGSTREAM("agreementscc", CCLOG_INFO, stream << "FindLatestAcceptedProposal: found "+std::to_string(funcid)+" funcid, couldn't retrieve hash" << std::endl);
	return zeroid;
}

// --- RPC implementations for transaction creation ---

// Transaction constructor for agreementcreate rpc.
// Creates transaction with 'p' function id, with bNewAgreement set to true.
UniValue AgreementCreate(const CPubKey& pk,uint64_t txfee,CPubKey destpub,std::string agreementname,uint256 agreementhash,int64_t deposit,CPubKey arbitratorpub,int64_t disputefee,uint256 refagreementtxid,int64_t payment)
{
	CPubKey mypk,offerorpub,signerpub,refarbitratorpub;
	int64_t dummyint64;
	uint256 dummyuint256,hashBlock;
	CScript opret;
	CTransaction refagreementtx;

	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	// Checking passed parameters.
	if (!(destpub.IsFullyValid()))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Destination pubkey must be valid");
	if (agreementname.empty() || agreementname.size() > 64)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement name cannot be empty and must be up to 64 characters");
	if (agreementhash == zeroid)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement hash must be a valid sha256 hash");
	if (deposit < CC_MARKER_VALUE)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Required deposit must be at least "+std::to_string(CC_MARKER_VALUE)+" satoshis");
	if (payment < 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Required payment must be positive");
	
	// Check arbitrator and disputefee.
	if (arbitratorpub.IsFullyValid())
	{
		if (disputefee < CC_MARKER_VALUE)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Required dispute fee must be at least "+std::to_string(CC_MARKER_VALUE)+" satoshis when arbitrator is defined");
	}
	else
		disputefee = 0;
	
	// If refagreementtxid is specified, make sure it is of a valid agreement transaction.
	if (refagreementtxid != zeroid)
	{
		if (myGetTransaction(refagreementtxid,refagreementtx,hashBlock) == 0 || refagreementtx.vout.size() == 0 ||
		DecodeAgreementOpRet(refagreementtx.vout.back().scriptPubKey) != 'c')
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified reference agreement couldn't be found or is invalid");

		// Get the agreement's offerorpub/signerpub/refarbitratorpub.
		if (GetAcceptedProposalData(refagreementtxid,offerorpub,signerpub,refarbitratorpub,dummyint64,dummyint64,dummyuint256) == zeroid)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Couldn't obtain member pubkeys of specified reference agreement");

		// Check if any pubkey specified in the proposal relates to any of the pubkeys specified in the ref agreement.
		else if (mypk != offerorpub && mypk != signerpub && destpub != offerorpub && destpub != signerpub)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Member pubkeys of referenced agreement don't match any of the pubkeys specified in the proposal");
	}

	opret = EncodeAgreementProposalOpRet(AGREEMENTCC_VERSION,mypk,destpub,agreementname,agreementhash,deposit,payment,refagreementtxid,true,arbitratorpub,disputefee);

	if (AddNormalinputs2(mtx, txfee + CC_RESPONSE_VALUE + CC_MARKER_VALUE * 3, 60) > 0) // vin.*: normal input
	{
		// vout.0: response baton output to srcpub/destpub CC 1of2 address
		mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_RESPONSE_VALUE, mypk, destpub)); 
		// vout.1: marker to srcpub CC address
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, mypk)); 
		// vout.2: marker to destpub CC address
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, destpub)); 
		if (arbitratorpub.IsFullyValid())
			// vout.3 (optional): marker to arbitratorpub CC address
			mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, arbitratorpub)); 
	
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}

	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds");
}

// Transaction constructor for agreementupdate rpc. 
// Creates transaction with 'p' function id, with bNewAgreement set to false and deposit set to -1.
UniValue AgreementUpdate(const CPubKey& pk,uint64_t txfee,uint256 agreementtxid,uint256 agreementhash,std::string agreementname,int64_t payment)
{
	uint8_t latestfuncid;
	CPubKey mypk,destpub,offerorpub,signerpub,arbitratorpub;
	CTransaction agreementtx,latesttx;
	int64_t dummyint64;
	int32_t vini,height,retcode;
	uint256 hashBlock,batontxid,dummyuint256;
	CScript opret;
	std::string latestname;

	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	// Checking passed parameters.
	if (agreementname.size() > 64)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement name must be up to 64 characters");
	if (agreementhash == zeroid)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement hash must be a valid sha256 hash");
	if (payment < 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Required payment must be positive");
	
	// Find the specified agreement.
	if (myGetTransaction(agreementtxid,agreementtx,hashBlock) == 0 || agreementtx.vout.size() == 0 ||
	DecodeAgreementOpRet(agreementtx.vout.back().scriptPubKey) != 'c')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified agreement transaction not found or is invalid");

	// Check if agreement is still active, by checking if its deposit has been spent or not.
	else if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) == 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified agreement is no longer active");

	// Get the agreement's offerorpub/signerpub/arbitratorpub.
	GetAcceptedProposalData(agreementtxid,offerorpub,signerpub,arbitratorpub,dummyint64,dummyint64,dummyuint256);

	// Check if mypk is eligible, set destpub as the other member of the agreement.
	if (mypk != offerorpub && mypk != signerpub)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "You are not a member of this agreement");

	if (mypk == offerorpub) destpub = signerpub;
	else destpub = offerorpub;

	// If agreementname is unspecified, fetch the name from the latest accepted proposal.
	if (agreementname.empty())
	{
		if (FindLatestAcceptedProposal(agreementtxid,cp,latestname,dummyuint256) == zeroid)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Couldn't locate latest agreement name");
		agreementname = latestname;
	}

	opret = EncodeAgreementProposalOpRet(AGREEMENTCC_VERSION,mypk,destpub,agreementname,agreementhash,-1,payment,agreementtxid,false,arbitratorpub,0);

	if (AddNormalinputs2(mtx, txfee + CC_RESPONSE_VALUE + CC_MARKER_VALUE * 3, 60) > 0) // vin.*: normal input
	{
		// vout.0: response baton output to srcpub/destpub CC 1of2 address
		mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_RESPONSE_VALUE, mypk, destpub)); 
		// vout.1: marker to srcpub CC address
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, mypk)); 
		// vout.2: marker to destpub CC address
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, destpub)); 
		if (arbitratorpub.IsFullyValid())
			// vout.3 (optional): marker to arbitratorpub CC address
			mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, arbitratorpub)); 
	
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}

	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds");
}

// Transaction constructor for agreementclose rpc. 
// Creates transaction with 'p' function id, with bNewAgreement set to false and deposit set to user defined amount (indicating a proposal to close agreement).
UniValue AgreementClose(const CPubKey& pk,uint64_t txfee,uint256 agreementtxid,uint256 agreementhash,int64_t depositcut,std::string agreementname,int64_t payment)
{
	uint8_t latestfuncid;
	CPubKey mypk,destpub,offerorpub,signerpub,arbitratorpub;
	CTransaction agreementtx,latesttx;
	int64_t dummyint64,deposit;
	int32_t vini,height,retcode;
	uint256 hashBlock,batontxid,dummyuint256;
	CScript opret;
	std::string latestname;

	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	// Checking passed parameters.
	if (agreementname.size() > 64)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement name must be up to 64 characters");
	if (agreementhash == zeroid)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement hash must be a valid sha256 hash");
	if (depositcut < 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Required depositcut must be positive");
	if (payment < 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Required payment must be positive");
	
	// Find the specified agreement.
	if (myGetTransaction(agreementtxid,agreementtx,hashBlock) == 0 || agreementtx.vout.size() == 0 ||
	DecodeAgreementOpRet(agreementtx.vout.back().scriptPubKey) != 'c')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified agreement transaction not found or is invalid");

	// Check if agreement is still active, by checking if its deposit has been spent or not.
	else if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) == 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified agreement is no longer active");
	
	// Get the agreement's offerorpub/signerpub/arbitratorpub.
	GetAcceptedProposalData(agreementtxid,offerorpub,signerpub,arbitratorpub,deposit,dummyint64,dummyuint256);

	if (depositcut > deposit)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Required deposit cut cannot exceed total agreement deposit");
	
	// Check if mypk is eligible, set destpub as the other member of the agreement.
	if (mypk != offerorpub && mypk != signerpub)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "You are not a member of this agreement");

	if (mypk == offerorpub) destpub = signerpub;
	else destpub = offerorpub;

	// If agreementname is unspecified, fetch the name from the latest accepted proposal.
	if (agreementname.empty())
	{
		if (FindLatestAcceptedProposal(agreementtxid,cp,latestname,dummyuint256) == zeroid)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Couldn't locate latest agreement name");
		agreementname = latestname;
	}

	opret = EncodeAgreementProposalOpRet(AGREEMENTCC_VERSION,mypk,destpub,agreementname,agreementhash,depositcut,payment,agreementtxid,false,arbitratorpub,0);

	if (AddNormalinputs2(mtx, txfee + CC_RESPONSE_VALUE + CC_MARKER_VALUE * 3, 60) > 0) // vin.*: normal input
	{
		// vout.0: response baton output to srcpub/destpub CC 1of2 address
		mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_RESPONSE_VALUE, mypk, destpub)); 
		// vout.1: marker to srcpub CC address
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, mypk)); 
		// vout.2: marker to destpub CC address
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, destpub)); 
		if (arbitratorpub.IsFullyValid())
			// vout.3 (optional): marker to arbitratorpub CC address
			mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, arbitratorpub)); 
	
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}

	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds");
}

// Transaction constructor for agreementstopproposal rpc.
// Creates transaction with 's' function id.
UniValue AgreementStopProposal(const CPubKey& pk,uint64_t txfee,uint256 proposaltxid,std::string cancelinfo)
{
	uint8_t mypriv[32],version;
	CPubKey mypk,srcpub,destpub,arbitratorpub;
	CTransaction proposaltx;
	CScript opret;
	int64_t deposit,payment,disputefee;
	int32_t vini,height,retcode;
	uint256 agreementhash,refagreementtxid,hashBlock,batontxid;
	std::string agreementname;
	char mutualCCaddress[65];
	bool bNewAgreement;

	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	// Checking passed parameters.
	if (cancelinfo.size() > 256)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Cancel information must be up to 256 characters");
	
	// Find the specified proposal transaction, extract its data.
	if (myGetTransaction(proposaltxid,proposaltx,hashBlock) == 0 || proposaltx.vout.size() == 0 ||
	DecodeAgreementProposalOpRet(proposaltx.vout.back().scriptPubKey,version,srcpub,destpub,agreementname,
	agreementhash,deposit,payment,refagreementtxid,bNewAgreement,arbitratorpub,disputefee) != 'p')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified proposal transaction not found or is invalid");

	// Check if the proposal is still open, by checking if its response baton has been spent or not.
	else if ((retcode = CCgetspenttxid(batontxid, vini, height, proposaltxid, 0)) == 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified proposal is no longer open");

	// Check if mypk is eligible to cancel this proposal.
	if (mypk != srcpub && mypk != destpub)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Your pubkey is not eligible for closing this proposal");

	opret = EncodeAgreementProposalCloseOpRet(AGREEMENTCC_VERSION,proposaltxid,mypk,cancelinfo);

	if (AddNormalinputs2(mtx, txfee, 5) > 0) // vin.0 to vin.2+*: normal input
	{
		// vin.1: baton input from proposal
		GetCCaddress1of2(cp, mutualCCaddress, srcpub, destpub);
		mtx.vin.push_back(CTxIn(proposaltxid,0,CScript()));
		Myprivkey(mypriv);
		CCaddr1of2set(cp, srcpub, destpub, mypriv, mutualCCaddress);

		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}

	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds");
}

// Transaction constructor for agreementaccept rpc.
// Examines the user defined proposaltxid, and based on its parameters:
//  Creates transaction with 'c' function id, if bNewAgreement is set to true in the proposal.
//  Creates transaction with 'u' function id, if bNewAgreement is set to false and deposit is set to < 0 in the proposal.
//  Creates transaction with 't' function id, if bNewAgreement is set to false and deposit is set to >= 0 in the proposal.
UniValue AgreementAccept(const CPubKey& pk,uint64_t txfee,uint256 proposaltxid)
{
	CTransaction proposaltx,agreementtx,latesttx;
	uint256 hashBlock,agreementhash,agreementtxid,batontxid,refagreementtxid;
	CPubKey mypk,srcpub,destpub,arbitratorpub,offerorpub,signerpub;
	uint8_t version,mypriv[32],latestfuncid;
	int64_t deposit,payment,disputefee,totaldeposit;
	int32_t vini,height,retcode;
	std::string agreementname;
	bool bNewAgreement;
	char mutualCCaddress[65];
	CScript opret;

	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	// Find the specified proposal transaction, extract its data.
	if (myGetTransaction(proposaltxid,proposaltx,hashBlock) == 0 || proposaltx.vout.size() == 0 ||
	DecodeAgreementProposalOpRet(proposaltx.vout.back().scriptPubKey,version,srcpub,destpub,agreementname,
	agreementhash,deposit,payment,agreementtxid,bNewAgreement,arbitratorpub,disputefee) != 'p')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified proposal transaction not found or is invalid");

	// Check if the proposal is still open, by checking if its response baton has been spent or not.
	else if ((retcode = CCgetspenttxid(batontxid, vini, height, proposaltxid, 0)) == 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified proposal is no longer open");

	// Check if mypk is eligible to accept this proposal.
	if (mypk != destpub)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Your pubkey is not the destination pubkey of this proposal");

	// If bNewAgreement is true, create 'c' type transaction. (agreement creation)
	if (bNewAgreement)
	{
		if (deposit < CC_MARKER_VALUE)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Required deposit in proposal is too low");

		opret = EncodeAgreementSigningOpRet(AGREEMENTCC_VERSION,proposaltxid);

		if (AddNormalinputs2(mtx, txfee + payment + deposit, 60) > 0) // vin.0 to vin.2+*: normal input
		{
			// vin.1: baton input from proposal
			GetCCaddress1of2(cp, mutualCCaddress, srcpub, mypk);
			mtx.vin.push_back(CTxIn(proposaltxid,0,CScript()));
			Myprivkey(mypriv);
			CCaddr1of2set(cp, srcpub, mypk, mypriv, mutualCCaddress);

			// vout.0: event log baton output to srcpub/destpub CC 1of2 address
			mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, srcpub, mypk));
			// vout.1: deposit / activity marker to global CC pubkey
			mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, deposit, GetUnspendable(cp, NULL))); 
			// vout.2: payment to proposal's srcpub normal address
			mtx.vout.push_back(CTxOut(CC_MARKER_VALUE + payment, CScript() << ParseHex(HexStr(srcpub)) << OP_CHECKSIG));
			// vout.3: marker to proposal's srcpub CC address
			mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, srcpub)); 
			// vout.4: marker to proposal's destpub CC address
			mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, mypk)); 
			if (arbitratorpub.IsFullyValid())
				// vout.5 (optional): marker to proposal's arbitratorpub CC address
				mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, arbitratorpub)); 

			return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
		}

		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds");
	}

	// Find the specified agreement.
	if (myGetTransaction(agreementtxid,agreementtx,hashBlock) == 0 || agreementtx.vout.size() == 0 ||
	DecodeAgreementOpRet(agreementtx.vout.back().scriptPubKey) != 'c')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Referenced agreement transaction not found or is invalid");

	// Check if agreement is still active, by checking if its deposit has been spent or not.
	else if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) == 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Referenced agreement is no longer active");

	// Verify that the agreement is not currently suspended.
	if (myGetTransaction(FindLatestAgreementEventTx(agreementtxid,cp,true),latesttx,hashBlock) != 0 &&
	latesttx.vout.size() > 0 &&
	(latestfuncid = DecodeAgreementOpRet(latesttx.vout.back().scriptPubKey)) != 0)
	{
		if (latestfuncid == 'd')
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Referenced agreement is currently suspended");
	}
	else
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Couldn't find latest event for referenced agreement or latest event is unconfirmed");

	// Get the agreement's offerorpub/signerpub/arbitratorpub, and total deposit
	GetAcceptedProposalData(agreementtxid,offerorpub,signerpub,arbitratorpub,totaldeposit,disputefee,refagreementtxid);

	// Check if the proposal source/destination pubkeys match the offeror/signer pubkeys of the referenced agreement.
	if (!(srcpub == offerorpub && destpub == signerpub) && !(srcpub == signerpub && destpub == offerorpub))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Proposal source/destination pubkeys don't match offeror/signer pubkeys of referenced agreement");

	// If deposit in proposal is < 0, create 'u' type transaction. (agreement update)
	else if (deposit < 0)
	{
		opret = EncodeAgreementUpdateOpRet(AGREEMENTCC_VERSION,agreementtxid,proposaltxid);

		if (AddNormalinputs2(mtx, txfee + payment, 60) > 0) // vin.0 to vin.3+*: normal input
		{
			GetCCaddress1of2(cp, mutualCCaddress, srcpub, mypk);
			// vin.1: baton input from latest agreement event log entry
			mtx.vin.push_back(CTxIn(latesttx.GetHash(),0,CScript()));
			// vin.2: baton input from proposal
			mtx.vin.push_back(CTxIn(proposaltxid,0,CScript()));
			Myprivkey(mypriv);
			CCaddr1of2set(cp, srcpub, mypk, mypriv, mutualCCaddress);

			// vout.0: event log baton output to srcpub/destpub CC 1of2 address
			mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, srcpub, mypk));
			// vout.1: payment to proposal's srcpub normal address
			mtx.vout.push_back(CTxOut(CC_MARKER_VALUE + payment, CScript() << ParseHex(HexStr(srcpub)) << OP_CHECKSIG));

			return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
		}

		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds");
	}

	// If deposit in proposal is >= 0, create 't' type transaction. (agreement closure)
	else
	{
		if (deposit > totaldeposit)
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Required deposit cut cannot exceed total agreement deposit");

		opret = EncodeAgreementCloseOpRet(AGREEMENTCC_VERSION,agreementtxid,proposaltxid,deposit);

		if (AddNormalinputs2(mtx, txfee + payment, 60) > 0) // vin.0 to vin.4+*: normal input
		{
			GetCCaddress1of2(cp, mutualCCaddress, srcpub, mypk);
			// vin.1: baton input from latest agreement event log entry
			mtx.vin.push_back(CTxIn(latesttx.GetHash(),0,CScript()));
			// vin.2: baton input from proposal
			mtx.vin.push_back(CTxIn(proposaltxid,0,CScript()));
			// vin.3: deposit from agreement
			mtx.vin.push_back(CTxIn(agreementtxid,1,CScript()));
			Myprivkey(mypriv);
			CCaddr1of2set(cp, srcpub, mypk, mypriv, mutualCCaddress);

			// vout.0: depositcut + payment to proposal's srcpub normal address
			mtx.vout.push_back(CTxOut(CC_MARKER_VALUE + deposit + payment, CScript() << ParseHex(HexStr(srcpub)) << OP_CHECKSIG));

			return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
		}

		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds");
	}
}

// Transaction constructor for agreementdispute rpc.
// Creates transaction with 'd' function id.
UniValue AgreementDispute(const CPubKey& pk,uint64_t txfee,uint256 agreementtxid,std::string disputeinfo,bool bFinalDispute)
{
	CTransaction agreementtx,latesttx;
	CPubKey mypk,destpub,offerorpub,signerpub,arbitratorpub;
	int64_t deposit,disputefee;
	uint8_t version,mypriv[32],latestfuncid;
	int32_t vini,height,retcode;
	uint256 refagreementtxid,batontxid,hashBlock;
	char mutualCCaddress[65];
	CScript opret;

	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	// Checking passed parameters.
	if (disputeinfo.size() > 256)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Dispute information must be up to 256 characters");
	
	// Find the specified agreement.
	else if (myGetTransaction(agreementtxid,agreementtx,hashBlock) == 0 || agreementtx.vout.size() == 0 ||
	DecodeAgreementOpRet(agreementtx.vout.back().scriptPubKey) != 'c')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Referenced agreement transaction not found or is invalid");

	// Check if agreement is still active, by checking if its deposit has been spent or not.
	else if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) == 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Referenced agreement is no longer active");

	// Verify that the agreement is not currently suspended.
	if (myGetTransaction(FindLatestAgreementEventTx(agreementtxid,cp,true),latesttx,hashBlock) != 0 &&
	latesttx.vout.size() > 0 &&
	(latestfuncid = DecodeAgreementOpRet(latesttx.vout.back().scriptPubKey)) != 0)
	{
		if (latestfuncid == 'd')
			CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Referenced agreement is currently suspended");
	}
	else
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Couldn't find latest event for referenced agreement or latest event is unconfirmed");

	// Check if it contains an arbitrator pubkey. (aka if disputes are enabled)
	GetAcceptedProposalData(agreementtxid,offerorpub,signerpub,arbitratorpub,deposit,disputefee,refagreementtxid);
	if (!(arbitratorpub.IsValid()))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement arbitrator pubkey invalid or unspecified, disputes are disabled");

	// Check if mypk is eligible for creating a dispute.
	if (mypk != offerorpub && mypk != signerpub)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Your pubkey is not eligible for opening a dispute for this agreement");

	if (mypk == offerorpub) destpub = signerpub;
	else destpub = offerorpub;

	opret = EncodeAgreementDisputeOpRet(AGREEMENTCC_VERSION,agreementtxid,mypk,destpub,disputeinfo,bFinalDispute);
	
	if (AddNormalinputs2(mtx, txfee + disputefee, 60) > 0) // vin.2+*: normal input
	{
		GetCCaddress1of2(cp, mutualCCaddress, destpub, mypk);
		// vin.1: baton input from latest agreement event log entry
		mtx.vin.push_back(CTxIn(latesttx.GetHash(),0,CScript()));
		Myprivkey(mypriv);
		CCaddr1of2set(cp, destpub, mypk, mypriv, mutualCCaddress);
		
		// vout.0: event log baton output + disputefee to arbitratorpub CC address
		mtx.vout.push_back(MakeCC1vout(EVAL_AGREEMENTS, CC_MARKER_VALUE + disputefee, arbitratorpub));
		
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}

	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds");
}

// Transaction constructor for agreementresolve rpc.
// Creates transaction with 'r' function id.
UniValue AgreementResolve(const CPubKey& pk,uint64_t txfee,uint256 disputetxid,int64_t depositcut,std::string resolutioninfo)
{
	CTransaction disputetx,agreementtx;
	CPubKey mypk,offerorpub,signerpub,arbitratorpub,srcpub,destpub;
	int64_t deposit,disputefee;
	uint8_t version;
	uint256 hashBlock,agreementtxid,refagreementtxid,batontxid;
	int32_t vini,height,retcode;
	CScript opret;
	std::string disputeinfo;
	char arbCCaddress[65];
	bool bFinalDispute;

	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	// Checking passed parameters.
	if (resolutioninfo.size() > 256)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Dispute resolution info must be up to 256 characters");

	// Get the dispute transaction.
	else if (myGetTransaction(disputetxid,disputetx,hashBlock) == 0 || disputetx.vout.size() == 0 ||
	DecodeAgreementOpRet(disputetx.vout.back().scriptPubKey) != 'd')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Referenced dispute transaction not found or is invalid");

	// Get dispute srcpub and destpub pubkeys.
	DecodeAgreementDisputeOpRet(disputetx.vout.back().scriptPubKey,version,agreementtxid,srcpub,destpub,disputeinfo,bFinalDispute);
	
	// Find the specified agreement.
	if (myGetTransaction(agreementtxid,agreementtx,hashBlock) == 0 || agreementtx.vout.size() == 0 ||
	DecodeAgreementOpRet(agreementtx.vout.back().scriptPubKey) != 'c')
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement from dispute not found or is invalid");

	// Check if agreement is still active, by checking if its deposit has been spent or not.
	else if ((retcode = CCgetspenttxid(batontxid, vini, height, agreementtxid, 1)) == 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement from dispute is no longer active");
	
	// Verify that the dispute is valid. (points to correct agreementtxid, srcpub/destpub is correct etc.)
	else if (FindLatestAgreementEventTx(agreementtxid,cp,true) != disputetxid)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Referenced dispute is not the latest event for the related agreement, or is unconfirmed");

	// Get the agreement data.
	GetAcceptedProposalData(agreementtxid,offerorpub,signerpub,arbitratorpub,deposit,disputefee,refagreementtxid);
	if (!(arbitratorpub.IsValid()))
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Agreement arbitrator pubkey invalid or unspecified, disputes are disabled");
	
	else if (mypk != arbitratorpub)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "You are not the arbitrator of the specified agreement");
	
	// Verify that depositcut <= deposit.
	else if (depositcut > deposit)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Deposit cut cannot exceed total deposit amount");
	
	// If depositcut < 0, then the deposit vout from the agreement must not be spent.
	else if (depositcut < 0 && bFinalDispute)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Cannot cancel dispute marked as final");
	
	opret = EncodeAgreementDisputeResolveOpRet(AGREEMENTCC_VERSION,agreementtxid,disputetxid,depositcut,resolutioninfo);
	
	if (AddNormalinputs2(mtx, txfee, 5) > 0) // vin.2/3+*: normal input
	{
		GetCCaddress(cp, arbCCaddress, mypk);
		// vin.1: baton input from latest agreement event log entry + disputefee
		mtx.vin.push_back(CTxIn(disputetxid,0,CScript()));
		if (depositcut >= 0) // if deposit was taken...
		{
			// vin.2 (optional): deposit from agreement
			mtx.vin.push_back(CTxIn(agreementtxid,1,CScript()));

			// vout.0: depositcut to dispute's srcpub normal address, assuming depositcut != 0
			if (depositcut > 0)
				mtx.vout.push_back(CTxOut(depositcut, CScript() << ParseHex(HexStr(srcpub)) << OP_CHECKSIG));
			// vout.1: (deposit - depositcut) to dispute's destpub normal address, assuming depositcut != deposit
			if (depositcut != deposit)
				mtx.vout.push_back(CTxOut((deposit - depositcut), CScript() << ParseHex(HexStr(destpub)) << OP_CHECKSIG));
		}
		else // if deposit was not taken...
		{
			// vout.0: event log baton output to srcpub/destpub CC 1of2 address
			mtx.vout.push_back(MakeCC1of2vout(EVAL_AGREEMENTS, CC_MARKER_VALUE, srcpub, destpub));
		}
		
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}

	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds");
}

// Transaction constructor for agreementunlock rpc.
// Creates transaction with 'n' function id.
UniValue AgreementUnlock(const CPubKey& pk,uint64_t txfee,uint256 agreementtxid,uint256 unlocktxid)
{
	// TBD
	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "agreementunlock not done yet");
}

// --- RPC implementations for transaction analysis ---

UniValue AgreementInfo(uint256 txid)
{
	UniValue result(UniValue::VOBJ);
	char str[67];
	uint256 hashBlock,proposaltxid,agreementhash,agreementtxid,disputetxid,refagreementtxid,batontxid,latesttxid,latesthash;
	uint8_t version,funcid,batonfuncid,latestfuncid;
	CPubKey srcpub,destpub,arbitratorpub,offerorpub,signerpub;
	CTransaction tx,batontx,proposaltx,latesttx;
	std::string agreementname,latestname,cancelinfo,disputeinfo,resolutioninfo;
	int32_t retcode,vini,height,numvouts;
	int64_t deposit,depositcut,payment,disputefee;
	bool bNewAgreement,bFinalDispute;

	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);

	if (myGetTransaction(txid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
	(funcid = DecodeAgreementOpRet(tx.vout[numvouts-1].scriptPubKey)) != 0)
	{
		result.push_back(Pair("result","success"));
		result.push_back(Pair("txid",txid.GetHex()));

		switch (funcid)
		{
			case 'p': // proposal
				result.push_back(Pair("type","proposal"));
				DecodeAgreementProposalOpRet(tx.vout[numvouts-1].scriptPubKey,version,srcpub,destpub,agreementname,
				agreementhash,deposit,payment,refagreementtxid,bNewAgreement,arbitratorpub,disputefee);

				result.push_back(Pair("source_pubkey",pubkey33_str(str,(uint8_t *)&srcpub)));
				result.push_back(Pair("destination_pubkey",pubkey33_str(str,(uint8_t *)&destpub)));
				
				if (bNewAgreement)
				{
					result.push_back(Pair("proposal_type","agreement_create"));
					result.push_back(Pair("proposed_agreement_name",agreementname));
					result.push_back(Pair("proposed_agreement_hash",agreementhash.GetHex()));
					result.push_back(Pair("required_deposit",(double)deposit/COIN));

					if (payment > 0)
						result.push_back(Pair("required_payment",(double)payment/COIN));
					
					if (arbitratorpub.IsValid())
					{
						result.push_back(Pair("disputes_enabled","true"));
						result.push_back(Pair("arbitrator_pubkey",pubkey33_str(str,(uint8_t *)&arbitratorpub)));
						result.push_back(Pair("dispute_fee",(double)disputefee/COIN));
					}
					else
						result.push_back(Pair("disputes_enabled","false"));
					
					if (refagreementtxid != zeroid)
						result.push_back(Pair("reference_agreement",refagreementtxid.GetHex()));
				}
				else
				{
					if (deposit < 0)
						result.push_back(Pair("proposal_type","agreement_update"));
					else
						result.push_back(Pair("proposal_type","agreement_close"));

					result.push_back(Pair("reference_agreement",refagreementtxid.GetHex()));

					result.push_back(Pair("proposed_agreement_name",agreementname));
					result.push_back(Pair("proposed_agreement_hash",agreementhash.GetHex()));
					
					if (deposit >= 0)
						result.push_back(Pair("deposit_cut_requested",(double)deposit/COIN));
					if (payment > 0)
						result.push_back(Pair("required_payment",(double)payment/COIN));
				}

				// Check status and spending txid.
				if ((retcode = CCgetspenttxid(batontxid,vini,height,txid,0)) == 0 &&
				myGetTransaction(batontxid,batontx,hashBlock) != 0 && batontx.vout.size() > 0 &&
				(batonfuncid = DecodeAgreementOpRet(batontx.vout.back().scriptPubKey)) != 0)
				{
					if (batonfuncid == 'c' || batonfuncid == 'u' || batonfuncid == 't')
						result.push_back(Pair("status","accepted"));
					else if (batonfuncid == 's')
						result.push_back(Pair("status","cancelled"));
					
					result.push_back(Pair("status_txid",batontxid.GetHex()));
				}
				else
					result.push_back(Pair("status","active"));
				
				break;
			case 's': // proposal cancel
				result.push_back(Pair("type","proposal_cancel"));
				DecodeAgreementProposalCloseOpRet(tx.vout[numvouts-1].scriptPubKey,version,proposaltxid,srcpub,cancelinfo);
				
				result.push_back(Pair("cancelled_proposal",proposaltxid.GetHex()));
				result.push_back(Pair("source_pubkey",pubkey33_str(str,(uint8_t *)&srcpub)));
				if (!(cancelinfo.empty()))
					result.push_back(Pair("cancel_info",cancelinfo));
				
				break;
			case 'c': // agreement
				result.push_back(Pair("type","agreement"));
				proposaltxid = GetAcceptedProposalData(txid,offerorpub,signerpub,arbitratorpub,deposit,disputefee,refagreementtxid);

				FindLatestAcceptedProposal(txid,cp,latestname,latesthash);
				result.push_back(Pair("latest_name",latestname));
				result.push_back(Pair("latest_hash",latesthash.GetHex()));
				
				result.push_back(Pair("offeror_pubkey",pubkey33_str(str,(uint8_t *)&offerorpub)));
				result.push_back(Pair("signer_pubkey",pubkey33_str(str,(uint8_t *)&signerpub)));
				if (arbitratorpub.IsValid())
				{
					result.push_back(Pair("arbitrator_pubkey",pubkey33_str(str,(uint8_t *)&arbitratorpub)));
					result.push_back(Pair("dispute_fee",(double)disputefee/COIN));
				}

				result.push_back(Pair("deposit",(double)deposit/COIN));

				// Get latest agreement event.
				latesttxid = FindLatestAgreementEventTx(txid,cp,false);
				if (myGetTransaction(latesttxid,latesttx,hashBlock) != 0)
					latestfuncid = DecodeAgreementOpRet(latesttx.vout.back().scriptPubKey);
				
				// Check agreement status.
				if ((retcode = CCgetspenttxid(batontxid, vini, height, txid, 1)) == 0)
				{
					if (latestfuncid == 'r')
						result.push_back(Pair("status","arbitrated"));
					else
						result.push_back(Pair("status","closed"));
				}
				else
				{
					if (latestfuncid == 'd')
						result.push_back(Pair("status","suspended"));
					else
						result.push_back(Pair("status","active"));
				}
				
				if (latesttxid != zeroid && latesttxid != txid)
					result.push_back(Pair("latest_event_txid",latesttxid.GetHex()));

				result.push_back(Pair("accepted_proposal",proposaltxid.GetHex()));
				if (refagreementtxid != zeroid)
					result.push_back(Pair("reference_agreement",refagreementtxid.GetHex()));
				
				break;
			case 'u': // update
				result.push_back(Pair("type","agreement_update"));
				proposaltxid = GetAcceptedProposalData(txid,offerorpub,signerpub,arbitratorpub,deposit,disputefee,agreementtxid);
				result.push_back(Pair("accepted_proposal",proposaltxid.GetHex()));
				result.push_back(Pair("agreement_txid",agreementtxid.GetHex()));

				if (myGetTransaction(proposaltxid,proposaltx,hashBlock) != 0 && 
				DecodeAgreementProposalOpRet(proposaltx.vout.back().scriptPubKey,version,srcpub,destpub,latestname,
				latesthash,deposit,payment,refagreementtxid,bNewAgreement,arbitratorpub,disputefee) == 'p')
				{
					result.push_back(Pair("new_name",latestname));
					result.push_back(Pair("new_hash",latesthash.GetHex()));
				}

				result.push_back(Pair("source_pubkey",pubkey33_str(str,(uint8_t *)&offerorpub)));
				result.push_back(Pair("destination_pubkey",pubkey33_str(str,(uint8_t *)&signerpub)));

				break;
			case 't': // close
				result.push_back(Pair("type","agreement_closure"));

				proposaltxid = GetAcceptedProposalData(txid,offerorpub,signerpub,arbitratorpub,depositcut,disputefee,agreementtxid);
				result.push_back(Pair("accepted_proposal",proposaltxid.GetHex()));
				result.push_back(Pair("agreement_txid",agreementtxid.GetHex()));

				if (myGetTransaction(proposaltxid,proposaltx,hashBlock) != 0 && 
				DecodeAgreementProposalOpRet(proposaltx.vout.back().scriptPubKey,version,srcpub,destpub,latestname,
				latesthash,deposit,payment,refagreementtxid,bNewAgreement,arbitratorpub,disputefee) == 'p')
				{
					result.push_back(Pair("new_name",latestname));
					result.push_back(Pair("new_hash",latesthash.GetHex()));
				}

				result.push_back(Pair("source_pubkey",pubkey33_str(str,(uint8_t *)&offerorpub)));
				result.push_back(Pair("destination_pubkey",pubkey33_str(str,(uint8_t *)&signerpub)));

				GetAcceptedProposalData(agreementtxid,offerorpub,signerpub,arbitratorpub,deposit,disputefee,refagreementtxid);

				result.push_back(Pair("deposit_cut_to_offeror",(double)depositcut/COIN));
				result.push_back(Pair("deposit_cut_to_signer",(double)(deposit-depositcut)/COIN));
				result.push_back(Pair("total_deposit",(double)deposit/COIN));

				break;
			case 'd': // dispute
				result.push_back(Pair("type","agreement_dispute"));
				DecodeAgreementDisputeOpRet(tx.vout[numvouts-1].scriptPubKey,version,agreementtxid,srcpub,destpub,disputeinfo,bFinalDispute);
				result.push_back(Pair("agreement_txid",agreementtxid.GetHex()));

				result.push_back(Pair("claimant_pubkey",pubkey33_str(str,(uint8_t *)&srcpub)));
				result.push_back(Pair("defendant_pubkey",pubkey33_str(str,(uint8_t *)&destpub)));
				
				result.push_back(Pair("is_final_dispute",bFinalDispute ? "true" : "false"));

				// Check dispute status.
				if ((retcode = CCgetspenttxid(batontxid, vini, height, txid, 0)) == 0)
				{
					result.push_back(Pair("is_resolved","true"));
					result.push_back(Pair("resolution_txid",batontxid.GetHex()));
				}
				else
					result.push_back(Pair("is_resolved","false"));
				
				if (!(disputeinfo.empty()))
					result.push_back(Pair("dispute_info",disputeinfo));

				break;
			case 'r': // resolution
				result.push_back(Pair("type","dispute_resolution"));
				DecodeAgreementDisputeResolveOpRet(tx.vout[numvouts-1].scriptPubKey,version,agreementtxid,disputetxid,depositcut,resolutioninfo);
				result.push_back(Pair("agreement_txid",agreementtxid.GetHex()));
				result.push_back(Pair("resolved_dispute",disputetxid.GetHex()));

				GetAcceptedProposalData(agreementtxid,offerorpub,signerpub,arbitratorpub,deposit,disputefee,refagreementtxid);
				
				if (depositcut >= 0)
				{
					result.push_back(Pair("agreement_closed","true"));
					result.push_back(Pair("deposit_cut_to_claimant",(double)depositcut/COIN));
					result.push_back(Pair("deposit_cut_to_defendant",(double)(deposit-depositcut)/COIN));
					result.push_back(Pair("total_deposit",(double)deposit/COIN));
				}
				else
					result.push_back(Pair("agreement_closed","false"));
				
				break;
			//case 'n': // unlock
			//	result.push_back(Pair("type","agreement_unlock"));
			//	break;
		}
		return (result);
	}

	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "invalid Agreements transaction id");
}

UniValue AgreementEventLog(uint256 agreementtxid,uint8_t flags,int64_t samplenum,bool bReverse)
{
	UniValue result(UniValue::VARR);
	CTransaction agreementtx,batontx;
	uint256 latesttxid,batontxid,hashBlock;
	int64_t total = 0LL;
	int32_t numvouts,vini,height;
	uint8_t funcid;

	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);

	if (myGetTransaction(agreementtxid,agreementtx,hashBlock) != 0 && (numvouts = agreementtx.vout.size()) > 0 &&
	DecodeAgreementOpRet(agreementtx.vout[numvouts-1].scriptPubKey) == 'c')
	{
		latesttxid = FindLatestAgreementEventTx(agreementtxid,cp,true);

		if (latesttxid != agreementtxid)
		{
			if (bReverse) // from latest event to oldest
				batontxid = latesttxid;
			else
				batontxid = agreementtxid;
			
			// Iterate through events while we haven't reached samplenum limits yet.
			while ((total < samplenum || samplenum == 0) &&
			// Fetch the transaction.
			myGetTransaction(batontxid,batontx,hashBlock) != 0 && batontx.vout.size() > 0 &&
			// Fetch function id.
			(funcid = DecodeAgreementOpRet(batontx.vout.back().scriptPubKey)) != 0)
			{
				switch(funcid)
				{
					case 'u':
						if (flags & ASF_UPDATES) result.push_back(batontxid.GetHex());
						break;
					case 't':
						if (flags & ASF_CLOSURES) result.push_back(batontxid.GetHex());
						break;
					case 'd':
						if (flags & ASF_DISPUTES) result.push_back(batontxid.GetHex());
						break;
					case 'r':
						if (flags & ASF_RESOLUTIONS) result.push_back(batontxid.GetHex());
						break;
				}
				
				if (batontxid != agreementtxid) total++;

				// If bReverse = true, stop searching if we found the original agreement txid. 
				// If bReverse = false, stop searching if we found the latest event txid. 
				if ((!bReverse && batontxid == latesttxid) || (bReverse && batontxid == agreementtxid))
					break;

				// Get previous or next event transaction.
				if (bReverse)
					batontxid = batontx.vin[1].prevout.hash;
				else
					CCgetspenttxid(batontxid,vini,height,batontx.GetHash(),0);
			}
		}
	}

	return(result);
}

UniValue AgreementReferences(const CPubKey& pk,uint256 agreementtxid)
{
	UniValue result(UniValue::VARR);
	CTransaction agreementtx,tx;
	CPubKey mypk,offerorpub,signerpub,arbitratorpub;
	uint256 txid,hashBlock,refagreementtxid;
	std::vector<uint256> txids;
	int64_t deposit,payment,disputefee;
	int32_t numvouts;
	char myCCaddr[65];

	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);
    mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());

	GetCCaddress(cp,myCCaddr,mypk);

	if (myGetTransaction(agreementtxid,agreementtx,hashBlock) != 0 && (numvouts = agreementtx.vout.size()) > 0 &&
	DecodeAgreementOpRet(agreementtx.vout[numvouts-1].scriptPubKey) == 'c')
	{
		SetCCtxids(txids,myCCaddr,true,EVAL_AGREEMENTS,CC_MARKER_VALUE,zeroid,'c');
		for (std::vector<uint256>::const_iterator it=txids.begin(); it!=txids.end(); it++)
		{
			txid = *it;
			if (myGetTransaction(txid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
			DecodeAgreementOpRet(tx.vout[numvouts-1].scriptPubKey) == 'c')
			{
				GetAcceptedProposalData(txid,offerorpub,signerpub,arbitratorpub,deposit,disputefee,refagreementtxid);
				if (refagreementtxid == agreementtxid)
					result.push_back(txid.GetHex());
			}
		}
	}
	
	return(result);
}

UniValue AgreementList(const CPubKey& pk,uint8_t flags,uint256 filtertxid)
{
	UniValue result(UniValue::VARR);
	CTransaction tx;
	CPubKey mypk,srcpub,destpub,arbitratorpub;
	uint256 txid,hashBlock,agreementhash,agreementtxid;
	uint8_t version;
	std::string agreementname;
	std::vector<uint256> proposaltxids,agreementtxids;
	int64_t deposit,payment,disputefee;
	int32_t numvouts;
	char myCCaddr[65];
	bool bNewAgreement;

	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_AGREEMENTS);
    mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());

	GetCCaddress(cp,myCCaddr,mypk);

	if (flags & ASF_PROPOSALS)
	{
		SetCCtxids(proposaltxids,myCCaddr,true,EVAL_AGREEMENTS,CC_MARKER_VALUE,zeroid,'p');
		for (std::vector<uint256>::const_iterator it=proposaltxids.begin(); it!=proposaltxids.end(); it++)
		{
			txid = *it;
			if (myGetTransaction(txid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
			DecodeAgreementOpRet(tx.vout[numvouts-1].scriptPubKey) == 'p')
			{
				if (filtertxid == zeroid)
					result.push_back(txid.GetHex());
				else
				{
					DecodeAgreementProposalOpRet(tx.vout.back().scriptPubKey,version,srcpub,destpub,agreementname,
					agreementhash,deposit,payment,agreementtxid,bNewAgreement,arbitratorpub,disputefee);

					if (!bNewAgreement && agreementtxid == filtertxid)
						result.push_back(txid.GetHex());
				}
			}
		}
	}
	if (flags & ASF_AGREEMENTS)
	{
		SetCCtxids(agreementtxids,myCCaddr,true,EVAL_AGREEMENTS,CC_MARKER_VALUE,zeroid,'c');
		for (std::vector<uint256>::const_iterator it=agreementtxids.begin(); it!=agreementtxids.end(); it++)
		{
			txid = *it;
			if (myGetTransaction(txid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
			DecodeAgreementOpRet(tx.vout[numvouts-1].scriptPubKey) == 'c')
				result.push_back(txid.GetHex());
		}
	}

    return(result);
}
