//Header file for Agreements


#ifndef CC_AGREEMENTS_H
#define CC_AGREEMENTS_H

#include "CCinclude.h"

bool AgreementsValidate(struct CCcontract_info* cpAgreement, Eval* eval, const CTransaction& tx, uint32_t nIn);
UniValue InitialProposalInfo(uint256 proposalid);
UniValue InitialProposalList();