#include "CCagreement.h"

#ifndef IS_CHARINSTR
#define IS_CHARINSTR(c, str) (std::string(str).find((char)(c)) != std::string::npos)
#endif

//Encoder and Decoder for ÍnitialProposal

//Encoder for Proposals
CScript EncodeValidateProposalopret(std::vector<uint8_t> origpubkey, int64_t duration, uint256 assetHash, vscript_t vopretNonfungible)
{
    std::vector<std::pair<uint8_t, vscript_t>> oprets;

    if (!vopretNonfungible.empty())
        oprets.push_back(std::make_pair(OPRETID_NONFUNGIBLEDATA, vopretNonfungible));
    return EncodeValidateProposalopret(origpubkey, duration, assetHash, oprets);
}
//
CScript EncodeValidateProposalopret(std::vector<uint8_t> origpubkey, int64_t duration, uint256 assetHash, std::vector<std::pair<uint8_t, vscript_t>> oprets)
{
    CScript opret;
    uint8_t evalcode = EVAL_AGREEMENT;
    uint8_t funcid = 'p'; // override the param



    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << origpubkey << duration << assetHash);

    //kig på dette
    for (auto o : oprets) {
           if (o.first != 0) {
               ss << (uint8_t)o.first;
               ss << o.second;
           }
       });

    return (opret);
}

//Decoder for Proposals
uint8_t DecodeInitialProposalOpret(const CScript& scriptPubKey, std::vector<uint8_t>& origpubkey, int64_t& duration, uint256& assetHash)
{
    std::vector<std::pair<uint8_t, vscript_t>> opretsDummy;
    return DecodeInitialProposalOpret(scriptPubKey, origpubkey, duration, assetHash);
}
//
uint8_t DecodeInitialProposalOpret(const CScript& scriptPubKey, std::vector<uint8_t>& origpubkey, int64_t& duration, uint256& assetHash, std::vector<std::pair<uint8_t, vscript_t>>& oprets)
{
    std::vector<uint8_t> vopret, vblob;
    uint8_t dummyEvalcode, funcid, opretId = 0;

    GetOpReturnData(scriptPubKey, vopret);
    oprets.clear();
    if (vopret.size() > 2 && vopret.begin()[0] == EVAL_AGREEMENTS && vopret.begin()[1] == 'p')
	{
        if (E_UNMARSHAL(
                vopret, ss >> dummyEvalcode; ss >> funcid; ss >> origpubkey; ss >> duration; ss >> assetHash;
                while (!ss.eof()) 
				{
                    ss >> opretId;
                    if (!ss.eof())
					{
                        ss >> vblob;
                        oprets.push_back(std::make_pair(opretId, vblob));
                    }
                }))
            /* if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> e; ss >> f; ss >> pk) != 0 && e == EVAL_AGREEMENTS) {
            return (funcid);*/ //det er måske denne her der skal bruges
        {
            return (funcid);
        }
    }
    LOGSTREAM((char*)"ccagreements", CCLOG_INFO, stream << "DecodeInitialProposalOpret() incorrect proposal opret" << std::endl);
    return (uint8_t)0;
}


//validate - need more work but isn't needed yet, but when work begins on the accepted proposal it will need to be implementet
//bool AgreementsValidate(struct CCcontract_info* cpAgreement, Eval* eval, const CTransaction& tx, uint32_t nIn)
//{
//    int32_t numvins = tx.vin.size();
//    int32_t numvouts = tx.vout.size();
//}

// Create the InitialProposal
std::string InitialProposal(int64_t txfee, int64_t duration, uint256 assetHash)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_AGREEMENTS);

    if (txfee == 0)
        txfee = 10000;

    CPubKey mypk = pubkey2pk(Mypubkey());

    //We use a function in the CC SDK, AddNormalinputs, to add the normal inputs to the mutable transaction.
	// find the equlivant to tokensupply for proposal - also if it's even needed or if it can be removed/modified.
    if (AddNormalinputs(mtx, mypk, 2 * txfee, 64) > 0)
	{
  //      // TotalPubkeyNormalInputs returns total of normal inputs signed with this pubkey
  //      int64_t mypkInputs = TotalPubkeyNormalInputs(mtx, mypk);
  //      if (mypkInputs < tokensupply) 
		//{
  //          // check that amount are really issued with mypk (because in the wallet there maybe other privkeys)
  //          CCerror = "some inputs signed not with -pubkey=pk";
  //          return std::string("");
  //      }

       // uint8_t destEvalCode = EVAL_AGREEMENTS; // not used
        mtx.vout.push_back(MakeCC1vout(cp->evalcode, txfee, mypk));

        return (FinalizeCCTx(0, cp, mtx, mypk, txfee, EncodeValidateProposalopret(Mypubkey(), duration, assetHash)));
    }

    CCerror = "cant find normal inputs";
    LOGSTREAM((char*)"ccagreements", CCLOG_INFO, stream << "InitialProposal() " << CCerror << std::endl);
    return std::string("");
}

//ProposalInfo
UniValue InitialProposalInfo(uint256 proposalid)
{
    UniValue result(UniValue::VOBJ);
    std::vector<std::pair<uint8_t, vscript_t>> oprets;
    uint256 hashBlock;
    CTransaction proposalbaseTx; x	
    int64_t duration;
    uint256 assetHash;
    struct CCcontract_info *cpProposal, proposalCCinfo;

    cpProposal = CCinit(&proposalCCinfo, EVAL_AGREEMENTS);

    if (!GetTransaction(tokenid, proposalbaseTx, hashBlock, false)) {
        fprintf(stderr, "InitialProposalInfo() cant find proposalid\n");
        result.push_back(Pair("result", "error"));
        result.push_back(Pair("error", "cant find proposalid"));
        return (result);
    }

    if (hashBlock.IsNull()) {
        result.push_back(Pair("result", "error"));
        result.push_back(Pair("error", "the transaction is still in mempool"));
        return (result);
    }

    result.push_back(Pair("result", "success"));
    result.push_back(Pair("proposalid", proposalid.GetHex()));
    result.push_back(Pair("owner", HexStr(origpubkey)));

    result.push_back(Pair("duration", duration));
    result.push_back(Pair("assetHash", assetHash));

    //maybe calculation for duration like we do with tokens expiretime

    return result;
}

//ProposalList
UniValue InitialProposalList()
{
    UniValue result(UniValue::VARR);
    std::vector<std::pair<CAddressIndexKey, CAmount>> addressIndex;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue>> addressIndexCCMarker;

    struct CCcontract_info *cp, C;
    uint256 txid, hashBlock;
    CTransaction vintx;
    std::vector<uint8_t> origpubkey;
    int64_t duration;
    uint256 assetHash;


    cp = CCinit(&C, EVAL_AGREEMENTS);

    auto addProposalId = [&](uint256 txid) {
        if (GetTransaction(txid, vintx, hashBlock, false) != 0) {
            if (vintx.vout.size() > 0 && DecodeInitialProposalOpret(vintx.vout[vintx.vout.size() - 1].scriptPubKey, origpubkey, duration, assetHash) != 0) {
                result.push_back(txid.GetHex());
            }
        }
    };

    SetCCtxids(addressIndex, cp->normaladdr, false); // find by old normal addr marker
    for (std::vector<std::pair<CAddressIndexKey, CAmount>>::const_iterator it = addressIndex.begin(); it != addressIndex.end(); it++) {
        addProposalId(it->first.txhash);
    }

    SetCCunspents(addressIndexCCMarker, cp->unspendableCCaddr, true); // find by burnable validated cc addr marker
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue>>::const_iterator it = addressIndexCCMarker.begin(); it != addressIndexCCMarker.end(); it++) {
        addProposalId(it->first.txhash);
    }

    return (result);
}
