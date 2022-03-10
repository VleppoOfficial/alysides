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

#ifndef CC_PAWNSHOP_H
#define CC_PAWNSHOP_H

#include "CCinclude.h"

bool AssetLoansValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);


bool Stub1Validate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);
bool Stub2Validate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);
bool Stub3Validate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

#endif