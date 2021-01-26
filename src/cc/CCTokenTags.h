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


#ifndef CC_TOKENTAGS_H
#define CC_TOKENTAGS_H

#include "CCinclude.h"

/// Returns pubkeys that have/had possession of the specified tokenid.
/// @param tokenid id of token to check for
/// @param minbalance if set, only pubkeys that have this many tokens of this id will be returned. If 0, lists every pubkey that ever has/had tokens with this id
UniValue TokenOwners(uint256 tokenid, int64_t minbalance);

/// Returns tokenids of tokens that the specified pubkey is in possession of.
/// @param pk pubkey to check for tokens
/// @param minbalance minimum balance of tokens for pubkey required for its id to be added to the array. If 0, lists ids of every token that the pubkey ever has/had
UniValue TokenInventory(const CPubKey pk, int64_t minbalance);

bool TokenTagsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

#endif
