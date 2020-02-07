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

/*

Settlements workflow:

1. Party 1 sends invoice/request to party 2 to pay (payment) to escrow. (payment) can be coins or tokens.
The request can be for an agreement invoice (with agreement id of course), standalone invoice, or loan/deposit.
The request includes a timelock value in seconds for the requests and escrows expiry time.
P1 -> request -> P2

2. Party 2 accepts request, pays the (payment) into escrow. If the request is an agreement invoice, party 2 must reclaim deposit.
For this step the request must not be expired. The escrow has a new timelock set up, with its value inherited from the request timelock.
P2 -> (payment) -> Escrow (Settlements 1of2 CC address)
Agreement -> deposit -> P2 (if agreement invoice)

3a. Party 1 sends (asset) to escrow, reclaims the coins/tokens that were put into escrow by party 2 at step 2.
(asset) can be coins or tokens, but must be opposite of (payment), i.e. if (payment) is in coins, (asset) must be in tokens, or vice versa. (GOOD PATH)
P1 -> (asset) -> Escrow -> (payment) -> P1

3b. Party 2 withdraws their (payment) from the escrow before party 1 fulfills their end of the deal. The timelock must be expired for this step to be available. (BAD PATH)
Escrow -> (payment) -> P2

4a. If initial request is an agreement or standalone invoice, party 2 claims (asset) put in the escrow at step 3a. (INVOICE END)
Escrow -> (asset) -> P2

4b. If initial request is a loan/deposit, party 1 sends (payment) to escrow and reclaim the (asset) from step 3a. (GOOD PATH - LOAN)
P1 -> (payment) -> Escrow -> (asset) -> P1

4c. If initial request is a loan/deposit, party 2 claims the (asset) from step 3a. The timelock must be expired for this step to be available. (BAD PATH - LOAN)
Escrow -> (asset) -> P2

5. If initial request is a loan/deposit, party 2 reclaims (payment) put in the escrow at step 4b. (LOAN END)
Escrow -> (payment) -> P2

RPC list:
P1 settlementcreate 'c' - used in step 1
P2 settlementaccept 'x' - used in step 2
P1 settlementassetsend 'a' - used in step 3a
P2 settlementpaymentclaim 'p' - used in steps 3b and 5
P1 settlementpaymentsend 'r' - used in step 4b
P2 settlementassetclaim 'l' - used in steps 4a and 4c

Agreement/settlement invoice workflow:
P1 settlementcreate
P2 settlementaccept
P1 settlementassetsend or P2 settlementpaymentclaim
P2 settlementassetclaim

Loan workflow:
P1 settlementcreate
P2 settlementaccept
P1 settlementassetsend or P2 settlementpaymentclaim
P1 settlementpaymentsend or P2 settlementassetclaim
P2 settlementpaymentclaim


*/

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