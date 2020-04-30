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

#ifndef CC_COMMITMENTS_H
#define CC_COMMITMENTS_H

#include "CCinclude.h"

#define AGREEMENTCC_VERSION 1
#define CC_TXFEE 10000
#define CC_MARKER_VALUE 10000

uint8_t DecodeAgreementOpRet(const CScript scriptPubKey);
CScript EncodeAgreementProposalOpRet(uint8_t version, uint8_t proposaltype, std::vector<uint8_t> srcpub, std::vector<uint8_t> destpub, std::vector<uint8_t> arbitratorpk, int64_t payment, int64_t arbitratorfee, int64_t depositval, uint256 datahash, uint256 agreementtxid, uint256 prevproposaltxid, std::string info);
uint8_t DecodeAgreementProposalOpRet(CScript scriptPubKey, uint8_t &version, uint8_t &proposaltype, std::vector<uint8_t> &srcpub, std::vector<uint8_t> &destpub, std::vector<uint8_t> &arbitratorpk, int64_t &payment, int64_t &arbitratorfee, int64_t &depositval, uint256 &datahash, uint256 &agreementtxid, uint256 &prevproposaltxid, std::string &info);
CScript EncodeAgreementProposalCloseOpRet(uint8_t version, uint256 proposaltxid, std::vector<uint8_t> srcpub);
uint8_t DecodeAgreementProposalCloseOpRet(CScript scriptPubKey, uint8_t &version, uint256 &proposaltxid, std::vector<uint8_t> &srcpub);
CScript EncodeAgreementSigningOpRet(uint8_t version, uint256 proposaltxid);
uint8_t DecodeAgreementSigningOpRet(CScript scriptPubKey, uint8_t &version, uint256 &proposaltxid);
CScript EncodeAgreementUpdateOpRet(uint8_t version, uint256 agreementtxid, uint256 proposaltxid);
uint8_t DecodeAgreementUpdateOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &proposaltxid);
CScript EncodeAgreementCloseOpRet(uint8_t version, uint256 agreementtxid, uint256 proposaltxid);
uint8_t DecodeAgreementCloseOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &proposaltxid);
CScript EncodeAgreementDisputeOpRet(uint8_t version, uint256 agreementtxid, std::vector<uint8_t> srcpub, uint256 datahash);
uint8_t DecodeAgreementDisputeOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, std::vector<uint8_t> &srcpub, uint256 &datahash);
CScript EncodeAgreementDisputeResolveOpRet(uint8_t version, uint256 agreementtxid, std::vector<uint8_t> rewardedpubkey);
uint8_t DecodeAgreementDisputeResolveOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, std::vector<uint8_t> &rewardedpubkey);
CScript EncodeAgreementUnlockOpRet(uint8_t version, uint256 agreementtxid, uint256 exchangetxid);
uint8_t DecodeAgreementUnlockOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &exchangetxid);

bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

bool GetAcceptedProposalOpRet(CTransaction tx, uint256 &proposaltxid, CScript &opret);
bool ValidateProposalOpRet(CScript opret, std::string &CCerror);
bool CompareProposals(CScript proposalopret, uint256 refproposaltxid, std::string &CCerror);
bool IsProposalSpent(uint256 proposaltxid, uint256 &spendingtxid, uint8_t &spendingfuncid);
bool GetAgreementInitialData(uint256 agreementtxid, uint256 &proposaltxid, std::vector<uint8_t> &sellerpk, std::vector<uint8_t> &clientpk, std::vector<uint8_t> &arbitratorpk, int64_t &firstarbitratorfee, int64_t &deposit, uint256 &firstdatahash, uint256 &refagreementtxid, std::string &firstinfo);
bool GetLatestAgreementUpdate(uint256 agreementtxid, uint256 &latesttxid, uint8_t &funcid);
void GetAgreementUpdateData(uint256 updatetxid, std::string &info, uint256 &datahash, int64_t &arbitratorfee, int64_t &depositsplit, int64_t &revision);

UniValue AgreementCreate(const CPubKey& pk, uint64_t txfee, std::string info, uint256 datahash, std::vector<uint8_t> destpub, std::vector<uint8_t> arbitrator, int64_t payment, int64_t arbitratorfee, int64_t deposit, uint256 prevproposaltxid, uint256 refagreementtxid);
UniValue AgreementUpdate(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, std::string info, uint256 datahash, int64_t payment, uint256 prevproposaltxid, int64_t newarbitratorfee);
UniValue AgreementClose(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, std::string info, uint256 datahash, int64_t depositcut, int64_t payment, uint256 prevproposaltxid);
UniValue AgreementStopProposal(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid);
UniValue AgreementAccept(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid);
UniValue AgreementDispute(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash);
UniValue AgreementResolve(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, std::vector<uint8_t> rewardedpubkey);

UniValue AgreementInfo(const CPubKey& pk, uint256 txid);
UniValue AgreementUpdateLog(uint256 agreementtxid, int64_t samplenum, bool backwards);
UniValue AgreementList();

#endif
