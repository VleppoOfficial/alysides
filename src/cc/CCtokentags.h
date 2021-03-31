/******************************************************************************
 * Copyright © 2014-2021 The SuperNET Developers.                             *
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

#define TOKENTAGSCC_VERSION 1
#define CC_TXFEE 10000
#define CC_MARKER_VALUE 10000

enum ETokenTagCreateFlags
{
    // Only creator of tag is allowed to update it
    TTF_CREATORONLY = 1, // 0b00000001
    // Fixed token ownership requirement
    TTF_CONSTREQS = 2, // 0b00000010
};

/// Main validation entry point of TokenTags CC.
/// @param cp CCcontract_info object with TokenTags CC variables (global CC address, global private key, etc.)
/// @param eval pointer to the CC dispatching object
/// @param tx the transaction to be validated
/// @param nIn not used here
/// @returns true if transaction is valid, otherwise false or calls eval->Invalid().
bool TokenTagsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

/*UniValue TokenTagCreate(const CPubKey& pk,uint64_t txfee,std::string name,std::vector<uint256> tokenids,std::vector<CAmount> updateamounts,uint8_t flags,int64_t maxupdates);
UniValue TokenTagUpdate(const CPubKey& pk,uint64_t txfee,uint256 tokentagid,std::string data,std::vector<CAmount> updateamounts);
UniValue TokenTagClose(const CPubKey& pk,uint64_t txfee,uint256 tokentagid,std::string data);

UniValue TokenTagInfo(uint256 txid);
UniValue TokenTagSamples(uint256 tokentagid, int64_t samplenum, bool bReverse);
UniValue TokenTagList(uint256 tokenid, CPubKey pubkey);*/

/// Returns pubkeys that have/had possession of the specified tokenid.
/// @param tokenid id of token to check for
/// @param minbalance if set, only pubkeys that have this many tokens of this id will be returned. If 0, lists every pubkey that ever has/had tokens with this id
/// @param maxdepth maximum amount of recursions allowed for token transfer transaction tree searches. Highly recommended to keep the default value to avoid stack overflows.
//template <class V>
UniValue TokenOwners(uint256 tokenid, int64_t minbalance, int64_t maxdepth);

/// Returns tokenids of tokens that the specified pubkey is in possession of.
/// @param pk pubkey to check for tokens
/// @param minbalance minimum balance of tokens for pubkey required for its id to be added to the array. If 0, lists ids of every token that the pubkey ever has/had
//template <class V>
UniValue TokenInventory(const CPubKey pk, int64_t minbalance);

#endif
