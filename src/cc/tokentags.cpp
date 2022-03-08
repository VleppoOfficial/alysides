/******************************************************************************
* Copyright © 2014-2022 The SuperNET Developers.                             *
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

#include "CCtokentags.h"
#include "CCtokens.h"
#include "CCtokens_impl.h"

/*
Token Tags module - preliminary design (not final, subject to change during build)
// TODO description

Tag create:
vin.0 to vin.m-1: tokens
vin.m to vin.n-1: normal input
vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
vout.1: tokens back to source address
vout.n-2: normal output for change (if any)
vout.n-1: OP_RETURN EVAL_TOKENTAGS 'c' version srcpub tokenid tokensupply updatesupply flags name data

Tag update:
vin.0: CC input from create or update vout.0
vin.1 to vin.m-1: tokens
vin.m to vin.n-1: normal input
vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
vout.1: tokens back to source address
vout.n-2: normal output for change (if any)
vout.n-1: OP_RETURN EVAL_TOKENTAGS 'u' version srcpub tokentagid newupdatesupply data

Tag update (escrow) (TBD):
vin.0: CC input from create or update vout.0
vin.1 to vin.n-1: normal input
vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
vout.n-2: normal output for change (if any)
vout.n-1: OP_RETURN EVAL_TOKENTAGS 'e' version srcpub tokentagid escrowtxid newupdatesupply data

User Flow

- Token owner uses tokentagcreate to set up a tag tied to a specific token than can be updated with new data, as long as the data author has possession of a specified amount of the tokens
- Token owner uses tokentagupdate to add a new entry to the tag created by the tokentagcreate transaction
- A list of sorted token tags related to a specific token can be retrieved by tokentaglist
- Information on a specific token tag can be retrieved by tokentaginfo

RPC List

tokentagcreate tokenid name tokensupply updatesupply [flags] [data]
tokentagupdate tokentagid [data] [newupdatesupply] [escrowtxid]
tokentaglist [tokenid]
tokentaginfo tokentagid

tokentagcreate flags list
TTF_ALLOWANYSUPPLY - allows updatesupply for this tag to be set to below 51% of the total token supply. By default, only values above 51% of the total token supply are allowed for updatesupply. Note: depending on the updatesupply value set, this flag may allow multiple parties that own this token to post update transactions to this tag at the same time. Exercise caution when setting this flag.
TTF_NOESCROWUPDATES - disables escrow type updates for this tag.
*/

/*
Heuristics for finding correct token tag for app:
You will need to know the tokenid of the token you're trying to find the tag/logbook for.
Look for tags that have been made by the token creator for this tokenid specifically.
If the token creator did not create any tags, look for tags that were made by any pubkey.
The earliest tag in this list is your "official" token tag.
If there are multiple tags that meet this criteria in the same block / same timestamp, choose the tag with the txid that has the lowest
numerical value when converting the hex to a number.
*/


// --- Start of consensus code ---

int64_t IsTokenTagsvout(struct CCcontract_info *cp,const CTransaction& tx, int32_t v, char* destaddr)
{
	char tmpaddr[64];
	if ( tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0 )
	{
		if ( Getscriptaddress(tmpaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(tmpaddr,destaddr) == 0 )
			return(tx.vout[v].nValue);
	}
	return(0);
}

// Returns tokens CC version of specified tokenid, alongside the appropriate evalcode and some stats about the token if successful. 
// Does not support tokensv0. Does recognise TokensV1, though the CC itself is incompatible with v1 tokens.
// If unsuccessful, returns 0.
static uint8_t GetTokenDetails(const uint256 tokenid, struct CCcontract_info *cp, int64_t &fullsupply)
{
	CScript opret;
	CTransaction tokencreatetx;
	std::string name, description;
    std::vector<uint8_t> origpubkey;
    std::vector<vuint8_t> oprets;
	uint256 hashBlock;
	struct CCcontract_info C;

	// Fetch transaction and extract opret, with the assumption that it is located in the last vout.
	if (!myGetTransaction(tokenid, tokencreatetx, hashBlock) || tokencreatetx.vout.size() == 0)
		return (0);
	opret = tokencreatetx.vout.back().scriptPubKey;

	// Iterate through tokens opret decoders until one of them returns non-zero.
	// note: there's probably a better, less brute-forcey way to do this but it should at least allow to easily add forward compatibility - Dan
	// add new version handling here
    if (TokensV2::DecodeTokenCreateOpRet(opret, origpubkey, name, description, oprets) != 0) // tokens v2
	{
		cp = CCinit(&C,EVAL_TOKENSV2);
		fullsupply = CCfullsupplyV2(tokenid);
        return 2;
	}
    else if (TokensV1::DecodeTokenCreateOpRet(opret, origpubkey, name, description, oprets) != 0) // tokens v1
	{
		cp = CCinit(&C,EVAL_TOKENS);
		fullsupply = CCfullsupply(tokenid);
        return 1;
	}
	return (0); // tokens v0 or invalid
}
// Overload that only returns token version.
static uint8_t GetTokenDetails(const uint256 tokenid)
{
	int64_t fullsupply;
	struct CCcontract_info *cp;
	return GetTokenDetails(tokenid, cp, fullsupply);
}

// OP_RETURN data encoders and decoders for all Token Tags transactions.
CScript EncodeTokenTagCreateOpRet(uint8_t version, CPubKey srcpub, uint256 tokenid, int64_t tokensupply, int64_t updatesupply, uint8_t flags, std::string name, std::string data)
{
	vscript_t vopret; uint8_t evalcode = EVAL_TOKENTAGS, funcid = 'c';
	vopret = E_MARSHAL(ss << evalcode << funcid << version << srcpub << tokenid << tokensupply << updatesupply << flags << name << data);
	switch (GetTokenDetails(tokenid))
	{
		default:
			break;
		case 2:
			return (TokensV2::EncodeTokenOpRet(tokenid, {}, { vopret }));
		// add new version handling here
	}
	return CScript();
}
uint8_t DecodeTokenTagCreateOpRet(const CScript scriptPubKey, uint8_t &version, CPubKey &srcpub, uint256 &tokenid, int64_t &tokensupply, int64_t &updatesupply, uint8_t &flags, std::string &name, std::string &data)
{
    std::vector<uint8_t> vopret,vOpretExtra; uint8_t evalcode, funcid; std::vector<CPubKey> pubkeys;
    std::vector<vscript_t> oprets;

	// add new version handling here
	if ((TokensV2::DecodeTokenOpRet(scriptPubKey,tokenid,pubkeys,oprets) != 0) &&
		GetOpReturnCCBlob(oprets, vOpretExtra) && vOpretExtra.size() > 0)
    {
        vopret = vOpretExtra;
    }
    else
		GetOpReturnData(scriptPubKey, vopret);
	if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> srcpub; ss >> tokenid; ss >> tokensupply; ss >> updatesupply; ss >> flags; ss >> name; ss >> data) != 0 && evalcode == EVAL_TOKENTAGS)
		return(funcid);
	return(0);
}

CScript EncodeTokenTagUpdateOpRet(uint256 tokenid, uint8_t version, CPubKey srcpub, uint256 tokentagid, int64_t newupdatesupply, std::string data)
{
	vscript_t vopret; uint8_t evalcode = EVAL_TOKENTAGS, funcid = 'u';
	vopret = E_MARSHAL(ss << evalcode << funcid << version << srcpub << tokentagid << newupdatesupply << data);
	switch (GetTokenDetails(tokenid))
	{
		default:
			break;
		case 2:
			return (TokensV2::EncodeTokenOpRet(tokenid, {}, { vopret }));
		// add new version handling here
	}
	return CScript();
}
uint8_t DecodeTokenTagUpdateOpRet(CScript scriptPubKey, uint8_t &version, CPubKey &srcpub, uint256 &tokentagid, int64_t &newupdatesupply, std::string &data)
{
    std::vector<uint8_t> vopret,vOpretExtra; uint8_t evalcode, funcid; uint256 tokenid; std::vector<CPubKey> pubkeys;
    std::vector<vscript_t> oprets;

	// add new version handling here
	if ((TokensV2::DecodeTokenOpRet(scriptPubKey,tokenid,pubkeys,oprets) != 0) &&
		GetOpReturnCCBlob(oprets, vOpretExtra) && vOpretExtra.size() > 0)
    {
        vopret = vOpretExtra;
    }
    else
		GetOpReturnData(scriptPubKey, vopret);
	if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> srcpub; ss >> tokentagid; ss >> newupdatesupply; ss >> data) != 0 && evalcode == EVAL_TOKENTAGS)
		return(funcid);
	return(0);
}

CScript EncodeTokenTagEscrowOpRet(uint8_t version, CPubKey srcpub, uint256 tokentagid, uint256 escrowtxid, int64_t newupdatesupply, std::string data)
{
	CScript opret; uint8_t evalcode = EVAL_TOKENTAGS, funcid = 'e';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << srcpub << tokentagid << escrowtxid << newupdatesupply << data);
	return(opret);
}
uint8_t DecodeTokenTagEscrowOpRet(CScript scriptPubKey, uint8_t &version, CPubKey &srcpub, uint256 &tokentagid, uint256 &escrowtxid, int64_t &newupdatesupply, std::string &data)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> srcpub; ss >> tokentagid; ss >> escrowtxid; ss >> newupdatesupply; ss >> data) != 0 && evalcode == EVAL_TOKENTAGS)
		return(funcid);
	return(0);
}

// Generic decoder for Token Tags transactions, returns function id.
uint8_t DecodeTokenTagOpRet(const CScript scriptPubKey)
{
	std::vector<uint8_t> vopret;
	CPubKey dummypubkey;
	int64_t dummyint64;
	std::vector<CAmount> updateamounts;
	uint256 dummyuint256;
	std::string dummystring;
	uint8_t evalcode, funcid, *script, dummyuint8;
	GetOpReturnData(scriptPubKey, vopret);
	script = (uint8_t *)vopret.data();
	if(script != NULL && vopret.size() > 2)
	{
		evalcode = script[0];
		if (evalcode != EVAL_TOKENTAGS)
		{
			LOGSTREAM("tokentagscc", CCLOG_DEBUG1, stream << "script[0] " << script[0] << " != EVAL_TOKENTAGS" << std::endl);
			return (uint8_t)0;
		}
		funcid = script[1];
		LOGSTREAM((char *)"tokentagscc", CCLOG_DEBUG2, stream << "DecodeTokenTagOpRet() decoded funcId=" << (char)(funcid ? funcid : ' ') << std::endl);
		switch (funcid)
		{
			case 'c':
				return DecodeTokenTagCreateOpRet(scriptPubKey, dummyuint8, dummypubkey, dummyuint256, dummyint64, dummyint64, dummyuint8, dummystring, dummystring);
			case 'u':
				return DecodeTokenTagUpdateOpRet(scriptPubKey, dummyuint8, dummypubkey, dummyuint256, dummyint64, dummystring);
			case 'e':
				return DecodeTokenTagEscrowOpRet(scriptPubKey, dummyuint8, dummypubkey, dummyuint256, dummyuint256, dummyint64, dummystring);
			default:
				LOGSTREAM((char *)"tokentagscc", CCLOG_DEBUG1, stream << "DecodeTokenTagOpRet() illegal funcid=" << (int)funcid << std::endl);
				return (uint8_t)0;
		}
	}
	else
		LOGSTREAM("tokentagscc",CCLOG_DEBUG1, stream << "not enough opret.[" << vopret.size() << "]" << std::endl);
	return (uint8_t)0;
}

// Checks the provided bit field and returns true if unused flags are left unset, depending on provided function id.
// This is done in case these flags become usable in a future update, preventing old transactions from setting these flags before the new flags are officially supported.
static bool CheckUnusedFlags(const uint8_t flags, uint8_t funcid)
{
	switch (funcid)
	{
		case 'c':
			return (!(flags & 32 || flags & 64 || flags & 128));
		default:
			break;
	}
	return false;
}

// Validator for specific CC inputs found in Token Tags transactions.
static bool ValidateTokenTagsCCVin(struct CCcontract_info *cp,Eval* eval,const CTransaction& tx,int32_t index,int32_t prevVout,char* fromaddr,int64_t amount)
{
	CTransaction prevTx;
	uint256 hashblock;
	int32_t numvouts;
	char tmpaddr[64];

	if ((*cp->ismyvin)(tx.vin[index].scriptSig) == 0)
		return eval->Invalid("vin."+std::to_string(index)+" is CC for this token tag tx!");
	
	// Verify previous transaction and its op_return.
	if (myGetTransactionCCV2(cp,tx.vin[index].prevout.hash,prevTx,hashblock) == 0)
		return eval->Invalid("vin."+std::to_string(index)+" tx does not exist!");
	
	numvouts = prevTx.vout.size();

	// For CC inputs, check if previous transaction has Token Tags opret set up correctly.
	if (DecodeTokenTagOpRet(prevTx.vout[numvouts-1].scriptPubKey) == 0)
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
	
	return (true);
}

// Validator for token inputs found in Token Tags transactions.
static bool ValidateTokenVinInTag(struct CCcontract_info *cpTokens,Eval* eval,const CTransaction& tx,int32_t index,uint256 tokenid,char* fromaddr,int64_t amount)
{
	CTransaction prevTx;
	uint256 hashblock,tmptokenid;
	int32_t numvouts;
	char tmpaddr[64];
	uint8_t funcid;
	std::vector<CPubKey> keys;
	std::vector<vuint8_t> oprets;

	if ((cpTokens->ismyvin)(tx.vin[index].scriptSig) && 
	myGetTransactionCCV2(cpTokens,tx.vin[index].prevout.hash,prevTx,hashblock) != 0 && 
	prevTx.vout.size() > 0 && 
	// add new version handling here
	((funcid = TokensV2::DecodeTokenOpRet(prevTx.vout.back().scriptPubKey, tmptokenid, keys, oprets)) != 0) && 
	((funcid == 'c' && prevTx.GetHash() == tokenid) || (funcid != 'c' && tmptokenid == tokenid)))
	{
		// If fromaddr != 0, validate prevout dest address.
		if (fromaddr != 0 && Getscriptaddress(tmpaddr,prevTx.vout[tx.vin[index].prevout.n].scriptPubKey) && strcmp(tmpaddr,fromaddr) != 0)
			return (false);

		// if amount > 0, validate amount.
		else if (amount > 0 && prevTx.vout[tx.vin[index].prevout.n].nValue != amount)
			return (false);
		
		return (true);
	}
	
	return (false);
}

bool TokenTagsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	CBlockIndex blockIdx;
	CTransaction tokentagtx,prevTx;
	uint256 tokentagid,tokenid,hashBlock,reftokentagid,refescrowtxid,escrowtxid;
	std::string data,name,refdata;
	uint8_t funcid,version,flags,tokensversion,prevfuncid;
	int32_t numvins,numvouts;
	int64_t tokensupply,updatesupply,fullsupply,requiredupdatesupply,newupdatesupply,prevupdatesupply,tokenamount;
	CPubKey srcpub,creatorpub,tagtxidpk,TokenTagspk,refpub;
	char tagCCaddress[KOMODO_ADDRESS_BUFSIZE],srctokenaddr[KOMODO_ADDRESS_BUFSIZE],*txidaddr;
	struct CCcontract_info *cpTokens;
	int i;

	// Check boundaries, and verify that input/output amounts are exact.
	numvins = tx.vin.size();
	numvouts = tx.vout.size();
	if (numvouts < 1)
		return eval->Invalid("No vouts!");

	CCOpretCheck(eval,tx,true,true,true);
	ExactAmounts(eval,tx,ASSETCHAINS_CCZEROTXFEE[EVAL_TOKENTAGS]?0:CC_TXFEE);

	// Check the op_return of the transaction and fetch its function id.
	if ((funcid = DecodeTokenTagOpRet(tx.vout[numvouts-1].scriptPubKey)) != 0)
	{
		fprintf(stderr,"validating TokenTags transaction type (%c)\n",funcid);

		TokenTagspk = GetUnspendable(cp, NULL);

		switch (funcid)
		{
			case 'c':
				// Tag create:
				// vin.0 to vin.m-1: tokens
				// vin.m to vin.n-1: normal input
				// vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
				// vout.1: tokens back to source address
				// vout.n-2: normal output for change (if any)
				// vout.n-1: OP_RETURN EVAL_TOKENTAGS 'c' version srcpub tokenid tokensupply updatesupply flags name data

				// Get the data from the transaction's op_return.
				if (!DecodeTokenTagCreateOpRet(tx.vout[numvouts-1].scriptPubKey,version,srcpub,tokenid,tokensupply,updatesupply,flags,name,data))
					return eval->Invalid("Token tag create transaction OP_RETURN data invalid!");
				
				// Checking if srcpub is a valid pubkey.
				else if (!(srcpub.IsValid()))
					return eval->Invalid("srcpub in token tag create tx is not a valid pubkey!");
				
				else if (TotalPubkeyNormalInputs(tx,srcpub) == 0 || TotalPubkeyCCInputs(tx,srcpub) == 0)
					return eval->Invalid("Found no normal and CC inputs signed by token tag's source pubkey!");

				// Check if tokenid and its version is correct.
				else if ((tokensversion = GetTokenDetails(tokenid,cpTokens,fullsupply)) == 0)
					return eval->Invalid("tokenid in token tag create tx is invalid or has invalid version!");
				else if (tokensversion == 1)
					return eval->Invalid("tokenid in token tag create tx is v1, which is not supported!");
	
				// Check full supply for tokenid.
				else if (tokensupply != fullsupply)
					return eval->Invalid("tokensupply in token tag create tx is not full supply for given tokenid!");
				
				// Flag checks - make sure there are no unused flags set.
				else if (!CheckUnusedFlags(flags, funcid))
					return eval->Invalid("Token tag create transaction has unused flag bits set!");
				
				// Check specified update supply, which must be more than 50% of token supply unless TTF_ALLOWANYSUPPLY is set.
				requiredupdatesupply = static_cast<int64_t>(tokensupply % 2 ? static_cast<double>(tokensupply) * 0.5 + 0.5 : static_cast<double>(tokensupply) * 0.5);

				if (!(flags & TTF_ALLOWANYSUPPLY) && updatesupply >= requiredupdatesupply)
					return eval->Invalid("Token tag create transaction has update supply set to less than "+std::to_string(requiredupdatesupply)+" tokens!");
				
				// Checking name and data - they can't be empty, and both params have a max size limit.
				else if (name.empty() || name.size() > TOKENTAGSCC_MAX_NAME_SIZE)
					return eval->Invalid("Token tag name in offer transaction is empty or exceeds max character limit!");
				else if (data.empty() || data.size() > TOKENTAGSCC_MAX_DATA_SIZE)
					return eval->Invalid("Token tag data in offer transaction is empty or exceeds max character limit!");
				
				tagtxidpk = CCtxidaddr(txidaddr,tokenid);
				GetCCaddress1of2(cp, tagCCaddress, TokenTagspk, tagtxidpk, true);
				GetCCaddress(cpTokens, srctokenaddr, srcpub, true);
				
				// Verify that vins are only tokens and normal inputs.
				i = 0;
				while (i < numvins)
				{  
					if (ValidateTokenVinInTag(cpTokens,eval,tx,i,tokenid,srctokenaddr,0))
						i++;
					else
						break;
				}
				if (ValidateNormalVins(eval,tx,i) == 0)
					return (false);
				
				// Check numvouts. (vout0, vout1, opret vout, and optional vout for change)
				else if (numvouts > 4)
					return eval->Invalid("Invalid number of vouts for 'c' type transaction!");
				
				// vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
				else if (ConstrainVoutV2(tx.vout[0], 1, tagCCaddress, CC_MARKER_VALUE, EVAL_TOKENTAGS) == 0)
					return eval->Invalid("vout.0 must be CC baton to global pubkey / tokenid-pubkey 1of2 CC address!");

				// vout.1: tokens back to source address
				else if (ConstrainVoutV2(tx.vout[1], 1, srctokenaddr, tokensupply, cp->evalcode) == 0)
					return eval->Invalid("vout.1 must be tokens back to source address!");
				
				LOGSTREAM("tokentagscc", CCLOG_INFO, stream << "TokenTagsValidate: 'c' tx validated" << std::endl);

				break;
			
			case 'u':
				// Token tag update (regular):
                // vin.0: CC input from create or update vout.0
				// vin.1 to vin.m-1: tokens
				// vin.m to vin.n-1: normal input
				// vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
				// vout.1: tokens back to source address
				// vout.n-2: normal output for change (if any)
				// vout.n-1: OP_RETURN EVAL_TOKENTAGS 'u' version srcpub tokentagid newupdatesupply data

				// Get the data from the transaction's op_return.
				if (!DecodeTokenTagUpdateOpRet(tx.vout[numvouts-1].scriptPubKey,version,srcpub,tokentagid,newupdatesupply,data))
					return eval->Invalid("Token tag update transaction OP_RETURN data invalid!");
				
				// Checking if srcpub is a valid pubkey.
				else if (!(srcpub.IsValid()))
					return eval->Invalid("srcpub in token tag update tx is not a valid pubkey!");
				
				else if (TotalPubkeyNormalInputs(tx,srcpub) == 0 || TotalPubkeyCCInputs(tx,srcpub) == 0)
					return eval->Invalid("Found no normal and CC inputs signed by token tag update's source pubkey!");

				// Find the specified token tag transaction, extract its data.
				else if (eval->GetTxConfirmed(tokentagid,tokentagtx,blockIdx) == 0 || !(blockIdx.IsValid()) || tokentagtx.vout.size() == 0 ||
				DecodeTokenTagCreateOpRet(tokentagtx.vout.back().scriptPubKey,version,creatorpub,tokenid,tokensupply,updatesupply,flags,name,refdata) != 'c')
					return eval->Invalid("Token tag update transaction references invalid or unconfirmed token tag!");
				
				// TTF_TAGCREATORONLY is set to prevent any other pubkey apart from the original creator from making updates for the tag.
				else if ((flags & TTF_TAGCREATORONLY) && srcpub != creatorpub)
					return eval->Invalid("Token tag update srcpub and creatorpub mismatch when TTF_TAGCREATORONLY set!");
				
				// TTF_CONSTREQS is set to prevent any other updatesupply from being specified compared to original updatesupply.
				else if ((flags & TTF_CONSTREQS) && newupdatesupply != updatesupply)
					return eval->Invalid("Token tag update newupdatesupply and previous updatesupply mismatch when TTF_CONSTREQS set!");
				
				// Check specified update supply, which must be more than 50% of token supply unless TTF_ALLOWANYSUPPLY is set.
				requiredupdatesupply = static_cast<int64_t>(tokensupply % 2 ? static_cast<double>(tokensupply) * 0.5 + 0.5 : static_cast<double>(tokensupply) * 0.5);
				if (!(flags & TTF_ALLOWANYSUPPLY) && newupdatesupply >= requiredupdatesupply)
					return eval->Invalid("Token tag update newupdatesupply must be at least "+std::to_string(requiredupdatesupply)+" tokens when TTF_ALLOWANYSUPPLY is not set!");
				
				// Check if tokenid and its version is correct.
				else if ((tokensversion = GetTokenDetails(tokenid,cpTokens,fullsupply)) == 0)
					return eval->Invalid("Token tag update references token tag with invalid tokenid!");
				else if (tokensversion == 1)
					return eval->Invalid("Token tag update references token tag with v1 tokenid!");
	
				// Checking data.
				else if (data.empty() || data.size() > TOKENTAGSCC_MAX_DATA_SIZE)
					return eval->Invalid("Token tag data in transaction is empty or exceeds max character limit!");
				
				tagtxidpk = CCtxidaddr(txidaddr,tokenid);
				GetCCaddress1of2(cp, tagCCaddress, TokenTagspk, tagtxidpk, true);
				GetCCaddress(cpTokens, srctokenaddr, srcpub, true);
				
				// vin.0: CC input from create or update vout.0
				// Find the previous transaction from vin.0, extract its data.
				if (!(eval->GetTxConfirmed(tx.vin[0].prevout.hash,prevTx,blockIdx)) || !(blockIdx.IsValid()) || prevTx.vout.size() == 0 || 
				(prevfuncid = DecodeTokenTagOpRet(prevTx.vout.back().scriptPubKey)) == 0)
					return eval->Invalid("Token tag update transaction has invalid or unconfirmed previous tag create or update transaction!");
					
				switch (prevfuncid)
				{
					default:
						return eval->Invalid("Token tag update transaction is attempting to spend an invalid previous transaction!");
					// For create transactions, make sure this is actually our tag creation transaction.
					case 'c':
						if (tx.vin[0].prevout.hash != tokentagid)
							return eval->Invalid("Token tag update transaction is attempting to spend a tag creation that is the wrong token tag!");
						prevupdatesupply = updatesupply;
						break;
					// For update transactions, make sure they're referencing the same tag as our own update.
					case 'u':
						DecodeTokenTagUpdateOpRet(prevTx.vout.back().scriptPubKey,version,refpub,reftokentagid,prevupdatesupply,refdata);
						if (reftokentagid != tokentagid)
							return eval->Invalid("Token tag update transaction is attempting to spend a tag update for the wrong token tag!");
						break;
					case 'e':
						DecodeTokenTagEscrowOpRet(prevTx.vout.back().scriptPubKey,version,refpub,reftokentagid,refescrowtxid,prevupdatesupply,refdata);
						if (reftokentagid != tokentagid)
							return eval->Invalid("Token tag update transaction is attempting to spend a tag update for the wrong token tag!");
						break;
				}
				// If the offer accepted by agreementtxid has TTF_AWAITNOTARIES set, make sure latest event is notarised.
				if (flags & TTF_AWAITNOTARIES && komodo_txnotarizedconfirmed(tx.vin[0].prevout.hash) == 0)
					return eval->Invalid("Token tag update transaction is attempting to spend an unnotarised previous creation / update with TTF_AWAITNOTARIES set in tag!");
				else if (ValidateTokenTagsCCVin(cp,eval,tx,0,0,tagCCaddress,CC_MARKER_VALUE) == 0)
					return (false);
				
				// Verify that vins are only tokens and normal inputs.
				i = 1;
				while (i < numvins)
				{  
					if (ValidateTokenVinInTag(cpTokens,eval,tx,i,tokenid,srctokenaddr,0))
						i++;
					else
						break;
				}
				if (ValidateNormalVins(eval,tx,i) == 0)
					return (false);

				// Check numvouts. (vout0, vout1, opret vout, and optional vout for change)
				else if (numvouts > 4)
					return eval->Invalid("Invalid number of vouts for 'u' type transaction!");
				
				// vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
				else if (ConstrainVoutV2(tx.vout[0], 1, tagCCaddress, CC_MARKER_VALUE, EVAL_TOKENTAGS) == 0)
					return eval->Invalid("vout.0 must be CC baton to global pubkey / tokenid-pubkey 1of2 CC address!");

				// vout.1: tokens back to source address
				tokenamount = tx.vout[1].nValue;
				if (tokenamount < prevupdatesupply)
					return eval->Invalid("vout.1 nValue is"+std::to_string(tokenamount)+", should be at least"+std::to_string(requiredupdatesupply)+"!");
				else if (ConstrainVoutV2(tx.vout[1], 1, srctokenaddr, tokenamount, cp->evalcode) == 0)
					return eval->Invalid("vout.1 must be tokens back to source address!");
				
				LOGSTREAM("tokentagscc", CCLOG_INFO, stream << "TokenTagsValidate: 'u' tx validated" << std::endl);

				break;

			case 'e':
				// Token tag update (escrow) (TBD):
				// vin.0: CC input from create or update vout.0
				// vin.1 to vin.n-1: normal input
				// vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
				// vout.n-2: normal output for change (if any)
				// vout.n-1: OP_RETURN EVAL_TOKENTAGS 'e' version srcpub tokentagid escrowtxid newupdatesupply data

				//////////////////////////////////////////////////////////////////////////////
				return eval->Invalid("Unexpected TokenTags 'e' function id, not built yet!");
				//////////////////////////////////////////////////////////////////////////////

				// Get the data from the transaction's op_return.
				if (!DecodeTokenTagEscrowOpRet(tx.vout[numvouts-1].scriptPubKey,version,srcpub,tokentagid,escrowtxid,newupdatesupply,data))
					return eval->Invalid("Token tag escrow update transaction OP_RETURN data invalid!");
				
				// Checking if srcpub is a valid pubkey.
				else if (!(srcpub.IsValid()))
					return eval->Invalid("srcpub in token tag escrow update tx is not a valid pubkey!");
				
				else if (TotalPubkeyNormalInputs(tx,srcpub) == 0 || TotalPubkeyCCInputs(tx,srcpub) == 0)
					return eval->Invalid("Found no normal and CC inputs signed by token tag escrow update's source pubkey!");

				// Find the specified token tag transaction, extract its data.
				else if (eval->GetTxConfirmed(tokentagid,tokentagtx,blockIdx) == 0 || !(blockIdx.IsValid()) || tokentagtx.vout.size() == 0 ||
				DecodeTokenTagCreateOpRet(tokentagtx.vout.back().scriptPubKey,version,creatorpub,tokenid,tokensupply,updatesupply,flags,name,refdata) != 'c')
					return eval->Invalid("Token tag escrow update transaction references invalid or unconfirmed token tag!");
				
				// TTF_TAGCREATORONLY is set to prevent any other pubkey apart from the original creator from making updates for the tag.
				else if ((flags & TTF_TAGCREATORONLY) && srcpub != creatorpub)
					return eval->Invalid("Token tag escrow update srcpub and creatorpub mismatch when TTF_TAGCREATORONLY set!");
				
				// TTF_NOESCROWUPDATES is set to prevent this type of update for the tag to be done.
				else if (flags & TTF_NOESCROWUPDATES)
					return eval->Invalid("Token tag escrow update cannot be done on tag with TTF_NOESCROWUPDATES set!");
				
				// TTF_CONSTREQS is set to prevent any other updatesupply from being specified compared to original updatesupply.
				else if ((flags & TTF_CONSTREQS) && newupdatesupply != updatesupply)
					return eval->Invalid("Token tag update newupdatesupply and previous updatesupply mismatch when TTF_CONSTREQS set!");
				
				// TODO: this is where we analyze and validate escrowtxid

				// Check specified update supply, which must be more than 50% of token supply unless TTF_ALLOWANYSUPPLY is set.
				requiredupdatesupply = static_cast<int64_t>(tokensupply % 2 ? static_cast<double>(tokensupply) * 0.5 + 0.5 : static_cast<double>(tokensupply) * 0.5);
				if (!(flags & TTF_ALLOWANYSUPPLY) && newupdatesupply >= requiredupdatesupply)
					return eval->Invalid("Token tag escrow update newupdatesupply must be at least "+std::to_string(requiredupdatesupply)+" tokens when TTF_ALLOWANYSUPPLY is not set!");
				
				// Check if tokenid and its version is correct.
				else if ((tokensversion = GetTokenDetails(tokenid,cpTokens,fullsupply)) == 0)
					return eval->Invalid("Token tag escrow update references token tag with invalid tokenid!");
				else if (tokensversion == 1)
					return eval->Invalid("Token tag escrow update references token tag with v1 tokenid!");
	
				// Checking data.
				else if (data.empty() || data.size() > TOKENTAGSCC_MAX_DATA_SIZE)
					return eval->Invalid("Token tag data in transaction is empty or exceeds max character limit!");
				
				tagtxidpk = CCtxidaddr(txidaddr,tokenid);
				GetCCaddress1of2(cp, tagCCaddress, TokenTagspk, tagtxidpk, true);
				
				// vin.0: CC input from create or update vout.0
				// Find the previous transaction from vin.0, extract its data.
				if (!(eval->GetTxConfirmed(tx.vin[0].prevout.hash,prevTx,blockIdx)) || !(blockIdx.IsValid()) || prevTx.vout.size() == 0 || 
				(prevfuncid = DecodeTokenTagOpRet(prevTx.vout.back().scriptPubKey)) == 0)
					return eval->Invalid("Token tag escrow update transaction has invalid or unconfirmed previous tag create or update transaction!");
					
				switch (prevfuncid)
				{
					default:
						return eval->Invalid("Token tag escrow update transaction is attempting to spend an invalid previous transaction!");
					// For create transactions, make sure this is actually our tag creation transaction.
					case 'c':
						if (tx.vin[0].prevout.hash != tokentagid)
							return eval->Invalid("Token tag escrow update transaction is attempting to spend a tag creation that is the wrong token tag!");
						prevupdatesupply = updatesupply;
						break;
					// For update transactions, make sure they're referencing the same tag as our own update.
					case 'u':
						DecodeTokenTagUpdateOpRet(prevTx.vout.back().scriptPubKey,version,refpub,reftokentagid,prevupdatesupply,refdata);
						if (reftokentagid != tokentagid)
							return eval->Invalid("Token tag escrow update transaction is attempting to spend a tag update for the wrong token tag!");
						break;
					case 'e':
						DecodeTokenTagEscrowOpRet(prevTx.vout.back().scriptPubKey,version,refpub,reftokentagid,refescrowtxid,prevupdatesupply,refdata);
						if (reftokentagid != tokentagid)
							return eval->Invalid("Token tag escrow update transaction is attempting to spend a tag update for the wrong token tag!");
						break;
				}
				// If the offer accepted by agreementtxid has TTF_AWAITNOTARIES set, make sure latest event is notarised.
				if (flags & TTF_AWAITNOTARIES && komodo_txnotarizedconfirmed(tx.vin[0].prevout.hash) == 0)
					return eval->Invalid("Token tag escrow update transaction is attempting to spend an unnotarised previous creation / update with TTF_AWAITNOTARIES set in tag!");
				else if (ValidateTokenTagsCCVin(cp,eval,tx,0,0,tagCCaddress,CC_MARKER_VALUE) == 0)
					return (false);
				
				if (ValidateNormalVins(eval,tx,1) == 0)
					return (false);

				// TODO: this is where we check if escrow contains at least prevupdatesupply tokens that are accessible to srcpub

				// Check numvouts. (vout0, opret vout, and optional vout for change)
				else if (numvouts > 3)
					return eval->Invalid("Invalid number of vouts for 'e' type transaction!");
				
				// vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
				else if (ConstrainVoutV2(tx.vout[0], 1, tagCCaddress, CC_MARKER_VALUE, EVAL_TOKENTAGS) == 0)
					return eval->Invalid("vout.0 must be CC baton to global pubkey / tokenid-pubkey 1of2 CC address!");

				LOGSTREAM("tokentagscc", CCLOG_INFO, stream << "TokenTagsValidate: 'e' tx validated" << std::endl);

				break;

			default:
				fprintf(stderr,"unexpected tokentags funcid (%c)\n",funcid);
				return eval->Invalid("Unexpected TokenTags function id!");
		}
	}
	else
		return eval->Invalid("Invalid TokenTags function id and/or data!");

	LOGSTREAM("tokentagscc", CCLOG_INFO, stream << "TokenTags transaction validated" << std::endl);
	return (true);
}

// --- End of consensus code ---

// --- Helper functions for RPC implementations ---

// Wrapper for AddTokenCCInputs that also checks for tokens CC version.
static CAmount AddTokenInputsToTag(struct CCcontract_info *cp, CMutableTransaction &mtx, const CPubKey &pk, uint256 tokenid, CAmount total, int32_t maxinputs, bool usemempool)
{
	switch (GetTokenDetails(tokenid))
	{
		default:
			break;
		case 2:
			return (cp->evalcode == EVAL_TOKENSV2) ? AddTokenCCInputs<TokensV2>(cp, mtx, pk, tokenid, total, maxinputs, usemempool) : 0;
		// add new version handling here
	}
	return 0;
}

// Wrapper for MakeTokensCC1vout that also checks for tokens CC version.
static CTxOut MakeTokensCCvoutforTag(uint256 tokenid, uint8_t evalcode, CAmount nValue, CPubKey pk, std::vector<vscript_t>* pvvData = nullptr)
{
	switch (GetTokenDetails(tokenid))
	{
		default:
			break;
		case 2:
			return TokensV2::MakeTokensCC1vout(evalcode,nValue,pk,pvvData);
		// add new version handling here
	}
	return CTxOut();
}

// Finds the function id of the latest confirmed transaction that spent the event log baton for the specified token tag.
// Returns 'c' if event log baton is unspent, or 0 if token tag with the specified txid couldn't be found.
// Also returns the txid of the latest confirmed update the function found in the latesttxid variable.
static uint8_t FindLatestConfirmedTagUpdate(uint256 tokentagid, struct CCcontract_info *cp, uint256 &latesttxid)
{
	CTransaction sourcetx, batontx;
	uint256 hashBlock, batontxid, refagreementtxid;
	int32_t vini, height, retcode;
	uint8_t funcid,version,flags;
	char tagCCaddress[KOMODO_ADDRESS_BUFSIZE], *txidaddr;
	CPubKey TokenTagspk,tagtxidpk,creatorpub;
	int64_t tokensupply, updatesupply;
	uint256 tokenid;
	std::string name, data;

	latesttxid = zeroid;

	// Get token tag creation transaction and its op_return, containing the tokenid.
	if (myGetTransactionCCV2(cp, tokentagid, sourcetx, hashBlock) && sourcetx.vout.size() > 0 &&
	DecodeTokenTagCreateOpRet(sourcetx.vout.back().scriptPubKey,version,creatorpub,tokenid,tokensupply,updatesupply,flags,name,data) != 0)
	{
		// A valid event baton vout for any type of event must be vout.0, and is sent to a special address created from the global CC pubkey, 
		// and a txid-pubkey created from the tag's tokenid.
		// To check if a vout is valid, we'll need to get this address first.
		// This address can be constructed out of the Token Tags global pubkey and the tokenid-pubkey using CCtxidaddr.
		TokenTagspk = GetUnspendable(cp, NULL);
		tagtxidpk = CCtxidaddr(txidaddr,tokenid);
		GetCCaddress1of2(cp, tagCCaddress, TokenTagspk, tagtxidpk, true);
		
		// Iterate through vout0 batons while we're finding valid token tag transactions that spent the last baton.
		while ((IsTokenTagsvout(cp,sourcetx,0,tagCCaddress) != 0) &&
		
		// Check if vout0 was spent.
		(retcode = CCgetspenttxid(batontxid, vini, height, sourcetx.GetHash(), 0)) == 0 &&

		// Get spending transaction and its op_return.
		(myGetTransactionCCV2(cp, batontxid, batontx, hashBlock)) && !hashBlock.IsNull() && batontx.vout.size() > 0 && 
		(funcid = DecodeTokenTagOpRet(batontx.vout.back().scriptPubKey)) != 0)
		{
			sourcetx = batontx;
		}

		latesttxid = sourcetx.GetHash();
		return funcid;
	}

	return (uint8_t)0;
}

// --- RPC implementations for transaction creation ---

// Transaction constructor for tokentagcreate rpc.
// Creates token tag transaction with 'c' function id.
UniValue TokenTagCreate(const CPubKey& pk,uint64_t txfee,uint256 tokenid,int64_t tokensupply,int64_t updatesupply,uint8_t flags,std::string name,std::string data)
{
	char str[67],*txidaddr;
	CPubKey mypk,TokenTagspk,tagtxidpk;
	CScript opret;
	int64_t fullsupply,requiredupdatesupply;
	uint8_t version,tokensversion;
	UniValue rawtx(UniValue::VOBJ), result(UniValue::VOBJ);
	
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C,*cpTokens;
	cp = CCinit(&C,EVAL_TOKENTAGS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	if (!(mypk.IsFullyValid()))
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Signing pubkey must be valid");
	
	// Check if tokenid and its version is correct.
	else if ((tokensversion = GetTokenDetails(tokenid,cpTokens,fullsupply)) == 0)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Token id invalid");
	else if (tokensversion == 1)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Token id is version 1, which is not supported by this CC");
	
	// Check full supply for tokenid.
	else if (tokensupply != fullsupply)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Specified token supply is not equal to found full supply for given token id");
	
	// Flag checks - make sure there are no unused flags set.
	else if (!CheckUnusedFlags(flags, 'c'))
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Can't set unused flag bits");
	
	std::cerr << "Checking required update supply" << std::endl;
	// Check specified update supply, which must be more than 50% of token supply unless TTF_ALLOWANYSUPPLY is set.
	requiredupdatesupply = (int64_t)(tokensupply % 2 ? (double)(tokensupply) * 0.5 + 0.5 : (double)(tokensupply) * 0.5);
	//requiredupdatesupply = static_cast<int64_t>(tokensupply % 2 ? static_cast<double>(tokensupply) * 0.5 + 0.5 : static_cast<double>(tokensupply) * 0.5);
	std::cerr << "Done" << std::endl;

	if (!(flags & TTF_ALLOWANYSUPPLY) && updatesupply >= requiredupdatesupply)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Update supply must be at least "+std::to_string(requiredupdatesupply)+" tokens");

	// Checking name and data.
	else if (name.empty() || name.size() > TOKENTAGSCC_MAX_NAME_SIZE)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Token tag name cannot be empty and must be up to "+std::to_string(TOKENTAGSCC_MAX_NAME_SIZE)+" characters");
	else if (data.empty() || data.size() > TOKENTAGSCC_MAX_DATA_SIZE)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Token tag data cannot be empty and must be up to "+std::to_string(TOKENTAGSCC_MAX_DATA_SIZE)+" characters");
	
	TokenTagspk = GetUnspendable(cp, NULL);
	tagtxidpk = CCtxidaddr(txidaddr,tokenid);
	
	opret = EncodeTokenTagCreateOpRet(TOKENTAGSCC_VERSION,mypk,tokenid,tokensupply,updatesupply,flags,name,data);

	std::cerr << "opret done" << std::endl;

	if (AddTokenInputsToTag(cpTokens,mtx,mypk,tokenid,tokensupply,64,false) == tokensupply) // vin.0 to vin.m-1: tokens
	{
        CCwrapper cond(MakeCCcond1(cpTokens->evalcode,mypk));
        CCAddVintxCond(cp,cond); 
		
        if (AddNormalinputs(mtx, mypk, txfee + CC_MARKER_VALUE, 5, pk.IsValid()) > 0) // vin.m to vin.n-1: normal input
        {
			// vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
			mtx.vout.push_back(MakeCC1of2voutMixed(cp->evalcode, CC_MARKER_VALUE, TokenTagspk, tagtxidpk));
			// vout.1: tokens back to source address
			mtx.vout.push_back(MakeTokensCCvoutforTag(tokenid, cpTokens->evalcode, tokensupply, mypk));

			rawtx = FinalizeCCV2Tx(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
        }
        else
			CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds or too many small value UTXOs");
    }
	else
    	CCERR_RESULT("tokentagscc",CCLOG_ERROR, stream << "You must have total supply of tokens in your tokens address!");

	if (rawtx[JSON_HEXTX].getValStr().size() > 0)
	{
		result.push_back(Pair(JSON_HEXTX, rawtx[JSON_HEXTX].getValStr()));
	}
	else
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Error reading hex created by FinalizeCCV2Tx");

	std::cerr << "hex done" << std::endl;

	// Return captured values here for debugging/verification before broadcasting.
	result.push_back(Pair("type","tag_create"));
	result.push_back(Pair("name",name));
	result.push_back(Pair("data",data));
	result.push_back(Pair("signing_key",pubkey33_str(str,(uint8_t *)&mypk)));
	result.push_back(Pair("tokenid",tokenid.GetHex()));
	result.push_back(Pair("required_update_supply",updatesupply));

	if (flags & TTF_TAGCREATORONLY)
		result.push_back(Pair("tag_creator_updates_only","true"));
	else
		result.push_back(Pair("tag_creator_updates_only","false"));

	if (flags & TTF_ALLOWANYSUPPLY)
		result.push_back(Pair("any_update_supply_enabled","true"));
	else
		result.push_back(Pair("any_update_supply_enabled","false"));
	
	if (flags & TTF_CONSTREQS)
		result.push_back(Pair("fixed_update_requirements","true"));
	else
		result.push_back(Pair("fixed_update_requirements","false"));

	if (flags & TTF_NOESCROWUPDATES)
		result.push_back(Pair("escrow_updates_enabled","false"));
	else
		result.push_back(Pair("escrow_updates_enabled","true"));

	if (flags & TTF_AWAITNOTARIES)
		result.push_back(Pair("notarisation_required","true"));
	else
		result.push_back(Pair("notarisation_required","false"));
	
	result.push_back(Pair("flags",flags));
	
	std::cerr << "func done" << std::endl;

	return (result);
}

// Transaction constructor for tokentagupdate rpc.
// Creates token tag transaction with 'u' function id.
UniValue TokenTagUpdate(const CPubKey& pk,uint64_t txfee,uint256 tokentagid,int64_t newupdatesupply,std::string data)
{
	char str[67],tagCCaddress[KOMODO_ADDRESS_BUFSIZE],*txidaddr;
	CAmount tokeninputs;
	CPubKey mypk,TokenTagspk,tagtxidpk,creatorpub,refpub;
	CTransaction tokentagtx,latesttx;
	CScript opret;
	int64_t fullsupply,tokensupply,updatesupply,prevupdatesupply,requiredupdatesupply;
	std::string refdata,name;
	uint8_t version,tokensversion,flags,latestfuncid;
	uint256 hashBlock,tokenid,latesttxid,reftokentagid,refescrowtxid;
	UniValue rawtx(UniValue::VOBJ), result(UniValue::VOBJ);
	
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C,*cpTokens;
	cp = CCinit(&C,EVAL_TOKENTAGS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	if (!(mypk.IsFullyValid()))
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Signing pubkey must be valid");

	// Find the specified token tag transaction, extract its data.
	if (myGetTransactionCCV2(cp,tokentagid,tokentagtx,hashBlock) == 0 || tokentagtx.vout.size() == 0 ||
	DecodeTokenTagCreateOpRet(tokentagtx.vout.back().scriptPubKey,version,creatorpub,tokenid,tokensupply,updatesupply,flags,name,refdata) != 'c')
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Specified token tag transaction not found or is invalid");
	else if (hashBlock.IsNull())
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Specified token tag transaction is still in mempool");

	// TTF_TAGCREATORONLY is set to prevent any other pubkey apart from the original creator from making updates for the tag.
	else if ((flags & TTF_TAGCREATORONLY) && mypk != creatorpub)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Tag update must be signed by original creator due to a setting in the tag requiring it");
	
	// TTF_CONSTREQS is set to prevent any other updatesupply from being specified compared to original updatesupply.
	else if ((flags & TTF_CONSTREQS) && newupdatesupply != updatesupply)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "New required update supply for tag cannot be changed due to a setting in the tag");
	
	// Check specified update supply, which must be more than 50% of token supply unless TTF_ALLOWANYSUPPLY is set.
	requiredupdatesupply = static_cast<int64_t>(tokensupply % 2 ? static_cast<double>(tokensupply) * 0.5 + 0.5 : static_cast<double>(tokensupply) * 0.5);
	if (!(flags & TTF_ALLOWANYSUPPLY) && newupdatesupply >= requiredupdatesupply)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "New update supply must be at least "+std::to_string(requiredupdatesupply)+" tokens");
	
	// Find latest tag update.
	latestfuncid = FindLatestConfirmedTagUpdate(tokentagid, cp, latesttxid);
	if (myGetTransactionCCV2(cp,latesttxid,latesttx,hashBlock) == 0 || latesttx.vout.size() == 0)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Couldn't find latest token tag update!");
	
	// Get latest update supply.
	switch (latestfuncid)
	{
		case 'c':
			if (latesttxid != tokentagid)
				CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Latest update txid and token tag creation id mismatch when latest update is 'c'");
			prevupdatesupply = updatesupply;
			break;
		case 'u':
			DecodeTokenTagUpdateOpRet(latesttx.vout.back().scriptPubKey,version,refpub,reftokentagid,prevupdatesupply,refdata);
			break;
		case 'e':
			DecodeTokenTagEscrowOpRet(latesttx.vout.back().scriptPubKey,version,refpub,reftokentagid,refescrowtxid,prevupdatesupply,refdata);
			break;
		default:
			CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Found incorrect funcid in latest token tag update!");
	}
	
	if (flags & TTF_AWAITNOTARIES && komodo_txnotarizedconfirmed(latesttxid) == 0)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Latest tag creation or update must be notarised due to having mandatory notarisation flag set");
	
	// Check if tokenid and its version is correct.
	else if ((tokensversion = GetTokenDetails(tokenid,cpTokens,fullsupply)) == 0)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Token id in tag invalid");
	else if (tokensversion == 1)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Token id in tag is version 1, which is not supported by this CC");
	
	// Checking data.
	else if (data.empty() || data.size() > TOKENTAGSCC_MAX_DATA_SIZE)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Token tag data cannot be empty and must be up to "+std::to_string(TOKENTAGSCC_MAX_DATA_SIZE)+" characters");
	
	TokenTagspk = GetUnspendable(cp, NULL);
	tagtxidpk = CCtxidaddr(txidaddr,tokenid);
	GetCCaddress1of2(cp, tagCCaddress, TokenTagspk, tagtxidpk, true);
	
	opret = EncodeTokenTagUpdateOpRet(tokenid,TOKENTAGSCC_VERSION,mypk,tokentagid,newupdatesupply,data);

	// vin.0: CC input from create or update vout.0
	mtx.vin.push_back(CTxIn(latesttxid,0,CScript()));
	CCaddr1of2set(cp,TokenTagspk,tagtxidpk,cp->CCpriv,tagCCaddress);
	CCwrapper cond(MakeCCcond1of2(cp->evalcode,TokenTagspk,tagtxidpk));
	CCAddVintxCond(cp,cond,cp->CCpriv);
	
	if ((tokeninputs = AddTokenInputsToTag(cpTokens,mtx,mypk,tokenid,prevupdatesupply,64,false)) >= prevupdatesupply) // vin.1 to vin.m-1: tokens
	{
        CCwrapper cond2(MakeCCcond1(cpTokens->evalcode,mypk));
        CCAddVintxCond(cp,cond2); 
		
        if (AddNormalinputs(mtx, mypk, txfee + CC_MARKER_VALUE, 5, pk.IsValid()) > 0) // vin.m to vin.n-1: normal input
        {
			// vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
			mtx.vout.push_back(MakeCC1of2voutMixed(cp->evalcode, CC_MARKER_VALUE, TokenTagspk, tagtxidpk));
			// vout.1: tokens back to source address
			// note: since vout1 goes back to sender's address, we don't need to calculate change, we only need to make sure that enough tokens are passed through this transaction.
			mtx.vout.push_back(MakeTokensCCvoutforTag(tokenid, cpTokens->evalcode, tokeninputs, mypk));
			
			rawtx = FinalizeCCV2Tx(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
        }
        else
			CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds or too many small value UTXOs");
    }
	else
    	CCERR_RESULT("tokentagscc",CCLOG_ERROR, stream << "You must have a supply of tokens equal to at least latest update's mandated supply in your tokens address!");

	if (rawtx[JSON_HEXTX].getValStr().size() > 0)
	{
		result.push_back(Pair(JSON_HEXTX, rawtx[JSON_HEXTX].getValStr()));
	}
	else
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Error reading hex created by FinalizeCCV2Tx");

	// Return captured values here for debugging/verification before broadcasting.
	result.push_back(Pair("type","tag_update"));
	result.push_back(Pair("token_tag_id",tokentagid.GetHex()));
	result.push_back(Pair("new_required_update_supply",newupdatesupply));
	result.push_back(Pair("previous_required_update_supply",prevupdatesupply));
	result.push_back(Pair("data",data));
	result.push_back(Pair("signing_key",pubkey33_str(str,(uint8_t *)&mypk)));
	result.push_back(Pair("tokenid",tokenid.GetHex()));
	
	return (result);
}

// Transaction constructor for tokentagescrowupdate rpc.
// Creates token tag transaction with 'e' function id.
UniValue TokenTagEscrowUpdate(const CPubKey& pk,uint64_t txfee,uint256 tokentagid,uint256 escrowtxid,int64_t newupdatesupply,std::string data)
{
	CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "tokentagescrowupdate not done yet");

	char str[67],tagCCaddress[KOMODO_ADDRESS_BUFSIZE],*txidaddr;
	CAmount tokeninputs;
	CPubKey mypk,TokenTagspk,tagtxidpk,creatorpub,refpub;
	CTransaction tokentagtx,latesttx,escrowtx;
	CScript opret;
	int64_t tokensupply,updatesupply,prevupdatesupply,requiredupdatesupply;
	std::string refdata,name;
	uint8_t version,flags,latestfuncid;
	uint256 hashBlock,tokenid,latesttxid,reftokentagid,refescrowtxid;
	UniValue rawtx(UniValue::VOBJ), result(UniValue::VOBJ);
	
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C,*cpTokens;
	cp = CCinit(&C,EVAL_TOKENTAGS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	if (!(mypk.IsFullyValid()))
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Signing pubkey must be valid");

	// Find the specified token tag transaction, extract its data.
	if (myGetTransactionCCV2(cp,tokentagid,tokentagtx,hashBlock) == 0 || tokentagtx.vout.size() == 0 ||
	DecodeTokenTagCreateOpRet(tokentagtx.vout.back().scriptPubKey,version,creatorpub,tokenid,tokensupply,updatesupply,flags,name,refdata) != 'c')
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Specified token tag transaction not found or is invalid");
	else if (hashBlock.IsNull())
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Specified token tag transaction is still in mempool");

	// TTF_NOESCROWUPDATES is set to prevent this type of update for the tag to be done.
	else if (flags & TTF_NOESCROWUPDATES)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Tag update cannot be done while tokens are in escrow due to a setting in the tag");
	
	// TTF_TAGCREATORONLY is set to prevent any other pubkey apart from the original creator from making updates for the tag.
	else if ((flags & TTF_TAGCREATORONLY) && mypk != creatorpub)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Tag update must be signed by original creator due to a setting in the tag requiring it");
	
	// TTF_CONSTREQS is set to prevent any other updatesupply from being specified compared to original updatesupply.
	else if ((flags & TTF_CONSTREQS) && newupdatesupply != updatesupply)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "New required update supply for tag cannot be changed due to a setting in the tag");
	
	// Check specified update supply, which must be more than 50% of token supply unless TTF_ALLOWANYSUPPLY is set.
	requiredupdatesupply = (tokensupply % 2) ? tokensupply * 0.5 + 1 : tokensupply * 0.5;
	if (!(flags & TTF_ALLOWANYSUPPLY) && newupdatesupply > requiredupdatesupply)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "New update supply must be at least "+std::to_string(requiredupdatesupply)+" tokens");
	
	// Find latest tag update.
	latestfuncid = FindLatestConfirmedTagUpdate(tokentagid, cp, latesttxid);
	if (myGetTransactionCCV2(cp,latesttxid,latesttx,hashBlock) == 0 || latesttx.vout.size() == 0)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Couldn't find latest token tag update!");
	
	// Get latest update supply.
	switch (latestfuncid)
	{
		case 'c':
			if (latesttxid != tokentagid)
				CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Latest update txid and token tag creation id mismatch when latest update is 'c'");
			prevupdatesupply = updatesupply;
			break;
		case 'u':
			DecodeTokenTagUpdateOpRet(latesttx.vout.back().scriptPubKey,version,refpub,reftokentagid,prevupdatesupply,refdata);
			break;
		case 'e':
			DecodeTokenTagEscrowOpRet(latesttx.vout.back().scriptPubKey,version,refpub,reftokentagid,refescrowtxid,prevupdatesupply,refdata);
			break;
		default:
			CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Found incorrect funcid in latest token tag update!");
	}
	
	if (flags & TTF_AWAITNOTARIES && komodo_txnotarizedconfirmed(latesttxid) == 0)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Latest tag creation or update must be notarised due to having mandatory notarisation flag set");
	
	// Find the escrow transaction, extract its data.
	else if (myGetTransactionCCV2(cp,escrowtxid,escrowtx,hashBlock) == 0 || escrowtx.vout.size() == 0)
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified escrow transaction not found or is invalid");
	else if (hashBlock.IsNull())
		CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Specified escrow transaction is still in mempool");
	
	// TODO: check what type of token escrow this is, and return a number that guesstimates how many tokens could be controlled by mypk.
	// Possible types could be:
	// - Heir 1of2 CC address (in this case we can treat heirfund tx creator or heir itself as owning the tokens inside)
	// - Assets bidding addresses
	// - Channels open transaction with tokenid specified?
	// etc.

	// Checking data.
	if (data.size() > TOKENTAGSCC_MAX_DATA_SIZE)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Token tag data must be up to "+std::to_string(TOKENTAGSCC_MAX_DATA_SIZE)+" characters");
	
	TokenTagspk = GetUnspendable(cp, NULL);
	tagtxidpk = CCtxidaddr(txidaddr,tokenid);
	GetCCaddress1of2(cp, tagCCaddress, TokenTagspk, tagtxidpk, true);
	
	opret = EncodeTokenTagEscrowOpRet(TOKENTAGSCC_VERSION,mypk,tokentagid,escrowtxid,newupdatesupply,data);

	// vin.0: CC input from create or update vout.0
	mtx.vin.push_back(CTxIn(latesttxid,0,CScript()));
	CCaddr1of2set(cp,TokenTagspk,tagtxidpk,cp->CCpriv,tagCCaddress);
	CCwrapper cond(MakeCCcond1of2(cp->evalcode,TokenTagspk,tagtxidpk));
	CCAddVintxCond(cp,cond,cp->CCpriv);
	
	if (AddNormalinputs(mtx, mypk, txfee + CC_MARKER_VALUE, 5, pk.IsValid()) > 0) // vin.1 to vin.n-1: normal input
	{
		// vout.0: baton to global pubkey / tokenid-pubkey 1of2 CC address
		mtx.vout.push_back(MakeCC1of2voutMixed(cp->evalcode, CC_MARKER_VALUE, TokenTagspk, tagtxidpk));
		
		rawtx = FinalizeCCV2Tx(pk.IsValid(),0,cp,mtx,mypk,txfee,opret);
	}
	else
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds or too many small value UTXOs");

	if (rawtx[JSON_HEXTX].getValStr().size() > 0)
	{
		result.push_back(Pair(JSON_HEXTX, rawtx[JSON_HEXTX].getValStr()));
	}
	else
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Error reading hex created by FinalizeCCV2Tx");

	// Return captured values here for debugging/verification before broadcasting.
	result.push_back(Pair("type","tag_escrow_update"));
	result.push_back(Pair("token_tag_id",tokentagid.GetHex()));
	result.push_back(Pair("escrow_id",escrowtxid.GetHex()));
	result.push_back(Pair("new_required_update_supply",newupdatesupply));
	result.push_back(Pair("previous_required_update_supply",prevupdatesupply));
	result.push_back(Pair("data",data));
	result.push_back(Pair("signing_key",pubkey33_str(str,(uint8_t *)&mypk)));
	result.push_back(Pair("tokenid",tokenid.GetHex()));
	
	return (result);
}

// --- RPC implementations for transaction analysis ---

UniValue TokenTagInfo(uint256 txid)
{
	UniValue result(UniValue::VOBJ);
	char str[67];
	uint256 hashBlock,latesttxid,tokenid,refescrowtxid,tokentagid,reftokentagid,escrowtxid;
	uint8_t version,funcid,flags,latestfuncid;
	CPubKey srcpub,refpub;
	CTransaction tx,latesttx;
	std::string name,data,refdata;
	int32_t numvouts;
	int64_t tokensupply,updatesupply,latestupdatesupply;

	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_TOKENTAGS);

	if (myGetTransactionCCV2(cp,txid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
	(funcid = DecodeTokenTagOpRet(tx.vout[numvouts-1].scriptPubKey)) != 0)
	{
		result.push_back(Pair("result","success"));
		result.push_back(Pair("txid",txid.GetHex()));

		switch (funcid)
		{
			case 'c':
				result.push_back(Pair("type","tag_create"));
				
				DecodeTokenTagCreateOpRet(tx.vout[numvouts-1].scriptPubKey,version,srcpub,tokenid,tokensupply,updatesupply,flags,name,data);

				result.push_back(Pair("name",name));
				result.push_back(Pair("creator_pubkey",pubkey33_str(str,(uint8_t *)&srcpub)));
				result.push_back(Pair("tokenid",tokenid.GetHex()));
				result.push_back(Pair("token_full_supply",tokensupply));

				if (flags & TTF_TAGCREATORONLY)
					result.push_back(Pair("tag_creator_updates_only","true"));
				else
					result.push_back(Pair("tag_creator_updates_only","false"));

				if (flags & TTF_ALLOWANYSUPPLY)
					result.push_back(Pair("any_update_supply_enabled","true"));
				else
					result.push_back(Pair("any_update_supply_enabled","false"));

				if (flags & TTF_CONSTREQS)
					result.push_back(Pair("fixed_update_requirements","true"));
				else
					result.push_back(Pair("fixed_update_requirements","false"));

				if (flags & TTF_NOESCROWUPDATES)
					result.push_back(Pair("escrow_updates_enabled","false"));
				else
					result.push_back(Pair("escrow_updates_enabled","true"));

				if (flags & TTF_AWAITNOTARIES)
					result.push_back(Pair("notarisation_required","true"));
				else
					result.push_back(Pair("notarisation_required","false"));

				result.push_back(Pair("flags",flags));
				
				// Find latest tag update.
				latestfuncid = FindLatestConfirmedTagUpdate(txid, cp, latesttxid);
				if (myGetTransactionCCV2(cp,latesttxid,latesttx,hashBlock) == 0 || latesttx.vout.size() == 0)
					CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Couldn't find latest token tag update!");

				// Get latest update supply.
				switch (latestfuncid)
				{
					case 'c':
						if (latesttxid != txid)
							CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Latest update txid and token tag creation id mismatch when latest update is 'c'");
						latestupdatesupply = updatesupply;
						break;
					case 'u':
						DecodeTokenTagUpdateOpRet(latesttx.vout.back().scriptPubKey,version,refpub,reftokentagid,latestupdatesupply,refdata);
						break;
					case 'e':
						DecodeTokenTagEscrowOpRet(latesttx.vout.back().scriptPubKey,version,refpub,reftokentagid,refescrowtxid,latestupdatesupply,refdata);
						break;
					default:
						CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Found incorrect funcid in latest token tag update!");
				}

				result.push_back(Pair("latest_update",latesttxid.GetHex()));
				result.push_back(Pair("tokens_required_to_update",latestupdatesupply));
				break;
			
			case 'u':
				result.push_back(Pair("type","tag_update"));
				
				DecodeTokenTagUpdateOpRet(tx.vout[numvouts-1].scriptPubKey,version,srcpub,tokentagid,updatesupply,data);

				result.push_back(Pair("token_tag_id",tokentagid.GetHex()));
				result.push_back(Pair("required_update_supply_set",updatesupply));
				result.push_back(Pair("data",data));
				result.push_back(Pair("signing_key",pubkey33_str(str,(uint8_t *)&srcpub)));
				break;
			
			case 'e':
				result.push_back(Pair("type","tag_escrow_update"));
				
				DecodeTokenTagEscrowOpRet(tx.vout[numvouts-1].scriptPubKey,version,srcpub,tokentagid,escrowtxid,updatesupply,data);

				result.push_back(Pair("token_tag_id",tokentagid.GetHex()));
				result.push_back(Pair("escrow_id",escrowtxid.GetHex()));
				result.push_back(Pair("required_update_supply_set",updatesupply));
				result.push_back(Pair("data",data));
				result.push_back(Pair("signing_key",pubkey33_str(str,(uint8_t *)&srcpub)));
				break;
		}
		return (result);
	}
	CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Invalid token tag transaction ID");
}

UniValue TokenTagSamples(const uint256 tokentagid,int64_t samplenum,bool bReverse)
{
	UniValue result(UniValue::VARR);
	CTransaction tokentagtx,batontx;
	uint256 latesttxid,batontxid,hashBlock;
	int64_t total = 0LL;
	int32_t numvouts,vini,height;
	uint8_t funcid;

	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_TOKENTAGS);

	if (myGetTransactionCCV2(cp,tokentagid,tokentagtx,hashBlock) != 0 && (numvouts = tokentagtx.vout.size()) > 0 &&
	DecodeTokenTagOpRet(tokentagtx.vout[numvouts-1].scriptPubKey) == 'c')
	{
		FindLatestConfirmedTagUpdate(tokentagid, cp, latesttxid);

		if (latesttxid != tokentagid)
		{
			if (bReverse) // from latest event to oldest
				batontxid = latesttxid;
			else
				batontxid = tokentagid;
			
			// Iterate through events while we haven't reached samplenum limits yet.
			while ((total < samplenum || samplenum == 0) &&
			// Fetch the transaction.
			myGetTransactionCCV2(cp,batontxid,batontx,hashBlock) != 0 && batontx.vout.size() > 0 &&
			// Fetch function id.
			DecodeTokenTagOpRet(batontx.vout.back().scriptPubKey) != 0)
			{
				result.push_back(batontxid.GetHex());
				
				if (batontxid != tokentagid) total++;

				// If bReverse = true, stop searching if we found the original token tag txid. 
				// If bReverse = false, stop searching if we found the latest update txid. 
				if ((!bReverse && batontxid == latesttxid) || (bReverse && batontxid == tokentagid))
					break;

				// Get previous or next update transaction.
				if (bReverse)
					batontxid = batontx.vin[0].prevout.hash;
				else
					CCgetspenttxid(batontxid,vini,height,batontx.GetHash(),0);
			}
		}
	}

	return(result);
}

UniValue TokenTagList(uint256 tokenid, CPubKey pubkey)
{
	UniValue result(UniValue::VARR);
	char tagCCaddress[KOMODO_ADDRESS_BUFSIZE],*txidaddr;
	CPubKey TokenTagspk,tagtxidpk;
	CTransaction tx;
	int32_t numvouts;
	std::vector<uint256> tokentagids;
	uint256 hashBlock;
	uint8_t version,flags,tokensversion;

	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_TOKENTAGS);

	if ((tokensversion = GetTokenDetails(tokenid)) == 0)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Token id in tag invalid");
	else if (tokensversion == 1)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Token id in tag is version 1, which is not supported by this CC");

	TokenTagspk = GetUnspendable(cp, NULL);
	tagtxidpk = CCtxidaddr(txidaddr,tokenid);
	GetCCaddress1of2(cp, tagCCaddress, TokenTagspk, tagtxidpk, true);

	SetCCtxids(tokentagids, tagCCaddress, true, cp->evalcode, CC_MARKER_VALUE, zeroid, 'c');

	for (std::vector<uint256>::const_iterator it = tokentagids.begin(); it != tokentagids.end(); it++)
	{
		if (myGetTransactionCCV2(cp,*it,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
		DecodeTokenTagOpRet(tx.vout[numvouts-1].scriptPubKey) == 'c' && (pubkey == CPubKey() || TotalPubkeyNormalInputs(tx,pubkey) != 0))
		{
			result.push_back((*it).GetHex());
		}
	}
	return (result);
}

// --- Useful misc functions for additional token transaction analysis ---

// internal function that looks for voutPubkeys or destaddrs in token tx oprets
// used by TokenOwners
/*template <class V>
void GetTokenOwnerList(const CTransaction tx, struct CCcontract_info *cp, uint256 tokenid, int64_t &depth, int64_t maxdepth, std::vector<CPubKey> &OwnerList)
{
    // Check "max depth" variable to avoid stack overflows from recursive calls
	if (depth > maxdepth)
	{
		return;
	}

    // Examine each vout in the tx (except last vout)
    for (int64_t n = 0; n <= tx.vout.size() - 1; n++)
    {
        CScript opret;
        uint256 tokenIdOpret, spendingtxid, hashBlock;
        CTransaction spendingtx;
        std::vector<vscript_t>  oprets;
        std::vector<CPubKey> voutPubkeys;
        int32_t vini, height;
		char destaddr[64];

        // We ignore every vout that's not a tokens vout, so we check for that
        if (IsTokensvout<V>(true, true, cp, NULL, tx, n, tokenid))
        {
			// Get the opret from either vout.n or tx.vout.back() scriptPubkey
			if (!getCCopret(tx.vout[n].scriptPubKey, opret))
				opret = tx.vout.back().scriptPubKey;
			uint8_t funcId = V::DecodeTokenOpRet(opret, tokenIdOpret, voutPubkeys, oprets);

			// Include only pubkeys from voutPubkeys arrays with 1 element.
			// If voutPubkeys size is >= 2 then the vout was probably sent to a CC 1of2 address, which
			// might have no true "owner" pubkey and is therefore outside the scope of this function
			if (voutPubkeys.size() == 1)
			{
				// Check if found pubkey is already in the list, if not, add it
				std::vector<CPubKey>::iterator it = std::find(OwnerList.begin(), OwnerList.end(), voutPubkeys[0]);
				if (it == OwnerList.end())
					OwnerList.push_back(voutPubkeys[0]);
			}
			
            // Check if this vout was spent, and if it was, find the tx that spent it
            if (CCgetspenttxid(spendingtxid, vini, height, tx.GetHash(), n) == 0 &&
            myGetTransaction(spendingtxid, spendingtx, hashBlock))
            {
				depth++;
                // Same procedure for the spending tx, until no more are found
                GetTokenOwnerList<V>(spendingtx, cp, tokenid, depth, maxdepth, OwnerList);
            }
        }
    }

	depth--;
    return;
}

template <class V>
UniValue TokenOwners(uint256 tokenid, int64_t minbalance, int64_t maxdepth)
{
    // NOTE: maybe add option to retrieve addresses instead, including 1of2 addresses?
	UniValue result(UniValue::VARR); 
    CTransaction tokenbaseTx; 
    uint256 hashBlock;
	uint8_t funcid;
    std::vector<uint8_t> origpubkey;
    std::string name, description; 
	std::vector<vscript_t>  oprets;
    std::vector<CPubKey> OwnerList;
	int64_t depth;
    char str[67];

    struct CCcontract_info *cpTokens, tokensCCinfo;
    cpTokens = CCinit(&tokensCCinfo, V::EvalCode());

    // Get token create tx
    if (!myGetTransaction(tokenid, tokenbaseTx, hashBlock) || (KOMODO_NSPV_FULLNODE && hashBlock.IsNull()))
    {
        LOGSTREAMFN(cctokens_log, CCLOG_INFO, stream << "cant find tokenid" << std::endl);
        return(result);
    }

    // Checking if passed tokenid is a token creation txid
    funcid = V::DecodeTokenCreateOpRet(tokenbaseTx.vout.back().scriptPubKey, origpubkey, name, description, oprets);
	if (tokenbaseTx.vout.size() > 0 && !IsTokenCreateFuncid(funcid))
	{
        LOGSTREAMFN(cctokens_log, CCLOG_INFO, stream << "passed tokenid isnt token creation txid" << std::endl);
        return(result);
    }

    // Get a full list of owners using a recursive looping function
    GetTokenOwnerList<V>(tokenbaseTx, cpTokens, tokenid, depth, maxdepth, OwnerList);

    // Add owners to result array
    for (auto owner : OwnerList)
        if (minbalance == 0 || GetTokenBalance<V>(owner, tokenid, false) >= minbalance)
			result.push_back(pubkey33_str(str,(uint8_t *)&owner));

	return result;
}

template <class V>
UniValue TokenInventory(const CPubKey pk, int64_t minbalance)
{
	UniValue result(UniValue::VARR); 
    char tokenaddr[KOMODO_ADDRESS_BUFSIZE];
    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
    std::vector<uint256> TokenList;

    struct CCcontract_info *cpTokens, tokensCCinfo;
    cpTokens = CCinit(&tokensCCinfo, EVAL_TOKENS);

	// Get token CC address of specified pubkey
    GetTokensCCaddress(cpTokens, tokenaddr, pk);

    // Get all CC outputs sent to this address
    SetCCtxids(addressIndex, tokenaddr, true);
    
    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it = addressIndex.begin(); it != addressIndex.end(); it++) 
    {
        std::vector<vscript_t>  oprets;
        std::vector<CPubKey> voutPubkeys;
        uint256 tokenIdInOpret, hashBlock;
        CTransaction vintx;
        CScript opret;

        // Find the tx that the CC output originates from
        if (myGetTransaction(it->first.txhash, vintx, hashBlock))
        {
            int32_t n = (int32_t)it->first.index;

            // skip markers
            if (IsTokenMarkerVout<V>(vintx.vout[n]))
                continue;

            // Get the opret from either vout.n or tx.vout.back() scriptPubkey
            if (!getCCopret(vintx.vout[n].scriptPubKey, opret))
                opret = vintx.vout.back().scriptPubKey;
            uint8_t funcid = V::DecodeTokenOpRet(opret, tokenIdInOpret, voutPubkeys, oprets);

            // If the vout is from a token creation tx, the tokenid will be hash of vintx
            if (IsTokenCreateFuncid(funcid))
                tokenIdInOpret = vintx.GetHash();
            
            // If the vout is not from a token creation tx, check if it is a token vout
            if (IsTokenCreateFuncid(funcid) || IsTokensvout<V>(true, true, cpTokens, NULL, vintx, n, tokenIdInOpret))
            {
                // Check if found tokenid is already in the list, if not, add it
                std::vector<uint256>::iterator it2 = std::find(TokenList.begin(), TokenList.end(), tokenIdInOpret);
                if (it2 == TokenList.end())
                    TokenList.push_back(tokenIdInOpret);
            }
        }
    }
    
    // Add token ids to result array
    for (auto tokenid : TokenList)
		if (minbalance == 0 || GetTokenBalance<V>(pk, tokenid, false) >= minbalance)
            result.push_back(tokenid.GetHex());

	return result;
}*/
