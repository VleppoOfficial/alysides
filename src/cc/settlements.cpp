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

#include "CCsettlements.h"

//===========================================================================
//
// Opret encoding/decoding functions
//
//===========================================================================

/*CScript EncodeAgreementCreateOpRet(uint8_t funcid,CPubKey pk)
{    
    CScript opret; uint8_t evalcode = EVAL_AGREEMENTS;
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << pk);
    return(opret);
}

uint8_t DecodeAgreementCreateOpRet(CPubKey &pk,CScript scriptPubKey)
{
    std::vector<uint8_t> vopret; uint8_t e,f;
    GetOpReturnData(scriptPubKey,vopret);
    if ( vopret.size() > 2 && E_UNMARSHAL(vopret,ss >> e; ss >> f; ss >> pk) != 0 && e == EVAL_AGREEMENTS )
    {
        return(f);
    }
    return(0);
}*/

//===========================================================================
//
// Validation
//
//===========================================================================

bool SettlementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	return(eval->Invalid("no validation yet"));
}

//===========================================================================
//
// Helper functions
//
//===========================================================================

//stuff

//===========================================================================
//
// RPCs
//
//===========================================================================

/*UniValue SettlementCreate(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> clientpubkey, int64_t deposit, int64_t timelock)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	
    struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	
    if ( txfee == 0 )
        txfee = 10000;
    CPubkey mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "not done yet");
}*/