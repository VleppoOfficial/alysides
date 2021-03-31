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

#include "CCTokenTags.h"
#include "CCtokens.h"

bool TokenTagsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	return eval->Invalid("not supported yet");
}

// internal function that looks for voutPubkeys in token tx oprets
// used by TokenOwners
void GetTokenOwnerPubkeys(const CTransaction tx, struct CCcontract_info *cp, uint256 tokenid, std::vector<CPubKey> &OwnerList)
{
    // TODO: add "max depth" variable to avoid stack overflows from recursive calls

    // Examine each vout in the tx (except last vout)
    for (int64_t n = 0; n <= tx.vout.size() - 1; n++)
    {
        CScript opret;
        uint256 tokenIdOpret, spendingtxid, hashBlock;
        CTransaction spendingtx;
        std::vector<vscript_t>  oprets;
        std::vector<CPubKey> voutPubkeys;
        int32_t vini, height;

        // We ignore every vout that's not a tokens vout, so we check for that
        if (IsTokensvout(true, true, cp, NULL, tx, n, tokenid))
        {
            // Get the opret from either vout.n or tx.vout.back() scriptPubkey
            if (!MyGetCCopretV2(tx.vout[n].scriptPubKey, opret))
                opret = tx.vout.back().scriptPubKey;
            uint8_t funcId = DecodeTokenOpRetV1(opret, tokenIdOpret, voutPubkeys, oprets);

            // Include only pubkeys from voutPubkeys arrays with 1 element.
            // If voutPubkeys size is >= 2 then the vout was probably sent to a CC 1of2 address, which
            // might have no true "owner" and is therefore outside the scope of this function
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
                // Same procedure for the spending tx, until no more are found
                GetTokenOwnerPubkeys(spendingtx, cp, tokenid, OwnerList);
            }
        }
    }

    return;
}

UniValue TokenOwners(uint256 tokenid, int64_t minbalance)
{
    // TODO: add option to retrieve addresses instead, including 1of2 addresses
	UniValue result(UniValue::VARR); 
    CTransaction tokenbaseTx; 
    uint256 hashBlock;
	uint8_t funcid;
    std::vector<uint8_t> origpubkey;
    std::string name, description; 
    std::vector<CPubKey> OwnerList;
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

    // Get a full list of owner pubkeys using a recursive looping function
    GetTokenOwnerPubkeys(tokenbaseTx, cpTokens, tokenid, OwnerList);
    // TODO: maybe remove/skip known CC global pubkeys from the list?

    // Add pubkeys to result array
    for (auto pk : OwnerList)
        if (minbalance == 0 || GetTokenBalance(pk, tokenid, false) >= minbalance)
            result.push_back(pubkey33_str(str,(uint8_t *)&pk));

	return result;
}

UniValue TokenInventory(const CPubKey pk, int64_t minbalance)
{
    // TODO: add option to specify an address instead of a pubkey
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
