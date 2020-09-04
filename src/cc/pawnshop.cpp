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

#include "CCPawnshop.h"
#include "CCagreements.h"
#include "CCtokens.h"

//===========================================================================
// Opret encoding/decoding functions
//===========================================================================

CScript EncodePawnshopCreateOpRet(uint8_t version,std::string name,CPubKey tokensupplier,CPubKey coinsupplier,uint32_t pawnshopflags,uint256 tokenid,int64_t numtokens,int64_t numcoins,uint256 agreementtxid)
{
	CScript opret; uint8_t evalcode = EVAL_PAWNSHOP, funcid = 'c';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << name << tokensupplier << coinsupplier << pawnshopflags << tokenid << numtokens << numcoins << agreementtxid);
	return(opret);
}
uint8_t DecodePawnshopCreateOpRet(CScript scriptPubKey,uint8_t &version,std::string &name,CPubKey &tokensupplier,CPubKey &coinsupplier,uint32_t &pawnshopflags,uint256 &tokenid,int64_t &numtokens,int64_t &numcoins,uint256 &agreementtxid)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> name; ss >> tokensupplier; ss >> coinsupplier; ss >> pawnshopflags; ss >> tokenid; ss >> numtokens; ss >> numcoins; ss >> agreementtxid) != 0 && evalcode == EVAL_PAWNSHOP)
		return(funcid);
	return(0);
}

///// not implemented yet /////
/*CScript EncodePawnshopScheduleOpRet(uint8_t version,uint256 createtxid,int64_t principal,uint64_t duedate,bool bRelative)
{
	CScript opret; uint8_t evalcode = EVAL_PAWNSHOP, funcid = 't';
	opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << version << principal << duedate << bRelative);
	return(opret);
}
uint8_t DecodePawnshopScheduleOpRet(CScript scriptPubKey,uint8_t &version,uint256 &createtxid,int64_t &principal,uint64_t &duedate,bool &bRelative)
{
	std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
	GetOpReturnData(scriptPubKey, vopret);
	if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> createtxid; ss >> principal; ss >> duedate; ss >> bRelative) != 0 && evalcode == EVAL_PAWNSHOP)
		return(funcid);
	return(0);
}*/
///// not implemented yet /////

CScript EncodePawnshopOpRet(uint8_t funcid,uint8_t version,uint256 createtxid,uint256 tokenid,CPubKey tokensupplier,CPubKey coinsupplier)
{
	CScript opret;
	uint8_t evalcode = EVAL_PAWNSHOP;
	vscript_t vopret;
	vopret = E_MARSHAL(ss << evalcode << funcid << version << createtxid);
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
uint8_t DecodePawnshopOpRet(const CScript scriptPubKey,uint8_t &version,uint256 &createtxid,uint256 &tokenid)
{
	std::vector<vscript_t> oprets;
	std::vector<uint8_t> vopret, vOpretExtra;
	uint8_t *script, evalcode, funcid;
	uint32_t pawnshopflags;
	std::vector<CPubKey> pubkeys;
	int64_t dummyamount;
	uint64_t dummyamount2;
	CPubKey dummypk;
	std::string name;
	uint256 dummytxid;
	bool dummybool;
	
	createtxid = tokenid = zeroid;
	
	if (DecodeTokenOpRetV1(scriptPubKey,tokenid,pubkeys,oprets) != 0 && GetOpReturnCCBlob(oprets, vOpretExtra) && vOpretExtra.size() > 0)
    {
		//std::cerr << "token opret" << std::endl;
        vopret = vOpretExtra;
    }
	else
	{
		//std::cerr << "non-token opret" << std::endl;
		GetOpReturnData(scriptPubKey, vopret);
	}
	
	script = (uint8_t *)vopret.data();
    if (script != NULL && vopret.size() > 2)
    {
		if (script[0] != EVAL_PAWNSHOP)
			return(0);

        funcid = script[1];
        switch (funcid)
        {
			case 'c':
				return DecodePawnshopCreateOpRet(scriptPubKey,version,name,dummypk,dummypk,pawnshopflags,dummytxid,dummyamount,dummyamount,dummytxid);
			//case 't':
			//	return DecodePawnshopScheduleOpRet(scriptPubKey,version,createtxid,dummyamount,dummyamount2,dummybool);
			default:
				if (E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> version; ss >> createtxid) != 0 && evalcode == EVAL_PAWNSHOP)
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

bool PawnshopValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	CPubKey tokensupplier, coinsupplier;
	CTransaction createtx;
	int32_t numvins, numvouts;
	int64_t numtokens, numcoins, coininputs, tokeninputs, coinoutputs, tokenoutputs, coinbalance, tokenbalance;
	std::string CCerror, name;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	uint256 hashBlock, createtxid, borrowtxid, agreementtxid, latesttxid, tokenid;
	uint8_t funcid, version, lastfuncid;
	uint32_t pawnshopflags;
	struct CCcontract_info *cpTokens, CTokens;
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	char tokenpk_coinaddr[65], tokenpk_tokenaddr[65], coinpk_coinaddr[65], coinpk_tokenaddr[65];
	
	numvins = tx.vin.size();
	numvouts = tx.vout.size();
	
	if (numvouts < 1)
		return eval->Invalid("no vouts");
	
	CCOpretCheck(eval,tx,true,true,true);
	ExactAmounts(eval,tx,ASSETCHAINS_CCZEROTXFEE[EVAL_PAWNSHOP]?0:CC_TXFEE);
	
	if ((funcid = DecodePawnshopOpRet(tx.vout[numvouts-1].scriptPubKey, version, createtxid, tokenid)) != 0)
	{
		if (!(myGetTransaction(createtxid,createtx,hashBlock) != 0 && createtx.vout.size() > 0 &&
		DecodePawnshopCreateOpRet(createtx.vout[createtx.vout.size()-1].scriptPubKey,version,name,tokensupplier,coinsupplier,pawnshopflags,tokenid,numtokens,numcoins,agreementtxid) == 'c'))
			return eval->Invalid("cannot find pawnshopcreate tx for PawnshopValidate!");
			
		if (funcid != 'x' && !ValidatePawnshopCreateTx(createtx, CCerror))
			return eval->Invalid(CCerror);
		
		if (PawnshopExactAmounts(cp,eval,tx,coininputs,tokeninputs) == false)
		{
			return (false);
		}
	
		switch (funcid)
		{
			case 'c':
				// pawnshop create:
				// vin.0 normal input
				// vout.0 CC baton to mutual 1of2 address
				// vout.1 CC marker to tokensupplier
				// vout.2 CC marker to coinsupplier
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_PAWNSHOP ‘c’ version tokensupplier coinsupplier pawnshopflags tokenid numtokens numcoins agreementtxid
				return eval->Invalid("unexpected PawnshopValidate for pawnshopcreate!");
			
			case 'f':
				// pawnshop fund:
				// vin.0 normal input
				// vin.1 .. vin.n-1: normal CC inputs
				// vout.0 normal payment to coin CC 1of2 addr
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_PAWNSHOP ‘f’ version createtxid tokenid tokensupplier coinsupplier
				return eval->Invalid("unexpected PawnshopValidate for pawnshopfund!");
			
			case 'p':
				// pawnshop pledge:
				// vin.0 normal input
				// vin.1 .. vin.n-1: token CC inputs
				// vout.0 token CC payment to token CC 1of2 addr
				// vout.1 token change (if any)
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_PAWNSHOP ‘p’ version createtxid tokenid tokensupplier coinsupplier
				return eval->Invalid("unexpected PawnshopValidate for pawnshoppledge!");
			
			case 't':
				// pawnshop schedule:
				// vin.0 normal input
				// vin.1 previous baton CC input
				// vout.0 next baton CC output
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_PAWNSHOP ‘t’ createtxid principal duedate
				return eval->Invalid("schedule tx not implemented yet!");

			case 'x':
				// pawnshop cancel:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs (if any)
				// vout.0 normal output to coinsupplier
				// vout.1 token CC output to tokensupplier
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_PAWNSHOP ‘x’ version createtxid tokenid tokensupplier coinsupplier
				
				// check if sent by token or coin provider pk
				if (TotalPubkeyCCInputs(tx, tokensupplier) == 0 && TotalPubkeyCCInputs(tx, coinsupplier) == 0)
					return eval->Invalid("found no cc inputs signed by any pawnshop member pubkey!");
				// get required addresses
				GetCCaddress(cpTokens, tokenpk_tokenaddr, tokensupplier);
				Getscriptaddress(coinpk_coinaddr, CScript() << ParseHex(HexStr(coinsupplier)) << OP_CHECKSIG);
				// check if pawnshop is closed
				if (!GetLatestPawnshopTxid(createtxid, latesttxid, lastfuncid) || lastfuncid == 'x' || lastfuncid == 'e' || lastfuncid == 's' || lastfuncid == 'r')
					return eval->Invalid("createtxid specified in tx is closed!");
				
				/*// check if borrow transaction exists
				if (!FindPawnshopTxidType(createtxid, 'b', borrowtxid))
					return eval->Invalid("borrow tx search failed!");
				if (borrowtxid != zeroid && TotalPubkeyCCInputs(tx, coinsupplier) == 0)
					return eval->Invalid("'x' tx after loan borrow tx must be signed by coin supplier pk!");*/
				
				// check deposit unlock condition
				if (pawnshopflags & PTF_REQUIREUNLOCK && CheckDepositUnlockCond(createtxid) >= 0)
					return eval->Invalid("defined deposit must not be unlocked for 'x' tx!");
				// get coinbalance and tokenbalance, check if both are sufficient
				coinbalance = GetPawnshopInputs(cp,createtx,PIF_COINS,unspentOutputs);
				tokenbalance = GetPawnshopInputs(cp,createtx,PIF_TOKENS,unspentOutputs);
				if (coinbalance >= numcoins && tokenbalance >= numtokens)
					return eval->Invalid("cannot cancel trade when escrow has enough coins and tokens!");
				// check vouts
				if (numvouts < 1)
					return eval->Invalid("not enough vouts!");
				// additional checks for different vout structures
				if (coininputs > 0 && tokeninputs > 0)
				{
					if (numvouts < 3)
						return eval->Invalid("not enough vouts!");
					else if (ConstrainVout(tx.vout[0], 0, coinpk_coinaddr, coininputs) == 0)
						return eval->Invalid("vout.0 must be normal output to coinsupplier coin address!");
					else if (ConstrainVout(tx.vout[1], 1, tokenpk_tokenaddr, tokeninputs) == 0 || 
						IsTokensvout(false, true, cpTokens, eval, tx, 1, tokenid) != tokeninputs)
						return eval->Invalid("vout.1 must be CC token output to tokensupplier token address!");
				}
				else if (coininputs > 0 && tokeninputs == 0)
				{
					if (numvouts < 2)
						return eval->Invalid("not enough vouts!");
					else if (ConstrainVout(tx.vout[0], 0, coinpk_coinaddr, coininputs) == 0)
						return eval->Invalid("vout.0 must be normal output to coinsupplier coin address!");
				}
				else if (coininputs == 0 && tokeninputs > 0)
				{
					if (numvouts < 2)
						return eval->Invalid("not enough vouts!");
					else if (ConstrainVout(tx.vout[0], 1, tokenpk_tokenaddr, tokeninputs) == 0 || 
						IsTokensvout(false, true, cpTokens, eval, tx, 0, tokenid) != tokeninputs)
						return eval->Invalid("vout.0 must be CC token output to tokensupplier token address!");
				}
				// check vins
				if (numvins < 2)
					return eval->Invalid("not enough vins!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal input!");
				else if ((*cp->ismyvin)(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC input!");
				else if (tx.vin[1].prevout.hash != latesttxid || tx.vin[1].prevout.n != 0)
					return eval->Invalid("vin.1 tx has invalid prevout data!");
				else if (numvins > 2 && (coininputs != coinbalance || tokeninputs != tokenbalance))
					return eval->Invalid("tx coin/token inputs do not match coin/token balance!");
				break;
			case 'e':
				// pawnshop exchange:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs
				// vout.0 normal output to tokensupplier
				// vout.1 token CC output to coinsupplier
				// vout.2 normal change from CC 1of2 addr to coinsupplier (if any)
				// vout.3 token CC change from CC 1of2 addr to tokensupplier (if any)
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_PAWNSHOP ‘e’ version createtxid tokenid tokensupplier coinsupplier
				
				// check if sent by token or coin provider pk
				if (TotalPubkeyCCInputs(tx, tokensupplier) == 0 && TotalPubkeyCCInputs(tx, coinsupplier) == 0)
					return eval->Invalid("found no cc inputs signed by any pawnshop member pubkey!");
				// get required addresses
				GetCCaddress(cpTokens, tokenpk_tokenaddr, tokensupplier);
				GetCCaddress(cpTokens, coinpk_tokenaddr, coinsupplier);
				Getscriptaddress(tokenpk_coinaddr, CScript() << ParseHex(HexStr(tokensupplier)) << OP_CHECKSIG);
				Getscriptaddress(coinpk_coinaddr, CScript() << ParseHex(HexStr(coinsupplier)) << OP_CHECKSIG);
				// check if pawnshop is closed
				if (!GetLatestPawnshopTxid(createtxid, latesttxid, lastfuncid) || lastfuncid == 'x' || lastfuncid == 'e' || lastfuncid == 's' || lastfuncid == 'r')
					return eval->Invalid("createtxid specified in tx is closed!");
				// check deposit unlock condition
				if (pawnshopflags & PTF_REQUIREUNLOCK && CheckDepositUnlockCond(createtxid) < 0)
					return eval->Invalid("defined deposit must be unlocked for 'e' tx!");
				// get coinbalance and tokenbalance, check if both are sufficient
				coinbalance = GetPawnshopInputs(cp,createtx,PIF_COINS,unspentOutputs);
				tokenbalance = GetPawnshopInputs(cp,createtx,PIF_TOKENS,unspentOutputs);
				if (coinbalance < numcoins || tokenbalance < numtokens)
					return eval->Invalid("not enough coins and tokens for 'e' tx!");
				// check vouts
				if (numvouts < 3)
					return eval->Invalid("not enough vouts!");
				else if (ConstrainVout(tx.vout[0], 0, tokenpk_coinaddr, numcoins) == 0)
					return eval->Invalid("vout.0 must be normal output to tokensupplier coin address!");
				else if (ConstrainVout(tx.vout[1], 1, coinpk_tokenaddr, numtokens) == 0 || IsTokensvout(false, true, cpTokens, eval, tx, 1, tokenid) != numtokens)
					return eval->Invalid("vout.1 must be CC token output to coinsupplier token address!");
				// additional checks for when coin and/or token escrow are overfilled somehow
				if (coinbalance - numcoins > 0 && tokenbalance - numtokens > 0)
				{
					if (numvouts < 5)
						return eval->Invalid("not enough vouts!");
					else if (ConstrainVout(tx.vout[2], 0, coinpk_coinaddr, coinbalance - numcoins) == 0)
						return eval->Invalid("vout.2 must be normal output to coinsupplier coin address!");
					else if (ConstrainVout(tx.vout[3], 1, tokenpk_tokenaddr, tokenbalance - numtokens) == 0 || 
						IsTokensvout(false, true, cpTokens, eval, tx, 3, tokenid) != tokenbalance - numtokens)
						return eval->Invalid("vout.3 must be CC token output to tokensupplier token address!");
				}
				else if (coinbalance - numcoins > 0 && tokenbalance - numtokens == 0)
				{
					if (numvouts < 4)
						return eval->Invalid("not enough vouts!");
					else if (ConstrainVout(tx.vout[2], 0, coinpk_coinaddr, coinbalance - numcoins) == 0)
						return eval->Invalid("vout.2 must be normal output to coinsupplier coin address!");
				}
				else if (coinbalance - numcoins == 0 && tokenbalance - numtokens > 0)
				{
					if (numvouts < 4)
						return eval->Invalid("not enough vouts!");
					else if (ConstrainVout(tx.vout[2], 1, tokenpk_tokenaddr, tokenbalance - numtokens) == 0 || 
						IsTokensvout(false, true, cpTokens, eval, tx, 2, tokenid) != tokenbalance - numtokens)
						return eval->Invalid("vout.2 must be CC token output to tokensupplier token address!");
				}
				// check vins
				if (numvins < 3)
					return eval->Invalid("not enough vins!");
				else if (IsCCInput(tx.vin[0].scriptSig) != 0)
					return eval->Invalid("vin.0 must be normal input!");
				else if ((*cp->ismyvin)(tx.vin[1].scriptSig) == 0)
					return eval->Invalid("vin.1 must be CC input!");
				else if (tx.vin[1].prevout.hash != latesttxid || tx.vin[1].prevout.n != 0)
					return eval->Invalid("vin.1 tx has invalid prevout data!");
				else if (coininputs != coinbalance || tokeninputs != tokenbalance)
					return eval->Invalid("tx coin/token inputs do not match coin/token balance!");
				break;
				
			case 'b':
				// pawnshop borrow:
				// vin.0 normal input
				// vin.1 previous baton CC input
				// vin.2 .. vin.n-1: valid CC coin outputs (no tokens)
				// vout.0 next baton CC output
				// vout.1 normal change from CC 1of2 addr to coinsupplier (if any)
				// vout.n-2 coins to tokensupplier & normal change
				// vout.n-1 OP_RETURN EVAL_PAWNSHOP ‘b’ createtxid
				return eval->Invalid("borrow tx not implemented yet!");
				
			case 's':
				// pawnshop seize:
				// vin.0 normal input
				// vin.1 previous CC 1of2 baton
				// vin.2 .. vin.n-1: valid CC coin or token outputs
				// vout.0 normal output to tokensupplier
				// vout.1 token CC output to coinsupplier
				// vout.2 token CC change from CC 1of2 addr to tokensupplier (if any)
				// vout.n-2 normal change (if any)
				// vout.n-1 OP_RETURN EVAL_PAWNSHOP ‘s’ version createtxid tokenid tokensupplier coinsupplier
				return eval->Invalid("seize tx not implemented yet!");
			
			case 'r':
				// pawnshop redeem:
				// tx structure design TBD
				return eval->Invalid("redeem tx not implemented yet!");
			
			default:
				fprintf(stderr,"unexpected pawnshop funcid (%c)\n",funcid);
				return eval->Invalid("unexpected pawnshop funcid!");
		}
	}
	else
		return eval->Invalid("must be valid pawnshop funcid!");
	
	LOGSTREAM("pawnshop", CCLOG_INFO, stream << "Pawnshop tx validated" << std::endl);
	std::cerr << "Pawnshop tx validated" << std::endl;
	
	return true;
}

//===========================================================================
// Helper functions
//===========================================================================

int64_t IsPawnshopvout(struct CCcontract_info *cp,const CTransaction& tx,bool mode,CPubKey tokensupplier,CPubKey coinsupplier,int32_t v)
{
	char destaddr[65],pawnshopaddr[65];
	if (mode == PIF_TOKENS)
	{
		GetTokensCCaddress1of2(cp,pawnshopaddr,tokensupplier,coinsupplier);
	}
	else
	{
		GetCCaddress1of2(cp,pawnshopaddr,tokensupplier,coinsupplier);
	}
	if (tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0)
	{
		if (Getscriptaddress(destaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(destaddr,pawnshopaddr) == 0)
			return(tx.vout[v].nValue);
	}
	return(0);
}

bool PawnshopExactAmounts(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, int64_t &coininputs, int64_t &tokeninputs)
{
	uint256 hashBlock, createtxid, refcreatetxid, tokenid, dummytxid;
	int32_t i, numvins, numvouts;
	int64_t nValue, dummyamount, outputs = 0;
	uint8_t dummychar, funcid, version;
	uint32_t pawnshopflags;
	CTransaction vinTx, createtx;
	char destaddr[65], tokenpk_coinaddr[65], tokenpk_tokenaddr[65], coinpk_coinaddr[65], coinpk_tokenaddr[65];
	CPubKey tokensupplier, coinsupplier;
	std::string name;
	struct CCcontract_info *cpTokens, CTokens;
	
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	coininputs = tokeninputs = 0;
	numvins = tx.vin.size();
    numvouts = tx.vout.size();
	
	if ((funcid = DecodePawnshopOpRet(tx.vout[tx.vout.size()-1].scriptPubKey, version, createtxid, tokenid)) != 0)
	{
		if (!myGetTransaction(createtxid, createtx, hashBlock) || 
		DecodePawnshopCreateOpRet(createtx.vout[createtx.vout.size()-1].scriptPubKey,version,name,tokensupplier,coinsupplier,pawnshopflags,dummytxid,dummyamount,dummyamount,dummytxid) == 0)
			return eval->Invalid("createtxid invalid!");
		
		GetCCaddress(cpTokens, tokenpk_tokenaddr, tokensupplier);
		GetCCaddress(cpTokens, coinpk_tokenaddr, coinsupplier);
		Getscriptaddress(tokenpk_coinaddr, CScript() << ParseHex(HexStr(tokensupplier)) << OP_CHECKSIG);
		Getscriptaddress(coinpk_coinaddr, CScript() << ParseHex(HexStr(coinsupplier)) << OP_CHECKSIG);
		
		switch (funcid)
		{
			case 'c':
				return (true);
			default:
				for (i = 2; i < numvins; i++)
				{
					//std::cerr << "checking vin." << i << std::endl;
					if ((*cp->ismyvin)(tx.vin[i].scriptSig) != 0)
					{
						//std::cerr << "vin." << i << " is pawnshop vin" << std::endl;
						if (eval->GetTxUnconfirmed(tx.vin[i].prevout.hash,vinTx,hashBlock) == 0)
							return eval->Invalid("always should find vin, but didn't!");
						else
						{
							if (hashBlock == zeroid)
								return eval->Invalid("can't draw funds from mempool!");
							
							if (DecodePawnshopOpRet(vinTx.vout[vinTx.vout.size() - 1].scriptPubKey,version,refcreatetxid,dummytxid) == 0 && 
							DecodeAgreementUnlockOpRet(vinTx.vout[vinTx.vout.size() - 1].scriptPubKey,version,dummytxid,refcreatetxid) == 0)
								return eval->Invalid("can't decode vinTx opret!");
								
							if (refcreatetxid != createtxid && vinTx.GetHash() != createtxid)
								return eval->Invalid("can't draw funds sent to different createtxid!");
							
							if ((nValue = IsPawnshopvout(cp,vinTx,PIF_COINS,tokensupplier,coinsupplier,tx.vin[i].prevout.n)) != 0)
								coininputs += nValue;
							else if ((nValue = IsPawnshopvout(cp,vinTx,PIF_TOKENS,tokensupplier,coinsupplier,tx.vin[i].prevout.n)) != 0)
								tokeninputs += nValue;
							
							//std::cerr << "vin." << i << " nValue:" << nValue << std::endl;
						}
					}
				}
				for (i = 0; i < numvouts - 2; i++)
				{
					//std::cerr << "checking vout." << i << std::endl;
					if ((nValue = IsPawnshopvout(cp,tx,PIF_COINS,tokensupplier,coinsupplier,i)) != 0 ||
					(nValue = IsPawnshopvout(cp,tx,PIF_TOKENS,tokensupplier,coinsupplier,i)) != 0)
						outputs += nValue;
					else if (Getscriptaddress(destaddr,tx.vout[i].scriptPubKey) > 0 &&
					(strcmp(destaddr,tokenpk_tokenaddr) == 0 || strcmp(destaddr,tokenpk_coinaddr) == 0 || strcmp(destaddr,coinpk_coinaddr) == 0 || strcmp(destaddr,coinpk_tokenaddr) == 0))
						outputs += tx.vout[i].nValue;
					
					//std::cerr << "vout." << i << " nValue:" << tx.vout[i].nValue << std::endl;
				}
				break;
		}
		
		if (coininputs + tokeninputs != outputs)
		{
			//std::cerr << "inputs " << coininputs+tokeninputs << " vs outputs " << outputs << std::endl;
			LOGSTREAM("pawnshopcc", CCLOG_INFO, stream << "inputs " << coininputs+tokeninputs << " vs outputs " << outputs << std::endl);			
			return eval->Invalid("mismatched inputs != outputs");
		} 
		else
			return (true);
	}
	else
    {
        return eval->Invalid("invalid op_return data");
    }
    return(false);
}

// Get latest update txid and funcid
bool GetLatestPawnshopTxid(uint256 createtxid, uint256 &latesttxid, uint8_t &funcid)
{
	int32_t vini, height, retcode;
	uint256 batontxid, sourcetxid, hashBlock, dummytxid;
	CTransaction createtx, batontx;
	uint8_t version;

	if (myGetTransaction(createtxid, createtx, hashBlock) == 0 || createtx.vout.size() <= 0)
	{
		std::cerr << "GetLatestPawnshopTxid: couldn't find pawnshop tx" << std::endl;
		return false;
	}
	if (DecodePawnshopOpRet(createtx.vout[createtx.vout.size() - 1].scriptPubKey,version,dummytxid,dummytxid) != 'c')
	{
		std::cerr << "GetLatestPawnshopTxid: pawnshop tx is not a create tx" << std::endl;
		return false;
	}
	if ((retcode = CCgetspenttxid(batontxid, vini, height, createtxid, 0)) != 0)
	{
		latesttxid = createtxid; // no updates, return createtxid
		funcid = 'c';
		return true;
	}
	sourcetxid = createtxid;
	while ((retcode = CCgetspenttxid(batontxid, vini, height, sourcetxid, 0)) == 0 && 
	myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 &&
	DecodePawnshopOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey,version,dummytxid,dummytxid) != 0)
	{
		funcid = DecodePawnshopOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey,version,dummytxid,dummytxid);
		switch (funcid)
		{
			case 't':
			case 'b':
				sourcetxid = batontxid;
				continue;
			case 'x':
			case 'e':
			case 's':
				sourcetxid = batontxid;
				break;
			default:
				std::cerr << "GetLatestPawnshopTxid: found an update, but it has incorrect funcid '" << funcid << "'" << std::endl;
				return false;
		}
		break;
	}
	latesttxid = sourcetxid;
	return true;
}

// Gets latest pawnshop update txid of the specified type
bool FindPawnshopTxidType(uint256 createtxid, uint8_t type, uint256 &typetxid)
{
	CTransaction batontx;
	uint256 batontxid, dummytxid, hashBlock;
	uint8_t version, funcid;
	if (!GetLatestPawnshopTxid(createtxid, batontxid, funcid))
	{
		std::cerr << "FindPawnshopTxidType: can't find latest update tx" << std::endl;
		return false;
	}
	if (batontxid == createtxid && funcid == 'c')
	{
		typetxid == createtxid;
		return true;
	}
	
	while (batontxid != createtxid && myGetTransaction(batontxid, batontx, hashBlock) && batontx.vout.size() > 0 &&
	batontx.vin.size() > 1 && batontx.vin[1].prevout.n == 0 &&
	(funcid = DecodePawnshopOpRet(batontx.vout[batontx.vout.size() - 1].scriptPubKey,version,dummytxid,dummytxid)) != 0)
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

// Check if agreement deposit is sent to this pawnshop's coin escrow
// returns -1 if no agreementtxid defined and/or deposit unlock disabled and/or deposit not unlocked yet, otherwise returns amount sent to pawnshop CC 1of2 addr
int64_t CheckDepositUnlockCond(uint256 createtxid)
{
	int32_t vini, height, retcode, numvouts;
	int64_t dummyamount;
	CPubKey tokensupplier, coinsupplier;
	uint256 refcreatetxid, unlocktxid, hashBlock, agreementtxid, dummytxid;
	CTransaction createtx, unlocktx;
	uint8_t version, unlockfuncid;
	uint32_t pawnshopflags;
	std::string name;
	struct CCcontract_info *cp, C;
	
	cp = CCinit(&C, EVAL_PAWNSHOP);
	
	// get pawnshop create tx from createtxid
	if (myGetTransaction(createtxid, createtx, hashBlock) && (numvouts = createtx.vout.size()) > 0 && 
	// decode pawnshop create tx opret 
	DecodePawnshopCreateOpRet(createtx.vout[numvouts - 1].scriptPubKey,version,name,tokensupplier,coinsupplier,pawnshopflags,dummytxid,dummyamount,dummyamount,agreementtxid) != 0 &&
	// check if deposit unlocking is allowed/required
	agreementtxid != zeroid && (pawnshopflags & PTF_REQUIREUNLOCK) &&
	// get unlocktxid from agreementtxid
	GetLatestAgreementUpdate(agreementtxid, unlocktxid, unlockfuncid) && unlockfuncid == 'n' &&
	// get agreement unlock tx from unlocktxid
	myGetTransaction(unlocktxid, unlocktx, hashBlock) && (numvouts = unlocktx.vout.size()) > 0 && 
	// decode agreement unlock tx opret
	DecodeAgreementUnlockOpRet(unlocktx.vout[numvouts - 1].scriptPubKey, version, dummytxid, refcreatetxid) != 0 && refcreatetxid == createtxid)
	{
		return (IsPawnshopvout(cp, unlocktx, PIF_COINS, tokensupplier, coinsupplier, 0));
	}

	return -1;
}

// check if pawnshop create transaction has valid data and sigs
bool ValidatePawnshopCreateTx(CTransaction opentx, std::string &CCerror)
{
	CPubKey CPK_seller, CPK_client, CPK_arbitrator, tokensupplier, coinsupplier;
	CTransaction tokentx, agreementtx;
	uint256 hashBlock, tokenid, agreementtxid, dummytxid;
	uint8_t version;
	uint32_t pawnshopflags;
	int32_t numvins, numvouts;
	int64_t numtokens, numcoins, dummyamount;
	std::vector<uint8_t> dummyPubkey, sellerpk, clientpk, arbitratorpk;
	std::string dummystr, name;
	char pawnshopaddr[65], tokenpkaddr[65], coinpkaddr[65];
	struct CCcontract_info *cp, C;
	
	cp = CCinit(&C, EVAL_PAWNSHOP);
	CCerror = "";
	
	if (DecodePawnshopCreateOpRet(opentx.vout[opentx.vout.size() - 1].scriptPubKey,version,name,tokensupplier,coinsupplier,pawnshopflags,tokenid,numtokens,numcoins,agreementtxid) == 0)
	{
		CCerror = "invalid pawnshop create opret!";
		return false;
	}
	if (name.size() == 0 || name.size() > 32)
	{
		CCerror = "name must not be empty and up to 32 chars!";
		return false;
	}
	if (tokenid == zeroid)
	{
		CCerror = "tokenid null or invalid in pawnshop create opret!";
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
		CCerror = "tokenid in pawnshop create opret is not a valid token creation txid!";
		return false;
	}
	if (numtokens < 1 || numtokens > CCfullsupply(tokenid) || numcoins < 1)
	{
		CCerror = "invalid numcoins or numtokens value in pawnshop create opret!";
		return false;
	}
	if (pawnshopflags & PTF_NOLOAN && pawnshopflags & PTF_NOTRADE)
	{
		CCerror = "both trading and lending disabled in pawnshop instance!";
		return false;
	}
	if (pawnshopflags & PTF_REQUIREUNLOCK && agreementtxid == zeroid)
	{
		CCerror = "PTF_REQUIREUNLOCK flag defined with no valid agreementtxid in createtx!";
		return false;
	}
	
	// at the moment, PTF_NOLOAN must be set while PTF_NOTRADE cannot be set
	// this will be removed when lending functionality is in place
	if (!(pawnshopflags & PTF_NOLOAN && !(pawnshopflags & PTF_NOTRADE)))
	{
		CCerror = "incorrect type in pawnshop create opret!";
		return false;
	}
	
	if (agreementtxid != zeroid)
	{
		if (!(myGetTransaction(agreementtxid,agreementtx,hashBlock) != 0 && agreementtx.vout.size() > 0 &&
		(DecodeAgreementOpRet(agreementtx.vout[agreementtx.vout.size()-1].scriptPubKey) == 'c')))
		{
			CCerror = "invalid agreement txid in pawnshop create opret!";
			return false;
		}
		
		GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, dummyamount, dummyamount, dummytxid, dummytxid, dummystr);
		
		CPK_seller = pubkey2pk(sellerpk);
		CPK_client = pubkey2pk(clientpk);
		CPK_arbitrator = pubkey2pk(arbitratorpk);
		
		if ((tokensupplier != CPK_seller && tokensupplier != CPK_client) || (coinsupplier != CPK_seller && coinsupplier != CPK_client))
		{
			CCerror = "agreement client and seller pubkeys doesn't match pawnshop coinsupplier and tokensupplier pubkeys!";
			return false;
		}
		if (!(TotalPubkeyNormalInputs(opentx,CPK_seller) + TotalPubkeyCCInputs(opentx,CPK_seller) > 0 ||
		TotalPubkeyNormalInputs(opentx,CPK_client) + TotalPubkeyCCInputs(opentx,CPK_client) > 0 ||
		(CPK_arbitrator.IsValid() && TotalPubkeyNormalInputs(opentx,CPK_arbitrator) + TotalPubkeyCCInputs(opentx,CPK_arbitrator) > 0)))
		{
			CCerror = "no valid inputs signed by any agreement party found in pawnshop create tx!";
			return false;
		}
	}
	
	/*for (auto const vin : opentx.vin)
	{
		if (IsCCInput(vin.scriptSig) != 0)
		{
			CCerror = "create tx contains CC input!";
			return false;
        }
    }*/

	GetCCaddress1of2(cp,pawnshopaddr,tokensupplier,coinsupplier);
	GetCCaddress(cp,tokenpkaddr,tokensupplier);
	GetCCaddress(cp,coinpkaddr,coinsupplier);
	
	if (ConstrainVout(opentx.vout[0], 1, pawnshopaddr, CC_BATON_VALUE) == 0)
	{
		CCerror = "create tx vout0 must be CC baton vout to pawnshop 1of2 address!";
		return false;
	}
	if (ConstrainVout(opentx.vout[1], 1, tokenpkaddr, CC_MARKER_VALUE) == 0)
	{
		CCerror = "create tx vout1 must be CC marker to tokensupplier addr!";
		return false;
	}
	if (ConstrainVout(opentx.vout[2], 1, coinpkaddr, CC_MARKER_VALUE) == 0)
	{
		CCerror = "create tx vout2 must be CC marker to coinsupplier addr!";
		return false;
	}
	return true;
}

int64_t GetPawnshopInputs(struct CCcontract_info *cp,CTransaction createtx,bool mode,std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &validUnspentOutputs)
{
	char pawnshopaddr[65];
	int64_t totalinputs = 0, numvouts, numtokens, numcoins;
	uint256 txid = zeroid, hashBlock, createtxid, agreementtxid, refagreementtxid, tokenid, fundtokenid, borrowtxid;
	CTransaction vintx;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	CPubKey tokensupplier, coinsupplier;
	uint8_t myprivkey[32], version, funcid;
	uint32_t pawnshopflags;
	std::string name;
	bool bHasBorrowed = false;
	
    if ((numvouts = createtx.vout.size()) > 0 && 
	DecodePawnshopCreateOpRet(createtx.vout[numvouts-1].scriptPubKey,version,name,tokensupplier,coinsupplier,pawnshopflags,tokenid,numtokens,numcoins,agreementtxid) == 'c')
    {
		if (mode == PIF_TOKENS)
		{
			GetTokensCCaddress1of2(cp,pawnshopaddr,tokensupplier,coinsupplier);
		}
		else
		{
			GetCCaddress1of2(cp,pawnshopaddr,tokensupplier,coinsupplier);
		}
		SetCCunspents(unspentOutputs,pawnshopaddr,true);
    }
    else
    {
        LOGSTREAM("pawnshopcc", CCLOG_INFO, stream << "invalid pawnshop create txid" << std::endl);
        return 0;
    }

	if (!FindPawnshopTxidType(createtx.GetHash(), 'b', borrowtxid))
	{
        LOGSTREAM("pawnshopcc", CCLOG_INFO, stream << "pawnshop borrow transaction search failed" << std::endl);
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
			// is this a pawnshop CC output?
			if (IsPawnshopvout(cp, vintx, mode, tokensupplier, coinsupplier, (int32_t)it->first.index) > 0 &&
			// if we're spending coins, is the output from a funding transaction to this pawnshop txid?
			((mode == PIF_COINS && DecodePawnshopOpRet(vintx.vout[numvouts-1].scriptPubKey, version, createtxid, fundtokenid) == 'f' && createtxid == createtx.GetHash() &&
			// are the coins provided by the coin supplier pubkey?
			fundtokenid == zeroid && (TotalPubkeyNormalInputs(vintx,coinsupplier) + TotalPubkeyCCInputs(vintx,coinsupplier) > 0 || 
			// if a borrow transaction exists in the pawnshop, the coins can also be provided by the token supplier pubkey
			bHasBorrowed && TotalPubkeyNormalInputs(vintx,tokensupplier) + TotalPubkeyCCInputs(vintx,tokensupplier) > 0)) || 
			// if we're spending tokens, is the output from a pledge transaction to this pawnshop txid?
			(mode == PIF_TOKENS && DecodePawnshopOpRet(vintx.vout[numvouts-1].scriptPubKey, version, createtxid, fundtokenid) == 'p' && createtxid == createtx.GetHash() &&
			// are the tokens provided by the token supplier pubkey?
			fundtokenid == tokenid && TotalPubkeyNormalInputs(vintx,tokensupplier) + TotalPubkeyCCInputs(vintx,tokensupplier) > 0)))
            {
                totalinputs += it->second.satoshis;
				validUnspentOutputs.push_back(*it);
            }
			// if pawnshop has an agreementtxid, is this a CC output from agreementunlock?
			else if (CheckDepositUnlockCond(createtx.GetHash()) > 0 && IsPawnshopvout(cp, vintx, PIF_COINS, tokensupplier, coinsupplier, (int32_t)it->first.index) > 0 &&
			DecodeAgreementUnlockOpRet(vintx.vout[numvouts-1].scriptPubKey, version, refagreementtxid, createtxid) == 'n' && 
			createtxid == createtx.GetHash() && refagreementtxid == agreementtxid)
			{
				totalinputs += it->second.satoshis;
				validUnspentOutputs.push_back(*it);
			}
        }
    }
    return (totalinputs);
}

int64_t AddPawnshopInputs(struct CCcontract_info *cp,CMutableTransaction &mtx,CTransaction createtx,bool mode,int32_t maxinputs)
{
	char pawnshopaddr[65];
	int64_t nValue, totalinputs = 0, numvouts, numtokens, numcoins;
	uint256 hashBlock, createtxid, agreementtxid, tokenid;
	CTransaction vintx;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	CPubKey tokensupplier, coinsupplier;
	uint8_t myprivkey[32], version, funcid;
	std::string name;
	uint32_t pawnshopflags;
	int32_t n = 0;
	
	if (maxinputs == 0 || GetPawnshopInputs(cp,createtx,mode,unspentOutputs) == 0)
		return 0;
	
	if ((numvouts = createtx.vout.size()) > 0 && 
	DecodePawnshopCreateOpRet(createtx.vout[numvouts-1].scriptPubKey,version,name,tokensupplier,coinsupplier,pawnshopflags,tokenid,numtokens,numcoins,agreementtxid) == 'c')
    {
		if (mode == PIF_TOKENS)
		{
			GetTokensCCaddress1of2(cp,pawnshopaddr,tokensupplier,coinsupplier);
		}
		else
		{
			GetCCaddress1of2(cp,pawnshopaddr,tokensupplier,coinsupplier);
		}
    }
	
	for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++)
    {    
		mtx.vin.push_back(CTxIn(it->first.txhash, (int32_t)it->first.index, CScript()));
		Myprivkey(myprivkey);
		if (mode == PIF_TOKENS)
		{
			CCaddrTokens1of2set(cp, tokensupplier, coinsupplier, myprivkey, pawnshopaddr);
		}
		else
		{
			CCaddr1of2set(cp, tokensupplier, coinsupplier, myprivkey, pawnshopaddr);
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

UniValue PawnshopCreate(const CPubKey& pk,uint64_t txfee,std::string name,CPubKey tokensupplier,CPubKey coinsupplier,int64_t numcoins,uint256 tokenid,int64_t numtokens,uint32_t pawnshopflags,uint256 agreementtxid)
{
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk, CPK_seller, CPK_client, CPK_arbitrator;
	CTransaction tokentx, agreementtx;
	int32_t vini, height;
	int64_t dummyamount;
	std::string dummystr;
	std::vector<uint8_t> dummyPubkey, sellerpk, clientpk, arbitratorpk;
	struct CCcontract_info *cp, *cpTokens, C, CTokens;
	uint256 hashBlock, spendingtxid, dummytxid;
	uint8_t version;
	
	cp = CCinit(&C, EVAL_PAWNSHOP);
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (txfee == 0)
	{
		txfee = CC_TXFEE;
	}
	
	if (name.size() == 0 || name.size() > 32)
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Name must not be empty and up to 32 chars");
	
	if (!(tokensupplier.IsFullyValid()))
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Token supplier pubkey invalid");
	
	if (!(coinsupplier.IsFullyValid()))
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Coin supplier pubkey invalid");
	
	if (tokensupplier == coinsupplier)
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Token supplier cannot be the same as coin supplier pubkey");
	
	if (!(myGetTransaction(tokenid,tokentx,hashBlock) != 0 && tokentx.vout.size() > 0 &&
	(DecodeTokenCreateOpRetV1(tokentx.vout[tokentx.vout.size()-1].scriptPubKey,dummyPubkey,dummystr,dummystr) != 0)))
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Tokenid is not a valid token creation txid");
		
	if (numtokens < 1)
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Required token amount must be above 0");
	
	if (numtokens > CCfullsupply(tokenid))
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Required token amount can't be higher than total token supply");
	
	if (numcoins < 1)
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Required coin amount must be above 0");
	
	if (pawnshopflags & PTF_NOLOAN && pawnshopflags & PTF_NOTRADE)
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Cannot disable both trading and lending");
	
	if (pawnshopflags & PTF_REQUIREUNLOCK && agreementtxid == zeroid)
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "PTF_REQUIREUNLOCK flag requires valid agreementtxid");
	
	// at the moment, PTF_NOLOAN must be set while PTF_NOTRADE cannot be set
	// this will be removed when lending functionality is in place
	if (!(pawnshopflags & PTF_NOLOAN && !(pawnshopflags & PTF_NOTRADE)))
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "PTF_NOLOAN (2) must be set while PTF_NOTRADE (4) cannot be set");
	
	if (agreementtxid != zeroid)
	{
		if (!(myGetTransaction(agreementtxid,agreementtx,hashBlock) != 0 && agreementtx.vout.size() > 0 &&
			(DecodeAgreementOpRet(agreementtx.vout[agreementtx.vout.size()-1].scriptPubKey) == 'c')))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Agreement txid is not a valid proposal signing txid");
		
		GetAgreementInitialData(agreementtxid, dummytxid, sellerpk, clientpk, arbitratorpk, dummyamount, dummyamount, dummytxid, dummytxid, dummystr);
		
		CPK_seller = pubkey2pk(sellerpk);
		CPK_client = pubkey2pk(clientpk);
		CPK_arbitrator = pubkey2pk(arbitratorpk);
		
		if (mypk != CPK_seller && mypk != CPK_client && mypk != CPK_arbitrator)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "You are not a member of the specified agreement");
		
		if ((tokensupplier != CPK_seller && tokensupplier != CPK_client) || (coinsupplier != CPK_seller && coinsupplier != CPK_client))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Agreement client and seller pubkeys doesn't match pawnshop coinsupplier and tokensupplier pubkeys");
		
		if (pawnshopflags & PTF_REQUIREUNLOCK)
		{
			if (CCgetspenttxid(spendingtxid, vini, height, agreementtxid, 2) == 0)
				CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Agreement deposit was already spent by txid " << spendingtxid.GetHex());
		}
	}

	if (AddNormalinputs2(mtx, txfee + CC_MARKER_VALUE * 2 + CC_BATON_VALUE, 64) >= txfee + CC_MARKER_VALUE * 2 + CC_BATON_VALUE)
	{
		mtx.vout.push_back(MakeCC1of2vout(EVAL_PAWNSHOP, CC_BATON_VALUE, tokensupplier, coinsupplier)); // vout.0 baton for status tracking sent to coins 1of2 addr
		mtx.vout.push_back(MakeCC1vout(EVAL_PAWNSHOP, CC_MARKER_VALUE, tokensupplier)); // vout.1 marker to tokensupplier
		mtx.vout.push_back(MakeCC1vout(EVAL_PAWNSHOP, CC_MARKER_VALUE, coinsupplier)); // vout.2 marker to coinsupplier

		return (FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodePawnshopCreateOpRet(PAWNSHOPCC_VERSION,name,tokensupplier,coinsupplier,pawnshopflags,tokenid,numtokens,numcoins,agreementtxid)));
	}
	CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Error adding normal inputs");
}

UniValue PawnshopFund(const CPubKey& pk, uint64_t txfee, uint256 createtxid, int64_t amount)
{
	char addr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk, tokensupplier, coinsupplier;
	CTransaction createtx;
	int32_t numvouts;
	int64_t numtokens, numcoins, coinbalance, tokenbalance, tokens = 0, inputs = 0;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	struct CCcontract_info *cp, C, *cpTokens, CTokens;
	uint256 hashBlock, tokenid, agreementtxid, borrowtxid = zeroid, latesttxid;
	uint8_t version, lastfuncid;
	std::string name;
	uint32_t pawnshopflags;
	
	cp = CCinit(&C, EVAL_PAWNSHOP);
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (txfee == 0)
	{
		txfee = CC_TXFEE;
	}	
	
	if (amount < 1)
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Funding amount must be above 0");
	
	if (createtxid != zeroid)
	{
		if (myGetTransaction(createtxid, createtx, hashBlock) == 0 || (numvouts = createtx.vout.size()) <= 0)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Can't find specified pawnshop instance txid " << createtxid.GetHex());
		
		if (!ValidatePawnshopCreateTx(createtx,CCerror))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << CCerror);
		
		if (!GetLatestPawnshopTxid(createtxid, latesttxid, lastfuncid) || lastfuncid == 'x' || lastfuncid == 'e' || lastfuncid == 's' || lastfuncid == 'r')
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop instance" << createtxid.GetHex() << " closed");
		
		// not relevant at the moment, will be refactored later
		/*if (!FindPawnshopTxidType(createtxid, 'b', borrowtxid))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop borrow transaction search failed, quitting");*/
		
		DecodePawnshopCreateOpRet(createtx.vout[numvouts - 1].scriptPubKey,version,name,tokensupplier,coinsupplier,pawnshopflags,tokenid,numtokens,numcoins,agreementtxid);
		
		//if (mypk != coinsupplier || (borrowtxid != zeroid && mypk != tokensupplier))
		if (mypk != coinsupplier)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Coins can only be sent by coin supplier pubkey"); // may be sent by tokensupplier when loan is in progress
		
		coinbalance = GetPawnshopInputs(cp,createtx,PIF_COINS,unspentOutputs);
		if (unspentOutputs.size() > PAWNSHOPCC_MAXVINS)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "utxo count in coin escrow exceeds withdrawable amount, close or cancel the pawnshop instance");
		tokenbalance = GetPawnshopInputs(cp,createtx,PIF_TOKENS,unspentOutputs);
		if (unspentOutputs.size() > PAWNSHOPCC_MAXVINS)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "utxo count in token escrow exceeds withdrawable amount, close or cancel the pawnshop instance");
		
		if (coinbalance >= numcoins)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop instance already has enough coins");
		else if (coinbalance + amount > numcoins)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Specified coin amount is higher than needed to fill pawnshop instance");
	}
	else
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Invalid createtxid");
	
	tokenid = zeroid;
	inputs = AddNormalinputs(mtx, mypk, txfee + amount, 64, pk.IsValid());

	if (inputs >= amount + txfee)
	{
		mtx.vout.push_back(MakeCC1of2vout(EVAL_PAWNSHOP, amount, tokensupplier, coinsupplier));
		return (FinalizeCCTxExt(pk.IsValid(), 0, cp, mtx, mypk, txfee, EncodePawnshopOpRet('f', PAWNSHOPCC_VERSION, createtxid, tokenid, tokensupplier, coinsupplier))); 
	}
	else
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Error adding funds");
}

UniValue PawnshopPledge(const CPubKey& pk, uint64_t txfee, uint256 createtxid)
{
	char addr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk, tokensupplier, coinsupplier;
	CTransaction createtx;
	int32_t numvouts;
	int64_t numtokens, numcoins, coinbalance, tokenbalance, tokens = 0, inputs = 0;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	struct CCcontract_info *cp, C, *cpTokens, CTokens;
	uint256 hashBlock, tokenid, agreementtxid, borrowtxid = zeroid, latesttxid;
	uint8_t version, lastfuncid;
	std::string name;
	uint32_t pawnshopflags;
	
	cp = CCinit(&C, EVAL_PAWNSHOP);
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (txfee == 0)
	{
		txfee = CC_TXFEE;
	}	
	
	if (createtxid != zeroid)
	{
		if (myGetTransaction(createtxid, createtx, hashBlock) == 0 || (numvouts = createtx.vout.size()) <= 0)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Can't find specified pawnshop instance txid " << createtxid.GetHex());
		
		if (!ValidatePawnshopCreateTx(createtx,CCerror))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << CCerror);
		
		if (!GetLatestPawnshopTxid(createtxid, latesttxid, lastfuncid) || lastfuncid == 'x' || lastfuncid == 'e' || lastfuncid == 's' || lastfuncid == 'r')
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop instance " << createtxid.GetHex() << " closed");
		
		// not relevant at the moment, will be refactored later
		/*if (!FindPawnshopTxidType(createtxid, 'b', borrowtxid))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop borrow transaction search failed, quitting");*/
		
		DecodePawnshopCreateOpRet(createtx.vout[numvouts - 1].scriptPubKey,version,name,tokensupplier,coinsupplier,pawnshopflags,tokenid,numtokens,numcoins,agreementtxid);
		
		if (mypk != tokensupplier)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Tokens can only be sent by token supplier pubkey");
		
		coinbalance = GetPawnshopInputs(cp,createtx,PIF_COINS,unspentOutputs);
		if (unspentOutputs.size() > PAWNSHOPCC_MAXVINS)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "utxo count in coin escrow exceeds withdrawable amount, close or cancel the pawnshop instance");
		tokenbalance = GetPawnshopInputs(cp,createtx,PIF_TOKENS,unspentOutputs);
		if (unspentOutputs.size() > PAWNSHOPCC_MAXVINS)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "utxo count in token escrow exceeds withdrawable amount, close or cancel the pawnshop instance");

		if (tokenbalance >= numtokens)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop already has enough tokens");
	}
	else
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Invalid createtxid");
	
	inputs = AddNormalinputs(mtx, mypk, txfee, 5, pk.IsValid());
	tokens = AddTokenCCInputs(cpTokens, mtx, mypk, tokenid, numtokens, 64); 
	if (tokens < numtokens)
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Couldn't find enough tokens for specified amount");
	
	if (inputs >= txfee && tokens >= numtokens)
	{
		mtx.vout.push_back(MakeTokensCC1of2vout(EVAL_PAWNSHOP, numtokens, tokensupplier, coinsupplier, NULL));
		if (tokens > numtokens)
		{
			mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, tokens - numtokens, mypk));
		}
		return (FinalizeCCTxExt(pk.IsValid(), 0, cp, mtx, mypk, txfee, EncodePawnshopOpRet('p', PAWNSHOPCC_VERSION, createtxid, tokenid, tokensupplier, coinsupplier))); 
	}
	else
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Error adding funds");
}

///// not implemented yet /////
UniValue PawnshopSchedule(const CPubKey& pk, uint64_t txfee, uint256 createtxid, int64_t principal, int64_t duedate, bool bRelative)
{
	CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "not implemented yet");
}
///// not implemented yet /////

UniValue PawnshopCancel(const CPubKey& pk, uint64_t txfee, uint256 createtxid)
{
	char pawnshopaddr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk, tokensupplier, coinsupplier;
	CTransaction createtx;
	int32_t numvouts;
	int64_t numtokens, numcoins, coinbalance, tokenbalance, tokens = 0, coins = 0, inputs = 0;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	struct CCcontract_info *cp, C, *cpTokens, CTokens;
	uint256 hashBlock, tokenid, agreementtxid, latesttxid, borrowtxid;
	uint8_t version, mypriv[32], lastfuncid;
	std::string name;
	uint32_t pawnshopflags;
	
	cp = CCinit(&C, EVAL_PAWNSHOP);
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (txfee == 0)
	{
		txfee = CC_TXFEE;
	}
	
	if (createtxid != zeroid)
	{
		if (myGetTransaction(createtxid, createtx, hashBlock) == 0 || (numvouts = createtx.vout.size()) <= 0)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Can't find specified pawnshop instance txid " << createtxid.GetHex());
		
		if (!ValidatePawnshopCreateTx(createtx,CCerror))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << CCerror);
		
		if (!GetLatestPawnshopTxid(createtxid, latesttxid, lastfuncid) || lastfuncid == 'x' || lastfuncid == 'e' || lastfuncid == 's' || lastfuncid == 'r')
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop instance " << createtxid.GetHex() << " closed");
		
		// not relevant at the moment, will be refactored later
		/*if (!FindPawnshopTxidType(createtxid, 'b', borrowtxid))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop borrow transaction search failed, quitting");
		else if (borrowtxid != zeroid && mypk != coinsupplier)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Loan in progress can only be cancelled by coin supplier pubkey");*/
		
		DecodePawnshopCreateOpRet(createtx.vout[numvouts - 1].scriptPubKey,version,name,tokensupplier,coinsupplier,pawnshopflags,tokenid,numtokens,numcoins,agreementtxid);
		
		if (mypk != tokensupplier && mypk != coinsupplier)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "You are not a valid party for pawnshop instance " << createtxid.GetHex());
		
		// If pawnshop has an associated agreement and deposit spending required, the deposit must NOT be sent to the pawnshop's coin escrow
		if (pawnshopflags & PTF_REQUIREUNLOCK && CheckDepositUnlockCond(createtxid) > -1)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Cannot cancel pawnshop instance if its associated agreement has deposit unlocked");
		
		coinbalance = GetPawnshopInputs(cp,createtx,PIF_COINS,unspentOutputs);
		tokenbalance = GetPawnshopInputs(cp,createtx,PIF_TOKENS,unspentOutputs);
		
		// NOTE: maybe this could be an optional feature enabled by setting a flag?
		if (coinbalance >= numcoins && tokenbalance >= numtokens)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Cannot cancel instance when escrow has enough coins and tokens");
	}
	else
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Invalid createtxid");
	
	// creating cancel ('x') transaction
	
	inputs = AddNormalinputs(mtx, mypk, txfee, 5, pk.IsValid()); // txfee
	if (inputs >= txfee)
	{
		GetCCaddress1of2(cp, pawnshopaddr, tokensupplier, coinsupplier);
		mtx.vin.push_back(CTxIn(createtxid,0,CScript())); // previous CC 1of2 baton vin
		Myprivkey(mypriv);
		CCaddr1of2set(cp, tokensupplier, coinsupplier, mypriv, pawnshopaddr);
	}
	else
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Error adding funds for txfee");
	
	coins = AddPawnshopInputs(cp, mtx, createtx, PIF_COINS, PAWNSHOPCC_MAXVINS); // coin from CC 1of2 addr vins
	tokens = AddPawnshopInputs(cp, mtx, createtx, PIF_TOKENS, PAWNSHOPCC_MAXVINS); // token from CC 1of2 addr vins
	
	if (coins > 0)
	{
		mtx.vout.push_back(CTxOut(coins, CScript() << ParseHex(HexStr(coinsupplier)) << OP_CHECKSIG)); // coins refund vout
	}
	if (tokens > 0)
	{
		mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, tokens, tokensupplier)); // tokens refund vout
	}
	return (FinalizeCCTxExt(pk.IsValid(), 0, cp, mtx, mypk, txfee, EncodePawnshopOpRet('x', PAWNSHOPCC_VERSION, createtxid, tokenid, tokensupplier, coinsupplier))); 
}

///// not implemented yet /////
UniValue PawnshopBorrow(const CPubKey& pk, uint64_t txfee, uint256 createtxid, uint256 scheduletxid)
{
	CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "not implemented yet");
}
UniValue PawnshopSeize(const CPubKey& pk, uint64_t txfee, uint256 createtxid)
{
	CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "not implemented yet");
}
///// not implemented yet /////

UniValue PawnshopExchange(const CPubKey& pk, uint64_t txfee, uint256 createtxid)
{
	char pawnshopaddr[65];
	CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk, tokensupplier, coinsupplier;
	CTransaction createtx;
	int32_t numvouts;
	int64_t numtokens, numcoins, coinbalance, tokenbalance, tokens = 0, coins = 0, inputs = 0;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
	struct CCcontract_info *cp, C, *cpTokens, CTokens;
	uint256 hashBlock, tokenid, agreementtxid, latesttxid, scheduletxid, borrowtxid;
	uint8_t version, mypriv[32], lastfuncid;
	std::string name;
	uint32_t pawnshopflags;
	
	cp = CCinit(&C, EVAL_PAWNSHOP);
	cpTokens = CCinit(&CTokens, EVAL_TOKENS);
	mypk = pk.IsValid() ? pk : pubkey2pk(Mypubkey());
	if (txfee == 0)
	{
		txfee = CC_TXFEE;
	}
	
	if (createtxid != zeroid)
	{
		if (myGetTransaction(createtxid, createtx, hashBlock) == 0 || (numvouts = createtx.vout.size()) <= 0)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Can't find specified pawnshop instance txid " << createtxid.GetHex());
		
		if (!ValidatePawnshopCreateTx(createtx,CCerror))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << CCerror);
		
		if (!GetLatestPawnshopTxid(createtxid, latesttxid, lastfuncid) || lastfuncid == 'x' || lastfuncid == 'e' || lastfuncid == 's' || lastfuncid == 'r')
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop instance " << createtxid.GetHex() << " closed");
		
		DecodePawnshopCreateOpRet(createtx.vout[numvouts - 1].scriptPubKey,version,name,tokensupplier,coinsupplier,pawnshopflags,tokenid,numtokens,numcoins,agreementtxid);
		
		if (mypk != tokensupplier && mypk != coinsupplier)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "You are not a valid party for pawnshop instance " << createtxid.GetHex());
		
		if (pawnshopflags & PTF_REQUIREUNLOCK && CheckDepositUnlockCond(createtxid) < 0)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Deposit from agreement " << agreementtxid.GetHex() << " must be unlocked first for pawnshop instance " << createtxid.GetHex());
		
		coinbalance = GetPawnshopInputs(cp,createtx,PIF_COINS,unspentOutputs);
		tokenbalance = GetPawnshopInputs(cp,createtx,PIF_TOKENS,unspentOutputs);
		
		if (coinbalance < numcoins && tokenbalance < numtokens)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Cannot exchange when instance doesn't have enough coins and tokens");
		
		// TODO: refactor below
		// checking for borrow transaction
		if (!FindPawnshopTxidType(createtxid, 'b', borrowtxid))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop borrow transaction search failed, quitting");
		// checking for latest schedule transaction
		if (!FindPawnshopTxidType(createtxid, 't', scheduletxid))
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Pawnshop schedule transaction search failed, quitting");
		
		if (pawnshopflags & PTF_NOTRADE && borrowtxid == zeroid)
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Cannot exchange when NOTRADE flag is set and no borrow transaction exists");
		
		// creating exchange ('e') transaction

		///// not implemented yet /////
		if (borrowtxid != zeroid)
		{
			CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "not implemented yet");
		}
		///// not implemented yet /////
		else
		{
			inputs = AddNormalinputs(mtx, mypk, txfee, 5, pk.IsValid()); // txfee
			if (inputs >= txfee)
			{
				GetCCaddress1of2(cp, pawnshopaddr, tokensupplier, coinsupplier);
				mtx.vin.push_back(CTxIn(createtxid,0,CScript())); // previous CC 1of2 baton vin
				Myprivkey(mypriv);
				CCaddr1of2set(cp, tokensupplier, coinsupplier, mypriv, pawnshopaddr);
			}
			else
				CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Error adding funds for txfee");
			
			coins = AddPawnshopInputs(cp, mtx, createtx, PIF_COINS, PAWNSHOPCC_MAXVINS); // coin from CC 1of2 addr vins
			tokens = AddPawnshopInputs(cp, mtx, createtx, PIF_TOKENS, PAWNSHOPCC_MAXVINS); // token from CC 1of2 addr vins
			
			if (coins < coinbalance || tokens < tokenbalance)
				CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Error adding pawnshop inputs");
			else
			{
				mtx.vout.push_back(CTxOut(coins, CScript() << ParseHex(HexStr(tokensupplier)) << OP_CHECKSIG)); // coins exchange vout
				mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, tokens, coinsupplier)); // tokens exchange vout
				
				if (coins - numcoins > 0)
				{
					mtx.vout.push_back(CTxOut(coins - numcoins, CScript() << ParseHex(HexStr(coinsupplier)) << OP_CHECKSIG)); // coins refund vout
				}
				if (tokens - numtokens > 0)
				{
					mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, tokens - numtokens, tokensupplier)); // tokens refund vout
				}
				return (FinalizeCCTxExt(pk.IsValid(), 0, cp, mtx, mypk, txfee, EncodePawnshopOpRet('e', PAWNSHOPCC_VERSION, createtxid, tokenid, tokensupplier, coinsupplier))); 
			}
		}
	}
	else
		CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Invalid createtxid");
}

//===========================================================================
// RPCs - informational
//===========================================================================
UniValue PawnshopInfo(const CPubKey& pk, uint256 createtxid)
{
	UniValue result(UniValue::VOBJ), flaglist(UniValue::VOBJ);
	CTransaction tx;
	uint256 hashBlock, tokenid, latesttxid, agreementtxid;
	int32_t numvouts;
	int64_t numtokens, numcoins;
	uint8_t version, funcid, lastfuncid;
	uint32_t pawnshopflags;
	CPubKey mypk, tokensupplier, coinsupplier;
	char str[67];
	std::string status, name;
	std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs, unspentTokenOutputs;
	struct CCcontract_info *cp, C;
	
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	if (myGetTransaction(createtxid,tx,hashBlock) != 0 && (numvouts = tx.vout.size()) > 0 &&
	(funcid = DecodePawnshopCreateOpRet(tx.vout[numvouts-1].scriptPubKey, version, name, tokensupplier, coinsupplier, pawnshopflags, tokenid, numtokens, numcoins, agreementtxid)) == 'c')
	{
		result.push_back(Pair("result", "success"));
		result.push_back(Pair("createtxid", createtxid.GetHex()));
		
		result.push_back(Pair("name", name));
		result.push_back(Pair("token_supplier", pubkey33_str(str,(uint8_t *)&tokensupplier)));
		result.push_back(Pair("coin_supplier", pubkey33_str(str,(uint8_t *)&coinsupplier)));
		
		if (agreementtxid != zeroid)
		{
			result.push_back(Pair("agreement_txid", agreementtxid.GetHex()));
		}
		
		result.push_back(Pair("tokenid", tokenid.GetHex()));
		result.push_back(Pair("required_tokens", numtokens));
		result.push_back(Pair("required_coins", (double)numcoins/COIN));
		
		cp = CCinit(&C, EVAL_PAWNSHOP);
		
		if (GetLatestPawnshopTxid(createtxid, latesttxid, lastfuncid))
		{
			switch (lastfuncid)
			{
				case 'x':
					status = "cancelled";
					break;
				case 'e':
					status = "closed";
					break;
				case 's':
					status = "seized";
					break;
				default:
					status = "open";
					//std::cerr << "looking for tokens" << std::endl;
					result.push_back(Pair("token_balance", GetPawnshopInputs(cp,tx,PIF_TOKENS,unspentTokenOutputs)));
					//std::cerr << "looking for coins" << std::endl;
					result.push_back(Pair("coin_balance", (double)GetPawnshopInputs(cp,tx,PIF_COINS,unspentOutputs)/COIN));
					break;
			}
			result.push_back(Pair("status", status));
		}
		
		flaglist.push_back(Pair("require_deposit_unlock", (pawnshopflags & PTF_REQUIREUNLOCK) ? "true" : "false"));
		flaglist.push_back(Pair("disable_loans", (pawnshopflags & PTF_NOLOAN) ? "true" : "false"));
		flaglist.push_back(Pair("disable_trading", (pawnshopflags & PTF_NOTRADE) ? "true" : "false"));
		result.push_back(Pair("flags", flaglist));
		
		return(result);
	}
	CCERR_RESULT("pawnshopcc", CCLOG_INFO, stream << "Invalid pawnshop instance creation txid");
}

UniValue PawnshopList(const CPubKey& pk)
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
	
	cp = CCinit(&C, EVAL_PAWNSHOP);
	mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	GetCCaddress(cp,myCCaddr,mypk);
	SetCCtxids(txids,myCCaddr,true,EVAL_PAWNSHOP,CC_MARKER_VALUE,zeroid,'c');
	
	for (std::vector<uint256>::const_iterator it=txids.begin(); it!=txids.end(); it++)
	{
		txid = *it;
		if ( myGetTransaction(txid,tx,hashBlock) != 0 && (numvouts= tx.vout.size()) > 0 )
		{
			if (DecodePawnshopOpRet(tx.vout[numvouts-1].scriptPubKey, version, dummytxid, dummytxid) == 'c')
			{				
				result.push_back(txid.GetHex());
			}
		}
	}
	return (result);
}
