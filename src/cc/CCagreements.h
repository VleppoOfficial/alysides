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

#ifndef CC_AGREEMENTS_H
#define CC_AGREEMENTS_H

#include "CCinclude.h"

#define AGREEMENTCC_VERSION 1
#define CC_TXFEE 10000
#define CC_MARKER_VALUE 10000
#define CC_RESPONSE_VALUE 50000

enum EAgreementTxSearchFlags
{
    // used in AgreementList
    ASF_PROPOSALS = 1, // 0b00000001
    ASF_AGREEMENTS = 2, // 0b00000010
    ASF_ALL = ASF_PROPOSALS|ASF_AGREEMENTS, // 0b00000011
    // used in AgreementEventLog
    ASF_UPDATES = 4, // 0b00000100
    ASF_CLOSURES = 8, // 0b00001000
    ASF_DISPUTES = 16, // 0b00010000
    ASF_RESOLUTIONS = 32, // 0b00100000
    ASF_ALLEVENTS = ASF_UPDATES|ASF_CLOSURES|ASF_DISPUTES|ASF_RESOLUTIONS, // 0b00111100
};

/// Main validation code of the Agreements CC. Is triggered when a transaction spending one or more CC outputs marked with the EVAL_AGREEMENTS eval code is broadcasted to the node network or when a block containing such a transaction is received by a node.
/// @param cp CCcontract_info object with Agreements CC variables (global CC address, global private key, etc.)
/// @param eval pointer to the CC dispatching object
/// @param tx the transaction to be validated
/// @param nIn not used here
/// @returns true if transaction is valid, otherwise false or calls eval->Invalid().
bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

UniValue AgreementCreate(const CPubKey& pk,uint64_t txfee,CPubKey destpub,std::string agreementname,uint256 agreementhash,int64_t deposit,CPubKey arbitratorpub,int64_t disputefee,uint256 refagreementtxid,int64_t payment);
UniValue AgreementUpdate(const CPubKey& pk,uint64_t txfee,uint256 agreementtxid,uint256 agreementhash,std::string agreementname,int64_t payment);
UniValue AgreementClose(const CPubKey& pk,uint64_t txfee,uint256 agreementtxid,uint256 agreementhash,int64_t depositcut,std::string agreementname,int64_t payment);
UniValue AgreementStopProposal(const CPubKey& pk,uint64_t txfee,uint256 proposaltxid,std::string cancelinfo);
UniValue AgreementAccept(const CPubKey& pk,uint64_t txfee,uint256 proposaltxid);
UniValue AgreementDispute(const CPubKey& pk,uint64_t txfee,uint256 agreementtxid,std::string disputeinfo,bool bFinalDispute);
UniValue AgreementResolve(const CPubKey& pk,uint64_t txfee,uint256 disputetxid,int64_t depositcut,std::string resolutioninfo);
UniValue AgreementUnlock(const CPubKey& pk,uint64_t txfee,uint256 agreementtxid,uint256 unlocktxid);

UniValue AgreementInfo(uint256 txid);
UniValue AgreementEventLog(uint256 agreementtxid,uint8_t flags,int64_t samplenum,bool bReverse);
UniValue AgreementReferences(const CPubKey& pk,uint256 agreementtxid);
UniValue AgreementList(const CPubKey& pk,uint8_t flags,uint256 filtertxid);

#endif
