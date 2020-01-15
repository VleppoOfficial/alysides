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


/*
 Agreements stuff
 */

#ifndef CC_AGREEMENTS_H
#define CC_AGREEMENTS_H

#include "CCinclude.h"

#define CC_MARKER_VALUE 10000
#define CC_RESPONSE_VALUE 20000
#define CC_MEDIATORFEE_MIN 10000
#define CC_DEPOSIT_MIN 10000

uint8_t DecodeAgreementOpRet(const CScript scriptPubKey, uint8_t &proposaltype);
CScript EncodeAgreementProposalOpRet(uint8_t proposaltype, std::vector<uint8_t> initiator, std::vector<uint8_t> receiver, std::vector<uint8_t> mediator, int64_t mediatorfee, int64_t deposit, int64_t depositcut, uint256 datahash, uint256 agreementtxid, uint256 prevproposaltxid, std::string name);
uint8_t DecodeAgreementProposalOpRet(CScript scriptPubKey, uint8_t &proposaltype, std::vector<uint8_t> &initiator, std::vector<uint8_t> &receiver, std::vector<uint8_t> &mediator, int64_t &mediatorfee, int64_t &deposit, int64_t &depositcut, uint256 &datahash, uint256 &agreementtxid, uint256 &prevproposaltxid, std::string &name);

bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

int64_t IsAgreementsvout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v);

UniValue AgreementPropose(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> buyer, std::vector<uint8_t> mediator, int64_t mediatorfee, int64_t deposit, uint256 prevproposaltxid, uint256 refagreementtxid);
UniValue AgreementUpdate(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash, std::vector<uint8_t> newmediator, uint256 prevproposaltxid);
UniValue AgreementTerminate(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash, uint64_t depositcut, uint256 prevproposaltxid);
UniValue AgreementAccept(const CPubKey& pk, uint64_t txfee, uint256 reftxid, uint256 datahash, uint256 agreementtxid);

UniValue AgreementList();

#endif

