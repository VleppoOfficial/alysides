//Header file for Agreements


#ifndef CC_AGREEMENTS_H
#define CC_AGREEMENTS_H

#include "CCinclude.h"

//bool AgreementsValidate(struct CCcontract_info* cpAgreement, Eval* eval, const CTransaction& tx, uint32_t nIn);
std::string CreateProposal(int64_t txfee, int64_t duration, uint256 assetHash);
UniValue InitialProposalInfo(uint256 proposalid);
//UniValue InitialProposalList();

#endif