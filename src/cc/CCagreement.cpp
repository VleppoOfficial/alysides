#include "CCagreement.h"

//opret encode / decode
CScript custom_opret(uint8_t funcid, CPubKey pk)
{
    CScript opret;
    uint8_t evalcode = EVAL_CUSTOM;
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << pk);
    return (opret);
}

uint8_t custom_opretdecode(CPubKey& pk, CScript scriptPubKey)
{
    std::vector<uint8_t> vopret;
    uint8_t e, f;
    GetOpReturnData(scriptPubKey, vopret);
    if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> e; ss >> f; ss >> pk) != 0 && e == EVAL_CUSTOM) {
        return (f);
    }
    return (0);
}


//validate
bool AgreementsValidate(struct CCcontract_info* cpAgreement, Eval* eval, const CTransaction& tx, uint32_t nIn)
{
    int32_t numvins = tx.vin.size();
    int32_t numvouts = tx.vout.size();
}
// Create the InitialProposal
std::string InitialProposal(int64_t txfee, int64_t duration, uint256 assetHash )
{


    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_TOKENS);

    if (txfee == 0)
        txfee = 10000;

	CPubKey mypk = pubkey2pk(Mypubkey());



// kig på dette
if (AddNormalinputs(mtx, mypk, tokensupply + 2 * txfee, 64) > 0)
{
    int64_t mypkInputs = TotalPubkeyNormalInputs(mtx, mypk);
    if (mypkInputs < tokensupply)
	{
        // check that tokens amount are really issued with mypk (because in the wallet there maybe other privkeys)
        CCerror = "some inputs signed not with -pubkey=pk";
        return std::string("");
    }
    uint8_t destEvalCode = EVAL_TOKENS;
    //If nonfungibleData exists, eval code is set to the one specified within nonfungibleData
    if (nonfungibleData.size() > 0)
        destEvalCode = nonfungibleData.begin()[0];

    // vout0 is a marker vout
    // NOTE: we should prevent spending fake-tokens from this marker in IsTokenvout():
    mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, txfee, GetUnspendable(cp, NULL))); // new marker to token cc addr, burnable and validated, vout pos now changed to 0 (from 1)
                                                                                   // vout1 issues the tokens to mypk
    mtx.vout.push_back(MakeTokensCC1vout(destEvalCode, tokensupply, mypk));
    // vout2 is a baton vout containing arbitrary updatable data
    CScript batonopret = EncodeTokenUpdateCCOpRet(assetHash, value, ccode, "");
    std::vector<std::vector<unsigned char>> vData = std::vector<std::vector<unsigned char>>();
    if (makeCCopret(batonopret, vData)) 
	{
        mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, 10000, GetUnspendable(cp, NULL), &vData)); // BATON_VOUT
        //fprintf(stderr, "vout size2.%li\n", mtx.vout.size());
        return (FinalizeCCTx(0, cp, mtx, mypk, txfee, EncodeTokenCreateOpRet(Mypubkey(), name, description, ownerPerc, tokenType, referenceTokenId, expiryTimeSec, nonfungibleData)));
    } else 
	{
        CCerror = "couldnt embed updatable data to baton vout";
        LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
        return std::string("");
    }
}
CCerror = "cant find normal inputs";
LOGSTREAM((char*)"cctokens", CCLOG_INFO, stream << "CreateToken() " << CCerror << std::endl);
return std::string("");
}

UniValue ProposalList()
{
}
