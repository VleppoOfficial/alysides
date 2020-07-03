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

#include "CCexchanges.h"
#include "CCagreements.h"
#include "CCtokens.h"

//===========================================================================
// Opret encoding/decoding functions
//===========================================================================

CScript EncodeExchangeOpenOpRet(uint8_t version,CPubKey tokensupplier,CPubKey coinsupplier,uint8_t exchangetype,uint256 tokenid,int64_t numtokens,int64_t numcoins,uint256 agreementtxid)
{
	CScript opret; uint8_t evalcode = EVAL_EXCHANGES, funcid = 'o';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << tokensupplier << coinsupplier << exchangetype << tokenid << numtokens << numcoins << agreementtxid);
	return(opret);
}
uint8_t DecodeExchangeOpenOpRet(CScript scriptPubKey,uint8_t &version,CPubKey &tokensupplier,CPubKey &coinsupplier,uint8_t &exchangetype,uint256 &tokenid,int64_t &numtokens,int64_t &numcoins,uint256 &agreementtxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> tokensupplier; ss >> coinsupplier; ss >> exchangetype; ss >> tokenid; ss >> numtokens; ss >> numcoins; ss >> agreementtxid) != 0 && evalcode == EVAL_EXCHANGES)
		return(funcid);
	return(0);
}

CScript EncodeExchangeLoanTermsOpRet(uint8_t version,uint256 exchangetxid,int64_t interest,int64_t duedate)
{
	CScript opret; uint8_t evalcode = EVAL_EXCHANGES, funcid = 'l';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << interest << duedate);
	return(opret);
}
uint8_t DecodeExchangeLoanTermsOpRet(CScript scriptPubKey,uint8_t &version,uint256 &exchangetxid,int64_t &interest,int64_t &duedate)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> exchangetxid; ss >> interest; ss >> duedate) != 0 && evalcode == EVAL_EXCHANGES)
		return(funcid);
	return(0);
}

CScript EncodeExchangeOpRet(uint8_t funcid,uint8_t version,uint256 exchangetxid,uint256 tokenid,CPubKey tokensupplier,CPubKey coinsupplier)
{
	CScript opret;
	uint8_t evalcode = EVAL_EXCHANGES;
	vscript_t vopret;
	vopret = E_MARSHAL(ss << evalcode << funcid << version << exchangetxid);
	if (tokenid != zeroid)
	{
		std::vector<CPubKey> pks;
		pks.push_back(tokensupplier);
		pks.push_back(coinsupplier);
		return (EncodeTokenOpRetV1(tokenid,pks, { vopret }));
	}
	opret << OP_RETURN << vopret;
	return(opret);
}
uint8_t DecodeExchangeOpRet(const CScript scriptPubKey,uint8_t &version,uint256 &exchangetxid,uint256 &tokenid)
{
	std::vector<vscript_t> oprets;
	std::vector<uint8_t> vopret, vOpretExtra;
	uint8_t *script, evalcode, funcid, exchangetype;
	std::vector<CPubKey> pubkeys;
	int64_t dummyamount;
	CPubKey dummypk; 
	uint256 dummytxid;
	
	exchangetxid = tokenid = zeroid;
	
	if (DecodeTokenOpRetV1(scriptPubKey,tokenid,pubkeys,oprets) != 0 && GetOpReturnCCBlob(oprets, vOpretExtra) && vOpretExtra.size() > 0)
    {
        vopret = vOpretExtra;
    }
	else
	{
		GetOpReturnData(scriptPubKey, vopret);
	}
	
	script = (uint8_t *)vopret.data();
    if (script != NULL && vopret.size() > 2)
    {
		if (script[0] != EVAL_EXCHANGES)
			return(0);

        funcid = script[1];
        switch (funcid)
        {
			case 'o':
				return DecodeExchangeOpenOpRet(scriptPubKey,version,dummypk,dummypk,exchangetype,dummytxid,dummyamount,dummyamount,dummytxid);
			case 'l':
				return DecodeExchangeLoanTermsOpRet(scriptPubKey,version,dummytxid,dummyamount,dummyamount);
			default:
				if (E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> exchangetxid) != 0 && evalcode == EVAL_EXCHANGES)
				{
					return(funcid);
				}
				return(0);
		}
	}
	return(0);
}

//===========================================================================
// Validation
//===========================================================================

bool ExchangesValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	int32_t numvins, numvouts;
	int64_t numtokens, numcoins, coininputs, tokeninputs, coinoutputs, tokenoutputs;
	uint8_t funcid, version, exchangetype; 
	uint256 hashBlock, exchangetxid, agreementtxid, tokenid;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	CPubKey tokensupplier, coinsupplier;
	CTransaction exchangetx;
	std::string CCerror;
	
	numvins = tx.vin.size();
	numvouts = tx.vout.size();
	
	if (numvouts < 1)
		return eval->Invalid("no vouts");
	CCOpretCheck(eval,tx,true,true,true);
	ExactAmounts(eval,tx,ASSETCHAINS_CCZEROTXFEE[EVAL_EXCHANGES]?0:CC_TXFEE);
	
	if ((funcid = DecodeExchangeOpRet(tx.vout[numvouts-1].scriptPubKey, version, exchangetxid, tokenid)) != 0)
	{
		if (!(myGetTransaction(exchangetxid,exchangetx,hashBlock) != 0 && exchangetx.vout.size() > 0 &&
		DecodeExchangeOpenOpRet(exchangetx.vout[exchangetx.vout.size()-1].scriptPubKey,version,tokensupplier,coinsupplier,exchangetype,tokenid,numtokens,numcoins,agreementtxid) == 'o'))
			return eval->Invalid("cannot find exchangeopen tx for ExchangesValidate!");
			
		if (funcid != 'c' && ValidateExchangeOpenTx(exchangetx, CCerror))
			return eval->Invalid(CCerror);
		
		/*if (ExchangesExactAmounts(cp,eval,tx,tokensupplier,coinsupplier,coininputs,tokeninputs,coinoutputs,tokenoutputs) == false)
		{
			return eval->Invalid("invalid exchange inputs vs. outputs!");
		}*/
	
		switch (funcid)
		{
			case 'o':
				// exchange open:
				// vin.0 normal input
				// vout.0 CC baton to mutual 1of2 address
				// vout.1 CC marker to tokensupplier
				// vout.2 CC marker to coinsupplier
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘o’ version tokensupplier coinsupplier exchangetype tokenid numtokens numcoins agreementtxid
				return eval->Invalid("unexpected ExchangesValidate for exchangeopen!");
			
			case 'f':
				// exchange fund:
				// vin.0 normal input
				// vin.1 .. vin.n-1: normal or token CC inputs
				// vout.0 token CC or normal payment to token/coin CC 1of2 addr
				// vout.1 token change (if any)
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘f’ version exchangetxid tokenid tokensupplier coinsupplier
				return eval->Invalid("unexpected ExchangesValidate for exchangefund!");
			
			case 'l':
				// exchange loan terms:
				// vin.0 normal input
				// vin.1 previous baton CC input
				// vout.0 next baton CC output
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘l’ exchangetxid interest duedate
				break;
				
			/*"Loan terms" or "l":
				Data constraints:
					(Type check) Exchange must be loan type
					(Duration) if borrow transaction exists, duration since borrow transaction must exceed due date from latest loan terms
					(Due date) new due date must be bigger than current one
				TX constraints:
					(Pubkey check) must be executed by coin provider pk
					(Tokens dest addr) no tokens should be moved anywhere
					(Coins dest addr) no coins should be moved anywhere*/

			case 'c':
				// exchange cancel:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs (if any)
				// vout.0 normal output to coinsupplier
				// vout.1 token CC output to tokensupplier
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘c’ version exchangetxid tokenid tokensupplier coinsupplier
				
				std::cerr << "validation - coins: " << GetExchangesInputs(cp,exchangetx,EIF_COINS,unspentOutputs) << std::endl;
				std::cerr << "validation - tokens: " << GetExchangesInputs(cp,exchangetx,EIF_TOKENS,unspentOutputs) << std::endl;
				break;

			/*"Cancel" or "c":
				Data constraints:
					(FindExchangeTxidType) if exchange contains borrow tx, can only be executed by coin provider pk
					(CheckDepositUnlockCond) If exchange contains no borrow tx, has an assoc. agreement and finalsettlement = true, the deposit must NOT be sent to the exchange's coin escrow
					(GetExchangesInputs) If exchange is trade type, token & coin escrow cannot be fully filled (tokens >= numtokens && coins >= numcoins).
				TX constraints:
					(Pubkey check) must be executed by token or coin provider pk
					(Tokens dest addr) all tokens must be sent to token provider
					(Coins dest addr) all coins must be sent to coin provider*/
					
			case 's':
				// exchange swap:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs
				// vout.0 normal output to tokensupplier
				// vout.1 token CC output to coinsupplier
				// vout.2 normal change from CC 1of2 addr to coinsupplier (if any)
				// vout.3 token CC change from CC 1of2 addr to tokensupplier (if any)
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘s’ version exchangetxid tokenid tokensupplier coinsupplier
				
			/*"Swap" or "s" (part of exchangeclose rpc):
				Data constraints:
					(Type check) Exchange must be trade type
					(GetExchangesInputs) Token escrow must contain >= numtokens, and coin escrow must contain >= numcoins
					(Signing check) at least one coin fund vin must be signed by coin provider pk
					(CheckDepositUnlockCond) If exchange has an assoc. agreement and finalsettlement = true, the deposit must be sent to the exchange's coin escrow
				TX constraints:
					(Pubkey check) must be executed by token or coin provider pk
					(Tokens dest addr) all tokens must be sent to coin provider. if (somehow) there are more tokens than numtokens, return remaining tokens to token provider
					(Coins dest addr) all coins must be sent to token provider. if there are more coins than numcoins, return remaining coins to coin provider*/
			
			case 'b':
				// exchange borrow:
				// vin.0 normal input
				// vin.1 previous baton CC input
				// vin.2 .. vin.n-1: valid CC coin outputs (no tokens)
				// vout.0 next baton CC output
				// vout.1 normal change from CC 1of2 addr to coinsupplier (if any)
				// vout.n-2 coins to tokensupplier & normal change
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘b’ exchangetxid
				break;
			
			/*"Borrow" or "b":
				Data constraints:
					(Type check) Exchange must be loan type
					(FindExchangeTxidType) exchange must not contain a borrow transaction
					(Signing check) at least one coin fund vin must be signed by coin provider pk
					(CheckDepositUnlockCond) If exchange has an assoc. agreement and finalsettlement = true, the deposit must be sent to the exchange's coin escrow
					(GetExchangesInputs) Token escrow must contain >= numtokens, and coin escrow must contain >= numcoins
				TX constraints:
					(Pubkey check) must be executed by token provider pk
					(Tokens dest addr) no tokens should be moved anywhere
					(Coins dest addr) all coins must be sent to token provider. if there are more coins than numcoins, return remaining coins to coin provider*/

			case 'p':
				// exchange repo:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs
				// vout.0 normal output to tokensupplier
				// vout.1 token CC output to coinsupplier
				// vout.2 token CC change from CC 1of2 addr to tokensupplier (if any)
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘p’ version exchangetxid tokenid tokensupplier coinsupplier
			
			/*"Repo" or "p":
				Data constraints:
					(Type check) Exchange must be loan type
					(FindExchangeTxidType) exchange must contain a borrow transaction
					(Duration) duration since borrow transaction must exceed due date from latest loan terms
					(GetExchangesInputs) Token escrow must contain >= numtokens, coin escrow must not contain >= numcoins + interest from latest loan terms
				TX constraints:
					(Pubkey check) must be executed by coin provider pk
					(Tokens dest addr) all tokens must be sent to coin provider. if (somehow) there are more tokens than numtokens, return remaining tokens to token provider
					(Coins dest addr) all coins must be sent to token provider.*/
			
			case 'r':
				// exchange release:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs
				// vout.0 normal output to coinsupplier
				// vout.1 token CC output to tokensupplier
				// vout.2 normal change from CC 1of2 addr to coinsupplier (if any)
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘r’ version exchangetxid tokenid tokensupplier coinsupplier
			
			/*"Release" or "r" (part of exchangeclose rpc):
				Data constraints:
					(Type check) Exchange must be loan type
					(FindExchangeTxidType) exchange must contain a borrow transaction
					(Signing check) at least one coin fund vin must be signed by token provider pk
					(GetExchangesInputs) Token escrow must contain >= numtokens, coin escrow must contain >= numcoins + interest from latest loan terms
				TX constraints:
					(Pubkey check) must be executed by token or coin provider pk
					(Tokens dest addr) all tokens must be sent to token provider
					(Coins dest addr) all coins must be sent to coin provider. if there are more coins than numcoins, return remaining coins to token provider*/

			default:
				fprintf(stderr,"unexpected exchanges funcid (%c)\n",funcid);
				return eval->Invalid("unexpected exchanges funcid!");
		}
	}
	else
		return eval->Invalid("must be valid exchanges funcid!");
	
	LOGSTREAM("exchanges", CCLOG_INFO, stream << "Exchanges tx validated" << std::endl);
	std::cerr << "Exchanges tx validated" << std::endl;
	
	return true;
}

//===========================================================================
// Helper functions
//===========================================================================

int64_t IsExchangesvout(struct CCcontract_info *cp,const CTransaction& tx,bool mode,CPubKey tokensupplier,CPubKey coinsupplier,int32_t v)
{
	char destaddr[65],exchangeaddr[65];
	if (mode == EIF_TOKENS)
	{
		GetTokensCCaddress1of2(cp,exchangeaddr,tokensupplier,coinsupplier);
	}
	else
	{
		GetCCaddress1of2(cp,exchangeaddr,tokensupplier,coinsupplier);
	}
	if (tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0)
	{
		if (Getscriptaddress(destaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(destaddr,exchangeaddr) == 0)
			return(tx.vout[v].nValue);
	}
	return(0); 
}

int64_t IsExchangesMarkervout(struct CCcontract_info *cp,const CTransaction& tx,CPubKey pubkey,int32_t v)
{
	char destaddr[65],ccaddr[65];
	GetCCaddress(cp,ccaddr,pubkey);
	if (tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0)
	{
		if (Getscriptaddress(destaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(destaddr,ccaddr) == 0)
			return(tx.vout[v].nValue);
	}
	return(0);
}

bool ExchangesExactAmounts(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, int64_t &coininputs, int64_t &tokeninputs, int64_t &coinoutputs, int64_t &tokenoutputs)
{
	uint256 hashBlock, exchangetxid, tokenid;
	int32_t i, numvins, numvouts;
	uint8_t funcid, version;
	CTransaction vinTx;
	
	coininputs = coinoutputs = 0;

	numvins = tx.vin.size();
    numvouts = tx.vout.size();

    
	
	
	if ((numvouts = tx.vout.size()) > 0 && (funcid = DecodeExchangeOpRet(tx.vout[numvouts-1].scriptPubKey, version, exchangetxid, tokenid)) != 0)
	{		
		switch (funcid)
		{
			case 'o':
				return (true);
			case 'l':
				// exchange loan terms:
				// vin.0 normal input
				// vin.1 previous baton CC input
				// vout.0 next baton CC output
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘l’ exchangetxid interest duedate
				return (false);
			case 'c':
				// exchange cancel:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs (if any)
				// vout.0 normal output to coinsupplier
				// vout.1 token CC output to tokensupplier
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘c’ version exchangetxid tokenid tokensupplier coinsupplier
				
				/*for (i=0; i<numvins; i++)
				{
					if ((*cp->ismyvin)(tx.vin[i].scriptSig) != 0)
					{
						if (eval->GetTxUnconfirmed(tx.vin[i].prevout.hash,vinTx,hashBlock) == 0)
							return eval->Invalid("always should find vin, but didnt");
						else
						{
							if ( hashBlock == zerohash )
								return eval->Invalid("cant rewards from mempool");
							if ( (assetoshis= IsRewardsvout(cp,vinTx,tx.vin[i].prevout.n,refsbits,reffundingtxid)) != 0 )
								inputs += assetoshis;
						}
					}
				}
				for (i=0; i<numvouts; i++)
				{
					//fprintf(stderr,"i.%d of numvouts.%d\n",i,numvouts);
					if ( (assetoshis= IsRewardsvout(cp,tx,i,refsbits,reffundingtxid)) != 0 )
						outputs += assetoshis;
				}
				*/
				
			case 's':
				// exchange swap:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs
				// vout.0 normal output to tokensupplier
				// vout.1 token CC output to coinsupplier
				// vout.2 normal change from CC 1of2 addr to coinsupplier (if any)
				// vout.3 token CC change from CC 1of2 addr to tokensupplier (if any)
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘s’ version exchangetxid tokenid tokensupplier coinsupplier
			case 'b':
				// exchange borrow:
				// vin.0 normal input
				// vin.1 previous baton CC input
				// vin.2 .. vin.n-1: valid CC coin outputs (no tokens)
				// vout.0 next baton CC output
				// vout.1 normal change from CC 1of2 addr to coinsupplier (if any)
				// vout.n-2 coins to tokensupplier & normal change
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘b’ exchangetxid
				return (false);
			case 'p':
				// exchange repo:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs
				// vout.0 normal output to tokensupplier
				// vout.1 token CC output to coinsupplier
				// vout.2 token CC change from CC 1of2 addr to tokensupplier (if any)
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘p’ version exchangetxid tokenid tokensupplier coinsupplier
				return (false);
			case 'r':
				// exchange release:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs
				// vout.0 normal output to coinsupplier
				// vout.1 token CC output to tokensupplier
				// vout.2 normal change from CC 1of2 addr to coinsupplier (if any)
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_EXCHANGES ‘r’ version exchangetxid tokenid tokensupplier coinsupplier
				return (false);
				break;
			/*case 'f':
				if ( eval->GetTxUnconfirmed(tx.vin[1].prevout.hash,vinTx,hashBlock) == 0 )
					return eval->Invalid("cant find vinTx");
				inputs = vinTx.vout[tx.vin[1].prevout.n].nValue;
				outputs = tx.vout[0].nValue + tx.vout[3].nValue; 
				break;
			case 'C':
				if ( eval->GetTxUnconfirmed(tx.vin[1].prevout.hash,vinTx,hashBlock) == 0 )
					return eval->Invalid("cant find vinTx");
				inputs = vinTx.vout[tx.vin[1].prevout.n].nValue;
				outputs = tx.vout[0].nValue; 
				break;
			case 'R':
				if ( eval->GetTxUnconfirmed(tx.vin[1].prevout.hash,vinTx,hashBlock) == 0 )
					return eval->Invalid("cant find vinTx");
				inputs = vinTx.vout[tx.vin[1].prevout.n].nValue;
				outputs = tx.vout[2].nValue; 
				break;*/
			default:
				return (false);
		}
		if (coininputs != coinoutputs)
		{
			LOGSTREAM("exchangescc", CCLOG_INFO, stream << "inputs " << coininputs << " vs outputs " << coinoutputs << std::endl);			
			return eval->Invalid("mismatched inputs != outputs");
		} 
		else return (true);	   
	}
	else
	{
		return eval->Invalid("invalid op_return data");
	}
	return (false);
}

/*bool RewardsExactAmounts(struct CCcontract_info *cp,Eval *eval,const CTransaction &tx,uint64_t txfee,uint64_t refsbits,uint256 reffundingtxid)
{
    static uint256 zerohash;
    CTransaction vinTx; uint256 hashBlock; int32_t i,numvins,numvouts; int64_t inputs=0,outputs=0,assetoshis;
    numvins = tx.vin.size();
    numvouts = tx.vout.size();
    for (i=0; i<numvins; i++)
    {
        if ( (*cp->ismyvin)(tx.vin[i].scriptSig) != 0 )
        {
            if ( eval->GetTxUnconfirmed(tx.vin[i].prevout.hash,vinTx,hashBlock) == 0 )
                return eval->Invalid("always should find vin, but didnt");
            else
            {
                if ( hashBlock == zerohash )
                    return eval->Invalid("cant rewards from mempool");
                if ( (assetoshis= IsRewardsvout(cp,vinTx,tx.vin[i].prevout.n,refsbits,reffundingtxid)) != 0 )
                    inputs += assetoshis;
            }
        }
    }
    for (i=0; i<numvouts; i++)
    {
        //fprintf(stderr,"i.%d of numvouts.%d\n",i,numvouts);
        if ( (assetoshis= IsRewardsvout(cp,tx,i,refsbits,reffundingtxid)) != 0 )
            outputs += assetoshis;
    }
    if ( inputs != outputs+txfee )
    {
        fprintf(stderr,"inputs %llu vs outputs %llu txfee %llu\n",(long long)inputs,(long long)outputs,(long long)txfee);
        return eval->Invalid("mismatched inputs != outputs + txfee");
    }
    else return(true);
}*/


// Get latest update txid and funcid
bool GetLatestExchangeTxid(uint256 exchangetxid, uint256 &latesttxid, uint8_t &funcid)
{
	int32_t vini, height, retcode;
	uint256 batontxid, sourcetxid, hashBlock, dummytxid;
	CTransaction exchangetx, batontx;
	uint8_t version;

	if (myGetTransaction(exchangetxid, exchangetx, hashBlock) == 0 || exchangetx.vout.size() <= 0)
	{
		std::cerr << "GetLatestExchangeTxid: couldn't find exchange tx" << std::endl;
		return false;
	}
	if (DecodeExchangeOpRet(exchangetx.vout[exchangetx.vout.size() - 1].scriptPubKey,version,dummytxid,dummytxid) != 'o')
	{
		std::cerr << "GetLatestExchangeTxid: exchange tx is not an opening tx" << std::endl;
		return false;
	}
	if ((retcode = CCgetspenttxid(batontxid, vini, height, exchangetxid, 0)) != 0)
	{
		latesttxid = exchangetxid; // no updates, return exchangetxid
		funcid = 'o';
		return true;
	}
	sourcetxid = exchangetxid;
	while ((retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 0)) == 0 && 
	myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0)
	{
		funcid = DecodeExchangeOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey,version,dummytxid,dummytxid);
		switch (funcid)
		{
			case 'l':
			case 'b':
				sourcetxid = batontxid;
				continue;
			case 'c':
			case 's':
			case 'r':
			case 'p':
				sourcetxid = batontxid;
				break;
			default:
				std::cerr << "GetLatestExchangeTxid: found an update, but it has incorrect funcid '" << funcid << "'" << std::endl;
				return false;
		}
		break;
	}
	latesttxid = sourcetxid;
	return true;
}

// Gets latest exchange update txid of the specified type
bool FindExchangeTxidType(uint256 exchangetxid, uint8_t type, uint256 &typetxid)
{
	CTransaction batontx;
	uint256 batontxid, dummytxid, hashBlock;
	uint8_t version, funcid;
	if (!GetLatestExchangeTxid(exchangetxid, batontxid, funcid))
	{
		std::cerr << "FindExchangeTxidType: can't find latest update tx" << std::endl;
		return false;
	}
	if (batontxid == exchangetxid && funcid == 'o')
	{
		typetxid == exchangetxid;
		return true;
	}
	
	while (batontxid != exchangetxid && myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 &&
	batontx.vin.size() > 1 && batontx.vin[1].prevout.n == 0 &&
	(funcid = DecodeExchangeOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey,version,dummytxid,dummytxid)) != 0)
	{
		if (funcid == type)
		{
			typetxid == batontxid;
			return true;
		}
		batontxid = batontx.vin[1].prevout.hash;
	}

	typetxid = zeroid;
	return true;
}

// Check if agreement deposit is sent to this exchange's coin escrow
// returns -1 if no agreementtxid defined or deposit unlock disabled, otherwise returns amount sent to exchange 1of2 CC address
int64_t CheckDepositUnlockCond(uint256 exchangetxid)
{
	int32_t vini, height, retcode, numvouts;
	int64_t dummyamount;
	CPubKey tokensupplier, coinsupplier;
	uint256 refexchangetxid, unlocktxid, hashBlock, agreementtxid, dummytxid;
	CTransaction exchangetx, unlocktx;
	uint8_t version, unlockfuncid, exchangetype;
	struct CCcontract_info *cp, C;
	
	cp = CCinit(&C, EVAL_EXCHANGES);
	
	if (myGetTransaction(exchangetxid, exchangetx, hashBlock) && (numvouts = exchangetx.vout.size()) > 0 && 
	DecodeExchangeOpenOpRet(exchangetx.vout[numvouts - 1].scriptPubKey,version,tokensupplier,coinsupplier,exchangetype,dummytxid,dummyamount,dummyamount,agreementtxid) != 0)
	{
		if (agreementtxid == zeroid || !(exchangetype & EXTF_DEPOSITUNLOCKABLE))
			return -1;
		else if (GetLatestAgreementUpdate(agreementtxid, unlocktxid, unlockfuncid) && unlockfuncid == 'n' &&
		myGetTransaction(unlocktxid, unlocktx, hashBlock) && (numvouts = unlocktx.vout.size()) > 0 && 
		DecodeAgreementUnlockOpRet(unlocktx.vout[numvouts - 1].scriptPubKey, version, dummytxid, refexchangetxid) != 0 &&
		refexchangetxid == exchangetxid)
		{
			return (IsExchangesvout(cp, unlocktx, EIF_COINS, tokensupplier, coinsupplier, 0));
		}
	}
	
	return 0;
}

// check if exchange open transaction has valid data and sigs
bool ValidateExchangeOpenTx(CTransaction opentx, std::string &CCerror)
{
	CPubKey CPK_seller, CPK_client, CPK_arbitrator, tokensupplier, coinsupplier;
	CTransaction tokentx, agreementtx;
	uint256 hashBlock, tokenid, agreementtxid, dummytxid;
	uint8_t version, exchangetype;
	int64_t numtokens, numcoins, dummyamount;
	std::vector<uint8_t> dummyPubkey, sellerpk, clientpk, arbitratorpk;
	std::string dummystr;
	
	CCerror = "";
	
	if (DecodeExchangeOpenOpRet(opentx.vout[opentx.vout.size() - 1].scriptPubKey,version,tokensupplier,coinsupplier,exchangetype,tokenid,numtokens,numcoins,agreementtxid) == 0)
	{
		CCerror = "invalid exchange open opret!";
		return false;
	}
	if (tokenid == zeroid)
	{
		CCerror = "tokenid null or invalid in exchange open opret!";
		return false;
	}
	if (!(tokensupplier.IsValid() && coinsupplier.IsValid()))
	{
		CCerror = "token or coin supplier pubkey invalid!";
		return false;
	}
	if (tokensupplier == coinsupplier)
	{
		CCerror = "token supplier cannot be the same as coin supplier pubkey!";
		return false;
	}
	if (!(myGetTransaction(tokenid,tokentx,hashBlock) != 0 && tokentx.vout.size() > 0 &&
	(DecodeTokenCreateOpRetV1(tokentx.vout[tokentx.vout.size()-1].scriptPubKey,dummyPubkey,dummystr,dummystr) != 0)))
	{
		CCerror = "tokenid in exchange open opret is not a valid token creation txid!";
		return false;
	}
	if (numtokens < 1 || numtokens > CCfullsupply(tokenid) || numcoins < 1)
	{
		CCerror = "invalid numcoins or numtokens value in exchange open opret!";
		return false;
	}
	if (!(exchangetype & EXTF_TRADE || exchangetype & EXTF_LOAN))
	{
		CCerror = "incorrect type in exchange open opret!";
		return false;
	}
	if (agreementtxid != zeroid)
	{
		if (!(myGetTransaction(agreementtxid,agreementtx,hashBlock) != 0 && agreementtx.vout.size() > 0 &&
		(DecodeAgreementOpRet(agreementtx.vout[agreementtx.vout.size()-1].scriptPubKey) == 'c')))
		{
			CCerror = "invalid agreement txid in exchange open opret!";
			return false;
		}
		
		GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, dummyamount, dummyamount, dummytxid, dummytxid, dummystr);
		
		CPK_seller = pubkey2pk(sellerpk);
		CPK_client = pubkey2pk(clientpk);
		CPK_arbitrator = pubkey2pk(arbitratorpk);
		
		if ((tokensupplier != CPK_seller && tokensupplier != CPK_client) || (coinsupplier != CPK_seller && coinsupplier != CPK_client))
		{
			CCerror = "agreement client and seller pubkeys doesn't match exchange coinsupplier and tokensupplier pubkeys!";
			return false;
		}
		if (!(TotalPubkeyNormalInputs(opentx,CPK_seller) + TotalPubkeyCCInputs(opentx,CPK_seller) > 0 ||
		TotalPubkeyNormalInputs(opentx,CPK_client) + TotalPubkeyCCInputs(opentx,CPK_client) > 0 ||
		(CPK_arbitrator.IsValid() && TotalPubkeyNormalInputs(opentx,CPK_arbitrator) + TotalPubkeyCCInputs(opentx,CPK_arbitrator) > 0)))
		{
			CCerror = "no valid inputs signed by any agreement party found in exchange open tx!";
			return false;
		}
	}
	// TODO: Check opentx structure
	return true;
}

int64_t GetExchangesInputs(struct CCcontract_info *cp,CTransaction exchangetx,bool mode,std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &validUnspentOutputs)
{
	char exchangeaddr[65];
	int64_t totalinputs = 0, numvouts, numtokens, numcoins;
	uint256 txid = zeroid, hashBlock, exchangetxid, agreementtxid, refagreementtxid, tokenid, fundtokenid, borrowtxid;
	CTransaction vintx;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	CPubKey tokensupplier, coinsupplier;
	uint8_t myprivkey[32], version, funcid, exchangetype;
	bool bHasBorrowed = false;
	
    if ((numvouts = exchangetx.vout.size()) > 0 && 
	DecodeExchangeOpenOpRet(exchangetx.vout[numvouts-1].scriptPubKey,version,tokensupplier,coinsupplier,exchangetype,tokenid,numtokens,numcoins,agreementtxid) == 'o')
    {
		if (mode == EIF_TOKENS)
		{
			GetTokensCCaddress1of2(cp,exchangeaddr,tokensupplier,coinsupplier);
		}
		else
		{
			GetCCaddress1of2(cp,exchangeaddr,tokensupplier,coinsupplier);
		}
		SetCCunspents(unspentOutputs,exchangeaddr,true);
    }
    else
    {
        LOGSTREAM("exchangescc", CCLOG_INFO, stream << "invalid exchange create txid" << std::endl);
        return 0;
    }

	if (!FindExchangeTxidType(exchangetx.GetHash(), 'b', borrowtxid))
	{
        LOGSTREAM("exchangescc", CCLOG_INFO, stream << "exchange borrow transaction search failed" << std::endl);
        return 0;
    }
	else if (borrowtxid != zeroid)
	{
		bHasBorrowed = true;
	}
	
	for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++)
    {
        txid = it->first.txhash;
        if (it->second.satoshis < 1)
            continue;
        if (myGetTransaction(txid, vintx, hashBlock) != 0 && (numvouts = vintx.vout.size()) > 0)
        {
			// is this an exchanges CC output?
			if (IsExchangesvout(cp, vintx, mode, tokensupplier, coinsupplier, (int32_t)it->first.index) > 0 &&
			// is the output from a funding transaction to this exchange txid?
			DecodeExchangeOpRet(vintx.vout[numvouts-1].scriptPubKey, version, exchangetxid, fundtokenid) == 'f' && exchangetxid == exchangetx.GetHash() &&
			// if we're spending coins, are they provided by the coin supplier pubkey?
			((mode == EIF_COINS && fundtokenid == zeroid && (TotalPubkeyNormalInputs(vintx,coinsupplier) + TotalPubkeyCCInputs(vintx,coinsupplier) > 0 || 
			// if a borrow transaction exists in the exchange, the coins can also be provided by the token supplier pubkey
			bHasBorrowed && TotalPubkeyNormalInputs(vintx,tokensupplier) + TotalPubkeyCCInputs(vintx,tokensupplier) > 0)) || 
			// if we're spending tokens, are they provided by the token supplier pubkey?
			(mode == EIF_TOKENS && fundtokenid == tokenid && TotalPubkeyNormalInputs(vintx,tokensupplier) + TotalPubkeyCCInputs(vintx,tokensupplier) > 0)))
            {
                totalinputs += it->second.satoshis;
				validUnspentOutputs.push_back(*it);
            }
			// if exchange has an agreementtxid, is this a CC output from agreementunlock?
			else if (CheckDepositUnlockCond(exchangetx.GetHash()) > 0 && IsExchangesvout(cp, vintx, EIF_COINS, tokensupplier, coinsupplier, (int32_t)it->first.index) > 0 &&
			DecodeAgreementUnlockOpRet(vintx.vout[numvouts-1].scriptPubKey, version, refagreementtxid, exchangetxid) == 'n' && 
			exchangetxid == exchangetx.GetHash() && refagreementtxid == agreementtxid)
			{
				totalinputs += it->second.satoshis;
				validUnspentOutputs.push_back(*it);
			}
        }
    }
    return (totalinputs);
}

int64_t AddExchangesInputs(struct CCcontract_info *cp,CMutableTransaction &mtx,CTransaction exchangetx,bool mode,int32_t maxinputs)
{
	char exchangeaddr[65];
	int64_t nValue, totalinputs = 0, numvouts, numtokens, numcoins;
	uint256 hashBlock, exchangetxid, agreementtxid, tokenid;
	CTransaction vintx;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	CPubKey tokensupplier, coinsupplier;
	uint8_t myprivkey[32], version, funcid, exchangetype;
	int32_t n = 0;
	
	if (maxinputs == 0 || GetExchangesInputs(cp,exchangetx,mode,unspentOutputs) == 0)
		return 0;
	
	if ((numvouts = exchangetx.vout.size()) > 0 && 
	DecodeExchangeOpenOpRet(exchangetx.vout[numvouts-1].scriptPubKey,version,tokensupplier,coinsupplier,exchangetype,tokenid,numtokens,numcoins,agreementtxid) == 'o')
    {
		if (mode == EIF_TOKENS)
		{
			GetTokensCCaddress1of2(cp,exchangeaddr,tokensupplier,coinsupplier);
		}
		else
		{
			GetCCaddress1of2(cp,exchangeaddr,tokensupplier,coinsupplier);
		}
    }
	
	for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++)
    {    
		mtx.vin.push_back(CTxIn(it->first.txhash, (int32_t)it->first.index, CScript()));
		Myprivkey(myprivkey);
		if (mode == EIF_TOKENS)
		{
			CCaddrTokens1of2set(cp, tokensupplier, coinsupplier, myprivkey, exchangeaddr);
		}
		else
		{
			CCaddr1of2set(cp, tokensupplier, coinsupplier, myprivkey, exchangeaddr);
		}
		memset(myprivkey, 0, 32);
				
		nValue = it->second.satoshis;
		totalinputs += nValue;
		n++;
		if (n >= maxinputs)
			break;
    }
    return (totalinputs);
}

//===========================================================================
// RPCs - tx creation
//===========================================================================

UniValue ExchangeOpen(const CPubKey& pk,uint64_t txfee,CPubKey tokensupplier,CPubKey coinsupplier,uint256 tokenid,int64_t numcoins,int64_t numtokens,uint8_t exchangetype,uint256 agreementtxid,bool bSpendDeposit)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk, CPK_seller, CPK_client;
	CTransaction tokentx, agreementtx;
	int32_t vini, height;
	int64_t dummyamount;
	std::string dummystr;
	std::vector<uint8_t> dummyPubkey, sellerpk, clientpk;
	struct CCcontract_info *cp, *cpTokens, C, CTokens;
	uint256 hashBlock, spendingtxid, dummytxid;
	uint8_t version;
	
	cp = CCinit(&C, EVAL_EXCHANGES);
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (txfee == 0)
	{
		txfee = CC_TXFEE;
	}
	
	if (!(tokensupplier.IsFullyValid()))
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Token supplier pubkey invalid");
	
	if (!(coinsupplier.IsFullyValid()))
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Toin supplier pubkey invalid");
	
	if (tokensupplier == coinsupplier)
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Token supplier cannot be the same as coin supplier pubkey");
	
	if (!(myGetTransaction(tokenid,tokentx,hashBlock) != 0 && tokentx.vout.size() > 0 &&
	(DecodeTokenCreateOpRetV1(tokentx.vout[tokentx.vout.size()-1].scriptPubKey,dummyPubkey,dummystr,dummystr) != 0)))
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Tokenid is not a valid token creation txid");
		
	if (numtokens < 1)
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Required token amount must be above 0");
	
	if (numtokens > CCfullsupply(tokenid))
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Required token amount can't be higher than total token supply");
	
	if (numcoins < 1)
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Required coin amount must be above 0");
	
	if (!(exchangetype & EXTF_TRADE || exchangetype & EXTF_LOAN))
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Incorrect exchange type");
	
	if (agreementtxid != zeroid)
	{
		if (!(myGetTransaction(agreementtxid,agreementtx,hashBlock) != 0 && agreementtx.vout.size() > 0 &&
			(DecodeAgreementOpRet(agreementtx.vout[agreementtx.vout.size()-1].scriptPubKey) == 'c')))
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Agreement txid is not a valid proposal signing txid");
		
		GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, dummyPubkey, dummyamount, dummyamount, dummytxid, dummytxid, dummystr);
		
		CPK_seller = pubkey2pk(sellerpk);
		CPK_client = pubkey2pk(clientpk);

		if ((tokensupplier != CPK_seller && tokensupplier != CPK_client) || (coinsupplier != CPK_seller && coinsupplier != CPK_client))
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Agreement client and seller pubkeys doesn't match exchange coinsupplier and tokensupplier pubkeys");
		
		if (bSpendDeposit)
		{
			exchangetype |= EXTF_DEPOSITUNLOCKABLE;

			if (CCgetspenttxid(spendingtxid, vini, height, agreementtxid, 2) == 0)
				CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Agreement deposit was already spent by txid " << spendingtxid.GetHex());
		}
	}

	if (AddNormalinputs2(mtx, txfee + CC_MARKER_VALUE * 2 + CC_BATON_VALUE, 64) >= txfee + CC_MARKER_VALUE * 2 + CC_BATON_VALUE)
	{
		mtx.vout.push_back(MakeCC1of2vout(EVAL_EXCHANGES, CC_BATON_VALUE, tokensupplier, coinsupplier)); // vout.0 baton for status tracking sent to coins 1of2 addr
		mtx.vout.push_back(MakeCC1vout(EVAL_EXCHANGES, CC_MARKER_VALUE, tokensupplier)); // vout.1 marker to tokensupplier
		mtx.vout.push_back(MakeCC1vout(EVAL_EXCHANGES, CC_MARKER_VALUE, coinsupplier)); // vout.2 marker to coinsupplier

		return (FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodeExchangeOpenOpRet(EXCHANGECC_VERSION,tokensupplier,coinsupplier,exchangetype,tokenid,numtokens,numcoins,agreementtxid)));
	}
	CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Error adding normal inputs");
}

UniValue ExchangeFund(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid, int64_t amount, bool useTokens)
{
	char addr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk, tokensupplier, coinsupplier;
	CTransaction exchangetx;
	int32_t numvouts;
	int64_t numtokens, numcoins, coinbalance, tokenbalance, tokens = 0, inputs = 0;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	struct CCcontract_info *cp, C, *cpTokens, CTokens;
	uint256 hashBlock, tokenid, agreementtxid, borrowtxid = zeroid, latesttxid;
	uint8_t version, exchangetype, lastfuncid;
	
	cp = CCinit(&C, EVAL_EXCHANGES);
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (txfee == 0)
	{
		txfee = CC_TXFEE;
	}	
	
	if (amount < 1)
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Funding amount must be above 0");
	
	if (exchangetxid != zeroid)
	{
		if (myGetTransaction(exchangetxid, exchangetx, hashBlock) == 0 || (numvouts = exchangetx.vout.size()) <= 0)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Can't find specified exchange txid " << exchangetxid.GetHex());
		
		if (!ValidateExchangeOpenTx(exchangetx,CCerror))
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << CCerror);
		
		if (!GetLatestExchangeTxid(exchangetxid, latesttxid, lastfuncid) || lastfuncid == 'c' || lastfuncid == 's' || lastfuncid == 'p' || lastfuncid == 'r')
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Exchange " << exchangetxid.GetHex() << " closed");
		
		if (!FindExchangeTxidType(exchangetxid, 'b', borrowtxid))
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Exchange borrow transaction search failed, quitting");
		
		DecodeExchangeOpenOpRet(exchangetx.vout[numvouts - 1].scriptPubKey,version,tokensupplier,coinsupplier,exchangetype,tokenid,numtokens,numcoins,agreementtxid);
		
		if (useTokens && mypk != tokensupplier)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Tokens can only be sent by token supplier pubkey");
		
		if (!useTokens && (mypk != coinsupplier || (borrowtxid != zeroid && mypk != tokensupplier)))
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Coins can only be sent by coin supplier pubkey, or token supplier if loan is in progress");
		
		coinbalance = GetExchangesInputs(cp,exchangetx,EIF_COINS,unspentOutputs);
		tokenbalance = GetExchangesInputs(cp,exchangetx,EIF_TOKENS,unspentOutputs);
		
		if (useTokens)
		{
			if (tokenbalance >= numtokens)
				CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Exchange already has enough tokens");
			else if (tokenbalance + amount > numtokens)
				CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Specified token amount is higher than needed to fill exchange");
		}
		else
		{
			if (coinbalance >= numcoins)
				CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Exchange already has enough coins");
			else if (coinbalance + amount > numcoins)
				CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Specified coin amount is higher than needed to fill exchange");
		}
	}
	else
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Invalid exchangetxid");
	
	if (useTokens)
	{
		inputs = AddNormalinputs(mtx, mypk, txfee, 5, pk.IsValid());
		tokens = AddTokenCCInputs(cpTokens, mtx, mypk, tokenid, amount, 64); 
		if (tokens < amount)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "couldn't find enough tokens for specified amount");
	}
	else
	{
		tokenid = zeroid;
		inputs = AddNormalinputs(mtx, mypk, txfee + amount, 64, pk.IsValid());
	}
	
	if (inputs + tokens >= amount + txfee)
	{
		if (useTokens)
		{
			mtx.vout.push_back(MakeTokensCC1of2vout(EVAL_EXCHANGES, amount, tokensupplier, coinsupplier, NULL));
			// MakeTokensCC1of2vout(uint8_t evalcode, CAmount nValue, CPubKey pk1, CPubKey pk2, std::vector<std::vector<unsigned char>>* vData)
		}
		else
		{
			mtx.vout.push_back(MakeCC1of2vout(EVAL_EXCHANGES, amount, tokensupplier, coinsupplier));
		}
		if (useTokens && tokens > amount)
		{
			mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, tokens - amount, mypk));
		}
		return (FinalizeCCTxExt(pk.IsValid(), 0, cp, mtx, mypk, txfee, EncodeExchangeOpRet('f', EXCHANGECC_VERSION, exchangetxid, tokenid, tokensupplier, coinsupplier))); 
	}
	else
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "error adding funds");
}

UniValue ExchangeLoanTerms(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid, int64_t interest, int64_t duedate)
{
	CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "incomplete");
}

UniValue ExchangeCancel(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid)
{
	char exchangeaddr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk, tokensupplier, coinsupplier;
	CTransaction exchangetx;
	int32_t numvouts;
	int64_t numtokens, numcoins, coinbalance, tokenbalance, tokens = 0, coins = 0, inputs = 0;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	struct CCcontract_info *cp, C, *cpTokens, CTokens;
	uint256 hashBlock, tokenid, agreementtxid, latesttxid, borrowtxid;
	uint8_t version, exchangetype, mypriv[32], lastfuncid;
	
	cp = CCinit(&C, EVAL_EXCHANGES);
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (txfee == 0)
	{
		txfee = CC_TXFEE;
	}
	
	/*"Cancel" or "c":
		Data constraints:
			(FindExchangeTxidType) if exchange contains borrow tx, can only be executed by coin provider pk
			(CheckDepositUnlockCond) If exchange contains no borrow tx, has an assoc. agreement and finalsettlement = true, the deposit must NOT be sent to the exchange's coin escrow
			(GetExchangesInputs) If exchange is trade type, token & coin escrow cannot be fully filled (tokens >= numtokens && coins >= numcoins).
		TX constraints:
			(Pubkey check) must be executed by token or coin provider pk
			(Tokens dest addr) all tokens must be sent to token provider
			(Coins dest addr) all coins must be sent to coin provider*/
	
	if (exchangetxid != zeroid)
	{
		if (myGetTransaction(exchangetxid, exchangetx, hashBlock) == 0 || (numvouts = exchangetx.vout.size()) <= 0)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Can't find specified exchange txid " << exchangetxid.GetHex());
		
		if (!ValidateExchangeOpenTx(exchangetx,CCerror))
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << CCerror);
		
		if (!GetLatestExchangeTxid(exchangetxid, latesttxid, lastfuncid) || lastfuncid == 'c' || lastfuncid == 's' || lastfuncid == 'p' || lastfuncid == 'r')
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Exchange " << exchangetxid.GetHex() << " closed");
		
		if (!FindExchangeTxidType(exchangetxid, 'b', borrowtxid))
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Exchange borrow transaction search failed, quitting");
		else if (borrowtxid != zeroid)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Cannot cancel when loan is in progress");
		
		DecodeExchangeOpenOpRet(exchangetx.vout[numvouts - 1].scriptPubKey,version,tokensupplier,coinsupplier,exchangetype,tokenid,numtokens,numcoins,agreementtxid);
		
		if (mypk != tokensupplier && mypk != coinsupplier)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "You are not a valid party for exchange " << exchangetxid.GetHex());
		
		// If exchange has an associated agreement and deposit spending enabled, the deposit must NOT be sent to the exchange's coin escrow
		if (CheckDepositUnlockCond(exchangetxid) > 0)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Cannot cancel exchange if its associated agreement has deposit unlocked");
		
		coinbalance = GetExchangesInputs(cp,exchangetx,EIF_COINS,unspentOutputs);
		tokenbalance = GetExchangesInputs(cp,exchangetx,EIF_TOKENS,unspentOutputs);
		
		if (exchangetype & EXTF_TRADE && coinbalance >= numcoins && tokenbalance >= numtokens)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Cannot cancel trade when escrow has enough coins and tokens");
	}
	else
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Invalid exchangetxid");
	
	inputs = AddNormalinputs(mtx, mypk, txfee, 5, pk.IsValid()); // txfee
	coins = AddExchangesInputs(cp, mtx, exchangetx, EIF_COINS, 60); // coin from CC 1of2 addr vins
	tokens = AddExchangesInputs(cp, mtx, exchangetx, EIF_TOKENS, 60); // token from CC 1of2 addr vins
	
	std::cerr << "coins: " << coins << std::endl;
	std::cerr << "tokens: " << tokens << std::endl;
	
	if (inputs >= txfee)
	{
		GetCCaddress1of2(cp, exchangeaddr, tokensupplier, coinsupplier);
		mtx.vin.push_back(CTxIn(exchangetxid,0,CScript())); // previous CC 1of2 baton vin
		Myprivkey(mypriv);
		CCaddr1of2set(cp, tokensupplier, coinsupplier, mypriv, exchangeaddr);
		if (coins > 0)
		{
			mtx.vout.push_back(CTxOut(coins, CScript() << ParseHex(HexStr(coinsupplier)) << OP_CHECKSIG)); // coins refund vout
		}
		if (tokens > 0)
		{
			mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, tokens, tokensupplier)); // tokens refund vout
		}
		return (FinalizeCCTxExt(pk.IsValid(), 0, cp, mtx, mypk, txfee, EncodeExchangeOpRet('c', EXCHANGECC_VERSION, exchangetxid, tokenid, tokensupplier, coinsupplier))); 
	}
	else
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "error adding inputs for txfee");
}

UniValue ExchangeBorrow(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid, uint256 loantermstxid)
{
	CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "incomplete");
	/*"Borrow" or "b":
		Data constraints:
			(Type check) Exchange must be loan type
			(FindExchangeTxidType) exchange must not contain a borrow transaction
			(Signing check) at least one coin fund vin must be signed by coin provider pk
			(CheckDepositUnlockCond) If exchange has an assoc. agreement and finalsettlement = true, the deposit must be sent to the exchange's coin escrow
			(GetExchangesInputs) Token escrow must contain >= numtokens, and coin escrow must contain >= numcoins
		TX constraints:
			(Pubkey check) must be executed by token provider pk
			(Tokens dest addr) no tokens should be moved anywhere
			(Coins dest addr) all coins must be sent to token provider. if there are more coins than numcoins, return remaining coins to coin provider*/
}
UniValue ExchangeRepo(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid)
{
	CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "incomplete");
	/*"Repo" or "p":
		Data constraints:
			(Type check) Exchange must be loan type
			(FindExchangeTxidType) exchange must contain a borrow transaction
			(Duration) duration since borrow transaction must exceed due date from latest loan terms
			(GetExchangesInputs) Token escrow must contain >= numtokens, coin escrow must not contain >= numcoins + interest from latest loan terms
		TX constraints:
			(Pubkey check) must be executed by coin provider pk
			(Tokens dest addr) all tokens must be sent to coin provider. if (somehow) there are more tokens than numtokens, return remaining tokens to token provider
			(Coins dest addr) all coins must be sent to token provider.*/
}
UniValue ExchangeClose(const CPubKey& pk, uint64_t txfee, uint256 exchangetxid)
{
	char exchangeaddr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk, tokensupplier, coinsupplier;
	CTransaction exchangetx;
	int32_t numvouts;
	int64_t numtokens, numcoins, coinbalance, tokenbalance, tokens = 0, coins = 0, inputs = 0;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	struct CCcontract_info *cp, C, *cpTokens, CTokens;
	uint256 hashBlock, tokenid, agreementtxid, latesttxid, loantermstxid, borrowtxid;
	uint8_t version, exchangetype, mypriv[32], lastfuncid;
	
	cp = CCinit(&C, EVAL_EXCHANGES);
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (txfee == 0)
	{
		txfee = CC_TXFEE;
	}
	
	if (exchangetxid != zeroid)
	{
		if (myGetTransaction(exchangetxid, exchangetx, hashBlock) == 0 || (numvouts = exchangetx.vout.size()) <= 0)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Can't find specified exchange txid " << exchangetxid.GetHex());
		
		if (!ValidateExchangeOpenTx(exchangetx,CCerror))
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << CCerror);
		
		if (!GetLatestExchangeTxid(exchangetxid, latesttxid, lastfuncid) || lastfuncid == 'c' || lastfuncid == 's' || lastfuncid == 'p' || lastfuncid == 'r')
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Exchange " << exchangetxid.GetHex() << " closed");
		
		// checking for borrow transaction
		if (!FindExchangeTxidType(exchangetxid, 'b', borrowtxid))
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Exchange borrow transaction search failed, quitting");
		
		// checking for latest loan terms transaction
		if (!FindExchangeTxidType(exchangetxid, 'l', loantermstxid))
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Exchange loan terms transaction search failed, quitting");
		
		DecodeExchangeOpenOpRet(exchangetx.vout[numvouts - 1].scriptPubKey,version,tokensupplier,coinsupplier,exchangetype,tokenid,numtokens,numcoins,agreementtxid);
		
		if (mypk != tokensupplier && mypk != coinsupplier)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "You are not a valid party for exchange " << exchangetxid.GetHex());
		
		if (CheckDepositUnlockCond(exchangetxid) == 0)
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Deposit from agreement " << agreementtxid.GetHex() << " must be unlocked first for exchange " << exchangetxid.GetHex());
		
		coinbalance = GetExchangesInputs(cp,exchangetx,EIF_COINS,unspentOutputs);
		tokenbalance = GetExchangesInputs(cp,exchangetx,EIF_TOKENS,unspentOutputs);
		
		if (exchangetype & EXTF_TRADE)
		{
			if (borrowtxid != zeroid)
				CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Found loan borrow transaction in trade type exchange");
			
			if (coinbalance < numcoins || tokenbalance < numtokens)
				CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Cannot close trade when escrow doesn't have enough coins and tokens");
			
			// creating swap ('s') transaction
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "'s'");
		}
		else if (exchangetype & EXTF_LOAN)
		{
			if (borrowtxid == zeroid || loantermstxid == zeroid)
				CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Loan has not been initiated yet, cannot close");
			
			// get data from loan terms txid
			
			if (tokenbalance < numtokens)
				CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Not enough tokens found in loan escrow");
			
			if (coinbalance < numcoins/*<- TODO: change to prepayment/interest/w/e*/)
				CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Cannot close loan when escrow doesn't have enough coins");
			
			// creating release ('r') transaction
			// not implemented yet
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "not implemented yet");
		}
		else
			CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Unsupported exchange type");
	}
	else
		CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "Invalid exchangetxid");
	
	/*"Swap" or "s" (part of exchangeclose rpc):
		Data constraints:
			(CheckDepositUnlockCond) If exchange has an assoc. agreement and finalsettlement = true, the deposit must be sent to the exchange's coin escrow
			(GetExchangesInputs) Token escrow must contain >= numtokens, and coin escrow must contain >= numcoins
			
		TX constraints:
			(Pubkey check) must be executed by token or coin provider pk
			(Tokens dest addr) all tokens must be sent to coin provider. if (somehow) there are more tokens than numtokens, return remaining tokens to token provider
			(Coins dest addr) all coins must be sent to token provider. if there are more coins than numcoins, return remaining coins to coin provider*/
			
	/*"Release" or "r" (part of exchangeclose rpc):
		Data constraints:
			(FindExchangeTxidType) exchange must contain a borrow transaction
			(GetExchangesInputs) Token escrow must contain >= numtokens, coin escrow must contain >= numcoins + interest from latest loan terms
		TX constraints:
			(Pubkey check) must be executed by token or coin provider pk
			(Tokens dest addr) all tokens must be sent to token provider
			(Coins dest addr) all coins must be sent to coin provider. if there are more coins than numcoins, return remaining coins to token provider*/
}

//===========================================================================
// RPCs - informational
//===========================================================================
UniValue ExchangeInfo(const CPubKey& pk, uint256 exchangetxid)
{
	UniValue result(UniValue::VOBJ);
	CTransaction tx;
	uint256 hashBlock, tokenid, agreementtxid;
	int32_t numvouts;
	int64_t numtokens, numcoins;
	uint8_t version, funcid, exchangetype;
	CPubKey mypk, tokensupplier, coinsupplier;
	char str[67];
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs, unspentTokenOutputs;
	struct CCcontract_info *cp, C;
	
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	if (myGetTransaction(exchangetxid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
	(funcid = DecodeExchangeOpenOpRet(tx.vout[numvouts-1].scriptPubKey, version, tokensupplier, coinsupplier, exchangetype, tokenid, numtokens, numcoins, agreementtxid)) == 'o')
	{
		result.push_back(Pair("result", "success"));
		result.push_back(Pair("exchangetxid", exchangetxid.GetHex()));
		
		result.push_back(Pair("token_supplier", pubkey33_str(str,(uint8_t *)&tokensupplier)));
		result.push_back(Pair("coin_supplier", pubkey33_str(str,(uint8_t *)&coinsupplier)));
		
		if (exchangetype & EXTF_TRADE)
			result.push_back(Pair("exchange_type", "trade"));
		else if (exchangetype & EXTF_LOAN)
			result.push_back(Pair("exchange_type", "loan"));
		
		result.push_back(Pair("tokenid", tokenid.GetHex()));
		result.push_back(Pair("required_tokens", numtokens));
		result.push_back(Pair("required_coins", numcoins));

		cp = CCinit(&C, EVAL_EXCHANGES);
		
		//std::cerr << "looking for tokens" << std::endl;
		result.push_back(Pair("token_balance", GetExchangesInputs(cp,tx,EIF_TOKENS,unspentTokenOutputs)));
		//std::cerr << "looking for coins" << std::endl;
		result.push_back(Pair("coin_balance", GetExchangesInputs(cp,tx,EIF_COINS,unspentOutputs)));

		
		/*
		List of info needed:
		exchangetype: trade (ask, bid), loan (pawn or lend)
		parties
		current coin fill
		current token fill
		loan
			latest loan param txid
			latest interest
			latest due date
			due date type
		borrow mode
			time left (either block height or in seconds)
			
		Statuses:
		Open
		Waiting for deposit
		Waiting for loan terms
		Waiting for borrower
		Loan in progress
		Loan default
		Closed / terminated

		Things to check in info:
		What type is the exchange? (trade/loan)
		Is the exchange connected to an agreement and its deposit? (agreementtxid/finalsettlement)
		Is the exchange open/succesful/failed? (.../closed/terminated/repo'd?)
		Have the loan parameters been defined? What are the latest? (waiting for loan terms/...)
		Is the deposit spent(agreements)? (waiting for deposit/...)
		Did the borrower take the loan? (waiting for borrower/...)
		Has the latest deadline been passed? (loan default/loan in progress)
			
		*/
		
		if (agreementtxid != zeroid)
		{
			result.push_back(Pair("agreement_txid", agreementtxid.GetHex()));
			if (exchangetype & EXTF_DEPOSITUNLOCKABLE)
				result.push_back(Pair("deposit_unlock_enabled", "true"));
			else
				result.push_back(Pair("deposit_unlock_enabled", "false"));
		}
		return(result);
	}
	CCERR_RESULT("exchangescc", CCLOG_INFO, stream << "invalid exchange creation txid");
}

UniValue ExchangeList(const CPubKey& pk)
{
	UniValue result(UniValue::VARR);
	std::vector<uint256> txids;
	struct CCcontract_info *cp, C;
	uint256 txid, dummytxid, hashBlock;
	CTransaction tx;
	char myCCaddr[65];
	int32_t numvouts;
	CPubKey mypk;
	uint8_t version;
	
	cp = CCinit(&C, EVAL_EXCHANGES);
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	GetCCaddress(cp,myCCaddr,mypk);
	SetCCtxids(txids,myCCaddr,true,EVAL_EXCHANGES,CC_MARKER_VALUE,zeroid,'o');
	
	for (std::vector<uint256>::const_iterator it=txids.begin(); it!=txids.end(); it++)
	{
		txid = *it;
		if ( myGetTransaction(txid,tx,hashBlock) != 0 && (numvouts= tx.vout.size()) > 0 )
		{
			if (DecodeExchangeOpRet(tx.vout[numvouts-1].scriptPubKey, version, dummytxid, dummytxid) == 'o')
			{				
				result.push_back(txid.GetHex());
			}
		}
	}
	return (result);
}
