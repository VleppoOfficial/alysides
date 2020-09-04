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

/*
The Pawnshop CC enables private token exchanges between 2 pubkeys (since the Assets CC is for public orders only) using a dual 1of2 address setup, 
one for coins and the other for tokens.
This design allows for three main use cases:
	
	* Standard exchange of coins and tokens between two parties. 
	* Settlement of agreements created with Agreements CC (either milestone or final settlements, using the agreementunlock rpc and the PTF_REQUIREUNLOCK flag)
	* Secured loans with borrower's tokens used as collateral (TBD, NOT IMPLEMENTED YET)

The following RPCs are a proof of concept and will be implemented at some point in the future:
	pawnshopschedule
		Creates a loan repayment schedule. This must be executed by the lender (coin supplier) pubkey before the coins in the coin 1of2 address can be borrowed.
	pawnshopborrow
		Sends coins from coin 1of2 address as normal output to the borrower's wallet, and initiates the loan. Only available to the borrower (token supplier) pubkey. Requires
		definition of a scheduletxid created from the pawnshopschedule rpc - this will be the "official" repayment schedule of the loan.
	pawnshopseize
		Claims the tokens from the token 1of2 address and sends them to lender's wallet. Only available to the lender (coin supplier) pubkey, if there is a loan in progress and
		the due date specified in the loan repayment schedule has passed.
	pawnshopredeem
		Claims specified amount of coins from coin 1of2 address and sends them to lender's wallet, as long as the amount doesn't exceed what is specified in the loan repayment 
		schedule for that specific date. (more details on how this will work TBD)
*/

#define PAWNSHOPCC_VERSION 1
#define PAWNSHOPCC_MAXVINS 500
#define CC_TXFEE 10000
#define CC_MARKER_VALUE 10000
#define CC_BATON_VALUE 10000

#include "CCinclude.h"

enum EPawnshopTypeFlags
{
	PTF_REQUIREUNLOCK = 1, // 0b00000001 - when set, pawnshop instance requires usage of agreementunlock before pawnshopexchange can be used. Requires valid agreementtxid defined.
	PTF_NOLOAN = 2, // 0b00000010 - when set, any loan-related rpcs like pawnshopborrow are disabled. Currently mandatory as loan functionality isn't built yet.
	PTF_NOTRADE = 4, // 0b00000100 - when set, disables pawnshopexchange before a loan is initiated. Currently always disabled.
	// more flags may be introduced in the future
};

enum EPawnshopInputsFlags
{
	PIF_COINS = 0,
	PIF_TOKENS = 1,
};

/// Encodes op_return data required for a valid Pawnshop instance creation transaction into a CScript object, and appends the 'c' function id.
/// @param version version of data structure
/// @param name name of the instance (max 32 chars)
/// @param tokensupplier token supplier's pubkey object
/// @param coinsupplier coin supplier's pubkey object
/// @param pawnshopflags optional flags for the instance
/// @param numtokens amount of tokens required
/// @param numcoins amount of coins required
/// @param agreementtxid agreement to link the Pawnshop instance to
/// @returns the encoded CScript object.
CScript EncodePawnshopCreateOpRet(uint8_t version,std::string name,CPubKey tokensupplier,CPubKey coinsupplier,uint32_t pawnshopflags,uint256 tokenid,int64_t numtokens,int64_t numcoins,uint256 agreementtxid);

/// Decodes op_return data required for a valid Pawnshop instance creation transaction (containing 'c' function id) from a CScript object.
/// @param scriptPubKey op_return data object
/// @param version version of data structure
/// @param name name of the instance (max 32 chars)
/// @param tokensupplier token supplier's pubkey object
/// @param coinsupplier coin supplier's pubkey object
/// @param pawnshopflags optional flags for the instance
/// @param numtokens amount of tokens required
/// @param numcoins amount of coins required
/// @param agreementtxid agreement to link the Pawnshop instance to
/// @returns the function id found within the CScript object. If data malformed or function is invalid, returns (uint8_t)0
uint8_t DecodePawnshopCreateOpRet(CScript scriptPubKey,uint8_t &version,std::string &name,CPubKey &tokensupplier,CPubKey &coinsupplier,uint32_t &pawnshopflags,uint256 &tokenid,int64_t &numtokens,int64_t &numcoins,uint256 &agreementtxid);

// not implemented yet //
//CScript EncodePawnshopScheduleOpRet(uint8_t version,uint256 createtxid,int64_t principal,uint64_t duedate,bool bRelative);
//uint8_t DecodePawnshopScheduleOpRet(CScript scriptPubKey,uint8_t &version,uint256 &createtxid,int64_t &principal,uint64_t &duedate,bool &bRelative);
// not implemented yet //

/// Encodes op_return data required for a generic Pawnshop transaction into a CScript object, and appends the specified function id.
/// @param funcid function id of the transaction
/// @param version version of data structure
/// @param createtxid id of the relevant Pawnshop instance
/// @param tokenid id of token of the instance
/// @param tokensupplier token supplier's pubkey object
/// @param coinsupplier coin supplier's pubkey object
/// @returns the encoded CScript object.
CScript EncodePawnshopOpRet(uint8_t funcid,uint8_t version,uint256 createtxid,uint256 tokenid,CPubKey tokensupplier,CPubKey coinsupplier);

/// Decodes op_return data required for a generic Pawnshop transaction from a CScript object.
/// @param scriptPubKey op_return data object
/// @param version version of data structure
/// @param createtxid id of the relevant Pawnshop instance
/// @param tokenid id of token of the instance
/// @returns the function id found within the CScript object. If data malformed or function is invalid, returns (uint8_t)0.
uint8_t DecodePawnshopOpRet(const CScript scriptPubKey,uint8_t &version,uint256 &createtxid,uint256 &tokenid);

/// Main validation code of the Pawnshop CC. Is triggered when a transaction spending one or more CC outputs marked with the EVAL_PAWNSHOP eval code is broadcasted to the node network or when a block containing such a transaction is received by a node.
/// @param cp CCcontract_info object with Pawnshop CC variables (global CC address, global private key, etc.)
/// @param eval pointer to the CC dispatching object
/// @param tx the transaction to be validated
/// @param nIn not used here
/// @returns true if transaction is valid, otherwise false or calls eval->Invalid().
bool PawnshopValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);

/// Helper function that determines if the specified transaction CC output is marked with the EVAL_PAWNSHOP eval code.
/// @param cp CCcontract_info object with Pawnshop CC variables (global CC address, global private key, etc.)
/// @param tx the transaction to check for outputs
/// @param mode whether to check for a token or coin output
/// @param tokensupplier token supplier's pubkey object
/// @param coinsupplier coin supplier's pubkey object
/// @returns value of the output if it is a Pawnshop output, otherwise 0.
int64_t IsPawnshopvout(struct CCcontract_info *cp,const CTransaction& tx,bool mode,CPubKey tokensupplier,CPubKey coinsupplier,int32_t v);

/// Helper function that checks if a Pawnshop transaction's inputs match its outputs (all coins & tokens from the Pawnshop escrow are sent to their correct destinations). In addition, it will check if the Pawnshop creation transaction IDs in all relevant op_return data are matched, to prevent direct transfer of from one Pawnshop instance to another.
/// @param cp CCcontract_info object with Pawnshop CC variables (global CC address, global private key, etc.)
/// @param eval pointer to the CC dispatching object
/// @param tx the transaction to check
/// @param coininputs total amount of coin CC inputs found 
/// @param tokeninputs total amount of token CC inputs found 
/// @returns true if the inputs & outputs match, otherwise false or calls eval->Invalid().
bool PawnshopExactAmounts(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, int64_t &coininputs, int64_t &tokeninputs);

/// Helper function that retrieves the id of the latest transaction containing an unspent baton CC output, and its function id. Currently only used for checking if the Pawnshop instance is closed or not.
/// @param createtxid the id of the Pawnshop instance creation transaction
/// @param latesttxid the id of the latest transaction
/// @param funcid the function id of the latest transaction
/// @returns true if the transaction id was retrieved successfully, otherwise false.
bool GetLatestPawnshopTxid(uint256 createtxid, uint256 &latesttxid, uint8_t &funcid);

/// Helper function that attempts to find the id of the latest transaction in the Pawnshop instance baton pattern with the specified function id. Currently only used for checking if the Pawnshop instance is closed or not.
/// @param createtxid the id of the Pawnshop instance creation transaction
/// @param type the function id to check for
/// @param typetxid the id of the latest transaction with the specified function id, if it exists
/// @returns true if the search was completed successfully, otherwise false.
bool FindPawnshopTxidType(uint256 createtxid, uint8_t type, uint256 &typetxid);

/// Helper function that checks if the specified Pawnshop instance's linked agreement has its deposit unlocked and sent to the instance's escrow.
/// @param createtxid the id of the Pawnshop instance creation transaction
/// @returns -1 if no agreementtxid in the instance is defined, or deposit is not unlocked yet, otherwise returns amount sent to the Pawnshop instance's escrow.
int64_t CheckDepositUnlockCond(uint256 createtxid);

/// Helper function that validates the contents of a Pawnshop instance creation CTransaction object. Used to validate Pawnshop instance creation transactions in RPC implementations and in validation code, since they are not validated by default.
/// @param opentx the transaction object to validate
/// @param CCerror error message if the transaction object is invalid
/// @returns true if the CTransaction object is valid, otherwise false.
bool ValidatePawnshopCreateTx(CTransaction opentx, std::string &CCerror);

/// Helper function that retrieves the current coin or token balance of the relevant Pawnshop CC 1of2 address. This function only returns valid unspent outputs and their total value - it cannot add them to a mutable transaction.
/// @param cp CCcontract_info object with Pawnshop CC variables (global CC address, global private key, etc.)
/// @param createtx the Pawnshop instance creation transaction object
/// @param mode whether to check for token or coin inputs
/// @param validUnspentOutputs the key-value pair vector of valid unspent outputs found within the address, is used to add inputs to withdrawal txns
/// @see AddPawnshopInputs
/// @returns amount of the current coin or token balance of the relevant Pawnshop CC 1of2 address.
int64_t GetPawnshopInputs(struct CCcontract_info *cp,CTransaction createtx,bool mode,std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &validUnspentOutputs);

/// Helper function that retrieves the available coin or token balance of the relevant Pawnshop CC 1of2 address. This function adds valid unspent outputs found using GetPawnshopInputs to the specified mutable transaction.
/// @param cp CCcontract_info object with Pawnshop CC variables (global CC address, global private key, etc.)
/// @param mtx the mutable transaction object, typically a transaction currently bring constructed
/// @param createtx the Pawnshop instance creation transaction object
/// @param mode whether to check for token or coin inputs
/// @param maxinputs max amount of inputs to add before returning early
/// @see GetPawnshopInputs
/// @returns amount of the coin or token balance taken from the relevant Pawnshop CC 1of2 address.
int64_t AddPawnshopInputs(struct CCcontract_info *cp,CMutableTransaction &mtx,CTransaction createtx,bool mode,int32_t maxinputs);

// for definitions of RPC implementations see the help sections in the pawnshoprpc.cpp file
UniValue PawnshopCreate(const CPubKey& pk,uint64_t txfee,std::string name,CPubKey tokensupplier,CPubKey coinsupplier,int64_t numcoins,uint256 tokenid,int64_t numtokens,uint32_t pawnshopflags,uint256 agreementtxid);
UniValue PawnshopFund(const CPubKey& pk, uint64_t txfee, uint256 createtxid, int64_t amount);
UniValue PawnshopPledge(const CPubKey& pk, uint64_t txfee, uint256 createtxid);
UniValue PawnshopCancel(const CPubKey& pk, uint64_t txfee, uint256 createtxid);
UniValue PawnshopExchange(const CPubKey& pk, uint64_t txfee, uint256 createtxid);

// not implemented yet //
UniValue PawnshopSchedule(const CPubKey& pk, uint64_t txfee, uint256 createtxid, int64_t principal, int64_t duedate, bool bRelative);
UniValue PawnshopBorrow(const CPubKey& pk, uint64_t txfee, uint256 createtxid, uint256 scheduletxid);
UniValue PawnshopSeize(const CPubKey& pk, uint64_t txfee, uint256 createtxid);
// not implemented yet //

UniValue PawnshopInfo(const CPubKey& pk, uint256 createtxid);
UniValue PawnshopList(const CPubKey& pk);

#endif
