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

CScript EncodeAgreementCreateOpRet(std::string name, uint256 datahash, std::vector<uint8_t> creatorpubkey, clientpubkey, int64_t deposit, int64_t timelock);
uint8_t DecodeAgreementCreateOpRet(CScript scriptPubKey, std::string &name, uint256 &datahash, std::vector<uint8_t> &creatorpubkey, &clientpubkey, int64_t &deposit, int64_t &timelock);

bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

int64_t IsAgreementsvout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v);

UniValue AgreementCreate(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> clientpubkey, int64_t deposit, int64_t timelock);

#endif

