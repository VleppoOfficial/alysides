/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
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

#include "CCtokens.h"
#include "importcoin.h"

/* TODO: correct this:
-----------------------------
 The SetTokenFillamounts() and ValidateTokenRemainder() work in tandem to calculate the vouts for a fill and to validate the vouts, respectively.
 
 This pair of functions are critical to make sure the trading is correct and is the trickiest part of the tokens contract.
 
 //vin.0: normal input
 //vin.1: unspendable.(vout.0 from buyoffer) buyTx.vout[0]
 //vin.2+: valid CC output satisfies buyoffer (*tx.vin[2])->nValue
 //vout.0: remaining amount of bid to unspendable
 //vout.1: vin.1 value to signer of vin.2
 //vout.2: vin.2 tokenoshis to original pubkey
 //vout.3: CC output for tokenoshis change (if any)
 //vout.4: normal output for change (if any)
 //vout.n-1: opreturn [EVAL_ASSETS] ['B'] [tokenid] [remaining token required] [origpubkey]
    ValidateTokenRemainder(remaining_price,tx.vout[0].nValue,nValue,tx.vout[1].nValue,tx.vout[2].nValue,totalunits);
 
 Yes, this is quite confusing...
 
 In ValidateTokenRemainder the naming convention is nValue is the coin/token with the offer on the books and "units" is what it is being paid in. The high level check is to make sure we didnt lose any coins or tokens, the harder to validate is the actual price paid as the "orderbook" is in terms of the combined nValue for the combined totalunits.
 
 We assume that the effective unit cost in the orderbook is valid and that that amount was paid and also that any remainder will be close enough in effective unit cost to not matter. At the edge cases, this will probably be not true and maybe some orders wont be practically fillable when reduced to fractional state. However, the original pubkey that created the offer can always reclaim it.
 ------------------------------
*/

// tx validation
bool TokensValidate(struct CCcontract_info* cp, Eval* eval, const CTransaction& tx, uint32_t nIn)
{
    //Make exception for early Rogue chain
    if (strcmp(ASSETCHAINS_SYMBOL, "ROGUE") == 0 && chainActive.Height() <= 12500)
        return true;

    CTransaction createTx, referenceTx; //the token creation tx
    uint256 hashBlock, tokenid = zeroid, referenceTokenId, dummyRefTokenId, updatetokenid;
    int32_t numvins = tx.vin.size(), numvouts = tx.vout.size(), numblocks, vini, height;
    int64_t outputs = 0, inputs = 0, expiryTimeSec;
    std::vector<CPubKey> vinTokenPubkeys, voutTokenPubkeys; // sender pubkey(s) and destpubkey(s)
    std::vector<uint8_t> creatorPubkey, dummyPubkey, updaterPubkey; // token creator pubkey
    char updaterPubkeyaddr[64], srcaddr[64], destaddr[64], burnaddr[64], testburnaddr[64];
    
    int32_t preventCCvins = -1, preventCCvouts = -1; // debugging
    
    uint8_t funcid, evalCodeInOpret; // the funcid and eval code embedded in the opret
    std::vector<std::pair<uint8_t, vscript_t>> oprets, refoprets; // additional data embedded in the opret
    std::string dummyName, dummyDescription, tokenType, refTokenType;
    double ownerPerc;
    bool isSpendingBaton = false;

    // check boundaries:
    if (numvouts < 1)
        return eval->Invalid("no vouts\n");
	
    if ((funcid = DecodeTokenOpRet(tx.vout[numvouts - 1].scriptPubKey, evalCodeInOpret, tokenid, oprets)) == 0)
        return eval->Invalid("TokenValidate: invalid opreturn payload");
	
    LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "TokensValidate funcId=" << (char)(funcid ? funcid : ' ') << " evalcode=" << std::hex << (int)cp->evalcode << std::endl);
    
    if (eval->GetTxUnconfirmed(tokenid, createTx, hashBlock) == 0 || !myGetTransaction(tokenid, createTx, hashBlock))
        return eval->Invalid("cant find token create txid");
    else if (funcid != 'c')
    {
        if (tokenid == zeroid)
            return eval->Invalid("illegal tokenid");
        else if (!TokensExactAmounts(true, cp, inputs, outputs, eval, tx, tokenid))
        {
            if (!eval->Valid())
                return false; //TokenExactAmounts must call eval->Invalid()!
            else
                return eval->Invalid("tokens cc inputs != cc outputs");
        }
		
        //get token create tx info
        if (createTx.vout.size() > 0 && DecodeTokenCreateOpRet(createTx.vout[createTx.vout.size() - 1].scriptPubKey, creatorPubkey, dummyName, dummyDescription, ownerPerc, tokenType, referenceTokenId, expiryTimeSec, oprets) != 'c')
        {
            return eval->Invalid("incorrect token create txid funcid\n");
        }
    }

    // validate spending from token cc addr: allowed only for burned non-fungible tokens:
    if (ExtractTokensCCVinPubkeys(tx, vinTokenPubkeys) && std::find(vinTokenPubkeys.begin(), vinTokenPubkeys.end(), GetUnspendable(cp, NULL)) != vinTokenPubkeys.end())
    {
        // validate spending from token unspendable cc addr:
        int64_t burnedAmount = HasBurnedTokensvouts(cp, eval, tx, tokenid);
        if (burnedAmount > 0)
        {
            vscript_t vopretNonfungible;
            GetNonfungibleData(tokenid, vopretNonfungible);
            if (vopretNonfungible.empty())
                return eval->Invalid("spending cc marker not supported for fungible tokens");
        }
    }

    //asset and licensing validation
    /*if (tokenType != "a" && tokenType != "m" && tokenType != "s") //might break other chain validation, don't use
        return eval->Invalid("invalid tokentype");*/
    if (tokenType == "m" || tokenType == "s")
    {
        if (CCduration(numblocks, tokenid) > expiryTimeSec && expiryTimeSec != 0)
            return eval->Invalid("license token is expired");
        if (referenceTokenId == zeroid)
            return eval->Invalid("license reftokenid is null");
        else if (eval->GetTxUnconfirmed(referenceTokenId, referenceTx, hashBlock) == 0 ||
            !myGetTransaction(referenceTokenId, referenceTx, hashBlock) ||
            DecodeTokenCreateOpRet(referenceTx.vout[referenceTx.vout.size() - 1].scriptPubKey, dummyPubkey, dummyName, dummyDescription, ownerPerc, refTokenType, dummyRefTokenId, expiryTimeSec, refoprets) != 'c')
            return eval->Invalid("couldn't find and decode reftokenid transaction for license");
        if (!((tokenType == "m" && refTokenType == "a") || (tokenType == "s" && refTokenType == "m")))
            return eval->Invalid("incorrect relation between tokentype and reftokentype for license");
    }
	
    //Non-update transactions cannot spend update baton
    for (int32_t i = 0; i < numvins; i++)
    {
        if((tx.vin[i].prevout.hash == tokenid && tx.vin[i].prevout.n == 2) || //in tokenid tx, baton vout is vout2
            (eval->GetTxUnconfirmed(tx.vin[i].prevout.hash, referenceTx, hashBlock) != 0 && myGetTransaction(tx.vin[i].prevout.hash, referenceTx, hashBlock) &&
            DecodeTokenUpdateOpRet(referenceTx.vout[referenceTx.vout.size() - 1].scriptPubKey, dummyPubkey, updatetokenid) == 'u' &&
            tx.vin[i].prevout.n == 0)) //in update tx, baton vout is vout0
            isSpendingBaton = true;
        //std::cerr << "tx.vin[" << i << "].prevout.hash hex=" << tx.vin[i].prevout.hash.GetHex() << std::endl;
    }
	
    if (funcid != 'u' && isSpendingBaton)
        return eval->Invalid("attempting to spend update batonvout in non-update tx");
	
    switch (funcid)
    {
        case 'c':
            // token create should not be validated as it has no CC inputs, so return 'invalid'
            // token tx structure for 'c':
            //vin.0: normal input
            //vout.0: normal marker
            //vout.1: issuance tokenoshis to CC
            //vout.2: token update baton with cc opret
            //vout.3 to n-2: normal output for change (if any)
            //vout.n-1: opreturn EVAL_TOKENS 'c' <tokenname> <description> <etc>
            return eval->Invalid("incorrect token funcid");

        case 't':
            // transfer
            // token tx structure for 't'
            //vin.0: normal input
            //vin.1 .. vin.n-1: valid CC outputs
            //vout.0 to n-2: tokenoshis output to CC
            //vout.n-2: normal output for change (if any)
            //vout.n-1: opreturn EVAL_TOKENS 't' tokenid <other contract payload>
            
            //Check if token amount is the same in vins and vouts of tx
            if (inputs == 0)
                return eval->Invalid("no token inputs for transfer");
            
            // retrieving destpubkey(s)
            if (tokenType == "s" && DecodeTokenTransferOpRet(tx.vout[numvouts - 1].scriptPubKey, tokenid, voutTokenPubkeys, oprets) != 't')
                return eval->Invalid("unable to verify token transfer tx funcid");
            
            //get burn pubkey token CC address
            GetTokensCCaddress(cp, burnaddr, pubkey2pk(ParseHex(CC_BURNPUBKEY)));
            
            // if token is sub-license and transaction vin isn't from creator vout
            if (tokenType == "s" && std::find(vinTokenPubkeys.begin(), vinTokenPubkeys.end(), pubkey2pk(creatorPubkey)) == vinTokenPubkeys.end())
            {
                //check if burn pubkey is specified in voutPubkeys
                if(std::find(voutTokenPubkeys.begin(), voutTokenPubkeys.end(), pubkey2pk(ParseHex(CC_BURNPUBKEY))) == voutTokenPubkeys.end())
                    return eval->Invalid("burn pubkey not specified in non-creator sub license transfer");
                else
                    for (auto vout : tx.vout)
                    {
                        if(!(vout.scriptPubKey.size() > 3 && vout.scriptPubKey[0] == OP_RETURN && vout.scriptPubKey[2] == EVAL_TOKENS) && !(Getscriptaddress(testburnaddr, vout.scriptPubKey)))
                            return eval->Invalid("couldn't get proper destination script address from a license transfer vout");
                        //only allow transfers to burn address (don't allow receiving change either - if we're burning "s" tokens, we have to burn all our stock)
                        if (vout.scriptPubKey.IsPayToCryptoCondition() && (!IsTokenMarkerVout(vout)) && (strcmp(testburnaddr, burnaddr) != 0))
                            return eval->Invalid("cannot transfer sub-license to non-burn address from pubkey other than creator pubkey");
                    }
            }
            
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "token transfer preliminarily validated inputs=" << inputs << "->outputs=" << outputs << " preventCCvins=" << preventCCvins << " preventCCvouts=" << preventCCvouts << std::endl);
            break; // breaking to other contract validation...
        
        case 'u':
            // token update
            // token tx structure for 'u':
            //vin.0: normal input
            //vin.1: previous token CC update baton
            //vout.0: next token update baton with cc opret
            //vout.1 to n-2: normal output for change (if any)
            //vout.n-1: opreturn EVAL_TOKENS 'u' pk tokenid

			if (inputs != 0)
                return eval->Invalid("update tx cannot have token inputs");
			
            //Checking if update tx is actually spending the baton
            if (!isSpendingBaton)
                return eval->Invalid("update tx is not spending update baton");
			
            if (DecodeTokenUpdateOpRet(tx.vout[tx.vout.size() - 1].scriptPubKey, updaterPubkey, updatetokenid) != 'u')
                return eval->Invalid("invalid update tx opret data");
            if (updatetokenid != tokenid)
                return eval->Invalid("incorrect tokenid in update tx opret");
			
            // Verifying updaterPubkey by checking tx.vin[0] and tx.vout[1] addresses
            // These addresses should be equal to each other, and updaterPubkey address should be equal to both
            if (!Getscriptaddress(destaddr, tx.vout[1].scriptPubKey))
                return eval->Invalid("couldn't locate vout1 destaddr");
            if (!(eval->GetTxUnconfirmed(tx.vin[0].prevout.hash, referenceTx, hashBlock) != 0 && myGetTransaction(tx.vin[0].prevout.hash, referenceTx, hashBlock) &&
                Getscriptaddress(srcaddr, referenceTx.vout[tx.vin[0].prevout.n].scriptPubKey)))
                return eval->Invalid("couldn't locate vin0 srcaddr");
            if (strcmp(srcaddr, destaddr) != 0)
                return eval->Invalid("normal input srcaddr != destaddr");
            if (!Getscriptaddress(updaterPubkeyaddr,CScript() << updaterPubkey << OP_CHECKSIG))
                return eval->Invalid("couldn't get updaterPubkey script address");
            if (strcmp(updaterPubkeyaddr, srcaddr) != 0 || strcmp(updaterPubkeyaddr, destaddr) != 0)
                return eval->Invalid("updaterPubkey address doesn't match srcaddr or destaddr");
            
            // if asset or master license: check if tokenid ownership percent > ownerperc
            if (tokenType == "a" || tokenType == "m")
            {
                if (GetTokenOwnershipPercent(pubkey2pk(updaterPubkey), tokenid) < ownerPerc)
                    return eval->Invalid("updater pubkey does not own enough tokens to update");
            }
            
            //if master or sub license: must be creator
            if (tokenType == "m" || tokenType == "s")
            {
                if (updaterPubkey != creatorPubkey)
                    return eval->Invalid("licenses must be updated by creator pubkey");
            }
			
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "token update preliminarily validated inputs=" << inputs << "->outputs=" << outputs << " preventCCvins=" << preventCCvins << " preventCCvouts=" << preventCCvouts << std::endl);
            break;
            
        //case 'whatever':
            //tx model
            //validation stuff
            //break;

        default:
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "illegal tokens funcid=" << (char)(funcid ? funcid : ' ') << std::endl);
            return eval->Invalid("unexpected token funcid");
    }

    return true;
}

// helper funcs:

// extract cc token vins' pubkeys:
bool ExtractTokensCCVinPubkeys(const CTransaction& tx, std::vector<CPubKey>& vinPubkeys)
{
    bool found = false;
    CPubKey pubkey;
    struct CCcontract_info *cpTokens, tokensC;

    cpTokens = CCinit(&tokensC, EVAL_TOKENS);
    vinPubkeys.clear();

    for (int32_t i = 0; i < tx.vin.size(); i++) {
        // check for cc token vins:
        if ((*cpTokens->ismyvin)(tx.vin[i].scriptSig)) {
            auto findEval = [](CC* cond, struct CCVisitor _) {
                bool r = false;

                if (cc_typeId(cond) == CC_Secp256k1) {
                    *(CPubKey*)_.context = buf2pk(cond->publicKey);
                    //std::cerr << "findEval found pubkey=" << HexStr(*(CPubKey*)_.context) << std::endl;
                    r = true;
                }
                // false for a match, true for continue
                return r ? 0 : 1;
            };

            CC* cond = GetCryptoCondition(tx.vin[i].scriptSig);

            if (cond) {
                CCVisitor visitor = {findEval, (uint8_t*)"", 0, &pubkey};
                bool out = !cc_visit(cond, visitor);
                cc_free(cond);

                if (pubkey.IsValid()) {
                    vinPubkeys.push_back(pubkey);
                    found = true;
                }
            }
        }
    }
    return found;
}

// this is just for log messages indentation fur debugging recursive calls:
thread_local uint32_t tokenValIndentSize = 0;

// validates opret for token tx:
uint8_t ValidateTokenOpret(CTransaction tx, uint256 tokenid)
{
    uint256 tokenidOpret = zeroid;
    uint8_t funcid;
    uint8_t dummyEvalCode;
    //std::vector<CPubKey> voutPubkeysDummy;
    std::vector<std::pair<uint8_t, vscript_t>> opretsDummy;

    // this is just for log messages indentation fur debugging recursive calls:
    std::string indentStr = std::string().append(tokenValIndentSize, '.');

    if (tx.vout.size() == 0)
        return (uint8_t)0;

    if ((funcid = DecodeTokenOpRet(tx.vout.back().scriptPubKey, dummyEvalCode, tokenidOpret, /*voutPubkeysDummy, */opretsDummy)) == 0) {
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << indentStr << "ValidateTokenOpret() DecodeTokenOpret could not parse opret for txid=" << tx.GetHash().GetHex() << std::endl);
        return (uint8_t)0;
    } else if (funcid == 'c') {
        if (tokenid != zeroid && tokenid == tx.GetHash()) {
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "ValidateTokenOpret() this is tokenbase 'c' tx, txid=" << tx.GetHash().GetHex() << " returning true" << std::endl);
            return funcid;
        } else {
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "ValidateTokenOpret() not my tokenbase txid=" << tx.GetHash().GetHex() << std::endl);
        }
    } else if (funcid == 'i') {
        if (tokenid != zeroid && tokenid == tx.GetHash()) {
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "ValidateTokenOpret() this is import 'i' tx, txid=" << tx.GetHash().GetHex() << " returning true" << std::endl);
            return funcid;
        } else {
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "ValidateTokenOpret() not my import txid=" << tx.GetHash().GetHex() << std::endl);
        }
    } else if (funcid == 't') {
        //std::cerr << indentStr << "ValidateTokenOpret() tokenid=" << tokenid.GetHex() << " tokenIdOpret=" << tokenidOpret.GetHex() << " txid=" << tx.GetHash().GetHex() << std::endl;
        if (tokenid != zeroid && tokenid == tokenidOpret) {
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "ValidateTokenOpret() this is a transfer 't' tx, txid=" << tx.GetHash().GetHex() << " returning true" << std::endl);
            return funcid;
        } else {
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "ValidateTokenOpret() not my tokenid=" << tokenidOpret.GetHex() << std::endl);
        }
    } else if (funcid == 'u') {
        if (tokenid != zeroid && tokenid == tokenidOpret) {
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "ValidateTokenOpret() this is a tokenupdate 'u' tx, txid=" << tx.GetHash().GetHex() << " returning true" << std::endl);
            return funcid;
        } else {
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "ValidateTokenOpret() not my tokenid=" << tokenidOpret.GetHex() << std::endl);
        }
    } else {
        LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "ValidateTokenOpret() not supported funcid=" << (char)funcid << " tokenIdOpret=" << tokenidOpret.GetHex() << " txid=" << tx.GetHash().GetHex() << std::endl);
    }
    return (uint8_t)0;
}

// remove token->unspendablePk (it is only for marker usage)
void FilterOutTokensUnspendablePk(const std::vector<CPubKey>& sourcePubkeys, std::vector<CPubKey>& destPubkeys)
{
    struct CCcontract_info *cpTokens, tokensC;
    cpTokens = CCinit(&tokensC, EVAL_TOKENS);
    CPubKey tokensUnspendablePk = GetUnspendable(cpTokens, NULL);
    destPubkeys.clear();

    for (auto pk : sourcePubkeys)
        if (pk != tokensUnspendablePk)
            destPubkeys.push_back(pk);
}

void FilterOutNonCCOprets(const std::vector<std::pair<uint8_t, vscript_t>>& oprets, vscript_t& vopret)
{
    vopret.clear();

    if (oprets.size() > 2) {
        LOGSTREAM("cctokens", CCLOG_INFO, stream << "FilterOutNonCCOprets() warning!! oprets.size > 2 currently not supported" << oprets.size() << std::endl);
        std::cerr << "FilterOutNonCCOprets() warning!! oprets.size > 2 currently not supported" << oprets.size() << std::endl;
    }

    for (auto o : oprets) {
        if (o.first < OPRETID_FIRSTNONCCDATA) { // skip burn, import, etc opret data
            vopret = o.second;                  // return first contract opret (more than 1 is not supported yet)
            break;
        }
    }
}

// Checks if the vout is a really Tokens CC vout
// also checks tokenid in opret or txid if this is 'c' tx
// goDeeper is true: the func also validates amounts of the passed transaction:
// it should be either sum(cc vins) == sum(cc vouts) or the transaction is the 'tokenbase' ('c') tx
// checkPubkeys is true: validates if the vout is token vout1 or token vout1of2. Should always be true!
int64_t IsTokensvout(bool goDeeper, bool checkPubkeys /*<--not used, always true*/, struct CCcontract_info* cp, Eval* eval, const CTransaction& tx, int32_t v, uint256 reftokenid)
{
    // this is just for log messages indentation fur debugging recursive calls:
    std::string indentStr = std::string().append(tokenValIndentSize, '.');

    LOGSTREAM((char*)"cctokens", CCLOG_DEBUG2, stream << indentStr << "IsTokensvout() entered for txid=" << tx.GetHash().GetHex() << " v=" << v << " for tokenid=" << reftokenid.GetHex() << std::endl);

    int32_t n = tx.vout.size();
    // just check boundaries:
    if (n == 0 || v < 0 || v >= n - 1) {
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << indentStr << "isTokensvout() incorrect params: (n == 0 or v < 0 or v >= n-1)"
                                                        << " v=" << v << " n=" << n << " returning 0" << std::endl);
        return (0);
    }

    if (tx.vout[v].scriptPubKey.IsPayToCryptoCondition()) {
        if (goDeeper) {
            //validate all tx
            int64_t myCCVinsAmount = 0, myCCVoutsAmount = 0;

            tokenValIndentSize++;
            // false --> because we already at the 1-st level ancestor tx and do not need to dereference ancestors of next levels
            bool isEqual = TokensExactAmounts(false, cp, myCCVinsAmount, myCCVoutsAmount, eval, tx, reftokenid);
            tokenValIndentSize--;

            if (!isEqual) {
                // if ccInputs != ccOutputs and it is not the tokenbase tx
                // this means it is possibly a fake tx (dimxy):
                
                //std::cerr << indentStr << "vout" << v << " has unequal cc inputs/outputs" << std::endl;
                if (reftokenid != tx.GetHash()) { // checking that this is the true tokenbase tx, by verifying that funcid=c, is done further in this function (dimxy)
                    
                    //std::cerr << indentStr << "vout" << v << " has is not tokencreate tx, returning 0" << std::endl;
                    LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << indentStr << "IsTokensvout() warning: for the verified tx detected a bad vintx=" << tx.GetHash().GetHex() << ": cc inputs != cc outputs and not the 'tokenbase' tx, skipping the verified tx" << std::endl);
                    return 0;
                }
            }
        }

        // token opret most important checks (tokenid == reftokenid, tokenid is non-zero, tx is 'tokenbase'):
        const uint8_t funcId = ValidateTokenOpret(tx, reftokenid);
        //std::cerr << indentStr << "IsTokensvout() ValidateTokenOpret returned=" << (char)(funcId?funcId:' ') << " for txid=" << tx.GetHash().GetHex() << " for tokenid=" << reftokenid.GetHex() << std::endl;
        if (funcId != 0) {
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG2, stream << indentStr << "IsTokensvout() ValidateTokenOpret returned not-null funcId=" << (char)(funcId ? funcId : ' ') << " for txid=" << tx.GetHash().GetHex() << " for tokenid=" << reftokenid.GetHex() << std::endl);

            uint8_t dummyEvalCode;
            uint256 tokenIdOpret;
            std::vector<CPubKey> voutPubkeys, voutPubkeysInOpret;
            vscript_t vopretExtra, vopretNonfungible;
            std::vector<std::pair<uint8_t, vscript_t>> oprets;

            uint8_t evalCodeNonfungible = 0;
            uint8_t evalCode1 = EVAL_TOKENS; // if both payloads are empty maybe it is a transfer to non-payload-one-eval-token vout like GatewaysClaim
            uint8_t evalCode2 = 0;           // will be checked if zero or not

            // test vouts for possible token use-cases:
            std::vector<std::pair<CTxOut, std::string>> testVouts;
			
            //DecodeTokenTransferOpRet used here since destpubkey array is expected to be returned
            DecodeTokenTransferOpRet(tx.vout.back().scriptPubKey, tokenIdOpret, voutPubkeysInOpret, oprets);
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG2, stream << "IsTokensvout() oprets.size()=" << oprets.size() << std::endl);

            // get assets/channels/gateways token data:
            FilterOutNonCCOprets(oprets, vopretExtra); // NOTE: only 1 additional evalcode in token opret is currently supported
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG2, stream << "IsTokensvout() vopretExtra=" << HexStr(vopretExtra) << std::endl);

            // get non-fungible data
            GetNonfungibleData(reftokenid, vopretNonfungible);
            FilterOutTokensUnspendablePk(voutPubkeysInOpret, voutPubkeys); // cannot send tokens to token unspendable cc addr (only marker is allowed there)

            // NOTE: evalcode order in vouts is important:
            // non-fungible-eval -> EVAL_TOKENS -> assets-eval

            if (vopretNonfungible.size() > 0)
                evalCodeNonfungible = evalCode1 = vopretNonfungible.begin()[0];
            if (vopretExtra.size() > 0)
                evalCode2 = vopretExtra.begin()[0];

            if (evalCode1 == EVAL_TOKENS && evalCode2 != 0) {
                evalCode1 = evalCode2; // for using MakeTokensCC1vout(evalcode,...) instead of MakeCC1vout(EVAL_TOKENS, evalcode...)
                evalCode2 = 0;
            }

            if (/*checkPubkeys &&*/ funcId != 'c') { // for 'c' there is no pubkeys
                                                     // verify that the vout is token by constructing vouts with the pubkeys in the opret:

                // maybe this is dual-eval 1 pubkey or 1of2 pubkey vout?
                if (voutPubkeys.size() >= 1 && voutPubkeys.size() <= 2) {
                    // check dual/three-eval 1 pubkey vout with the first pubkey
                    testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode1, evalCode2, tx.vout[v].nValue, voutPubkeys[0]), std::string("three-eval cc1 pk[0]")));
                    if (evalCode2 != 0)
                        // also check in backward evalcode order
                        testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode2, evalCode1, tx.vout[v].nValue, voutPubkeys[0]), std::string("three-eval cc1 pk[0] backward-eval")));

                    if (voutPubkeys.size() == 2) {
                        // check dual/three eval 1of2 pubkeys vout
                        testVouts.push_back(std::make_pair(MakeTokensCC1of2vout(evalCode1, evalCode2, tx.vout[v].nValue, voutPubkeys[0], voutPubkeys[1]), std::string("three-eval cc1of2")));
                        // check dual/three eval 1 pubkey vout with the second pubkey
                        testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode1, evalCode2, tx.vout[v].nValue, voutPubkeys[1]), std::string("three-eval cc1 pk[1]")));
                        if (evalCode2 != 0) {
                            // also check in backward evalcode order:
                            // check dual/three eval 1of2 pubkeys vout
                            testVouts.push_back(std::make_pair(MakeTokensCC1of2vout(evalCode2, evalCode1, tx.vout[v].nValue, voutPubkeys[0], voutPubkeys[1]), std::string("three-eval cc1of2 backward-eval")));
                            // check dual/three eval 1 pubkey vout with the second pubkey
                            testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode2, evalCode1, tx.vout[v].nValue, voutPubkeys[1]), std::string("three-eval cc1 pk[1] backward-eval")));
                        }
                    }

                    // maybe this is like gatewayclaim to single-eval token?
                    if (evalCodeNonfungible == 0) // do not allow to convert non-fungible to fungible token
                        testVouts.push_back(std::make_pair(MakeCC1vout(EVAL_TOKENS, tx.vout[v].nValue, voutPubkeys[0]), std::string("single-eval cc1 pk[0]")));

                    // maybe this is like FillSell for non-fungible token?
                    if (evalCode1 != 0)
                        testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode1, tx.vout[v].nValue, voutPubkeys[0]), std::string("dual-eval-token cc1 pk[0]")));
                    if (evalCode2 != 0)
                        testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode2, tx.vout[v].nValue, voutPubkeys[0]), std::string("dual-eval2-token cc1 pk[0]")));

                    // the same for pk[1]:
                    if (voutPubkeys.size() == 2) {
                        if (evalCodeNonfungible == 0) // do not allow to convert non-fungible to fungible token
                            testVouts.push_back(std::make_pair(MakeCC1vout(EVAL_TOKENS, tx.vout[v].nValue, voutPubkeys[1]), std::string("single-eval cc1 pk[1]")));
                        if (evalCode1 != 0)
                            testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode1, tx.vout[v].nValue, voutPubkeys[1]), std::string("dual-eval-token cc1 pk[1]")));
                        if (evalCode2 != 0)
                            testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode2, tx.vout[v].nValue, voutPubkeys[1]), std::string("dual-eval2-token cc1 pk[1]")));
                    }
                }
				
				if (funcId != 'u') //this seems to break update tx validation, can be skipped safely since update txes have some additional constraints regarding token transfers - dan
				{
					//special check for tx when spending from 1of2 CC address and one of pubkeys is global CC pubkey
					struct CCcontract_info *cpEvalCode1,CEvalCode1;
					cpEvalCode1 = CCinit(&CEvalCode1,evalCode1);
					CPubKey pk=GetUnspendable(cpEvalCode1,0);
					testVouts.push_back( std::make_pair(MakeTokensCC1of2vout(evalCode1, tx.vout[v].nValue, voutPubkeys[0], pk), std::string("dual-eval1 pegscc cc1of2 pk[0] globalccpk")) ); 
					if (voutPubkeys.size() == 2) testVouts.push_back( std::make_pair(MakeTokensCC1of2vout(evalCode1, tx.vout[v].nValue, voutPubkeys[1], pk), std::string("dual-eval1 pegscc cc1of2 pk[1] globalccpk")) );
					if (evalCode2!=0)
					{
						struct CCcontract_info *cpEvalCode2,CEvalCode2;
						cpEvalCode2 = CCinit(&CEvalCode2,evalCode2);
						CPubKey pk=GetUnspendable(cpEvalCode2,0);
						testVouts.push_back( std::make_pair(MakeTokensCC1of2vout(evalCode2, tx.vout[v].nValue, voutPubkeys[0], pk), std::string("dual-eval2 pegscc cc1of2 pk[0] globalccpk")) ); 
						if (voutPubkeys.size() == 2) testVouts.push_back( std::make_pair(MakeTokensCC1of2vout(evalCode2, tx.vout[v].nValue, voutPubkeys[1], pk), std::string("dual-eval2 pegscc cc1of2 pk[1] globalccpk")) );
					}
				}

				// maybe it is single-eval or dual/three-eval token change?
				std::vector<CPubKey> vinPubkeys, vinPubkeysUnfiltered;
				ExtractTokensCCVinPubkeys(tx, vinPubkeysUnfiltered);
                FilterOutTokensUnspendablePk(vinPubkeysUnfiltered, vinPubkeys);  // cannot send tokens to token unspendable cc addr (only marker is allowed there)

                for (std::vector<CPubKey>::iterator it = vinPubkeys.begin(); it != vinPubkeys.end(); it++) {
                    if (evalCodeNonfungible == 0) // do not allow to convert non-fungible to fungible token
                        testVouts.push_back(std::make_pair(MakeCC1vout(EVAL_TOKENS, tx.vout[v].nValue, *it), std::string("single-eval cc1 self vin pk")));
                    testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode1, evalCode2, tx.vout[v].nValue, *it), std::string("three-eval cc1 self vin pk")));

                    if (evalCode2 != 0)
                        // also check in backward evalcode order:
                        testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode2, evalCode1, tx.vout[v].nValue, *it), std::string("three-eval cc1 self vin pk backward-eval")));
                }

                // try all test vouts:
                for (auto t : testVouts) {
                    if (t.first == tx.vout[v]) { // test vout matches
                        LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "IsTokensvout() valid amount=" << tx.vout[v].nValue << " msg=" << t.second << " evalCode=" << (int)evalCode1 << " evalCode2=" << (int)evalCode2 << " txid=" << tx.GetHash().GetHex() << " tokenid=" << reftokenid.GetHex() << std::endl);
                        return tx.vout[v].nValue;
                    }
                }
            } else { // funcid == 'c'

                if (!tx.IsCoinImport()) {
                    vscript_t vorigPubkey;
                    std::string dummyName, dummyDescription, dummyTokenType;
                    uint256 dummyRefTokenId;
                    double dummyOwnerPerc;
                    int64_t dummyExpiryTimeSec;
                    std::vector<std::pair<uint8_t, vscript_t>> oprets;

                    if (DecodeTokenCreateOpRet(tx.vout.back().scriptPubKey, vorigPubkey, dummyName, dummyDescription, dummyOwnerPerc, dummyTokenType, dummyRefTokenId, dummyExpiryTimeSec, oprets) == 0) {
                        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << indentStr << "IsTokensvout() could not decode create opret"
                                                                        << " for txid=" << tx.GetHash().GetHex() << " for tokenid=" << reftokenid.GetHex() << std::endl);
                        return 0;
                    }

                    CPubKey origPubkey = pubkey2pk(vorigPubkey);


                    // TODO: add voutPubkeys for 'c' tx

                    /* this would not work for imported tokens:
                    // for 'c' recognize the tokens only to token originator pubkey (but not to unspendable <-- closed sec violation)
                    // maybe this is like gatewayclaim to single-eval token?
                    if (evalCodeNonfungible == 0)  // do not allow to convert non-fungible to fungible token
                        testVouts.push_back(std::make_pair(MakeCC1vout(EVAL_TOKENS, tx.vout[v].nValue, origPubkey), std::string("single-eval cc1 orig-pk")));
                    // maybe this is like FillSell for non-fungible token?
                    if (evalCode1 != 0)
                        testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode1, tx.vout[v].nValue, origPubkey), std::string("dual-eval-token cc1 orig-pk")));   */

                    // note: this would not work if there are several pubkeys in the tokencreator's wallet (AddNormalinputs does not use pubkey param):
                    // for tokenbase tx check that normal inputs sent from origpubkey > cc outputs
                    int64_t ccOutputs = 0;
                    for (auto vout : tx.vout)
                        if (vout.scriptPubKey.IsPayToCryptoCondition() //TODO: add voutPubkey validation
                            && (!IsTokenMarkerVout(vout) && !IsTokenBatonVout(vout)))               // should not be marker or baton here
                            ccOutputs += vout.nValue;

                    int64_t normalInputs = TotalPubkeyNormalInputs(tx, origPubkey); // check if normal inputs are really signed by originator pubkey (someone not cheating with originator pubkey)
                    LOGSTREAM("cctokens", CCLOG_DEBUG2, stream << indentStr << "IsTokensvout() normalInputs=" << normalInputs << " ccOutputs=" << ccOutputs << " for tokenbase=" << reftokenid.GetHex() << std::endl);

                    if (normalInputs >= ccOutputs) {
                        LOGSTREAM("cctokens", CCLOG_DEBUG2, stream << indentStr << "IsTokensvout() assured normalInputs >= ccOutputs"
                                                                   << " for tokenbase=" << reftokenid.GetHex() << std::endl);
                        if (!IsTokenMarkerVout(tx.vout[v]) && !IsTokenBatonVout(tx.vout[v])) // exclude marker and baton
                            return tx.vout[v].nValue;
                        else
                            return 0; // vout is good, but do not take marker into account
                    } else {
                        LOGSTREAM("cctokens", CCLOG_INFO, stream << indentStr << "IsTokensvout() skipping vout not fulfilled normalInputs >= ccOutput"
                                                                 << " for tokenbase=" << reftokenid.GetHex() << " normalInputs=" << normalInputs << " ccOutputs=" << ccOutputs << std::endl);
                    }
                } else {
                    // imported tokens are checked in the eval::ImportCoin() validation code
                    if (!IsTokenMarkerVout(tx.vout[v]) && !IsTokenBatonVout(tx.vout[v])) // exclude marker and baton
                        return tx.vout[v].nValue;
                    else
                        return 0; // vout is good, but do not take marker into account
                }
            }
            LOGSTREAM("cctokens", CCLOG_DEBUG1, stream << indentStr << "IsTokensvout() no valid vouts evalCode=" << (int)evalCode1 << " evalCode2=" << (int)evalCode2 << " for txid=" << tx.GetHash().GetHex() << " for tokenid=" << reftokenid.GetHex() << std::endl);
        }
        //std::cerr << indentStr; fprintf(stderr,"IsTokensvout() CC vout v.%d of n=%d amount=%.8f txid=%s\n",v,n,(double)0/COIN, tx.GetHash().GetHex().c_str());
    }
    //std::cerr << indentStr; fprintf(stderr,"IsTokensvout() normal output v.%d %.8f\n",v,(double)tx.vout[v].nValue/COIN);
    return (0);
}

bool IsTokenMarkerVout(CTxOut vout)
{
    struct CCcontract_info *cpTokens, CCtokens_info;
    cpTokens = CCinit(&CCtokens_info, EVAL_TOKENS);
	if (!IsTokenBatonVout(vout))
		return vout == MakeCC1vout(EVAL_TOKENS, vout.nValue, GetUnspendable(cpTokens, NULL));
	else
		return false;
}

bool IsTokenBatonVout(CTxOut vout)
{
    CScript opret;
    uint256 assetHash;
    int64_t value;
    std::string ccode, message;
    
    if (vout.nValue == 10000 &&
        getCCopret(vout.scriptPubKey, opret) &&
        (DecodeTokenUpdateCCOpRet(opret, assetHash, value, ccode, message) == 'u'))
    {
        //std::cerr << "Located a baton vout" << std::endl;
        return true;
    }
    return false;
}

// compares cc inputs vs cc outputs (to prevent feeding vouts from normal inputs)
bool TokensExactAmounts(bool goDeeper, struct CCcontract_info* cp, int64_t& inputs, int64_t& outputs, Eval* eval, const CTransaction& tx, uint256 reftokenid)
{
    CTransaction vinTx;
    uint256 hashBlock;
    int64_t tokenoshis;

    struct CCcontract_info *cpTokens, tokensC;
    cpTokens = CCinit(&tokensC, EVAL_TOKENS);

    int32_t numvins = tx.vin.size();
    int32_t numvouts = tx.vout.size();
    inputs = outputs = 0;

    // this is just for log messages indentation for debugging recursive calls:
    std::string indentStr = std::string().append(tokenValIndentSize, '.');

    LOGSTREAM((char*)"cctokens", CCLOG_DEBUG2, stream << indentStr << "TokensExactAmounts() entered for txid=" << tx.GetHash().GetHex() << " for tokenid=" << reftokenid.GetHex() << std::endl);

    for (int32_t i = 0; i < numvins; i++) { // check for additional contracts which may send tokens to the Tokens contract
        if ((*cpTokens->ismyvin)(tx.vin[i].scriptSig) /*|| IsVinAllowed(tx.vin[i].scriptSig) != 0*/) {
            //std::cerr << indentStr << "TokensExactAmounts() eval is true=" << (eval != NULL) << " ismyvin=ok for_i=" << i << std::endl;
            // we are not inside the validation code -- dimxy
            if ((eval && eval->GetTxUnconfirmed(tx.vin[i].prevout.hash, vinTx, hashBlock) == 0) || (!eval && !myGetTransaction(tx.vin[i].prevout.hash, vinTx, hashBlock))) {
                LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << indentStr << "TokensExactAmounts() cannot read vintx for i." << i << " numvins." << numvins << std::endl);
                return (!eval) ? false : eval->Invalid("always should find vin tx, but didnt");
            } else {
                LOGSTREAM((char*)"cctokens", CCLOG_DEBUG2, stream << indentStr << "TokenExactAmounts() checking vintx.vout for tx.vin[" << i << "] nValue=" << vinTx.vout[tx.vin[i].prevout.n].nValue << std::endl);

                // validate vouts of vintx
                tokenValIndentSize++;
                tokenoshis = IsTokensvout(goDeeper, true, cpTokens, eval, vinTx, tx.vin[i].prevout.n, reftokenid);
                tokenValIndentSize--;
                if (tokenoshis != 0) {
                    LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "TokensExactAmounts() adding vintx.vout for tx.vin[" << i << "] tokenoshis=" << tokenoshis << std::endl);
                    inputs += tokenoshis;
                }
            }
        }
    }

    for (int32_t i = 0; i < numvouts - 1; i++) // 'numvouts-1' <-- do not check opret
    {
        LOGSTREAM((char*)"cctokens", CCLOG_DEBUG2, stream << indentStr << "TokenExactAmounts() recursively checking tx.vout[" << i << "] nValue=" << tx.vout[i].nValue << std::endl);

        //std::cerr << "TokenExactAmounts() recursively checking txid[" << tx.GetHash().GetHex() << "].vout[" << i << "] nValue=" << tx.vout[i].nValue << std::endl; //dan
        
        // Note: we pass in here IsTokenvout(false,...) because we don't need to call TokenExactAmounts() recursively from IsTokensvout here
        // indeed, if we pass 'true' we'll be checking this tx vout again
        tokenValIndentSize++;
        tokenoshis = IsTokensvout(false /*<--do not recursion here*/, true /*<--exclude non-tokens vouts*/, cpTokens, eval, tx, i, reftokenid);
        tokenValIndentSize--;
        
        if (tokenoshis != 0) {
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "TokensExactAmounts() adding tx.vout[" << i << "] tokenoshis=" << tokenoshis << std::endl);
            outputs += tokenoshis;
        }
    }
	
    //std::cerr << indentStr << "TokensExactAmounts() inputs=" << inputs << " outputs=" << outputs << " for txid=" << tx.GetHash().GetHex() << std::endl;

    if (inputs != outputs) {
        if (tx.GetHash() != reftokenid)
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << indentStr << "TokenExactAmounts() found unequal token cc inputs=" << inputs << " vs cc outputs=" << outputs << " for txid=" << tx.GetHash().GetHex() << " and this is not the create tx" << std::endl);
        //fprintf(stderr,"inputs %llu vs outputs %llu\n",(long long)inputs,(long long)outputs);
        return false; // do not call eval->Invalid() here!
    } else
        return true;
}


// get non-fungible data from 'tokenbase' tx (the data might be empty)
void GetNonfungibleData(uint256 tokenid, vscript_t& vopretNonfungible)
{
    CTransaction tokenbasetx;
    uint256 hashBlock;

    if (!myGetTransaction(tokenid, tokenbasetx, hashBlock)) {
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "GetNonfungibleData() cound not load token creation tx=" << tokenid.GetHex() << std::endl);
        return;
    }

    vopretNonfungible.clear();
    // check if it is non-fungible tx and get its second evalcode from non-fungible payload
    if (tokenbasetx.vout.size() > 0) {
        std::vector<uint8_t> origpubkey;
        std::string name, description;
        std::vector<std::pair<uint8_t, vscript_t>> oprets;

        if (DecodeTokenCreateOpRet(tokenbasetx.vout.back().scriptPubKey, origpubkey, name, description, oprets) == 'c') {
            GetOpretBlob(oprets, OPRETID_NONFUNGIBLEDATA, vopretNonfungible);
        }
    }
}

// overload, adds inputs from token cc addr
int64_t AddTokenCCInputs(struct CCcontract_info* cp, CMutableTransaction& mtx, CPubKey pk, uint256 tokenid, int64_t total, int32_t maxinputs)
{
    vscript_t vopretNonfungibleDummy;
    return AddTokenCCInputs(cp, mtx, pk, tokenid, total, maxinputs, vopretNonfungibleDummy);
}

// adds inputs from token cc addr and returns non-fungible opret payload if present
// also sets evalcode in cp, if needed
int64_t AddTokenCCInputs(struct CCcontract_info* cp, CMutableTransaction& mtx, CPubKey pk, uint256 tokenid, int64_t total, int32_t maxinputs, vscript_t& vopretNonfungible)
{
    char tokenaddr[64], destaddr[64];
    int64_t threshold, nValue, price, totalinputs = 0;
    int32_t n = 0;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue>> unspentOutputs;

    GetNonfungibleData(tokenid, vopretNonfungible);
    if (vopretNonfungible.size() > 0)
        cp->additionalTokensEvalcode2 = vopretNonfungible.begin()[0];

    GetTokensCCaddress(cp, tokenaddr, pk);
    SetCCunspents(unspentOutputs, tokenaddr, true);

    if (unspentOutputs.empty()) {
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "AddTokenCCInputs() no utxos for token dual/three eval addr=" << tokenaddr << " evalcode=" << (int)cp->evalcode << " additionalTokensEvalcode2=" << (int)cp->additionalTokensEvalcode2 << std::endl);
    }

    threshold = total / (maxinputs != 0 ? maxinputs : CC_MAXVINS);

    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue>>::const_iterator it = unspentOutputs.begin(); it != unspentOutputs.end(); it++) {
        CTransaction vintx;
        uint256 hashBlock;
        uint256 vintxid = it->first.txhash;
        int32_t vout = (int32_t)it->first.index;

        if (it->second.satoshis < threshold) // this should work also for non-fungible tokens (there should be only 1 satoshi for non-fungible token issue)
            continue;

        int32_t ivin;
		for (ivin = 0; ivin < mtx.vin.size(); ivin ++)
			if (vintxid == mtx.vin[ivin].prevout.hash && vout == mtx.vin[ivin].prevout.n)
				break;
		if (ivin != mtx.vin.size()) // that is, the tx.vout is already added to mtx.vin (in some previous calls)
			continue;

		if (myGetTransaction(vintxid, vintx, hashBlock) != 0)
		{
			Getscriptaddress(destaddr, vintx.vout[vout].scriptPubKey);
			if (strcmp(destaddr, tokenaddr) != 0 && 
                strcmp(destaddr, cp->unspendableCCaddr) != 0 &&   // TODO: check why this. Should not we add token inputs from unspendable cc addr if mypubkey is used?
                strcmp(destaddr, cp->unspendableaddr2) != 0)      // or the logic is to allow to spend all available tokens (what about unspendableaddr3)?
				continue;
			
            LOGSTREAM((char *)"cctokens", CCLOG_DEBUG1, stream << "AddTokenCCInputs() check vintx vout destaddress=" << destaddr << " amount=" << vintx.vout[vout].nValue << std::endl);

			if ((nValue = IsTokensvout(true, true/*<--add only valid token uxtos */, cp, NULL, vintx, vout, tokenid)) > 0 && myIsutxo_spentinmempool(ignoretxid,ignorevin,vintxid, vout) == 0)
			{
				//for non-fungible tokens check payload:
                if (!vopretNonfungible.empty()) {
                    vscript_t vopret;

                    // check if it is non-fungible token:
                    GetNonfungibleData(tokenid, vopret);
                    if (vopret != vopretNonfungible) {
                        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "AddTokenCCInputs() found incorrect non-fungible opret payload for vintxid=" << vintxid.GetHex() << std::endl);
                        continue;
                    }
                    // non-fungible evalCode2 cc contract should also check if there exists only one non-fungible vout with amount = 1
                }

                if (total != 0 && maxinputs != 0) // if it is not just to calc amount...
                    mtx.vin.push_back(CTxIn(vintxid, vout, CScript()));

                nValue = it->second.satoshis;
                totalinputs += nValue;
                LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << "AddTokenCCInputs() adding input nValue=" << nValue << std::endl);
                n++;

                if ((total > 0 && totalinputs >= total) || (maxinputs > 0 && n >= maxinputs))
                    break;
            }
        }
    }

    //std::cerr << "AddTokenCCInputs() found totalinputs=" << totalinputs << std::endl;
    return (totalinputs);
}

// checks if any token vouts are sent to 'dead' pubkey
int64_t HasBurnedTokensvouts(struct CCcontract_info* cp, Eval* eval, const CTransaction& tx, uint256 reftokenid)
{
    uint8_t dummyEvalCode;
    uint256 tokenIdOpret;
    std::vector<CPubKey> voutPubkeys, voutPubkeysDummy;
    std::vector<std::pair<uint8_t, vscript_t>> oprets;
    vscript_t vopretExtra, vopretNonfungible;

    uint8_t evalCode = EVAL_TOKENS; // if both payloads are empty maybe it is a transfer to non-payload-one-eval-token vout like GatewaysClaim
    uint8_t evalCode2 = 0;          // will be checked if zero or not

    // test vouts for possible token use-cases:
    std::vector<std::pair<CTxOut, std::string>> testVouts;

    int32_t n = tx.vout.size();
    // just check boundaries:
    if (n == 0) {
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "HasBurnedTokensvouts() incorrect params: tx.vout.size() == 0, txid=" << tx.GetHash().GetHex() << std::endl);
        return (0);
    }

    if (DecodeTokenOpRet(tx.vout.back().scriptPubKey, dummyEvalCode, tokenIdOpret, oprets) == 0) {
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "HasBurnedTokensvouts() cannot parse opret DecodeTokenOpRet returned 0, txid=" << tx.GetHash().GetHex() << std::endl);
        return 0;
    }

    // get assets/channels/gateways token data:
    FilterOutNonCCOprets(oprets, vopretExtra); // NOTE: only 1 additional evalcode in token opret is currently supported

    LOGSTREAM((char*)"cctokens", CCLOG_DEBUG2, stream << "HasBurnedTokensvouts() vopretExtra=" << HexStr(vopretExtra) << std::endl);

    GetNonfungibleData(reftokenid, vopretNonfungible);

    if (vopretNonfungible.size() > 0)
        evalCode = vopretNonfungible.begin()[0];
    if (vopretExtra.size() > 0)
        evalCode2 = vopretExtra.begin()[0];

    if (evalCode == EVAL_TOKENS && evalCode2 != 0) {
        evalCode = evalCode2;
        evalCode2 = 0;
    }

    voutPubkeys.push_back(pubkey2pk(ParseHex(CC_BURNPUBKEY)));

    int64_t burnedAmount = 0;

    for (int i = 0; i < tx.vout.size(); i++) {
        if (tx.vout[i].scriptPubKey.IsPayToCryptoCondition()) {
            // make all possible token vouts for dead pk:
            for (std::vector<CPubKey>::iterator it = voutPubkeys.begin(); it != voutPubkeys.end(); it++) {
                testVouts.push_back(std::make_pair(MakeCC1vout(EVAL_TOKENS, tx.vout[i].nValue, *it), std::string("single-eval cc1 burn pk")));
                testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode, evalCode2, tx.vout[i].nValue, *it), std::string("three-eval cc1 burn pk")));

                if (evalCode2 != 0)
                    // also check in backward evalcode order:
                    testVouts.push_back(std::make_pair(MakeTokensCC1vout(evalCode2, evalCode, tx.vout[i].nValue, *it), std::string("three-eval cc1 burn pk backward-eval")));
            }

            // try all test vouts:
            for (auto t : testVouts) {
                if (t.first == tx.vout[i]) {
                    LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << "HasBurnedTokensvouts() burned amount=" << tx.vout[i].nValue << " msg=" << t.second << " evalCode=" << (int)evalCode << " evalCode2=" << (int)evalCode2 << " txid=" << tx.GetHash().GetHex() << " tokenid=" << reftokenid.GetHex() << std::endl);
                    burnedAmount += tx.vout[i].nValue;
                }
            }
            LOGSTREAM((char*)"cctokens", CCLOG_DEBUG2, stream << "HasBurnedTokensvouts() total burned=" << burnedAmount << " evalCode=" << (int)evalCode << " evalCode2=" << (int)evalCode2 << " for txid=" << tx.GetHash().GetHex() << " for tokenid=" << reftokenid.GetHex() << std::endl);
        }
    }

    return burnedAmount;
}

CPubKey GetTokenOriginatorPubKey(CScript scriptPubKey)
{
    uint8_t funcId, evalCode;
    uint256 tokenid;
    std::vector<CPubKey> voutTokenPubkeys;
    std::vector<std::pair<uint8_t, vscript_t>> oprets;

    if ((funcId = DecodeTokenOpRet(scriptPubKey, evalCode, tokenid, voutTokenPubkeys, oprets)) != 0) {
        CTransaction tokenbasetx;
        uint256 hashBlock;

        if (myGetTransaction(tokenid, tokenbasetx, hashBlock) && tokenbasetx.vout.size() > 0) {
            vscript_t vorigpubkey;
            std::string name, desc;
            if (DecodeTokenCreateOpRet(tokenbasetx.vout.back().scriptPubKey, vorigpubkey, name, desc) != 0)
                return pubkey2pk(vorigpubkey);
        }
    }
    return CPubKey(); //return invalid pubkey
}

// returns the percentage of tokenid total supply that is owned by the provided pubkey
// moved to separate function for accessibility
double GetTokenOwnershipPercent(CPubKey pk, uint256 tokenid)
{
    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_TOKENS);
    char CCaddr[64];
    GetCCaddress(cp,CCaddr,pk);
    double balance = static_cast<double>(CCtoken_balance(CCaddr, tokenid));
    double supply = static_cast<double>(CCfullsupply(tokenid));
    return (balance / supply * 100);
}

// gets the latest baton txid of a token
// used in TokenUpdate and TokenViewUpdate's recursive mode
bool GetLatestTokenUpdate(uint256 tokenid, uint256 &latesttxid)
{
    int32_t vini, height, retcode;
    std::vector<std::pair<uint8_t, vscript_t>> oprets;
    uint256 batontxid, sourcetxid = tokenid, hashBlock;
    CTransaction txBaton;
    uint8_t funcId, evalcode;

    // special handling for token creation tx - in this tx, baton vout is vout2
    if (!(GetTransaction(tokenid, txBaton, hashBlock, true) &&
        !hashBlock.IsNull() &&
        txBaton.vout.size() > 2 &&
        (funcId = DecodeTokenOpRet(txBaton.vout.back().scriptPubKey, evalcode, tokenid, oprets)) == 'c' &&
        txBaton.vout[2].nValue == 10000))
    {
        return 0;
    }
    // find an update tx which spent the token create baton vout, if it exists
    if ((retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 2)) == 0 &&
        GetTransaction(batontxid, txBaton, hashBlock, true) &&
        !hashBlock.IsNull() &&
        txBaton.vout.size() > 0 &&
        txBaton.vout[0].nValue == 10000 && 
        (funcId = DecodeTokenOpRet(txBaton.vout.back().scriptPubKey, evalcode, tokenid, oprets)) == 'u')
    {
        sourcetxid = batontxid;
    }
    else
    {
        latesttxid = sourcetxid;
        return 1;
    }
    
    latesttxid = sourcetxid;
    
    // baton vout should be vout0 from now on
    while ((retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 0)) == 0)  // find a tx which spent the baton vout
    {
        if (GetTransaction(batontxid, txBaton, hashBlock, true) &&  // load the transaction which spent the baton
            !hashBlock.IsNull() &&                           // tx not in mempool
            txBaton.vout.size() > 0 &&             
            txBaton.vout[0].nValue == 10000 &&     // check baton fee 
            (funcId = DecodeTokenOpRet(txBaton.vout.back().scriptPubKey, evalcode, tokenid, oprets)) == 'u') // decode opreturn
        {
            sourcetxid = batontxid;
        }
        else
        {
            return 0;
        }
        latesttxid = sourcetxid;
    }
    return 1;
}

// returns token creation signed raw tx
std::string CreateToken(int64_t txfee, int64_t tokensupply, std::string name, std::string description, double ownerPerc, std::string tokenType, uint256 assetHash, int64_t value, std::string ccode, uint256 referenceTokenId, int64_t expiryTimeSec, vscript_t nonfungibleData)
{
    CTransaction refTokenBaseTx;
    uint256 hashBlock, dummyRefTokenId;
    std::vector<uint8_t> dummyPubkey;
    int64_t refTokenSupply, refExpiryTimeSec;
    std::string dummyName, dummyDescription, refTokenType;
    double refOwnerPerc;
    int32_t numblocks;

    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_TOKENS);

    if (txfee == 0)
        txfee = 10000;

    CPubKey mypk = pubkey2pk(Mypubkey());

    //Checking if the specified tokensupply is valid
    if (tokensupply < 0) {
        CCerror = "negative tokensupply";
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() =" << CCerror << "=" << tokensupply << std::endl);
        return std::string("");
    }
    //Checking if tokensupply is equal to 1 if nonfungibleData isn't empty
    if (!nonfungibleData.empty() && tokensupply != 1) {
        CCerror = "for non-fungible tokens tokensupply should be equal to 1";
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
        return std::string("");
    }
    //Checking the token name and description size
    if (name.size() > 32 || description.size() > 1024) // this is also checked on rpc level
    {
        LOGSTREAM((char*)"cctokens", CCLOG_DEBUG1, stream << "name len=" << name.size() << " or description len=" << description.size() << " is too big" << std::endl);
        CCerror = "name should be <= 32, description should be <= 1024";
        return ("");
    }
    //Checking the estimated value
    if (value < 1 && value != 0 )
    {
        CCerror = "Estimated value must be positive and cannot be less than 1 satoshi if not 0";
        return std::string("");
    }

    //If token type is "a" and referenceTokenId is (properly) defined, check if it is valid. Else, ignore as it is optional for "a" types
    if (tokenType == "a" && referenceTokenId != zeroid) {
        if (!GetTransaction(referenceTokenId, refTokenBaseTx, hashBlock, false)) {
            CCerror = "cant find reference tokenid";
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
            return std::string("");
        }
    }
    if (tokenType == "m" || tokenType == "s")
    {
        //Checking if tokensupply is equal to 1 if token is master license type
        if (tokenType == "m" && tokensupply != 1) {
            CCerror = "for master license tokens tokensupply should be equal to 1";
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
            return std::string("");
        }
        //checking if referenceTokenId exists
        if (!GetTransaction(referenceTokenId, refTokenBaseTx, hashBlock, false)) {
            CCerror = "cant find reference tokenid";
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
            return std::string("");
        }
        double ownedRefTokenPerc = GetTokenOwnershipPercent(mypk, referenceTokenId);
        //checking reference tokenid opret
        if (refTokenBaseTx.vout.size() > 0 && DecodeTokenCreateOpRet(refTokenBaseTx.vout[refTokenBaseTx.vout.size() - 1].scriptPubKey, dummyPubkey, dummyName, dummyDescription, refOwnerPerc, refTokenType, dummyRefTokenId, refExpiryTimeSec) != 'c') {
            CCerror = "reference tokenid isn't token creation txid";
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
            return std::string("");
        }
        //master licenses must reference digital assets owned by mypk
        if (tokenType == "m" && (refTokenType != "a" || ownedRefTokenPerc < refOwnerPerc)) {
            CCerror = "for master license tokens reference tokenid must be of type 'a' and owned by this pubkey";
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
            return std::string("");
        }
        //sub-licenses must reference unexpired master licenses owned by mypk
        if (tokenType == "s" && (refTokenType != "m" || ownedRefTokenPerc < refOwnerPerc)) {
            CCerror = "for sub-license tokens reference tokenid must be of type 'm' and owned by this pubkey";
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
            return std::string("");
        }
        if (refExpiryTimeSec != 0)
        {
            if (tokenType == "s" && (CCduration(numblocks, referenceTokenId) > refExpiryTimeSec))
            {
                CCerror = "Can't create a sub-license if master license is expired";
                LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
                return std::string("");
            }
            if (tokenType == "s" && (refExpiryTimeSec - CCduration(numblocks, referenceTokenId) < expiryTimeSec))
            {
                CCerror = "Sub-license expire time can't be longer then time left until master license expires";
                LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
                return std::string("");
            }
        }
    }

	if (AddNormalinputs2(mtx, tokensupply + 2 * txfee, 64) > 0)  // add normal inputs only from mypk
	{
        int64_t mypkInputs = TotalPubkeyNormalInputs(mtx, mypk);  
        if (mypkInputs < tokensupply) {     // check that tokens amount are really issued with mypk (because in the wallet there maybe other privkeys)
            CCerror = "some inputs signed not with -pubkey=pk";
            return std::string("");
        }
        uint8_t destEvalCode = EVAL_TOKENS;
        //If nonfungibleData exists, eval code is set to the one specified within nonfungibleData
        if (nonfungibleData.size() > 0)
            destEvalCode = nonfungibleData.begin()[0];

        // vout0 is a marker vout
        // NOTE: we should prevent spending fake-tokens from this marker in IsTokenvout():
        mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, txfee, GetUnspendable(cp, NULL))); // new marker to token cc addr, burnable and validated, vout pos now changed to 0 (from 1)
        // vout1 issues the tokens to mypk
        mtx.vout.push_back(MakeTokensCC1vout(destEvalCode, tokensupply, mypk));
        // vout2 is a baton vout containing arbitrary updatable data (should be prevented from spending in IsTokensVout)
        CScript batonopret = EncodeTokenUpdateCCOpRet(assetHash,value,ccode,"");
        std::vector<std::vector<unsigned char>> vData = std::vector<std::vector<unsigned char>>();
        if (makeCCopret(batonopret, vData))
        {
            mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, 10000, GetUnspendable(cp, NULL), &vData));  // BATON_VOUT
            //fprintf(stderr, "vout size2.%li\n", mtx.vout.size());
            return (FinalizeCCTx(0, cp, mtx, mypk, txfee, EncodeTokenCreateOpRet(Mypubkey(), name, description, ownerPerc, tokenType, referenceTokenId, expiryTimeSec, nonfungibleData)));
        }
        else
        {
            CCerror = "couldnt embed updatable data to baton vout";
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
            return std::string("");
        }
    }
    CCerror = "cant find normal inputs";
    LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
    return std::string("");
}

std::string UpdateToken(int64_t txfee, uint256 tokenid, uint256 assetHash, int64_t value, std::string ccode, std::string message)
{
    CTransaction tokenBaseTx;
    uint256 hashBlock, dummyRefTokenId, latesttxid;
    std::vector<uint8_t> dummyPubkey;
    int64_t refExpiryTimeSec;
    std::string dummyName, dummyDescription, refTokenType;
    double refOwnerPerc;
    
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_TOKENS);
    
    if (txfee == 0)
        txfee = 10000;

    CPubKey mypk = pubkey2pk(Mypubkey());
    
    if (GetTransaction(tokenid, tokenBaseTx, hashBlock, false) == 0)
    {
        CCerror = "cant find tokenid";
        return std::string("");
    }
    double ownedRefTokenPerc = GetTokenOwnershipPercent(mypk, tokenid);
    //checking tokenid create opret
    if (tokenBaseTx.vout.size() > 0 && DecodeTokenCreateOpRet(tokenBaseTx.vout[tokenBaseTx.vout.size() - 1].scriptPubKey, dummyPubkey, dummyName, dummyDescription, refOwnerPerc, refTokenType, dummyRefTokenId, refExpiryTimeSec) != 'c')
    {
        CCerror = "tokenid isn't token creation txid";
        return std::string("");
    }
    //checking if token is owned by mypk
    if (ownedRefTokenPerc < refOwnerPerc)
    {
        CCerror = "tokenid must be owned by this pubkey";
        return std::string("");
    }
    //getting the latest update txid (can be the same as tokenid)
    if (!GetLatestTokenUpdate(tokenid, latesttxid))
    {
        CCerror = "cannot find latest token update";
        return std::string("");
    }
    //Checking the estimated value
    if (value < 1 && value != 0 )
    {
        CCerror = "Estimated value must be positive and cannot be less than 1 satoshi if not 0";
        return std::string("");
    }
    //Checking the ccode and message size
    if (ccode.size() != 3 || message.size() > 128) // this is also checked on rpc level
    {
        CCerror = "ccode size should be 3, message size should be <= 128";
        return std::string("");
    }
    
    if (AddNormalinputs(mtx, mypk, txfee + 20000, 64) > 0)
    {
        int64_t mypkInputs = TotalPubkeyNormalInputs(mtx, mypk);
        if (mypkInputs < 20000) {
            CCerror = "some inputs signed not with -pubkey=pk";
            return std::string("");
        }
        if (latesttxid == tokenid)
        {
            mtx.vin.push_back(CTxIn(tokenid,2,CScript()));
            //fprintf(stderr, "vin size.%li\n", mtx.vin.size());
        }
        else if (latesttxid != zeroid)
        {
            mtx.vin.push_back(CTxIn(latesttxid,0,CScript()));
            //fprintf(stderr, "vin size.%li\n", mtx.vin.size());
        }
        else
        {
            CCerror = "latest update txid invalid";
            return std::string("");
        }
        uint8_t destEvalCode = EVAL_TOKENS;
        CScript batonopret = EncodeTokenUpdateCCOpRet(assetHash, value, ccode, message);
        std::vector<std::vector<unsigned char>> vData = std::vector<std::vector<unsigned char>>();
        if (makeCCopret(batonopret, vData))
        {
            //vout0 is next batonvout with cc opret payload
            mtx.vout.push_back(MakeCC1vout(destEvalCode, 10000, GetUnspendable(cp, NULL), &vData));
            //vout1 sends some funds back to mypk, later used for validation
            mtx.vout.push_back(CTxOut(10000,CScript() << ParseHex(HexStr(mypk)) << OP_CHECKSIG));
            //fprintf(stderr, "vout size2.%li\n", mtx.vout.size());
            return (FinalizeCCTx(0, cp, mtx, mypk, txfee, EncodeTokenUpdateOpRet(Mypubkey(), tokenid)));
        }
        else
        {
            CCerror = "couldnt embed updatable data to baton vout";
            return std::string("");
        }
    }
    CCerror = "cant find normal inputs";
    return std::string("");
}

// transfer tokens to another pubkey
// param additionalEvalCode allows transfer of dual-eval non-fungible tokens
std::string TokenTransfer(int64_t txfee, uint256 tokenid, vscript_t destpubkey, int64_t total)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    uint64_t mask;
    int64_t CCchange = 0, inputs = 0;
    struct CCcontract_info *cp, C;
    vscript_t vopretNonfungible, vopretEmpty;

    if (total < 0)
    {
        CCerror = strprintf("negative total");
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << CCerror << "=" << total << std::endl);
        return ("");
    }

    cp = CCinit(&C, EVAL_TOKENS);

	if (txfee == 0)
		txfee = 10000;
	CPubKey mypk = pubkey2pk(Mypubkey());
    /*if ( cp->tokens1of2addr[0] == 0 )
    {
        GetTokensCCaddress(cp, cp->tokens1of2addr, mypk);
        fprintf(stderr,"set tokens1of2addr <- %s\n",cp->tokens1of2addr);
    }*/
    if (AddNormalinputs(mtx, mypk, txfee, 3) > 0)
	{
		mask = ~((1LL << mtx.vin.size()) - 1);  // seems, mask is not used anymore
        
		if ((inputs = AddTokenCCInputs(cp, mtx, mypk, tokenid, total, 60, vopretNonfungible)) > 0)  // NOTE: AddTokenCCInputs might set cp->additionalEvalCode which is used in FinalizeCCtx!
		{
			if (inputs < total) {   //added dimxy
                CCerror = strprintf("insufficient token inputs");
                LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "TokenTransfer() " << CCerror << std::endl);
                return std::string("");
            }

            uint8_t destEvalCode = EVAL_TOKENS;
            if (vopretNonfungible.size() > 0)
                destEvalCode = vopretNonfungible.begin()[0];

            if (inputs > total)
                CCchange = (inputs - total);
            mtx.vout.push_back(MakeTokensCC1vout(destEvalCode, total, pubkey2pk(destpubkey))); // if destEvalCode == EVAL_TOKENS then it is actually MakeCC1vout(EVAL_TOKENS,...)
            if (CCchange != 0)
                mtx.vout.push_back(MakeTokensCC1vout(destEvalCode, CCchange, mypk));

            std::vector<CPubKey> voutTokenPubkeys;
            voutTokenPubkeys.push_back(pubkey2pk(destpubkey)); // dest pubkey for validating vout
            
            return FinalizeCCTx(mask, cp, mtx, mypk, txfee, EncodeTokenTransferOpRet(tokenid, voutTokenPubkeys, std::make_pair((uint8_t)0, vopretEmpty)));
        }
        else
        {
            CCerror = strprintf("no token inputs");
            LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "TokenTransfer() " << CCerror << " for amount=" << total << std::endl);
        }
    //} else fprintf(stderr,"numoutputs.%d != numamounts.%d\n",n,(int32_t)amounts.size());
    }
    else
    {
        CCerror = strprintf("insufficient normal inputs for tx fee");
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "TokenTransfer() " << CCerror << std::endl);
    }
    return ("");
}

UniValue TokenViewUpdates(uint256 tokenid, int32_t samplenum, int recursive)
{
    UniValue result(UniValue::VOBJ);
    int64_t total = 0LL, amount, value;
    int32_t vini, height, retcode;
    std::vector<std::pair<uint8_t, vscript_t>> oprets;
    std::vector<uint8_t> updaterPubkey;
    uint256 batontxid, sourcetxid = tokenid, latesttxid, assetHash, hashBlock, tokenIdInOpret;
    CTransaction txBaton;
    uint8_t funcId, evalcode;
    CScript batonopret;
    std::string ccode, message, dummyName, origdescription;

    if (recursive == 0) //from earliest to latest
    {
        // special handling for token creation tx - in this tx, baton vout is vout2
        if (GetTransaction(tokenid, txBaton, hashBlock, true) &&
        !hashBlock.IsNull() &&
        txBaton.vout.size() > 2 &&
        (funcId = DecodeTokenCreateOpRet(txBaton.vout.back().scriptPubKey, updaterPubkey, dummyName, origdescription)) == 'c' &&
        txBaton.vout[2].nValue == 10000 &&
        getCCopret(txBaton.vout[2].scriptPubKey, batonopret) &&
        (funcId = DecodeTokenUpdateCCOpRet(batonopret, assetHash, value, ccode, message) == 'u'))
        {
                total++;
                UniValue data(UniValue::VOBJ);
                data.push_back(Pair("creatorPubkey", HexStr(updaterPubkey))); 
                data.push_back(Pair("hash", assetHash.GetHex())); 
                data.push_back(Pair("value", (double)value/COIN));
                data.push_back(Pair("ccode", ccode));
                if (!origdescription.empty())
                    data.push_back(Pair("description", origdescription));
                result.push_back(Pair(tokenid.GetHex(), data));
        }
        else
        {
            result.push_back(Pair("result", "error"));
            result.push_back(Pair("error", "couldn't decode token creation txid, possibly still in mempool"));
            return (result);
        }
        
        if (!(total < samplenum || samplenum == 0))
            return (result);

        // find an update tx which spent the token create baton vout, if it exists
        if ((retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 2)) == 0 &&
            GetTransaction(batontxid, txBaton, hashBlock, true) &&
            !hashBlock.IsNull() &&
            txBaton.vout.size() > 0 &&
            txBaton.vout[0].nValue == 10000 &&
            (funcId = DecodeTokenUpdateOpRet(txBaton.vout.back().scriptPubKey, updaterPubkey, tokenIdInOpret)) == 'u' &&
            getCCopret(txBaton.vout[0].scriptPubKey, batonopret) &&
            (funcId = DecodeTokenUpdateCCOpRet(batonopret, assetHash, value, ccode, message) == 'u'))
        {
            total++;
            UniValue data(UniValue::VOBJ);
            data.push_back(Pair("updaterPubkey", HexStr(updaterPubkey))); 
            data.push_back(Pair("hash", assetHash.GetHex()));
            data.push_back(Pair("value", (double)value/COIN));
            data.push_back(Pair("ccode", ccode));
            if (!message.empty())
                    data.push_back(Pair("message", message));
            result.push_back(Pair(batontxid.GetHex(), data));
            sourcetxid = batontxid;
        }
        else
        {
            return (result);
        }
    
        if (!(total < samplenum || samplenum == 0))
            return (result);
        
        // baton vout should be vout0 from now on
        while ((retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 0)) == 0)  // find a tx which spent the baton vout
        {
            if (GetTransaction(batontxid, txBaton, hashBlock, true) &&  // load the transaction which spent the baton
                !hashBlock.IsNull() &&                           // tx not in mempool
                txBaton.vout.size() > 0 &&             
                txBaton.vout[0].nValue == 10000 &&     // check baton fee 
                (funcId = DecodeTokenUpdateOpRet(txBaton.vout.back().scriptPubKey, updaterPubkey, tokenIdInOpret)) == 'u' && // decode opreturn
                getCCopret(txBaton.vout[0].scriptPubKey, batonopret) &&
                (funcId = DecodeTokenUpdateCCOpRet(batonopret, assetHash, value, ccode, message) == 'u'))
            {
                total++;
                UniValue data(UniValue::VOBJ);
                data.push_back(Pair("updaterPubkey", HexStr(updaterPubkey))); 
                data.push_back(Pair("hash", assetHash.GetHex()));
                data.push_back(Pair("value", (double)value/COIN));
                data.push_back(Pair("ccode", ccode));
                if (!message.empty())
                    data.push_back(Pair("message", message));
                result.push_back(Pair(batontxid.GetHex(), data));
                sourcetxid = batontxid;
            }
            else
            {
                result.push_back(Pair(batontxid.GetHex(), "error: couldn't decode"));
                return (result);
            }
        if (!(total < samplenum || samplenum == 0))
            break;
        }
        return (result);
    }
    else //from latest to earliest
    {
        if (!GetLatestTokenUpdate(tokenid, latesttxid))
        {
            result.push_back(Pair("result", "error"));
            result.push_back(Pair("error", "latest update is still in mempool or tokenid isnt token creation txid"));
            return (result);
        }
        sourcetxid = latesttxid;
        while (sourcetxid != tokenid)
        {
            if (GetTransaction(sourcetxid, txBaton, hashBlock, true) &&  // load the transaction which spent the baton
                !hashBlock.IsNull() &&                           // tx not in mempool
                txBaton.vout.size() > 0 &&             
                txBaton.vout[0].nValue == 10000 &&     // check baton fee 
                (funcId = DecodeTokenUpdateOpRet(txBaton.vout.back().scriptPubKey, updaterPubkey, tokenIdInOpret)) == 'u' && // decode opreturn
                getCCopret(txBaton.vout[0].scriptPubKey, batonopret) &&
                (funcId = DecodeTokenUpdateCCOpRet(batonopret, assetHash, value, ccode, message) == 'u'))
            {
                total++;
                UniValue data(UniValue::VOBJ);
                data.push_back(Pair("updaterPubkey", HexStr(updaterPubkey))); 
                data.push_back(Pair("hash", assetHash.GetHex()));
                data.push_back(Pair("value", (double)value/COIN));
                data.push_back(Pair("ccode", ccode));
                if (!message.empty())
                    data.push_back(Pair("message", message));
                result.push_back(Pair(sourcetxid.GetHex(), data));
            }
            else
            {
                result.push_back(Pair(batontxid.GetHex(), "error: couldn't decode"));
                return (result);
            }
            if (!(total < samplenum || samplenum == 0))
                break;
            if (GetTransaction((batontxid = txBaton.vin[1].prevout.hash), txBaton, hashBlock) && !hashBlock.IsNull())
            {
                sourcetxid = batontxid;
            }
        }
        if (!(total < samplenum || samplenum == 0))
            return (result);
        // special handling for token creation tx - in this tx, baton vout is vout2
        if (sourcetxid == tokenid) //unlikely to fail, hopefully
        {
            if (GetTransaction(sourcetxid, txBaton, hashBlock, true) &&
            !hashBlock.IsNull() &&
            txBaton.vout.size() > 2 &&
            (funcId = DecodeTokenCreateOpRet(txBaton.vout.back().scriptPubKey, updaterPubkey, dummyName, origdescription)) == 'c' &&
            txBaton.vout[2].nValue == 10000 &&
            getCCopret(txBaton.vout[2].scriptPubKey, batonopret) &&
            (funcId = DecodeTokenUpdateCCOpRet(batonopret, assetHash, value, ccode, message) == 'u'))
            {
                total++;
                UniValue data(UniValue::VOBJ);
                data.push_back(Pair("creatorPubkey", HexStr(updaterPubkey))); 
                data.push_back(Pair("hash", assetHash.GetHex()));
                data.push_back(Pair("value", (double)value/COIN));
                data.push_back(Pair("ccode", ccode));
                if (!origdescription.empty())
                    data.push_back(Pair("description", origdescription));
                result.push_back(Pair(tokenid.GetHex(), data));
            }
            else
            {
                result.push_back(Pair("error", "couldn't decode token creation txid"));
                return (result);
            }
        }
        else
            result.push_back(Pair("error", "initial sample txid isnt token creation txid"));
        return (result);
    }
}

int64_t GetTokenBalance(CPubKey pk, uint256 tokenid)
{
    uint256 hashBlock;
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    CTransaction tokentx;

    // CCerror = strprintf("obsolete, cannot return correct value without eval");
    // return 0;

	if (myGetTransaction(tokenid, tokentx, hashBlock) == 0)
	{
        LOGSTREAM((char *)"cctokens", CCLOG_INFO, stream << "cant find tokenid" << std::endl);
		CCerror = strprintf("cant find tokenid");
		return 0;
	}

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_TOKENS);
    return (AddTokenCCInputs(cp, mtx, pk, tokenid, 0, 0));
}

UniValue TokenInfo(uint256 tokenid)
{
    UniValue result(UniValue::VOBJ);
    uint256 hashBlock, referenceTokenId;
    CTransaction tokenbaseTx;
    std::vector<uint8_t> origpubkey;
    std::vector<std::pair<uint8_t, vscript_t>> oprets;
    vscript_t vopretNonfungible;
    std::string name, description, tokenType;
    double ownerPerc;
    struct CCcontract_info *cpTokens, tokensCCinfo;
    int64_t timeleft, supply = 0, expiryTimeSec;
    int32_t numblocks;
    uint64_t durationSec = 0;

    cpTokens = CCinit(&tokensCCinfo, EVAL_TOKENS);

	if( !myGetTransaction(tokenid, tokenbaseTx, hashBlock) )
	{
		fprintf(stderr, "TokenInfo() cant find tokenid\n");
		result.push_back(Pair("result", "error"));
		result.push_back(Pair("error", "cant find tokenid"));
		return(result);
	}
    if ( KOMODO_NSPV_FULLNODE && hashBlock.IsNull()) {
        result.push_back(Pair("result", "error"));
        result.push_back(Pair("error", "the transaction is still in mempool"));
        return (result);
    }

    if (tokenbaseTx.vout.size() > 0 && DecodeTokenCreateOpRet(tokenbaseTx.vout[tokenbaseTx.vout.size() - 1].scriptPubKey, origpubkey, name, description, ownerPerc, tokenType, referenceTokenId, expiryTimeSec, oprets) != 'c') {
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
    result.push_back(Pair("tokenType", tokenType));

    if (referenceTokenId != zeroid)
        result.push_back(Pair("referenceTokenId", referenceTokenId.GetHex()));

    if (tokenType == "m" || tokenType == "s") {
        durationSec = CCduration(numblocks, tokenid);
        bool isExpired = (durationSec > expiryTimeSec) ? true : false;
        if (expiryTimeSec == 0)
            isExpired = false;
        timeleft = expiryTimeSec - durationSec; // added to calculate time left
        result.push_back(Pair("expirytime", expiryTimeSec));
        result.push_back(Pair("timeleft", timeleft));
        result.push_back(Pair("isExpired", isExpired));
    }

    if (tokenType == "a") {
        result.push_back(Pair("ownerPerc", ownerPerc));
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

/*UniValue TokenOwners(uint256 tokenid, int currentonly)
{
	UniValue result(UniValue::VARR);
	
	check if tokenid is tokencreate id
	txid = token create id;
	do VoutShenanigansloop on txid
	
	VoutShenanigansloop (txid):
		get tx info from txid, get its funcid
		check if it is valid (funcid = 't', token transfer etc.) if not, break or return
		collect destpubkey(s), if not seen yet slap it into the Owners array
		for each vout in current tx:
			if it is a token vout (not regular or marker or baton!), and it has been spent
				if spending tx not seen yet (not in FoundTxId array)
				{
					slap it into the FoundTxId array
					do VoutShenanigansloop on the txid that spent it
				}
			if not spent, leave it alone and move on.
}*/

UniValue TokenOwners(uint256 tokenid, int currentonly)
{
	UniValue result(UniValue::VARR);
	struct CCcontract_info *cpTokens, C;
    cpTokens = CCinit(&C, EVAL_TOKENS);
	
	std::vector<uint256> foundtxids;
	uint256 hashBlock;
    CTransaction tokenbaseTx;
	std::vector<uint8_t> origpubkey, ownerpk;
    std::string name, description;
	
	//std::vector<CPubKey> &voutPubkeys;
	std::vector<std::vector<uint8_t>> owners;
	
	if (!myGetTransaction(tokenid, tokenbaseTx, hashBlock))
	{
		fprintf(stderr, "TokenOwners() cant find tokenid\n");
		return(result);
	}
	if ( KOMODO_NSPV_FULLNODE && hashBlock.IsNull())
	{
		fprintf(stderr, "the transaction is still in mempool\n");
        return (result);
    }
	if (tokenbaseTx.vout.size() > 0 && DecodeTokenCreateOpRet(tokenbaseTx.vout[tokenbaseTx.vout.size() - 1].scriptPubKey, origpubkey, name, description) != 'c')
	{
        fprintf(stderr, "tokenid isnt token creation txid\n");
        return(result);
    }
	
	owners.push_back(origpubkey);
	
	//VoutShenanigansloop (txid)
	
	/*
	VoutShenanigansloop (txid):
		get tx info from txid, get its funcid
		check if it is valid (funcid = 't', token transfer etc.) if not, break or return
		collect destpubkey(s), if not seen yet slap it into the Owners array
		for each vout in current tx:
			if it is a token vout (not regular or marker or baton!), and it has been spent
				if spending tx not seen yet (not in FoundTxId array)
				{
					slap it into the FoundTxId array
					do VoutShenanigansloop on the txid that spent it
				}
			if not spent, leave it alone and move on.
	*/
	
	// by this point we should have the owners array filled with pubkeys
	// the array needs to be checked thru, and based on currentonly flag and dupe status, pushed to result
	
	/*
	Converting CPubkey to vscript_t (or std::vector<uint8_t>):
		vscript_t(mypk.begin(), mypk.end())
	*/
	
	for (std::vector<std::vector<uint8_t>>::const_iterator it = owners.begin(); it != owners.end(); it++)
	{
		ownerpk = std::vector<uint8_t>(*it.begin(), *it.end());
		result.push_back(HexStr(ownerpk));
	}
	
    //result.push_back(tokenid.GetHex());
    result.push_back(currentonly);
    return(result);
}

UniValue TokenList()
{
	UniValue result(UniValue::VARR);
	std::vector<uint256> txids, foundtxids;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressIndexCCMarker;

	struct CCcontract_info *cp, C; uint256 txid, hashBlock;
	CTransaction vintx; std::vector<uint8_t> origpubkey;
	std::string name, description;

	cp = CCinit(&C, EVAL_TOKENS);

    auto addTokenId = [&](uint256 txid) {
        if (myGetTransaction(txid, vintx, hashBlock) != 0) {
            if (vintx.vout.size() > 0 && DecodeTokenCreateOpRet(vintx.vout[vintx.vout.size() - 1].scriptPubKey, origpubkey, name, description) != 0) {
				if (std::find(foundtxids.begin(), foundtxids.end(), txid) == foundtxids.end()) {
					result.push_back(txid.GetHex());
					foundtxids.push_back(txid); // added to remove duplicate txids since update batons are treated here as markers - dan
				}
            }
        }
    };

	SetCCtxids(txids, cp->normaladdr,false,cp->evalcode,zeroid,'c');                      // find by old normal addr marker
   	for (std::vector<uint256>::const_iterator it = txids.begin(); it != txids.end(); it++) 	{
        addTokenId(*it);
	}

    SetCCunspents(addressIndexCCMarker, cp->unspendableCCaddr,true);    // find by burnable validated cc addr marker
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressIndexCCMarker.begin(); it != addressIndexCCMarker.end(); it++) {
        addTokenId(it->first.txhash);
    }

	return(result);
}