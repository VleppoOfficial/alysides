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

#include "CCPawnshop.h"

/*
Asset Loans module - preliminary design (not final, subject to change during build)

User Flow

- Lender uses assetloancreate to offer an amount of funds for the specified tokens and define a repayment schedule, while putting up the offered funds into escrow
- Borrower uses assetloantakeout to accept the conditions of the loan, and putting up the specified tokens into escrow as collateral while taking the funds stored by the assetloancreate transaction
- Borrower uses assetloanfund to add funds to the loan escrow as repayment
- Lender may use assetloanclaim to claim repaid funds from the loan escrow before the final repayment is scheduled
- Borrower uses assetloanredeem to complete the loan and redeem the collateral after all repayments have been made
- Lender may use assetloanrepo to repossess the collateral if the borrower defaults (i.e. has not repaid enough funds in time according to the repayment schedule)
- Lender may cancel the loan before it starts using assetloancancel, or forgive the loan after it starts by using assetloanredeem
- A list of sorted loans related to the user can be retrieved by assetloanlist
- Information on a specific loan can be retrieved by assetloaninfo 
RPC List

assetloancreate destpub offeramount flags {tokenid1:amount1,tokenid2:amount2,...}  {height1:amount1,height2:amount2,...} [prevloantxid] [agreementtxid]
assetloancancel loantxid
assetloantakeout loantxid
assetloanfund loantxid amount
assetloanclaim loantxid
assetloanredeem loantxid
assetloanrepo loantxid
assetloanlist [all|proposed|active|cancelled|repoed|redeemed|amended]
assetloaninfo loantxid 
assetloancreate flags list

ALF_AGREEMENTUNLOCK - loan can be used to close Agreements if successful
ALF_ALLOWLENDERUSE - allows lender to perform operations using the collateral with modules that are supported (Token Tags)
ALF_ALLOWBORROWERUSE - allows borrower to perform operations using the collateral with modules that are supported (Token Tags)
ALF_NOCLAIMFUND - disables assetloanclaim rpc for this loan
ALF_NOEARLYREDEEM - disables assetloanredeem for borrower until all loan due dates have passed
ALF_NOEXTERNALFUNDS - only fund transactions signed by borrower pubkey are valid
Asset Loans transaction types description

create:
vins.0+: normal inputs
vout.0: amount of offered funds to mutual CC 1of2 address
vout.1: CC marker to srcpub address
vout.2: CC marker to destpub address
vout.3: normal output for change (if any)
vout.n-1: opreturn [EVAL_ASSETLOANS] ['c'] [version] [srcpub] [destpub] [offeramount] [flags] [prevloantxid] [agreementtxid] [tokenlistsize] [tokenid1:amount1] [tokenid...:amount...] [schedulesize] [height1:amount1] [height...:amount...]

cancel:
vin.0: normal input
vin.1: CC input from create vout.0
vout.0: normal output for refund + change (if any)
vout.n-1: opreturn [EVAL_ASSETLOANS] ['x'] [version] [loantxid]
 
takeout:
vin.0: normal input
vin.1: CC input from create vout.0
vins.2+: tokens from borrower's address or previous loan's CC 1of2 token address to be used as collateral
vins.m+ (m=2+tokenlistsize): coins from previous loan CC 1of2 address (if any)
vout.0: CC marker + surplus coins to mutual CC 1of2 address
vout.1+: tokens to mutual CC token 1of2 address
vouts.o+ (o=1+tokenlistsize): CC output for token change (if any)
vout.n-2: vin.1 + vins.m+ coins & normal output for change (if any)
vout.n-1: opreturn [EVAL_ASSETLOANS] ['t'] [version] [loantxid]

fund:
vins.0+: normal inputs
vout.0: coins to mutual CC 1of2 address
vout.1: normal output for change (if any)
vout.n-1: opreturn [EVAL_ASSETLOANS] ['f'] [version] [loantxid] [amount] [srcpub] 
claim:
vin.0: normal input
vin.1+: CC inputs from mutual CC 1of2 address
vout.0: CC change back to mutual CC 1of2 address
vout.1: coins from vin.1+ & normal output for change (if any)
vout.n-1: opreturn [EVAL_ASSETLOANS] ['F'] [version] [loantxid] [amount]
 
redeem:
vin.0: normal input
vin.1: CC input from takeout vout.0
vins.2+: tokens from loan CC token 1of2 address
vins.m+ (m=2+tokenlistsize): coins from loan CC 1of2 address
vout.0+: tokens to borrower's CC token address
vouts.o+ (o=tokenlistsize): normal output to lender's address (if any)
vout.n-2: normal output for change (if any)
vout.n-1: opreturn [EVAL_ASSETLOANS] ['r'] [version] [loantxid] [srcpub]

repo:
vin.0: normal input
vin.1: CC input from takeout vout.0
vins.2+: tokens from loan CC token 1of2 address
vins.m+ (m=2+tokenlistsize): coins from loan CC 1of2 address
vout.0+: tokens to lender's CC token address
vouts.o+ (o=tokenlistsize): normal output to lender's address (if any)
vout.n-2: normal output for change (if any)
vout.n-1: opreturn [EVAL_ASSETLOANS] ['R'] [version] [loantxid]

insert:
types of txes
tx vins/vouts
consensus code flow
function list
	what these functions do, params, outputs
functions that explicitly use tokens functions (need to separate them, in case tokens gets updated again)
RPC impl
helper funcs, if needed
*/

bool PawnshopValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	return eval->Invalid("not supported yet");
}