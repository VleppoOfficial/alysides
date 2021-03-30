/******************************************************************************
* Copyright © 2014-2021 The SuperNET Developers.                             *
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

/*
TODO: Insert description of token tags here.

Heuristics for finding correct token tag for app:
You will need to know the tokenid of the token you're trying to find the tag/logbook for.
Look for tags that have been made by the token creator for this tokenid specifically.
If the token creator did not create any tags, look for tags that were made by any pubkey.
Choose the tag(s) that have proven ownership of the biggest amount of the token supply.
Prioritize tags that do not have the "only creator can update" flag.
The earliest tag in this list is your "official" token tag.
If there are multiple tags that meet this criteria in the same block / same timestamp, choose the tag with the txid that has the lowest
numerical value when converting the hex to a number.

Token tag creation:
vin.0: normal input
vin.1: tokens
...
vin.n-1: normal input
vout.0: marker to global CC address w/ OP_RETURN EVAL_TOKENTAGS 'c' version srcpub flags maxupdates updateamount
vout.1: tokens
vout.n-2: normal change
vout.n-1: empty op_return

Token tag update:
vin.0: normal input
vin.1: previous global OP_RETURN marker
...
vin.n-1: normal input
vout.0: marker to global CC address w/ OP_RETURN EVAL_TOKENTAGS 'u' version tokentagid srcpub data updateamount
vout.n-2: normal change
vout.n-1: empty op_return

*/

// --- Start of consensus code ---
/*
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

// OP_RETURN data encoders and decoders for all Agreements transactions.
CScript EncodeTokenTagCreateOpRet(uint8_t version, std::string name, CPubKey srcpub, uint8_t flags, int64_t maxupdates, std::vector<CAmount> updateamounts)
{
	CScript opret; uint8_t evalcode = EVAL_TOKENTAGS, funcid = 'c';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << name << srcpub << flags << maxupdates << updateamounts);
	return(opret);
}
uint8_t DecodeTokenTagCreateOpRet(CScript scriptPubKey, uint8_t &version, std::string &name, CPubKey &srcpub, uint8_t &flags, int64_t &maxupdates, std::vector<CAmount> &updateamounts)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> name; ss >> srcpub; ss >> flags; ss >> maxupdates; ss >> updateamounts) != 0 && evalcode == EVAL_TOKENTAGS)
		return(funcid);
	return(0);
}

CScript EncodeTokenTagUpdateOpRet(uint8_t version, uint256 tokentagid, CPubKey srcpub, std::string data, std::vector<CAmount> updateamounts)
{
	CScript opret; uint8_t evalcode = EVAL_TOKENTAGS, funcid = 'u';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << tokentagid << srcpub << data << updateamounts);
	return(opret);
}
uint8_t DecodeTokenTagUpdateOpRet(CScript scriptPubKey, uint8_t &version, uint256 &tokentagid, CPubKey &srcpub, std::string &data, std::vector<CAmount> &updateamounts)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if(vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> tokentagid; ss >> srcpub; ss >> data; ss >> updateamounts) != 0 && evalcode == EVAL_TOKENTAGS)
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
				return DecodeTokenTagCreateOpRet(scriptPubKey, dummyuint8, dummystring, dummypubkey, dummyuint8, dummyint64, updateamounts);
			case 'u':
				return DecodeTokenTagUpdateOpRet(scriptPubKey, dummyuint8, dummyuint256, dummypubkey, dummystring, updateamounts);
			default:
				LOGSTREAM((char *)"tokentagscc", CCLOG_DEBUG1, stream << "DecodeTokenTagOpRet() illegal funcid=" << (int)funcid << std::endl);
				return (uint8_t)0;
		}
	}
	else
		LOGSTREAM("tokentagscc",CCLOG_DEBUG1, stream << "not enough opret.[" << vopret.size() << "]" << std::endl);
	return (uint8_t)0;
}

// Validator for normal inputs in TokenTags transactions.
bool ValidateTokenTagsNormalVins(Eval* eval, const CTransaction& tx, int32_t index)
{
	for (int i=index;i<tx.vin.size();i++)
		if (IsCCInput(tx.vin[i].scriptSig) != 0 )
			return eval->Invalid("vin."+std::to_string(i)+" is normal for tokentags tx!");
	return (true);
}

// Validator for CC inputs found in TokenTags transactions.
bool ValidateTokenTagsCCVin(struct CCcontract_info *cp,Eval* eval,const CTransaction& tx,int32_t index,int32_t prevVout,uint256 prevtxid,char* fromaddr,int64_t amount)
{
	CTransaction prevTx;
	uint256 hashblock;
	int32_t numvouts;
	char tmpaddr[64];
	CScript opret;

	// Check if a vin is a TokenTags CC vin.
	if ((*cp->ismyvin)(tx.vin[index].scriptSig) == 0)
		return eval->Invalid("vin."+std::to_string(index)+" is TokenTags CC for TokenTags tx!");

	// Verify previous transaction and its op_return.
	else if (myGetTransaction(tx.vin[index].prevout.hash,prevTx,hashblock) == 0)
		return eval->Invalid("vin."+std::to_string(index)+" tx does not exist!");
	else if ((numvouts=prevTx.vout.size()) == 0 || !MyGetCCopretV2(prevTx.vout[0].scriptPubKey, opret) || DecodeTokenTagOpRet(opret) == 0) 
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
		return eval->Invalid("invalid vin."+std::to_string(index)+" tx, expecting "+prevtxid.GetHex()+", got "+prevTx.GetHash().GetHex()+" !");
	
	return (true);
}

// Finds the txid of the latest valid transaction that spent the previous update baton for the specified token tag.
// Returns latest update txid, or zeroid if token tag couldn't be found.
// Also returns the update number, can be used to check how many updates exist for the specified tag.
// TODO fix
uint256 GetLatestConfirmedTagUpdate(struct CCcontract_info *cp,uint256 tokentagid,int64_t &updatenum)
{
	CTransaction sourcetx, batontx;
	uint256 hashBlock, batontxid;
	int32_t vini, height, retcode;
	uint8_t funcid;
	char globalCCaddress[65];
	CScript opret;

	updatenum = 0;

	// Get tokentag creation transaction and its op_return.
	if (myGetTransaction(tokentagid, sourcetx, hashBlock) && sourcetx.vout.size() > 0 &&
	MyGetCCopretV2(sourcetx.vout[0].scriptPubKey, opret) && DecodeTokenTagOpRet(opret) == 'c')
	{
		GetCCaddress(cp, globalCCaddress, GetUnspendable(cp, NULL));

		// Iterate through vout0 batons while we're finding valid Agreements transactions that spent the last baton.
		while ((IsTokenTagsvout(cp,sourcetx,0,globalCCaddress) == CC_MARKER_VALUE) &&
		
		// Check if vout0 was spent.
		(retcode = CCgetspenttxid(batontxid, vini, height, sourcetx.GetHash(), 0)) == 0 &&

		// Get spending transaction and its op_return.
		myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 && 
		(funcid = DecodeTokenTagOpRet(batontx.vout[0].scriptPubKey)) == 'u' &&
		
		// Check if the blockheight of the batontx is below or equal to current block height.
		(komodo_blockheight(hashBlock) <= chainActive.Height()))
		{
			updatenum++;
			sourcetx = batontx;
		}

		return sourcetx.GetHash();
	}

	return zeroid;
}

// Gets all valid token IDs from the token tag create tx.
// In order for a token vout to be considered valid it must:
// - Have a CC opret with a tokenid and single destination pubkey equal to token tag creator pubkey
// - Be a valid token vout (check with IsTokensvout)
// - Have its nValue be equal to the full supply of the specified token
std::vector<uint256> GetValidTagTokenIds(struct CCcontract_info *cpTokens,const CTransaction& createtx)
{
	CScript opret;
	std::vector<uint256> tokenidlist;
	std::vector<CPubKey> voutPubkeys;
    std::vector<vscript_t> oprets;
	uint256 tokenid;
	int32_t numvouts;
	uint8_t version, flags;
	std::string name;
	CPubKey srcpub;
	int64_t maxupdates;
	std::vector<CAmount> updateamounts;

	numvouts = createtx.vout.size();
	tokenidlist.clear();

	// Get the creator pubkey.
	if (!MyGetCCopretV2(createtx.vout[0].scriptPubKey, opret) || 
	DecodeTokenTagCreateOpRet(opret,version,name,srcpub,flags,maxupdates,updateamounts) != 'c')
	{
		return tokenidlist;
	}

	for (int i = 1; i < numvouts - 1; i++)
	{
		tokenid = zeroid;
		voutPubkeys.clear();
		opret = CScript();
		
		// std::cerr << "checking vout "+std::to_string(i)+"" << std::endl;
		// if (MyGetCCopretV2(createtx.vout[i].scriptPubKey, opret))
		// 	std::cerr << "MyGetCCopretV2 completed" << std::endl;
		// if (DecodeTokenOpRetV1(opret, tokenid, voutPubkeys, oprets) != 0)
		// 	std::cerr << "DecodeTokenOpRetV1 completed" << std::endl;
		// if (voutPubkeys.size() == 1 && voutPubkeys[0] == destpub)
		// 	std::cerr << "voutPubkeys completed" << std::endl;
		// if (IsTokensvout(true, true, cpTokens, NULL, createtx, i, tokenid) > 0)
		// 	std::cerr << "IsTokensvout completed" << std::endl;
		// if (createtx.vout[i].nValue == CCfullsupply(tokenid))
		// 	std::cerr << "CCfullsupply completed" << std::endl;
		
		if (MyGetCCopretV2(createtx.vout[i].scriptPubKey, opret) &&
		DecodeTokenOpRetV1(opret, tokenid, voutPubkeys, oprets) != 0 &&
		voutPubkeys.size() == 1 && voutPubkeys[0] == srcpub &&
		IsTokensvout(true, true, cpTokens, NULL, createtx, i, tokenid) > 0 &&
		createtx.vout[i].nValue == CCfullsupply(tokenid))
		{
			tokenidlist.push_back(tokenid);
		}
	}

	return tokenidlist;
}

// Modified version of CCtoken_balance from CCtx.cpp, includes IsTokensvout check.
// Used in validation code to check if the submitting pubkey owns enough tokens in order to update the referenced tag.
int64_t CCTokenBalance(struct CCcontract_info *cpTokens,char *tokenaddr,uint256 reftokenid)
{
    int64_t sum = 0;
	CTransaction tx;
	uint256 tokenid,txid,hashBlock; 
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;

    SetCCunspents(unspentOutputs,tokenaddr,true);
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++)
    {
        txid = it->first.txhash;

        if (myGetTransaction(txid,tx,hashBlock) != 0 && tx.vout.size() > 0 &&
		komodo_blockheight(hashBlock) <= chainActive.Height() &&
		IsTokensvout(true, true, cpTokens, NULL, tx, it->first.index, reftokenid) > 0)
		{
            sum += it->second.satoshis;
        }
    }
    return(sum);
}

bool ValidateTokenTagCreateTx(struct CCcontract_info *cp,Eval* eval,const CTransaction& createtx)
{
	std::vector<uint256> tokenlist;
	CScript opret;
	uint8_t version, flags;
	CPubKey srcpub;
	char globalCCaddress[65];
	int64_t maxupdates;
	std::string name;
	std::vector<CAmount> updateamounts;

    LOGSTREAM("agreementscc", CCLOG_INFO, stream << "ValidateTokenTagCreateTx: initiated" << std::endl);

	struct CCcontract_info *cpTokens, tokensC;
	cpTokens = CCinit(&tokensC, EVAL_TOKENS);

	//numvins = createtx.vin.size();
	//numvouts = createtx.vout.size();
	
	CCOpretCheck(eval,createtx,true,true,true);
	ExactAmounts(eval,createtx,ASSETCHAINS_CCZEROTXFEE[EVAL_TOKENTAGS]?0:CC_TXFEE);

	GetCCaddress(cp, globalCCaddress, GetUnspendable(cp, NULL));

	// Check vout0, make sure its embedded opret is correct.
	if (!MyGetCCopretV2(createtx.vout[0].scriptPubKey, opret) || DecodeTokenTagCreateOpRet(opret,version,name,srcpub,
    flags,maxupdates,updateamounts) != 'c')
        return eval->Invalid("Token tag creation transaction data invalid!");

	// Check name length.
	else if (name.empty() || name.size() > 32)
		return eval->Invalid("Name of token tag create transaction empty or longer than 32 characters!");
	
	// Verify that vout0 is a TokenTags vout and that it has the correct value and destination.
	else if (ConstrainVout(createtx.vout[0],1,globalCCaddress,CC_MARKER_VALUE)==0)
		return eval->Invalid("vout.0 is tokentags CC marker vout to global CC address for token tag create transaction!");
    
	// Check vin0, verify that it is a normal input and that it was signed by srcpub.
	else if (IsCCInput(createtx.vin[0].scriptSig) != 0 || TotalPubkeyNormalInputs(createtx,srcpub) == 0)
		return eval->Invalid("vin.0 of token tag create transaction must be normal input signed by creator pubkey!");
	
	// Check other vouts, make sure there's at least one Tokens vout, with valid tokenid and full token supply value.
	// TokensValidate should make sure that the corresponding Tokens vin amounts match the value of the vouts.
	tokenlist = GetValidTagTokenIds(cpTokens, createtx);
	if (tokenlist.empty())
		return eval->Invalid("no valid token vouts found in token tag create transaction!");
	
	// Compare tokenid list size with updateamounts size, they need to match.
	if (tokenlist.size() != updateamounts.size())
		return eval->Invalid("Mismatched amount of tokenids vs updateamounts found in token tag create transaction!");
	
	// For each tokenid in list, check to make sure its respective required updateamount is sane (above 0, <= max supply).
	int i = 0;
	for (auto tokenid : tokenlist)
	{
		if (updateamounts[i] <= 0 || updateamounts[i] > CCfullsupply(tokenid))
			return eval->Invalid("Invalid updateamount for tokenid "+tokenid.GetHex()+" found in token tag create transaction!");
		i++;
	}

    return true;
}*/

bool TokenTagsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	CTransaction tokentagtx;
	uint256 tokentagid,hashBlock,prevupdatetxid;
	std::string data,name;
	uint8_t funcid,version,flags;
	std::vector<CAmount> updateamounts,origupdateamounts;
	int64_t maxupdates,updatenum,tokenbalance;
	int32_t numvins,numvouts;
	CScript opret;
	CPubKey srcpub,origsrcpub;
	std::vector<uint256> tokenlist;
	char globalCCaddress[65], srcCCaddress[65], tokenaddr[65];
	int i = 0;

	struct CCcontract_info *cpTokens, tokensC;
	cpTokens = CCinit(&tokensC, EVAL_TOKENS);

	return eval->Invalid("not done yet!");

	// Check boundaries, and verify that input/output amounts are exact.
	/*numvins = tx.vin.size();
	numvouts = tx.vout.size();
	if (numvouts < 1)
		return eval->Invalid("No vouts!");

	CCOpretCheck(eval,tx,true,true,true);
	ExactAmounts(eval,tx,ASSETCHAINS_CCZEROTXFEE[EVAL_TOKENTAGS]?0:CC_TXFEE);

	// Check the op_return of the transaction and fetch its function id.
	if (!MyGetCCopretV2(tx.vout[0].scriptPubKey, opret) || (funcid = DecodeTokenTagOpRet(opret)) != 0)
	{
		GetCCaddress(cp, globalCCaddress, GetUnspendable(cp, NULL));

		switch (funcid)
		{
			case 'c':
				// Token tag creation:
                // vin.0: normal input
                // vin.1: tokens
                // ...
                // vin.n-1: normal input
                // vout.0: baton to global CC address w/ OP_RETURN EVAL_TOKENTAGS 'c' version srcpub flags maxupdates updateamount
                // vout.1: tokens
                // vout.n-2: normal change
				// vout.n-1: empty op_return

				// Create transactions shouldn't trigger validation directly as they shouldn't contain tokentags CC inputs. 
				return eval->Invalid("unexpected TokenTagsValidate for 'c' type transaction!");

            case 'u':
				// Token tag update:
                // vin.0: normal input
                // vin.1: previous global OP_RETURN marker
                // ...
                // vin.n-1: normal input
                // vout.0: marker to global CC address w/ OP_RETURN EVAL_TOKENTAGS 'u' version tokentagid srcpub data updateamount
                // vout.n-2: normal change
				// vout.n-1: empty op_return

                // Get the data from the transaction's op_return.
                DecodeTokenTagUpdateOpRet(opret,version,tokentagid,srcpub,data,updateamounts);
				
                // Check appended data length (should be <= 128 chars).
				if (data.size() > 128)
					return eval->Invalid("data string over 128 chars!");
                
                // Get the tokentagid transaction.
				else if (myGetTransaction(tokentagid,tokentagtx,hashBlock) == 0 || tokentagtx.vout.size() == 0 ||
				!MyGetCCopretV2(tokentagtx.vout[0].scriptPubKey,opret) || DecodeTokenTagOpRet(opret) != 'c')
					return eval->Invalid("Original token tag create transaction not found!");

				// Validate the tokentagid transaction.
                else if (ValidateTokenTagCreateTx(cp,eval,tokentagtx) == 0)
					return (false);
                
                // Get the data from the tokentagid transaction's op_return.
				DecodeTokenTagCreateOpRet(opret,version,name,origsrcpub,flags,maxupdates,origupdateamounts);

                // If the TTF_CREATORONLY flag is set, make sure the update is signed by original srcpub.
                if (flags & TTF_CREATORONLY && srcpub != origsrcpub)
					return eval->Invalid("Signing pubkey of tag update transaction is not the tag creator pubkey!");
				
                // If the TTF_CONSTREQS flag is set, make sure the required token amounts for updates are kept the same.
                if (flags & TTF_CONSTREQS && updateamounts != origupdateamounts)
					return eval->Invalid("New required token amounts for updates are not the same as original requirements!");
                
				// TODO: why not just fetch the vinTx for the tx being validated, that would be more robust wouldn't it?
				// Get latest update from previous transaction.
				prevupdatetxid = GetLatestConfirmedTagUpdate(cp,tokentagid,updatenum);

				// Checking to make sure we don't exceed max allowed updates.
				if (updatenum >= maxupdates)
					return eval->Invalid("Maximum allowed amount of updates for this token tag exceeded, max updates is "+std::to_string(maxupdates)+", got "+std::to_string(updatenum)+"!");
                
                // Get the tokenids from the create tx.
				tokenlist = GetValidTagTokenIds(cpTokens, tokentagtx);

				GetTokensCCaddress(cpTokens, tokenaddr, srcpub);
				GetCCaddress(cp, srcCCaddress, srcpub);

                // Check token balance of tokenid from srcpub, at this blockheight. Compare with latest updateamount.
				for (auto tokenid : tokenlist)
				{
					if ((tokenbalance = CCTokenBalance(cpTokens,tokenaddr,tokenid)) < updateamounts[i])
						return eval->Invalid("Creator pubkey of token tag update doesn't own enough tokens for id: "+tokenid.GetHex()+", need "+std::to_string(updateamounts[i])+", got "+std::to_string(tokenbalance)+"!");
					i++;
				}

				// Check vout boundaries for tag update transaction.
                if (numvouts > 2)
					return eval->Invalid("Too many vouts for 's' type transaction!");
                
                // Verify that vin.0 was signed by srcpub.
				else if (IsCCInput(tx.vin[0].scriptSig) != 0 || TotalPubkeyNormalInputs(tx,srcpub) == 0)
					return eval->Invalid("vin.0 must be normal input signed by transaction creator pubkey!");

				// Verify that vin.1 was signed correctly.
				else if (ValidateTokenTagsCCVin(cp,eval,tx,1,0,prevupdatetxid,srcCCaddress,0) == 0)
					return (false);

				// Token tag updates shouldn't have any additional CC inputs.
				else if (ValidateTokenTagsNormalVins(eval,tx,2) == 0)
					return (false);

                // vout.0 must be global marker to CC address.
                else if (ConstrainVout(tx.vout[0], 1, globalCCaddress, CC_MARKER_VALUE) == 0)
					return eval->Invalid("vout.0 must be CC marker to TokenTags global CC address!");

				break;

			default:
				fprintf(stderr,"unexpected tokentags funcid (%c)\n",funcid);
				return eval->Invalid("Unexpected TokenTags function id!");
		}
	}
	else
		return eval->Invalid("Invalid TokenTags function id and/or data!");

	LOGSTREAM("tokentagscc", CCLOG_INFO, stream << "TokenTags transaction validated" << std::endl);
	return (true);*/
}

// --- End of consensus code ---

// --- Helper functions for RPC implementations ---

// --- RPC implementations for transaction creation ---
/*
UniValue TokenTagCreate(const CPubKey& pk,uint64_t txfee,std::string name,std::vector<uint256> tokenids,std::vector<CAmount> updateamounts,uint8_t flags,int64_t maxupdates)
{
	CScript opret;
	CPubKey mypk;
	int64_t total, inputs;
	UniValue sigData;
	char tokenaddr[KOMODO_ADDRESS_BUFSIZE];

	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_TOKENTAGS);
	struct CCcontract_info *cpTokens,CTokens;
	cpTokens = CCinit(&CTokens,EVAL_TOKENS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	if (!(mypk.IsFullyValid()))
	{
        CCerror = "mypk is not set or invalid";
        return NullUniValue;
    }

	// Temporary check for flags, will be removed/modified once more flags are introduced.
	if (flags > 2)
	{
        CCerror = "Unsupported flags set, only TTF_CREATORONLY and TTF_CONSTREQS currently available";
        return NullUniValue;
    }

	if (name.empty() || name.size() > 32)
	{
        CCerror = "Name should not be empty and be <= 32 characters";
		return NullUniValue;
	}
	if (tokenids.size() != updateamounts.size())
	{
        CCerror = "Invalid parameter, mismatched amount of specified tokenids vs updateamounts";
        return NullUniValue;
    }
	if (maxupdates < -1)
	{
        CCerror = "Invalid maxupdates, must be -1, 0 or any positive number";
        return NullUniValue;
    }

	opret = EncodeTokenTagCreateOpRet(TOKENTAGSCC_VERSION, name, mypk, flags, maxupdates, updateamounts);
	vscript_t vopret;
    GetOpReturnData(opret, vopret);
    std::vector<vscript_t> vData { vopret };

    GetTokensCCaddress(cpTokens, tokenaddr, mypk);

	if (AddNormalinputs2(mtx, txfee + CC_MARKER_VALUE, 5) > 0) // vin.*: normal input
	{
		// vout.0: marker to global CC address, with ccopret
		mtx.vout.push_back(MakeCC1vout(EVAL_TOKENTAGS, CC_MARKER_VALUE, GetUnspendable(cp, NULL), &vData));

		int i = 0;
		// Collecting specified tokenids and sending them back to the same address, to prove full ownership.
		for (std::vector<uint256>::const_iterator tokenid = tokenids.begin(); tokenid != tokenids.end(); tokenid++)
		{
			
			total = CCfullsupply(*tokenid);
			if (updateamounts[i] > total)
			{
				CCerror = "Invalid updateamount for tokenid "+(*tokenid).GetHex()+", exceeds max token supply";
				return NullUniValue;
			}

			// vin.1-*: tokens
			if ((inputs = AddTokenCCInputs(cpTokens, mtx, tokenaddr, *tokenid, total, 60)) > 0)
			{
				if (inputs < total)
				{
					CCerror = "Insufficient token inputs for tokenid "+(*tokenid).GetHex()+", retrieved "+std::to_string(inputs)+", requires "+std::to_string(total)+"";
					return NullUniValue;
				}
				else
				{
					std::vector<CPubKey> pks;
					pks.push_back(mypk);
					CScript tokenopret = EncodeTokenOpRetV1(*tokenid, pks, {});
        			vscript_t tokenvopret;
        			GetOpReturnData(tokenopret, tokenvopret);
        			std::vector<vscript_t> tokenvData { tokenvopret };

					// vout.1-*: tokens
            		mtx.vout.push_back(MakeTokensCC1vout(EVAL_TOKENS, inputs, mypk, &tokenvData));
				}
			}
			i++;
		}

		sigData = FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,CScript());
		if (!ResultHasTx(sigData))
		{
            CCerror = "Couldn't finalize token tag create transaction";
            return NullUniValue;
        }

		return sigData;
	}
    
	CCerror = "Error adding normal inputs, check if you have available funds or too many small value UTXOs";
	return NullUniValue;
}

UniValue TokenTagUpdate(const CPubKey& pk,uint64_t txfee,uint256 tokentagid,std::string data,std::vector<CAmount> newupdateamounts)
{
	CPubKey mypk, srcpub, tmp_srcpub;
	CTransaction tokentagtx, latesttx;
	uint256 hashBlock, latesttxid, tmp_tagid;
	CScript opret;
	uint8_t version, flags;
	std::string name, tmp_data;
	int64_t maxupdates, updatenum, i;
	char tokenaddr[KOMODO_ADDRESS_BUFSIZE], globalCCaddress[KOMODO_ADDRESS_BUFSIZE];
	std::vector<CAmount> updateamounts;
	std::vector<uint256> tokenids;
	
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cp,C;
	cp = CCinit(&C,EVAL_TOKENTAGS);
	struct CCcontract_info *cpTokens,CTokens;
	cpTokens = CCinit(&CTokens,EVAL_TOKENS);
	if (txfee == 0)
		txfee = CC_TXFEE;
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());

	if (!(mypk.IsFullyValid()))
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "mypk is not set or invalid");
	
	if (data.empty() || data.size() > 128)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Data should not be empty and be <= 128 characters");
	
	// Get tokentag creation transaction and its op_return.
	if (myGetTransaction(tokentagid, tokentagtx, hashBlock) == 0 || tokentagtx.vout.size() == 0 ||
	!MyGetCCopretV2(tokentagtx.vout[0].scriptPubKey, opret) || 
	DecodeTokenTagCreateOpRet(opret,version,name,srcpub,flags,maxupdates,updateamounts) != 'c')
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Specified token tag create transaction ID invalid");
	
	// Note: token tag create transaction may be invalid since there's no consensus code entry point for it.
	// We'll do a quick check to make sure we're not trying to update an invalid tag - main validation is done by ValidateTokenTagCreateTx.
	if (!(tokenids = GetValidTagTokenIds(cpTokens,tokentagtx)).empty())
	{
		// Compare tokenid list size with origupdateamounts size, they need to match.
		if (tokenids.size() != updateamounts.size())
			CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Mismatched amount of specified tokenids vs updateamounts within the specified token tag");
		
		// For each tokenid in list, check to make sure its respective required updateamount is sane (above 0, <= max supply).
		i = 0;
		for (auto tokenid : tokenids)
		{
			if (updateamounts[i] <= 0 || updateamounts[i] > CCfullsupply(tokenid))
				CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Invalid updateamount for tokenid "+tokenid.GetHex()+" within the specified token tag");
			i++;
		}
	}
	else
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Couldn't find valid token IDs within the specified token tag");

	latesttxid = GetLatestConfirmedTagUpdate(cp,tokentagid,updatenum);

	if (maxupdates > 0 && updatenum >= maxupdates)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Specified token tag cannot be updated anymore");
	// TODO also check if token tag closed manually

	// Check TTF_CREATORONLY constraints.
	if (flags & TTF_CREATORONLY && mypk != srcpub)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "This token tag cannot be updated by pubkeys other than tag's creator pubkey");
	
	// Get the latest updateamounts if latesttxid is not tokentagid.
	if (latesttxid != tokentagid)
	{
		myGetTransaction(latesttxid, latesttx, hashBlock);
		!MyGetCCopretV2(latesttx.vout[0].scriptPubKey, opret);
		DecodeTokenTagUpdateOpRet(opret,version,tmp_tagid,tmp_srcpub,tmp_data,updateamounts); // intentional updateamounts overwrite
	}

	// Check newupdateamounts.
	if (newupdateamounts.empty())
		newupdateamounts = updateamounts;
	else if (newupdateamounts.size() != updateamounts.size())
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Mismatched number of new updateamounts vs previous updateamounts");
	
	// Check TTF_CONSTREQS constraints.
	if (flags & TTF_CONSTREQS && newupdateamounts != updateamounts)
		CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "This token tag cannot have its updateamounts changed");
	
	GetTokensCCaddress(cpTokens, tokenaddr, mypk);

	// Check token balance of tokenid from mypk, compare with latest updateamount.
	i = 0;
	for (auto tokenid : tokenids)
	{
		if (CCTokenBalance(cpTokens,tokenaddr,tokenid) < updateamounts[i])
			CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "You don't own enough tokens for id: "+tokenid.GetHex()+", need at least "+std::to_string(updateamounts[i])+"");
		i++;
	}
	
	opret = EncodeTokenTagUpdateOpRet(TOKENTAGSCC_VERSION, tokentagid, mypk, data, newupdateamounts);
	vscript_t vopret;
    GetOpReturnData(opret, vopret);
    std::vector<vscript_t> vData { vopret };

	if (AddNormalinputs2(mtx, txfee, 5) > 0) // vin.0: normal input
	{
		// vin.1: previous global OP_RETURN marker
		GetCCaddress(cp, globalCCaddress, GetUnspendable(cp, NULL));
		mtx.vin.push_back(CTxIn(latesttxid,0,CScript()));

		// vout.0: marker to global CC address, with ccopret
		mtx.vout.push_back(MakeCC1vout(EVAL_TOKENTAGS, CC_MARKER_VALUE, GetUnspendable(cp, NULL), &vData));
		
		return FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,CScript());
	}

	CCERR_RESULT("agreementscc", CCLOG_INFO, stream << "Error adding normal inputs, check if you have available funds or too many small value UTXOs");
}

UniValue TokenTagClose(const CPubKey& pk,uint64_t txfee,uint256 tokentagid,std::string data)
{
	CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "not done yet");
}

// --- RPC implementations for transaction analysis ---

UniValue TokenTagInfo(uint256 txid)
{
	UniValue result(UniValue::VOBJ);
	char str[67];
	uint256 hashBlock,latesttxid;
	uint8_t version,funcid,flags;
	CPubKey srcpub;
	CTransaction tx;
	std::string name;
	int32_t numvouts;
	int64_t maxupdates,updatenum;
	std::vector<uint256> tokenids;
	std::vector<CAmount> updateamounts;
	CScript opret;

	struct CCcontract_info *cp,C,*cpTokens,CTokens;
	cp = CCinit(&C,EVAL_TOKENTAGS);
	cpTokens = CCinit(&CTokens,EVAL_TOKENS);

	if (myGetTransaction(txid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
	MyGetCCopretV2(tx.vout[0].scriptPubKey, opret) &&
	(funcid = DecodeTokenTagCreateOpRet(opret,version,name,srcpub,flags,maxupdates,updateamounts)) == 'c')
	{
		if ((tokenids = GetValidTagTokenIds(cpTokens,tx)).empty())
			CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Couldn't find valid token IDs within the specified token tag");

		if ((latesttxid = GetLatestConfirmedTagUpdate(cp,txid,updatenum)) == zeroid)
			CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Couldn't find latest confirmed token tag update");

		result.push_back(Pair("result","success"));
		result.push_back(Pair("txid",txid.GetHex()));

		result.push_back(Pair("name",name));
		result.push_back(Pair("creator_pubkey",pubkey33_str(str,(uint8_t *)&srcpub)));

		result.push_back(Pair("latest_update",latesttxid.GetHex()));
		result.push_back(Pair("updates",updatenum));
		result.push_back(Pair("max_updates",maxupdates));

		// TODO iterate thru tokenids and updateamounts here

		int i = 0;
		for (auto tokenid : tokenids)
		{
       		result.push_back(Pair("tokenid"+std::to_string(i)+"",tokenid.GetHex()));
			i++;
		}

		return (result);
	}

	CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "Invalid token tag creation transaction ID");
}

UniValue TokenTagSamples(uint256 tokentagid, int64_t samplenum, bool bReverse)
{
	CCERR_RESULT("tokentagscc", CCLOG_INFO, stream << "not done yet");
}

UniValue TokenTagList(uint256 tokenid, CPubKey pubkey)
{
	UniValue result(UniValue::VARR);
	std::vector<uint256> tokentagids, tokenids;
	char globalCCaddress[KOMODO_ADDRESS_BUFSIZE];
	CTransaction tx;
	uint256 hashBlock;
	CScript opret;
	uint8_t version,flags;
	CPubKey srcpub;
	std::string name;
	int64_t maxupdates;
	std::vector<CAmount> updateamounts;

	struct CCcontract_info *cp,C,*cpTokens,CTokens;
	cp = CCinit(&C,EVAL_TOKENTAGS);
	cpTokens = CCinit(&CTokens,EVAL_TOKENS);

	GetCCaddress(cp, globalCCaddress, GetUnspendable(cp, NULL));

	SetCCtxids(tokentagids, globalCCaddress, true, EVAL_TOKENTAGS, CC_MARKER_VALUE, zeroid, 'c');
	// TODO: look into SetCCtxids with filtertxid, how does it work?

	for (std::vector<uint256>::const_iterator it = tokentagids.begin(); it != tokentagids.end(); it++)
	{
		tokenids.clear();
		opret = CScript();

		if (myGetTransaction(*it, tx, hashBlock) != 0 && tx.vout.size() > 0 &&
		MyGetCCopretV2(tx.vout[0].scriptPubKey, opret) && DecodeTokenTagOpRet(opret) == 'c')
		{
			// Check for referenced tokenid, if specified.
			if (tokenid != zeroid && ((tokenids = GetValidTagTokenIds(cpTokens,tx)).empty() ||
			std::find(tokenids.begin(), tokenids.end(), tokenid) == tokenids.end()))
			{
				continue;
			}
			
			// Check for the creator pubkey, if specified.
			if (pubkey.IsFullyValid())
			{
				DecodeTokenTagCreateOpRet(opret,version,name,srcpub,flags,maxupdates,updateamounts);
				if (srcpub != pubkey)
					continue;
			}

			result.push_back((*it).GetHex());
        }
	}

	return (result);
}*/

// --- Useful misc functions for additional token transaction analysis ---

// internal function that looks for voutPubkeys or destaddrs in token tx oprets
// used by TokenOwners
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
        if (IsTokensvout(true, true, cp, NULL, tx, n, tokenid))
        {
			// Get the opret from either vout.n or tx.vout.back() scriptPubkey
			if (!MyGetCCopretV2(tx.vout[n].scriptPubKey, opret))
				opret = tx.vout.back().scriptPubKey;
			uint8_t funcId = DecodeTokenOpRetV1(opret, tokenIdOpret, voutPubkeys, oprets);

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
                GetTokenOwnerList(spendingtx, cp, tokenid, depth, maxdepth, OwnerList);
            }
        }
    }

	depth--;
    return;
}

UniValue TokenOwners(uint256 tokenid, int64_t minbalance, int64_t maxdepth)
{
    // NOTE: maybe add option to retrieve addresses instead, including 1of2 addresses?
	UniValue result(UniValue::VARR); 
    CTransaction tokenbaseTx; 
    uint256 hashBlock;
	uint8_t funcid;
    std::vector<uint8_t> origpubkey;
    std::string name, description; 
    std::vector<CPubKey> OwnerList;
	int64_t depth;
    char str[67];

    struct CCcontract_info *cpTokens, tokensCCinfo;
    cpTokens = CCinit(&tokensCCinfo, EVAL_TOKENS);

    // Get token create tx
    if (!myGetTransaction(tokenid, tokenbaseTx, hashBlock) || (KOMODO_NSPV_FULLNODE && hashBlock.IsNull()))
    {
        LOGSTREAMFN(cctokens_log, CCLOG_INFO, stream << "cant find tokenid" << std::endl);
        return(result);
    }

    // Checking if passed tokenid is a token creation txid
    funcid = DecodeTokenCreateOpRetV1(tokenbaseTx.vout.back().scriptPubKey, origpubkey, name, description);
	if (tokenbaseTx.vout.size() > 0 && !IsTokenCreateFuncid(funcid))
	{
        LOGSTREAMFN(cctokens_log, CCLOG_INFO, stream << "passed tokenid isnt token creation txid" << std::endl);
        return(result);
    }

    // Get a full list of owners using a recursive looping function
    GetTokenOwnerList(tokenbaseTx, cpTokens, tokenid, depth, maxdepth, OwnerList);

    // Add owners to result array
    for (auto owner : OwnerList)
        if (minbalance == 0 || GetTokenBalance(owner, tokenid, false) >= minbalance)
			result.push_back(pubkey33_str(str,(uint8_t *)&owner));

	return result;
}

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
            if (IsTokenMarkerVout(vintx.vout[n]))
                continue;

            // Get the opret from either vout.n or tx.vout.back() scriptPubkey
            if (!MyGetCCopretV2(vintx.vout[n].scriptPubKey, opret))
                opret = vintx.vout.back().scriptPubKey;
            uint8_t funcid = DecodeTokenOpRetV1(opret, tokenIdInOpret, voutPubkeys, oprets);

            // If the vout is from a token creation tx, the tokenid will be hash of vintx
            if (IsTokenCreateFuncid(funcid))
                tokenIdInOpret = vintx.GetHash();
            
            // If the vout is not from a token creation tx, check if it is a token vout
            if (IsTokenCreateFuncid(funcid) || IsTokensvout(true, true, cpTokens, NULL, vintx, n, tokenIdInOpret))
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
		if (minbalance == 0 || GetTokenBalance(pk, tokenid, false) >= minbalance)
            result.push_back(tokenid.GetHex());

	return result;
}
