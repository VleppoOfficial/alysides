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
 Commitments stuff
 */

#ifndef CC_COMMITMENTS_H
#define CC_COMMITMENTS_H

#include "CCinclude.h"

#define COMMITMENTCC_VERSION 1
#define CC_TXFEE 10000
#define CC_MARKER_VALUE 10000
#define CC_RESPONSE_VALUE 20000

uint8_t DecodeCommitmentOpRet(const CScript scriptPubKey);
CScript EncodeCommitmentProposalOpRet(uint8_t proposaltype, std::vector<uint8_t> initiator, std::vector<uint8_t> receiver, std::vector<uint8_t> arbitrator, int64_t expiryTimeSec, int64_t arbitratorfee, int64_t prepayment, int64_t prepaymentcut, uint256 datahash, uint256 agreementtxid, uint256 prevproposaltxid, std::string name);
uint8_t DecodeCommitmentProposalOpRet(CScript scriptPubKey, uint8_t &proposaltype, std::vector<uint8_t> &initiator, std::vector<uint8_t> &receiver, std::vector<uint8_t> &arbitrator, int64_t &expiryTimeSec, int64_t &arbitratorfee, int64_t &prepayment, int64_t &prepaymentcut, uint256 &datahash, uint256 &agreementtxid, uint256 &prevproposaltxid, std::string &name);
CScript EncodeCommitmentProposalCloseOpRet(uint256 proposaltxid, std::vector<uint8_t> initiator);
uint8_t DecodeCommitmentProposalCloseOpRet(CScript scriptPubKey, uint256 &proposaltxid, std::vector<uint8_t> &initiator);
CScript EncodeCommitmentSigningOpRet(uint256 proposaltxid);
uint8_t DecodeCommitmentSigningOpRet(CScript scriptPubKey, uint256 &proposaltxid);
CScript EncodeCommitmentUpdateOpRet(uint256 agreementtxid, uint256 proposaltxid);
uint8_t DecodeCommitmentUpdateOpRet(CScript scriptPubKey, uint256 &agreementtxid, uint256 &proposaltxid);
CScript EncodeCommitmentDisputeOpRet(uint256 agreementtxid, uint256 disputehash);
uint8_t DecodeCommitmentDisputeOpRet(CScript scriptPubKey, uint256 &agreementtxid, uint256 &disputehash);
CScript EncodeCommitmentDisputeResolveOpRet(uint256 agreementtxid, uint256 disputetxid, std::vector<uint8_t> rewardedpubkey);
uint8_t DecodeCommitmentDisputeResolveOpRet(CScript scriptPubKey, uint256 &agreementtxid, uint256 &disputetxid, std::vector<uint8_t> &rewardedpubkey);

bool CommitmentsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

int64_t IsCommitmentsVout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v);

UniValue CommitmentPropose(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> buyer, std::vector<uint8_t> arbitrator, int64_t expiryTimeSec, int64_t arbitratorfee, int64_t prepayment, uint256 prevproposaltxid, uint256 refagreementtxid);
UniValue CommitmentRequestUpdate(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash, std::vector<uint8_t> newarbitrator, uint256 prevproposaltxid);
UniValue CommitmentRequestCancel(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash, uint64_t prepaymentcut, uint256 prevproposaltxid);
UniValue CommitmentCloseProposal(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid);
UniValue CommitmentAccept(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid);

UniValue CommitmentList();

#endif

