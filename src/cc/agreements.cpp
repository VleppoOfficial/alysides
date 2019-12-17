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

#include "CCagreements.h"

/*
	AgreementCreate: (no CC inputs - don't validate)
	vins.* normal input
	vout.0 marker (for viewing in agreementlist, spend it to cancel agreement)
	vout.1 proposal accept CC output - spend as client to approve proposal (sent to client's CC addr)
	vout.2 update baton (sent to 1of2 addr)
	vout.3 dispute baton (sent to 1of2 addr)
	vout.n-2 change
	vout.n-1 OP_RETURN EVAL_AGREEMENTS 'n' name datahash creatorpubkey clientpubkey [deposit] [timelock]
	
	AgreementAccept:
	vins.* normal input
	vin.n-1 proposal accept CC input (only client can spend)
	vout.0 invoice CC output (may include deposit, spend to mark agreement as paid & reclaim deposit)
	vout.n-2 change
	vout.n-1 OP_RETURN EVAL_AGREEMENTS 'a' ...
	
	AgreementDispute: <- need validation rules on timeframe
	vins.* normal input
	vin.n-1 previous dispute baton
	vout.0 next dispute baton (sent to 1of2 addr)
	vout.1 dispute resolve CC output (spend to resolve dispute - sent to own address)
	vout.n-2 change
	vout.n-1 OP_RETURN EVAL_AGREEMENTS 'd' ...
	
	AgreementDisputeResolve:
	vins.* normal input
	vin.n-1 dispute resolve CC input
	vout.n-2 change
	vout.n-1 OP_RETURN EVAL_AGREEMENTS 'r' ...
	
	AgreementUpdate: <- need validation rules on permissions
	vins.* normal input
	vin.n-1 previous update baton
	----
	vout.0 next update baton (sent to 1of2 addr)
	vout.n-2 change
	vout.n-1 OP_RETURN EVAL_AGREEMENTS 'u' ...
	
	AgreementCancel: <- need validation rules on permissions
	vins.* normal input
	vin.n-1 agreement marker
	----
	vout.n-2 change
	vout.n-1 OP_RETURN EVAL_AGREEMENTS 'c' ...
	
	
*/

//===========================================================================
//
// Opret encoding/decoding functions
//
//===========================================================================

CScript EncodeAgreementCreateOpRet(std::string name, uint256 datahash, std::vector<uint8_t> creatorpubkey, clientpubkey, int64_t deposit, timelock)
{
    CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'n';
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << name << datahash << creatorpubkey << clientpubkey << deposit << timelock);
    return(opret);
}

uint8_t DecodeAgreementCreateOpRet(CScript scriptPubKey, std::string &name, uint256 &datahash, std::vector<uint8_t> &creatorpubkey, &clientpubkey, int64_t &deposit, &timelock)
{
    std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
    GetOpReturnData(scriptPubKey, vopret);
    if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> name; ss >> datahash; ss >> creatorpubkey; ss >> clientpubkey; ss >> deposit; ss >> timelock) != 0 && evalcode == EVAL_AGREEMENTS)
    {
        return(funcid);
    }
    return(0);
}

//===========================================================================
//
// Validation
//
//===========================================================================

bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	return(eval->Invalid("no validation yet"));
}

//===========================================================================
//
// Helper functions
//
//===========================================================================

int64_t IsAgreementsvout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v)
{
    char destaddr[64];
    if ( tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0 )
    {
        if ( Getscriptaddress(destaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(destaddr,cp->unspendableCCaddr) == 0 )
            return(tx.vout[v].nValue);
    }
    return(0);
}

//===========================================================================
//
// RPCs
//
//===========================================================================

UniValue AgreementCreate(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> clientpubkey, int64_t deposit, int64_t timelock)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
    struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	
    if ( txfee == 0 )
        txfee = 10000;
    mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "not done yet");
	
    /*for(auto txid : bindtxids)
    {
        if (myGetTransaction(txid,tx,hashBlock)==0 || (numvouts=tx.vout.size())<=0)
            CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cant find bindtxid " << txid.GetHex());
        if (DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tmptokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2,wiftype)!='B')
            CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "invalid bindtxid " << txid.GetHex());
    }
	
    if ( AddNormalinputs(mtx,mypk,amount,64,pk.IsValid()) >= txfee + deposit )
    {
        for (int i=0; i<100; i++) mtx.vout.push_back(MakeCC1vout(EVAL_PEGS,(amount-txfee)/100,pegspk));
        return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodePegsCreateOpRet(bindtxids)));
    }
	
    CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");*/
}

/*
UniValue AgreementUpdate(agreementtxid);
UniValue AgreementCancel(agreementtxid);
UniValue AgreementDisputeCreate(agreementtxid);
UniValue AgreementDisputeResolve(agreementtxid);
UniValue AgreementAcceptProposal(agreementtxid);
*/
