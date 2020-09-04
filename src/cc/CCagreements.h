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

/*
The Agreements CC enables anyone to create a blockchain representation of a bilateral agreement.
An agreement created using this CC features, among other things, the ability to store the checksum of off-chain contract documents (or an oracletxid) to prevent tampering, 
a two party approval protocol for actions such as updates, terminations, and a dispute resolution system which utilizes an arbitrator, a mutually agreed upon third party.
To create an active contract between two parties, the seller must first use the agreementcreate RPC to create a proposal and designate a client pubkey, 
which will need to execute the agreementaccept RPC and sign the proposal.
*/

#ifndef CC_AGREEMENTS_H
#define CC_AGREEMENTS_H

#include "CCinclude.h"

#define AGREEMENTCC_VERSION 1
#define CC_TXFEE 10000
#define CC_MARKER_VALUE 10000

/// Decodes op_return data marked with EVAL_AGREEMENTS using a decoder function corresponding to the found function id.
/// @param scriptPubKey op_return data object
/// @returns function id of provided op_return data. If the function id is invalid or the data structure does not match specifications for its function id, returns (uint8_t)0. 
uint8_t DecodeAgreementOpRet(const CScript scriptPubKey);

/// Encodes op_return data required for a valid agreement proposal transaction into a CScript object, and appends the 'p' function id.
/// @param version version of data structure
/// @param proposaltype type of proposal. Available types: 'p' (new agreement), 'u' (update agreement), 'c' (close agreement)
/// @param srcpub byte vector containing pubkey of transaction creator
/// @param destpub byte vector containing pubkey of intended recipient of the proposal (required for 'u' and 'c' proposal types)
/// @param arbitratorpk byte vector containing pubkey of intended arbitrator of the proposed agreement
/// @param payment payment to srcpub required for valid proposal acceptance
/// @param arbitratorfee payment to arbitratorpk required for valid disputes of proposed agreement
/// @param depositval deposit required
/// @param datahash field for arbitrary sha256 hash
/// @param agreementtxid transaction id of a valid  agreement in the blockchain. Required for 'u' and 'c' proposal type data
/// @param prevproposaltxid transaction id of another proposal to be superseded by this proposal
/// @param name field for arbitrary data string
/// @returns the encoded CScript object.
CScript EncodeAgreementProposalOpRet(uint8_t version, uint8_t proposaltype, std::vector<uint8_t> srcpub, std::vector<uint8_t> destpub, std::vector<uint8_t> arbitratorpk, int64_t payment, int64_t arbitratorfee, int64_t depositval, uint256 datahash, uint256 agreementtxid, uint256 prevproposaltxid, std::string name);

/// Encodes op_return data required for a valid agreement proposal transaction into a CScript object, and appends the 'p' function id.
/// @param scriptPubKey op_return data object
/// @param version version of data structure
/// @param proposaltype type of proposal. Available types: 'p' (new agreement), 'u' (update agreement), 'c' (close agreement)
/// @param srcpub byte vector containing pubkey of transaction creator
/// @param destpub byte vector containing pubkey of intended recipient of the proposal (required for 'u' and 'c' proposal types)
/// @param arbitratorpk byte vector containing pubkey of intended arbitrator of the proposed agreement
/// @param payment payment to srcpub required for valid proposal acceptance
/// @param arbitratorfee payment to arbitratorpk required for valid disputes of proposed agreement
/// @param depositval deposit required
/// @param datahash field for arbitrary sha256 hash
/// @param agreementtxid transaction id of a valid  agreement in the blockchain. Required for 'u' and 'c' proposal type data
/// @param prevproposaltxid transaction id of another proposal to be superseded by this proposal
/// @param name field for arbitrary data string
/// @returns the function id found within the CScript object. If data malformed or function is invalid, returns (uint8_t)0.
uint8_t DecodeAgreementProposalOpRet(CScript scriptPubKey, uint8_t &version, uint8_t &proposaltype, std::vector<uint8_t> &srcpub, std::vector<uint8_t> &destpub, std::vector<uint8_t> &arbitratorpk, int64_t &payment, int64_t &arbitratorfee, int64_t &depositval, uint256 &datahash, uint256 &agreementtxid, uint256 &prevproposaltxid, std::string &name);

/// Encodes op_return data required for a valid agreement proposal closure transaction into a CScript object, and appends the 't' function id.
/// @param version version of data structure
/// @param proposaltxid transaction id of proposal to be closed
/// @param srcpub byte vector containing pubkey of transaction creator
/// @returns the encoded CScript object.
CScript EncodeAgreementProposalCloseOpRet(uint8_t version, uint256 proposaltxid, std::vector<uint8_t> srcpub);

/// Decodes op_return data required for a valid agreement proposal closure transaction (containing 't' function id) from a CScript object.
/// @param scriptPubKey op_return data object
/// @param version version of data structure
/// @param proposaltxid transaction id of proposal to be closed
/// @param srcpub byte vector containing pubkey of transaction creator
/// @returns the function id found within the CScript object. If data malformed or function is invalid, returns (uint8_t)0.
uint8_t DecodeAgreementProposalCloseOpRet(CScript scriptPubKey, uint8_t &version, uint256 &proposaltxid, std::vector<uint8_t> &srcpub);

/// Encodes op_return data required for a valid agreement proposal signing transaction into a CScript object, and appends the 'c' function id.
/// @param version version of data structure
/// @param proposaltxid transaction id of proposal to be accepted
/// @returns the encoded CScript object.
CScript EncodeAgreementSigningOpRet(uint8_t version, uint256 proposaltxid);

/// Decodes op_return data required for a valid agreement proposal closure transaction (containing 'c' function id) from a CScript object.
/// @param scriptPubKey op_return data object
/// @param version version of data structure
/// @param proposaltxid transaction id of proposal to be accepted
/// @returns the function id found within the CScript object. If data malformed or function is invalid, returns (uint8_t)0.
uint8_t DecodeAgreementSigningOpRet(CScript scriptPubKey, uint8_t &version, uint256 &proposaltxid);

/// Encodes op_return data required for a valid agreement update transaction into a CScript object, and appends the 'u' function id.
/// @param version version of data structure
/// @param agreementtxid transaction id of agreement to be updated
/// @param proposaltxid transaction id of update proposal to be accepted
/// @returns the encoded CScript object.
CScript EncodeAgreementUpdateOpRet(uint8_t version, uint256 agreementtxid, uint256 proposaltxid);

/// Decodes op_return data required for a valid agreement update transaction (containing 'u' function id) from a CScript object.
/// @param scriptPubKey op_return data object
/// @param version version of data structure
/// @param agreementtxid transaction id of agreement to be updated
/// @param proposaltxid transaction id of update proposal to be accepted
/// @returns the function id found within the CScript object. If data malformed or function is invalid, returns (uint8_t)0.
uint8_t DecodeAgreementUpdateOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &proposaltxid);

/// Encodes op_return data required for a valid agreement closure transaction into a CScript object, and appends the 's' function id.
/// @param version version of data structure
/// @param agreementtxid transaction id of agreement to be closed
/// @param proposaltxid transaction id of closure proposal to be accepted
/// @returns the encoded CScript object.
CScript EncodeAgreementCloseOpRet(uint8_t version, uint256 agreementtxid, uint256 proposaltxid);

/// Decodes op_return data required for a valid agreement closure transaction (containing 's' function id) from a CScript object.
/// @param scriptPubKey op_return data object
/// @param version version of data structure
/// @param agreementtxid transaction id of agreement that was closed
/// @param proposaltxid transaction id of closure proposal that was accepted
/// @returns the function id found within the CScript object. If data malformed or function is invalid, returns (uint8_t)0.
uint8_t DecodeAgreementCloseOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &proposaltxid);

/// Encodes op_return data required for a valid agreement dispute transaction into a CScript object, and appends the 'd' function id.
/// @param version version of data structure
/// @param agreementtxid transaction id of agreement in dispute
/// @param srcpub byte vector containing pubkey of transaction creator
/// @param datahash field for arbitrary sha256 hash
/// @returns the encoded CScript object.
CScript EncodeAgreementDisputeOpRet(uint8_t version, uint256 agreementtxid, std::vector<uint8_t> srcpub, uint256 datahash);

/// Decodes op_return data required for a valid agreement dispute transaction (containing 'd' function id) from a CScript object.
/// @param scriptPubKey op_return data object
/// @param version version of data structure
/// @param agreementtxid transaction id of agreement in dispute
/// @param datahash field for arbitrary sha256 hash
/// @returns the function id found within the CScript object. If data malformed or function is invalid, returns (uint8_t)0.
uint8_t DecodeAgreementDisputeOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, std::vector<uint8_t> &srcpub, uint256 &datahash);

/// Encodes op_return data required for a valid agreement dispute resolution transaction into a CScript object, and appends the 'r' function id.
/// @param version version of data structure
/// @param agreementtxid transaction id of agreement in dispute to be resolved
/// @param rewardedpubkey byte vector containing pubkey of the dispute "winner", according to the arbitrator
/// @returns the encoded CScript object.
CScript EncodeAgreementDisputeResolveOpRet(uint8_t version, uint256 agreementtxid, std::vector<uint8_t> rewardedpubkey);

/// Decodes op_return data required for a valid agreement dispute resolution transaction (containing 'u' function id) from a CScript object.
/// @param scriptPubKey op_return data object
/// @param version version of data structure
/// @param agreementtxid transaction id of agreement in dispute that was resolved
/// @param rewardedpubkey byte vector containing pubkey of the dispute "winner", according to the arbitrator
/// @returns the function id found within the CScript object. If data malformed or function is invalid, returns (uint8_t)0.
uint8_t DecodeAgreementDisputeResolveOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, std::vector<uint8_t> &rewardedpubkey);

/// Encodes op_return data required for a valid agreement unlock transaction into a CScript object, and appends the 'n' function id.
/// @param version version of data structure
/// @param agreementtxid transaction id of agreement to have its deposit unlocked
/// @param pawnshoptxid transaction id of the Pawnshop instance where the deposit funds are sent
/// @returns the encoded CScript object.
CScript EncodeAgreementUnlockOpRet(uint8_t version, uint256 agreementtxid, uint256 pawnshoptxid);

/// Decodes op_return data required for a valid agreement update transaction (containing 'n' function id) from a CScript object.
/// @param scriptPubKey op_return data object
/// @param version version of data structure
/// @param agreementtxid transaction id of agreement that had its deposit unlocked
/// @param pawnshoptxid transaction id of the Pawnshop instance where the deposit funds are sent
/// @returns the function id found within the CScript object. If data malformed or function is invalid, returns (uint8_t)0.
uint8_t DecodeAgreementUnlockOpRet(CScript scriptPubKey, uint8_t &version, uint256 &agreementtxid, uint256 &pawnshoptxid);

/// Main validation code of the Agreements CC. Is triggered when a transaction spending one or more CC outputs marked with the EVAL_AGREEMENTS eval code is broadcasted to the node network or when a block containing such a transaction is received by a node.
/// @param cp CCcontract_info object with Agreements CC variables (global CC address, global private key, etc.)
/// @param eval pointer to the CC dispatching object
/// @param tx the transaction to be validated
/// @param nIn not used here
/// @returns true if transaction is valid, otherwise false or calls eval->Invalid().
bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

/// Helper function that extracts the CScript op_return data object from the proposal transaction CTransaction object. Used to help extract required data for validating proposal acceptance transactions.
/// @param tx proposal transaction object
/// @param proposaltxid id of the proposal transaction
/// @param opret the CScript op_return data object
/// @returns true if the CScript object was retrieved successfully, otherwise false.
bool GetAcceptedProposalOpRet(CTransaction tx, uint256 &proposaltxid, CScript &opret);

/// Helper function that validates a CScript op_return data object of a proposal transaction. Used to validate proposal submit transactions in RPC implementations and in validation code, since they are not validated by default.
/// @param opret the CScript op_return data object
/// @param CCerror error message if the data object is invalid
/// @returns true if the CScript object is valid, otherwise false.
bool ValidateProposalOpRet(CScript opret, std::string &CCerror);

/// Helper function that validates a CScript op_return data object of a proposal transaction in reference to another existing proposal on the blockchain. Used to validate proposal transactions that amend other proposals in RPC implementations and in validation code.
/// @param proposalopret the CScript op_return data object
/// @param refproposaltxid the id of an existing proposal transaction for comparison
/// @param CCerror error message if the data object is invalid
/// @returns true if the CScript object is valid, otherwise false.
bool CompareProposals(CScript proposalopret, uint256 refproposaltxid, std::string &CCerror);

/// Helper function that validates a CScript op_return data object of a proposal transaction in reference to another existing proposal on the blockchain. Used to validate proposal transactions that amend other proposals in RPC implementations and in validation code.
/// @param proposaltxid the id of the proposal transaction
/// @param spendingtxid the id of the transaction that spent the baton, if it exists
/// @param spendingfuncid the function id of the spending transaction, if it exists
/// @returns true if the proposal baton is spent, otherwise false.
bool IsProposalSpent(uint256 proposaltxid, uint256 &spendingtxid, uint8_t &spendingfuncid);

/// Helper function that retrieves static data corresponding to the specified id of an agreement transaction.
/// @param agreementtxid the id of the agreement transaction
/// @param proposaltxid the id of the accepted proposal transaction of the agreement
/// @param sellerpk byte vector of the seller pubkey
/// @param clientpk byte vector of the client pubkey
/// @param arbitratorpk byte vector of the arbitrator pubkey
/// @param firstarbitratorfee arbitrator fee amount defined in the accepted proposal transaction
/// @param deposit deposit amount allocated for the agreement
/// @param firstdatahash arbitrary sha256 hash defined in the accepted proposal transaction
/// @param refagreementtxid id of the reference (master) agreement transaction
/// @param firstname arbitrary data string defined in the accepted proposal transaction
/// @returns true if the data was retrieved successfully, otherwise false.
bool GetAgreementInitialData(uint256 agreementtxid, uint256 &proposaltxid, std::vector<uint8_t> &sellerpk, std::vector<uint8_t> &clientpk, std::vector<uint8_t> &arbitratorpk, int64_t &firstarbitratorfee, int64_t &deposit, uint256 &firstdatahash, uint256 &refagreementtxid, std::string &firstname);

/// Helper function that retrieves the data related to a specific agreement update transaction. Since the data is stored within a CScript object of the accepted update proposal's transaction rather than the update acceptance transaction itself, this function can be used to retrieve the data easily with only an agreement update id as a known argument.
/// @param agreementtxid the id of the agreement transaction
/// @param latesttxid the id of the latest transaction
/// @param funcid the function id of the latest transaction
/// @returns true if the transaction id was retrieved successfully, otherwise false.
bool GetLatestAgreementUpdate(uint256 agreementtxid, uint256 &latesttxid, uint8_t &funcid);

/// Helper function that retrieves the data related to a specific agreement update transaction. Since the data is stored within a CScript object of the accepted update proposal's transaction rather than the update acceptance transaction itself, this function can be used to retrieve the data easily with only an agreement update id as a known argument.
/// @param updatetxid the id of the agreement update transaction
/// @param name arbitrary data string defined in the accepted update proposal transaction
/// @param datahash arbitrary sha256 hash defined in the accepted update proposal transaction
/// @param clientpk byte vector of the client pubkey
/// @param arbitratorfee arbitrator fee defined in the accepted update proposal transaction
/// @param depositsplit deposit split defined in the accepted update proposal transaction. Only used if the update proposal is of closure type ('s' function id)
/// @param revision revision number of the specified agreement update transaction
/// @returns true if the data was retrieved successfully, otherwise false.
void GetAgreementUpdateData(uint256 updatetxid, std::string &name, uint256 &datahash, int64_t &arbitratorfee, int64_t &depositsplit, int64_t &revision);

// for definitions of RPC implementations see the help sections in the pawnshoprpc.cpp file
UniValue AgreementCreate(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> destpub, std::vector<uint8_t> arbitrator, int64_t payment, int64_t arbitratorfee, int64_t deposit, uint256 prevproposaltxid, uint256 refagreementtxid);
UniValue AgreementUpdate(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, std::string name, uint256 datahash, int64_t payment, uint256 prevproposaltxid, int64_t newarbitratorfee);
UniValue AgreementClose(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, std::string name, uint256 datahash, int64_t depositcut, int64_t payment, uint256 prevproposaltxid);
UniValue AgreementStopProposal(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid);
UniValue AgreementAccept(const CPubKey& pk, uint64_t txfee, uint256 proposaltxid);
UniValue AgreementDispute(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 datahash);
UniValue AgreementResolve(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, std::vector<uint8_t> rewardedpubkey);
UniValue AgreementUnlock(const CPubKey& pk, uint64_t txfee, uint256 agreementtxid, uint256 pawnshoptxid);

UniValue AgreementInfo(uint256 txid);
UniValue AgreementUpdateLog(uint256 agreementtxid, int64_t samplenum, bool backwards);
UniValue AgreementProposals(CPubKey pk, uint256 agreementtxid);
UniValue AgreementSubcontracts(uint256 agreementtxid);
UniValue AgreementInventory(CPubKey pk);
UniValue AgreementSettlements(const CPubKey& pk, uint256 agreementtxid, bool bActiveOnly);
UniValue AgreementList();

#endif
