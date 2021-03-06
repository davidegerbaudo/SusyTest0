#include <cassert>
#include <cmath> // isnan
#include <cfloat> // FLT_MAX, FLT_MIN
#include <iomanip> // setw, setprecision
#include <sstream>      // std::ostringstream
#include "TCanvas.h"
#include "TSystem.h"
#include "TVector2.h"

#include "SusyTest0/SusySelection.h"
#include "SusyTest0/SusyPlotter.h"

#include "ChargeFlip/chargeFlip.h"
#include "SusyNtuple/MCWeighter.h"
#include "SusyTest0/EventFlags.h"
#include "SusyTest0/criteria.h"
#include "SusyTest0/kinematic.h"
#include "SusyTest0/utils.h"


using namespace std;
using namespace Susy;
namespace swh = susy::wh;
namespace swk = susy::wh::kin;

std::string SusySelection::WeightComponents::str() const
{
  std::ostringstream oss;
  oss<<" susynt: "<<susynt
     <<" lepSf: "<<lepSf
     <<" btag: "<<btag
     <<" trigger: "<<trigger
     <<" qflip: "<<qflip
     <<" fake: "<<fake;
  return oss.str();
}
//-----------------------------------------
SusySelection::SusySelection() :
  m_xsReader(NULL),
  m_tupleMaker("",""),
  m_writeTuple(false),
  m_debugThisEvent(false),
  m_outTupleFile(""),
  m_trigObj(NULL),
  m_useMCTrig(false),
  m_chargeFlip(NULL),
  m_mcWeighter(NULL),
  m_w(1.0),
  m_useXsReader(false),
  m_xsFromReader(-1.0),
  m_qflipProb(0.0),
  m_nInvalidQflip(0)
{
  resetAllCounters();
  setAnaType(Ana_2LepWH);
  setSelectTaus(true);
  initChargeFlipTool();
}
void SusySelection::Begin(TTree* /*tree*/)
{
  SusyNtAna::Begin(0);
  if(m_dbg) cout << "SusySelection::Begin" << endl;
  string period = "Moriond";
  bool useReweightUtils = false;
  m_trigObj = new DilTrigLogic(period, useReweightUtils);
  if(m_useMCTrig) m_trigObj->useMCTrigger();
  if( m_useXsReader ){
    m_xsReader = new XSReader();
    m_xsReader->setDebug(m_dbg);
    m_xsReader->LoadXSInfo();
  } // end if(m_useXsReader)
  if(m_writeTuple) {
      if(endswith(m_outTupleFile, ".root") && m_tupleMaker.init(m_outTupleFile, "SusySel"))
          cout<<"initialized ntuple file "<<m_outTupleFile<<endl;
      else {
          cout<<"cannot initialize ntuple file '"<<m_outTupleFile<<"'"<<endl;
          m_writeTuple = false;
      }
  }
}
//-----------------------------------------
void SusySelection::Init(TTree* tree)
{
    SusyNtAna::Init(tree);
    initMcWeighter(tree);
}
//-----------------------------------------
JetVector SusySelection::filterClJets(const JetVector &jets, JVFUncertaintyTool* jvfTool, SusyNtSys sys, AnalysisType anaType)
{
    JetVector oj;
    for(size_t i=0; i<jets.size(); ++i)
      if(SusyNtTools::isCentralLightJet(jets[i], jvfTool, sys, anaType)) oj.push_back(jets[i]);
    return oj;
}
//-----------------------------------------
Bool_t SusySelection::Process(Long64_t entry)
{
  m_printer.countAndPrint(cout);
  GetEntry(entry);
  clearObjects();
  cacheStaticWeightComponents();
  increment(n_readin, m_weightComponents);
  bool removeLepsFromIso(false), allowQflip(true);
  selectObjects(NtSys_NOM, removeLepsFromIso, TauID_medium);
  swh::EventFlags eventFlags(computeEventFlags());
  incrementCounters(eventFlags, m_weightComponents);
  if(eventFlags.failAny()) return kTRUE;
  m_debugThisEvent = susy::isEventInList(nt.evt()->event);

  const JetVector&   bj = m_baseJets;
  const LeptonVector& l = m_signalLeptons;
  if(l.size()>1) assignNonStaticWeightComponents(computeNonStaticWeightComponents(l, bj, susy::wh::WH_CENTRAL));
  else return false;
  VarFlag_t varsFlags = computeSsFlags(m_signalLeptons, m_signalTaus, m_signalJets2Lep, m_met, susy::wh::WH_CENTRAL, allowQflip);
  const SsPassFlags &ssf = varsFlags.second;
  incrementSsCounters(ssf, m_weightComponents);
  if(ssf.lepPt) {
      if(m_writeTuple) {
          double weight(m_weightComponents.product());
          unsigned int run(nt.evt()->run), event(nt.evt()->event);
          LeptonVector anyLep(getAnyElOrMu(nt));
          LeptonVector lowPtLep(subtract_vector(anyLep, m_baseLeptons));
          const Lepton *l0 = m_signalLeptons[0];
          const Lepton *l1 = m_signalLeptons[1];
          const JetVector clJets(SusySelection::filterClJets(m_signalJets2Lep, m_jvfTool, NtSys_NOM, m_anaType));
          m_tupleMaker.fill(weight, run, event, *l0, *l1, *m_met, lowPtLep, clJets);
      }
  }
  return kTRUE;
}
//-----------------------------------------
void SusySelection::Terminate()
{
    if(m_writeTuple) m_tupleMaker.close();
  SusyNtAna::Terminate();
  if(m_dbg) cout << "SusySelection::Terminate" << endl;
  dumpEventCounters();
  if(m_xsReader) delete m_xsReader;
  if(m_chargeFlip) delete m_chargeFlip;
  if(m_mcWeighter) delete m_mcWeighter;
  if(m_nInvalidQflip)
      cout<<"Number of invalid charge flip probabilities: "<<m_nInvalidQflip<<" (those were set to 0.0)"<<endl;
}
//-----------------------------------------
void SusySelection::increment(float counters[], const WeightComponents &wc)
{
  counters[kRaw ] += 1.0;
  counters[kEvt ] += wc.gen;
  counters[kPU  ] += wc.gen * wc.pileup;
  counters[kLSF ] += wc.gen * wc.lepSf;
  counters[kBtag] += wc.gen * wc.btag;
  counters[kTrig] += wc.gen * wc.trigger;
  counters[kAll ] += wc.product();
}
//-----------------------------------------
susy::wh::EventFlags SusySelection::computeEventFlags()
{
    swh::EventFlags f;
    if(m_dbg) cout << "SusySelection::selectEvent" << endl;
    int flag = nt.evt()->cutFlags[NtSys_NOM];
    const LeptonVector &bleps = m_baseLeptons;
    const JetVector     &jets = m_baseJets;
    const JetVector    &pjets = m_preJets;
    const Susy::Met      *met = m_met;
    uint run = nt.evt()->run;
    bool mc = nt.evt()->isMC;
    float mllMin(20);
    if(passGRL        (flag           ))  f.grl         = true;
    if(passLarErr     (flag           ))  f.larErr      = true;
    if(passTileErr    (flag           ))  f.tileErr     = true;
    if(passTTCVeto    (flag           ))  f.ttcVeto     = true;
    if(passGoodVtx    (flag           ))  f.goodVtx     = true;
    if(passTileTripCut(flag           ))  f.tileTrip    = true;
    if(passLAr        (flag           ))  f.lAr         = true;
    if(!hasBadJet     (jets           ))  f.badJet      = true;
    if(passDeadRegions(pjets,met,run,mc)) f.deadRegions = true;
    if(!hasBadMuon    (m_preMuons     ))  f.badMuon     = true;
    if(!hasCosmicMuon (m_baseMuons    ))  f.cosmicMuon  = true;
    if(passHfor       (               ))  f.hfor        = true;
    if(bleps.size() >= 2               )  f.ge2blep     = true;
    if(bleps.size() == 2               )  f.eq2blep     = true;
    if(susy::passMllMin(bleps, mllMin ))  f.mllMin      = true;
    return f;
}
//-----------------------------------------
void SusySelection::incrementCounters(const susy::wh::EventFlags &f, const WeightComponents &w)
{
  if(f.grl        ) increment(n_pass_Grl     , w); else return;
  if(f.larErr     ) increment(n_pass_LarErr  , w); else return;
  if(f.tileErr    ) increment(n_pass_TileErr , w); else return;
  if(f.ttcVeto    ) increment(n_pass_TTCVeto , w); else return;
  if(f.goodVtx    ) increment(n_pass_GoodVtx , w); else return;
  if(f.tileTrip   ) increment(n_pass_TileTrip, w); else return;
  if(f.lAr        ) increment(n_pass_LAr     , w); else return;
  if(f.badJet     ) increment(n_pass_BadJet  , w); else return;
  if(f.deadRegions) increment(n_pass_FEBCut  , w); else return;
  if(f.badMuon    ) increment(n_pass_BadMuon , w); else return;
  if(f.cosmicMuon ) increment(n_pass_Cosmic  , w); else return;
  if(f.hfor       ) increment(n_pass_hfor    , w); else return;
  if(f.ge2blep    ) increment(n_pass_ge2l    , w); else return;
  if(f.eq2blep    ) increment(n_pass_eq2l    , w); else return;
  if(f.mllMin     ) increment(n_pass_mll     , w); else return;
}
//-----------------------------------------
SusySelection::VarFlag_t SusySelection::computeSsFlags(LeptonVector& leptons,
                                                       const TauVector& taus,
                                                       const JetVector& jets,
                                                       const Met *met,
                                                       const susy::wh::Systematic sys,
                                                       bool allowQflip)
{
  SsPassFlags f;
  swk::DilepVars v;
  const LeptonVector &ls = leptons;
  LeptonVector     &ncls = leptons; // non-const leptons: can be modified by qflip
  const JetVector    &js = jets;
  Met ncmet(*m_met); // non-const met \todo: should modify a non-const input
  if(leptons.size()>1) {
      f.updateLlFlags(*leptons[0], *leptons[1]);
      f = assignNjetFlags(js, m_jvfTool, sys2ntsys(sys), m_anaType, f);
      DiLepEvtType ll(getDiLepEvtType(leptons));
      if(ll==ET_me) ll = ET_em;
      bool update4mom(true); // charge flip
      bool mc(nt.evt()->isMC), data(!mc);
      bool sameSign = allowQflip ? sameSignOrQflip(ncls, ncmet, susy::wh::WH_CENTRAL, update4mom, mc) : susy::sameSign(ncls);
      met = &ncmet; // after qflip, use potentially smeared lep and met
      LeptonVector anyLeptons(getAnyElOrMu(nt));
      LeptonVector lowPtLep(subtract_vector(anyLeptons, m_baseLeptons));
      v = swk::compute2lVars(ncls, met, jets, lowPtLep);
      v.weight = m_weightComponents.product();
      v.qflipWeight = m_weightComponents.qflip;
      if(susy::passNlepMin(ls, 2))         f.eq2l       =true;
      if(m_signalTaus.size()==0)           f.tauVeto    =true;
      if(passTrig2L     (ls))              f.trig2l     =true;
      if(passTrig2LMatch(ls))              f.trig2lmatch=true;
      if(data || susy::isTrueDilepton(ls)) f.true2l     =true;
      if(sameSign)                         f.sameSign   =true;
      f.veto3rdL = v.l3veto;
      if     (f.eq1j) SusySelection::passSrWh1j(v, f);
      else if(f.ge2j) SusySelection::passSrWh2j(v, f);
  }
  return std::make_pair(v, f);
}
//-----------------------------------------
void SusySelection::incrementSsCounters(const SsPassFlags &f, const WeightComponents &wc)
{
    assert((f.ee != f.em) || (f.em != f.mm));
    DiLepEvtType ll(f.ee ? ET_ee : f.em ? ET_em : ET_mm);
    increment(n_pass_category[ll], wc);

    if(f.eq2l       ) increment(n_pass_nSigLep  [ll], wc); else return;
    if(f.tauVeto    ) increment(n_pass_tauVeto  [ll], wc); else return;
    if(f.trig2l     ) increment(n_pass_tr2L     [ll], wc); else return;
    if(f.trig2lmatch) increment(n_pass_tr2LMatch[ll], wc); else return;
    if(f.true2l     ) increment(n_pass_mcTrue2l [ll], wc); else return;
    if(f.sameSign   ) increment(n_pass_ss       [ll], wc); else return;
    if(f.veto3rdL   ) increment(n_pass_3rdLep   [ll], wc); else return;
    if(f.fjveto     ) increment(n_pass_fjVeto   [ll], wc); else return;
    if(f.bjveto     ) increment(n_pass_bjVeto   [ll], wc); else return;
    if(f.ge1j       ) increment(n_pass_ge1j     [ll], wc); else return;
    if(f.ge1j) {
        if(f.ge1j    ) increment((f.eq1j ? n_pass_eq1j         [ll] : n_pass_ge2j         [ll]), wc);
        if(f.lepPt   ) increment((f.eq1j ? n_pass_eq1jlepPt    [ll] : n_pass_ge2jlepPt    [ll]), wc); else return;
        if(f.zllVeto ) increment((f.eq1j ? n_pass_eq1jmllZveto [ll] : n_pass_ge2jmllZveto [ll]), wc); else return;
        if(f.dEtall  ) increment((f.eq1j ? n_pass_eq1jDetall   [ll] : n_pass_ge2jDetall   [ll]), wc); else return;
        if(f.maxMt   ) increment((f.eq1j ? n_pass_eq1jMtMax    [ll] : n_pass_ge2jMtMax    [ll]), wc); else return;
        if(f.mljj    ) increment((f.eq1j ? n_pass_eq1jMlj      [ll] : n_pass_ge2jMljj     [ll]), wc); else return;
        if(f.ht      ) increment((f.eq1j ? n_pass_eq1jht       [ll] : n_pass_ge2jht       [ll]), wc); else return;
        if(f.metrel  ) increment((f.eq1j ? n_pass_eq1jmetRel   [ll] : n_pass_ge2jmetRel   [ll]), wc); else return;
        if(f.mtllmet ) increment((f.eq1j ? n_pass_eq1jmWwt     [ll] : n_pass_ge2jmWwt     [ll]), wc); else return;
    }
}
//-----------------------------------------
bool SusySelection::passHfor(Susy::SusyNtObject &nto)
{
    // DG : inheriting hardcoded magic values from HforToolD3PD.cxx, dah.
    const int kill(4);
    return nto.evt()->hfor != kill;
}
//-----------------------------------------
bool SusySelection::passTrig2L(const LeptonVector& leptons, DilTrigLogic *dtl, float met, Event* evt)
{
  if(leptons.size() != 2 || !dtl) return false;
  return dtl->passDilEvtTrig(leptons, met, evt);
}
//-----------------------------------------
bool SusySelection::passTrig2LMatch(const LeptonVector& leptons, DilTrigLogic *dtl, float met, Event* evt)
{
  if(leptons.size() != 2 || !dtl) return false;
  return dtl->passDilTrigMatch(leptons, met, evt);
}
//-----------------------------------------
bool SusySelection::passTrig2LwithMatch(const LeptonVector& leptons, DilTrigLogic *dtl, float met, Event* evt)
{
  return (passTrig2L(leptons, dtl, met, evt) && passTrig2LMatch(leptons, dtl, met, evt));
}
//-----------------------------------------
bool SusySelection::sameSignOrQflip(LeptonVector& leptons, Met &met,
                                    const susy::wh::Systematic sys,
                                    bool update4mom, bool isMc)
{
    if(leptons.size()>1) {
        bool isSS(susy::sameSign(leptons)), isOS(!isSS);
        bool canBeQflip(isMc && isOS && (leptons[0]->isEle() || leptons[1]->isEle()));
        if(canBeQflip) {
            m_qflipProb = computeChargeFlipProb(leptons, met, sys, update4mom);
            m_weightComponents.qflip = m_qflipProb;
            return true;
        }
        else return isSS;
    }
    return false;
}
//-----------------------------------------
bool SusySelection::passJetVeto(const JetVector& jets, const susy::wh::Systematic sys)
{
  // Require no light, b, or forward jets
  int N_L20 = numberOfCLJets(jets, m_jvfTool, sys2ntsys(sys), m_anaType);
  int N_B20 = numberOfCBJets(jets);
  int N_F30 = numberOfFJets(jets);
  return (N_L20 + N_B20 + N_F30 == 0);
}
//-----------------------------------------
bool SusySelection::passbJetVeto(const JetVector& jets)
{
  // Reject if there is a b jet using 2L definition
  int N_B20 = numberOfCBJets(jets);
  return (N_B20 == 0);
}
//-----------------------------------------
bool SusySelection::passfJetVeto(const JetVector& jets)
{
  return (0 == numberOfFJets(jets));
}
//-----------------------------------------
bool SusySelection::passMetRelMin(const Met *met, const LeptonVector& leptons,
                                  const JetVector& jets, float minVal){
  float metrel = getMetRel(met,leptons,jets);
  return (minVal < metrel);
}
//-----------------------------------------
bool SusySelection::passMuonRelIso(const LeptonVector &leptons, float maxVal)
{
  for(size_t i=0; i<leptons.size(); ++i){
    const Susy::Lepton* l = leptons[i];
    if(l->isMu()){
      const Muon* mu = static_cast<const Muon*>(l);
      if(!mu) continue;
      float etcone30 = muEtConeCorr(mu, m_baseElectrons, m_baseMuons,
                                    nt.evt()->nVtx, nt.evt()->isMC);
      if(mu->Pt() && (etcone30/mu->Pt() > maxVal)) return false;
    } // end if(isMu)
  } // end for(i)
  return true;
}
//-----------------------------------------
void SusySelection::cacheStaticWeightComponents()
{
  m_weightComponents.reset();
  if(!nt.evt()->isMC) {m_weightComponents.reset(); return;}
  m_weightComponents.gen = nt.evt()->w;
  m_weightComponents.pileup = nt.evt()->wPileup;
//   bool useSumwMap(true);
//   m_weightComponents.susynt = (m_useXsReader ?
//                                computeEventWeightXsFromReader(LUMI_A_L) :
//                                SusyNtAna::getEventWeight(LUMI_A_L, useSumwMap));
  //   const bool useSumwMap(true);
  //   const bool useProcSumw(true);
  //   const bool useSusyXsec(true);
  //m_weightComponents.susynt = getEventWeight(LUMI_A_L, useSumwMap, useProcSumw, useSusyXsec);
  const MCWeighter::WeightSys wSys = MCWeighter::Sys_NOM;
  m_weightComponents.susynt = m_mcWeighter->getMCWeight(nt.evt(), LUMI_A_L, wSys);
  float genpu(m_weightComponents.gen*m_weightComponents.pileup);
  m_weightComponents.norm = (genpu != 0.0 ? m_weightComponents.susynt/genpu : 1.0);
}
//-----------------------------------------
SusySelection::WeightComponents SusySelection::computeNonStaticWeightComponents(cvl_t& leptons, cvj_t& jets, const susy::wh::Systematic sys)
{
    WeightComponents wc;
    if(nt.evt()->isMC) {
        bool trigSfNeeded(true), btagSfNeeded(true); // todo some of these calls are expensive; compute only what's needed
        BTagSys bsys = sys2ntbsys(sys);
        wc.lepSf   = susy::getLeptonEff2Lep(leptons, sys);
        wc.trigger = (trigSfNeeded ? getTriggerWeight2Lep(leptons, sys) : 1.0);
        wc.btag    = (btagSfNeeded ? getBTagWeight(jets, nt.evt(), bsys) : 1.0);
    }
    return wc;
}
//-----------------------------------------
void SusySelection::assignNonStaticWeightComponents(const WeightComponents &wc)
{
    bool isData(!nt.evt()->isMC);
    if(isData) m_weightComponents.reset(); // DG 2014-03-07: not sure we actually need to reset (all?) components
    else{
        m_weightComponents.lepSf = wc.lepSf;
        m_weightComponents.trigger = wc.trigger;
        m_weightComponents.btag = wc.btag;
    }
}
//-----------------------------------------
float SusySelection::getBTagWeight(cvj_t& jets, const Event* evt, const BTagSys sys)
{
    JetVector tempJets = SusyNtTools::getBTagSFJets2Lep(jets);
    return bTagSF(evt, tempJets, evt->mcChannel, sys);
}
//-----------------------------------------
float SusySelection::getTriggerWeight2Lep(const LeptonVector &leptons, const susy::wh::Systematic sys)
{
  float trigW = 1.0;
  // if m_useMCTrig, then we are dropping evts with DilTrigLogic::passDil*, not weighting them
  // DG Jun2013 : verify this with Matt & Josephine
  if(!m_useMCTrig){
      const SusyNtSys ntsys = susy::wh::sys2ntsys(sys);
      if(leptons.size()==2) trigW = m_trigObj->getTriggerWeight(leptons,
                                                                nt.evt()->isMC,
                                                                m_met->Et,
                                                                m_signalJets2Lep.size(),
                                                                nt.evt()->nVtx,
                                                                ntsys);
    bool twIsInvalid(isnan(trigW) || trigW<0.0);
    assert(!twIsInvalid);
    if(twIsInvalid){
      if(m_dbg)
        cout<<"SusySelection::getTriggerWeight: invalid weigth "<<trigW<<", using 0.0"<<endl;
      trigW = (twIsInvalid ? 0.0 : trigW);
    }
  }
  return trigW;
}
//-----------------------------------------
// helper function: write header with event types
std::string lineLabelsPerEventType(const string *labels, int colWidth){
  std::ostringstream oss;
  for(int i=0; i<ET_N-1; ++i)
    oss<<std::setw(colWidth)<<labels[i];
  oss<<std::setw(colWidth)<<"em+me";
  return oss.str();
}
// helper function: for a given weight type, write line with counts for each event type
std::string lineCountersPerEventType(const float cnt[ET_N][kWeightTypesN],
                                     int weightType, int colWidth){
  std::ostringstream oss;
  // bool raw(weightType==WT_Raw);
  // int precision(raw ? 0 : 2); // DG Aug2013 not working properly tobefixed
  for(int i=0; i<ET_N-1; ++i)
    oss<<std::setw(colWidth)
      //<<(raw ? std::fixed : "")
      //<<std::setprecision(precision)
       <<cnt[i][weightType];
  oss<<std::setw(colWidth)<<(cnt[ET_em][weightType] + cnt[ET_me][weightType]);
  return oss.str();
}
void SusySelection::dumpEventCounters()
{
  string v_ET[] = {"ee","mm","em","me"};
  string v_WT[] = {"Raw","Event","Pileup","LeptonSF","btagSF","TrigSF","All"};
  int colWidth(10);
  int &cw = colWidth;
  using std::setw;
  int nCols(ET_N-1);
  string topRule(nCols*colWidth, '*');
  string midRule(nCols*colWidth, '-');
  // define a function reference to shorten lines
  string (&lcpet)(const float cnt[ET_N][kWeightTypesN], int weightType, int colWidth) = lineCountersPerEventType;
  for(int w=0; w<kWeightTypesN; ++w){
    cout<<topRule                                                    <<endl;
    cout<<"Event counts for weight: "<< v_WT             [w]         <<endl;
    cout<<midRule                                                    <<endl;
    cout<<"input:           : "<<setw(cw)<<n_readin           [w]    <<endl;
    cout<<"GRL              : "<<setw(cw)<<n_pass_Grl         [w]    <<endl;
    cout<<"LarErr           : "<<setw(cw)<<n_pass_LarErr      [w]    <<endl;
    cout<<"TileErr          : "<<setw(cw)<<n_pass_TileErr     [w]    <<endl;
    cout<<"TTCVeto          : "<<setw(cw)<<n_pass_TTCVeto     [w]    <<endl;
    cout<<"GoodVtx          : "<<setw(cw)<<n_pass_GoodVtx     [w]    <<endl;
    cout<<"TileTripCut      : "<<setw(cw)<<n_pass_TileTrip    [w]    <<endl;
    cout<<"LAr:             : "<<setw(cw)<<n_pass_LAr         [w]    <<endl;
    cout<<"BadJet:          : "<<setw(cw)<<n_pass_BadJet      [w]    <<endl;
    cout<<"FEB:             : "<<setw(cw)<<n_pass_FEBCut      [w]    <<endl;
    cout<<"BadMu:           : "<<setw(cw)<<n_pass_BadMuon     [w]    <<endl;
    cout<<"Cosmic:          : "<<setw(cw)<<n_pass_Cosmic      [w]    <<endl;
    cout<<"hfor:            : "<<setw(cw)<<n_pass_hfor        [w]    <<endl;
    cout<<"Htautau veto     : "<<setw(cw)<<n_pass_HttVeto     [w]    <<endl;
    cout<<"atleast 2        : "<<setw(cw)<<n_pass_ge2l        [w]    <<endl;
    cout<<"exactly 2        : "<<setw(cw)<<n_pass_eq2l        [w]    <<endl;
    cout<<"mll              : "<<setw(cw)<<n_pass_mll         [w]    <<endl;
    cout<<"nSigLep          : "<<setw(cw)<<n_pass_signalLep   [w]    <<endl;
    cout<<"   ------  Start Comparison Here ------ "                 <<endl;
    cout<<"Dilepton type    : "<<lineLabelsPerEventType(v_ET, cw)    <<endl;
    cout<<"category         : "<<lcpet(n_pass_category       , w, cw)<<endl;
    cout<<"nSigLep          : "<<lcpet(n_pass_nSigLep        , w, cw)<<endl;
    cout<<"tauVeto          : "<<lcpet(n_pass_tauVeto        , w, cw)<<endl;
    cout<<"trig:            : "<<lcpet(n_pass_tr2L           , w, cw)<<endl;
    cout<<"trig match:      : "<<lcpet(n_pass_tr2LMatch      , w, cw)<<endl;
    cout<<"mc prompt2l      : "<<lcpet(n_pass_mcTrue2l       , w, cw)<<endl;
    cout<<"SS:              : "<<lcpet(n_pass_ss             , w, cw)<<endl;
    cout<<"3rdLepVeto       : "<<lcpet(n_pass_3rdLep         , w, cw)<<endl;
    cout<<"fjVeto           : "<<lcpet(n_pass_fjVeto         , w, cw)<<endl;
    cout<<"bjVeto           : "<<lcpet(n_pass_bjVeto         , w, cw)<<endl;
    cout<<"ge1j             : "<<lcpet(n_pass_ge1j           , w, cw)<<endl;
    cout<<midRule                                                    <<endl;
    cout<<"eq1j             : "<<lcpet(n_pass_eq1j           , w, cw)<<endl;
    cout<<"lepPt            : "<<lcpet(n_pass_eq1jlepPt      , w, cw)<<endl;
    cout<<"mllZveto         : "<<lcpet(n_pass_eq1jmllZveto   , w, cw)<<endl;
    cout<<"Detall           : "<<lcpet(n_pass_eq1jDetall     , w, cw)<<endl;
    cout<<"MtMax            : "<<lcpet(n_pass_eq1jMtMax      , w, cw)<<endl;
    cout<<"Mlj              : "<<lcpet(n_pass_eq1jMlj        , w, cw)<<endl;
    cout<<"ht               : "<<lcpet(n_pass_eq1jht         , w, cw)<<endl;
    cout<<"metRel           : "<<lcpet(n_pass_eq1jmetRel     , w, cw)<<endl;
    cout<<"mWwt             : "<<lcpet(n_pass_eq1jmWwt       , w, cw)<<endl;
    cout<<midRule                                                    <<endl;
    cout<<"ge2j             : "<<lcpet(n_pass_ge2j           , w, cw)<<endl;
    cout<<"lepPt            : "<<lcpet(n_pass_ge2jlepPt      , w, cw)<<endl;
    cout<<"mllZveto         : "<<lcpet(n_pass_ge2jmllZveto   , w, cw)<<endl;
    cout<<"Detall           : "<<lcpet(n_pass_ge2jDetall     , w, cw)<<endl;
    cout<<"MtMax            : "<<lcpet(n_pass_ge2jMtMax      , w, cw)<<endl;
    cout<<"Mljj             : "<<lcpet(n_pass_ge2jMljj       , w, cw)<<endl;
    cout<<"ht               : "<<lcpet(n_pass_ge2jht         , w, cw)<<endl;
    cout<<"metRel           : "<<lcpet(n_pass_ge2jmetRel     , w, cw)<<endl;
    cout<<"mWwt             : "<<lcpet(n_pass_ge2jmWwt       , w, cw)<<endl;
    cout<<midRule                                                    <<endl;
    cout<<"   ------  Control regions ------ "                       <<endl;
    cout<<"cr1jzv           : "<<lcpet(n_pass_cr1jzv         , w, cw)<<endl;
    cout<<"cr1jfake         : "<<lcpet(n_pass_cr1jfake       , w, cw)<<endl;
    cout<<"cr1jzvfake       : "<<lcpet(n_pass_cr1jzvfake     , w, cw)<<endl;
    cout<<midRule                                                    <<endl;
    cout<<"cr2jzv           : "<<lcpet(n_pass_cr2jzv         , w, cw)<<endl;
    cout<<"cr2jfake         : "<<lcpet(n_pass_cr2jfake       , w, cw)<<endl;
    cout<<"cr2jzvfake       : "<<lcpet(n_pass_cr2jzvfake     , w, cw)<<endl;
    cout<<midRule                                                    <<endl;
    cout<<"ewkSs            : "<<lcpet(n_pass_ewkSs          , w, cw)<<endl;
    cout<<"ewkSsLoose       : "<<lcpet(n_pass_ewkSsLoose     , w, cw)<<endl;
    cout<<"ewkSsLea         : "<<lcpet(n_pass_ewkSsLea       , w, cw)<<endl;
    cout<<midRule                                                    <<endl;
  }// end for(w)
}
//-----------------------------------------
float SusySelection::getXsFromReader()
{
  if(!m_useXsReader || !m_xsReader) return -1.0;
  bool xsIsNotCached(m_xsFromReader < 0.0); // was initialized to -1
  if(xsIsNotCached){
    int dsid(static_cast<int>(nt.evt()->mcChannel));
    m_xsFromReader = m_xsReader->GetXS(dsid);
    if(m_dbg) cout<<"SusySelection::getXsFromReader: got "<<m_xsFromReader<<" for "<<dsid<<endl;
  }
  return m_xsFromReader;
}
//-----------------------------------------
float SusySelection::computeEventWeightXsFromReader(float lumi)
{
  float defaultXsec = nt.evt()->xsec;
  assert(defaultXsec != 0.0);
  return (getEventWeight(lumi) * getXsFromReader() / defaultXsec);
}
//-----------------------------------------
float SusySelection::computeChargeFlipProb(const LeptonVector &leptons, const Met &met,
                                           const susy::wh::Systematic systematic)
{ // todo: avoid duplication with method below (this one doesn't modify inputs)
  cvl_t &ls = leptons;
  if(ls.size()<2 || !ls[0] || !ls[1] || !m_chargeFlip) return 0.0;
  Lepton *l0(ls[0]), *l1(ls[1]);
  int pdg0(susy::pdgIdFromLep(l0)), pdg1(susy::pdgIdFromLep(l1));
  TLorentzVector smearedLv0(*l0), smearedLv1(*l1);
  TVector2 smearedMet(met.lv().Px(), met.lv().Py());
  int sys = (systematic==swh::WH_BKGMETHODUP   ? +1 :
             systematic==swh::WH_BKGMETHODDOWN ? -1 :
             0); // convert to the convention used in ChargeFlip
  bool isData=false;
  m_chargeFlip->setSeed(nt.evt()->event);
  float flipProb(m_chargeFlip->OS2SS(pdg0, &smearedLv0, pdg1, &smearedLv1, sys, isData, chargeFlip::dataonly));
  float overlapFrac(m_chargeFlip->overlapFrac().first);
  // DG 2014 this is our own (WH) syst unc (~50%); qflip lhood stat unc is negligible so just multiply it
  if(systematic==swh::WH_BKGMETHODUP)   overlapFrac *= 1.5;
  if(systematic==swh::WH_BKGMETHODDOWN) overlapFrac *= 0.5;
  validateQflipProb(flipProb, overlapFrac, l0, l1, nt.evt(), m_nInvalidQflip);
  return flipProb*overlapFrac;
}
//-----------------------------------------
float SusySelection::computeChargeFlipProb(LeptonVector &leptons, Met &met,
                                           const susy::wh::Systematic systematic,
                                           bool update4mom)
{
  cvl_t &ls = leptons;
  if(ls.size()<2 || !ls[0] || !ls[1] || !m_chargeFlip) return 0.0;
  Lepton *l0(ls[0]), *l1(ls[1]);
  int pdg0(susy::pdgIdFromLep(l0)), pdg1(susy::pdgIdFromLep(l1));
  TLorentzVector smearedLv0(*l0), smearedLv1(*l1);
  TVector2 smearedMet(met.lv().Px(), met.lv().Py());
  int sys = (systematic==swh::WH_BKGMETHODUP   ? +1 :
             systematic==swh::WH_BKGMETHODDOWN ? -1 :
             0); // convert to the convention used in ChargeFlip
  /*
  cout<<"OS2SS args: "
      <<" event   "<<nt.evt()->event
      <<" pdg0 "<<pdg0
      <<" lv0 px: "<<smearedLv0.Px()<<" py: "<<smearedLv0.Py()<<" pz: "<<smearedLv0.Pz()
      <<" pdg1 "<<pdg1
      <<" lv1 px: "<<smearedLv1.Px()<<" py: "<<smearedLv1.Py()<<" pz: "<<smearedLv1.Pz()
      <<" met px: "<<smearedMet.Px()<<" py: "<<smearedMet.Py()
      <<endl;
  */
  bool isData=false;
  m_chargeFlip->setSeed(nt.evt()->event);
  float flipProb(m_chargeFlip->OS2SS(pdg0, &smearedLv0, pdg1, &smearedLv1, sys, isData, chargeFlip::dataonly));
  float overlapFrac(m_chargeFlip->overlapFrac().first);
  // DG 2014 this is our own (WH) syst unc (~50%); qflip lhood stat unc is negligible so just multiply it
  if(systematic==swh::WH_BKGMETHODUP)   overlapFrac *= 1.5;
  if(systematic==swh::WH_BKGMETHODDOWN) overlapFrac *= 0.5;
  if(update4mom) {
    m_unsmeared_lv0 = (*l0);
    m_unsmeared_lv1 = (*l1);
    m_unsmeared_met = met;
    l0->SetPtEtaPhiM(smearedLv0.Pt(), smearedLv0.Eta(), smearedLv0.Phi(), smearedLv0.M());
    l1->SetPtEtaPhiM(smearedLv1.Pt(), smearedLv1.Eta(), smearedLv1.Phi(), smearedLv1.M());
    met.Et = smearedMet.Mod();
    met.phi = TVector2::Phi_mpi_pi(smearedMet.Phi());
  }
  validateQflipProb(flipProb, overlapFrac, l0, l1, nt.evt(), m_nInvalidQflip);
  return flipProb*overlapFrac;
}
//-----------------------------------------
bool SusySelection::validateQflipProb(float &probability, float &overlap, const Lepton *l0, const Lepton *l1,
                                      const Event *event, unsigned int &invalidCounter)
{
    bool isValid=true;
    if(probability<0.0 || probability>1.0){
        isValid=false;
        invalidCounter++;
        if(invalidCounter<100){
            cout<<"SusySelection::validateQflipProb "
                <<"error: unphysical probability "<<probability<<" * "<<overlap<<endl;
            cout<<" setting it to 0.0"<<endl;
            if(event && l0 && l1) {
                event->print();
                l0->print();
                l1->print();
            }
        }
        probability=0.0;
    }
    return isValid;
}
//-----------------------------------------
susy::wh::Chan SusySelection::getChan(const LeptonVector& leps)
{
  uint ie = 0;
  uint im = 0;
  for(uint i=0; i<leps.size(); ++i){
    if( leps.at(i)->isEle() ) ie++;
    else if( leps.at(i)->isMu() ) im++;
  }
  if( ie == 2 && im == 0 ) return susy::wh::Ch_ee;
  if( ie == 1 && im == 1 ) return susy::wh::Ch_em;
  if( ie == 0 && im == 2 ) return susy::wh::Ch_mm;
  cout<<"Not ee/mm/em... Number Electrons: "<<ie<<" Number Muons: "<<im<<endl;
  return susy::wh::Ch_N; // not in range
}
//-----------------------------------------
SsPassFlags SusySelection::assignNjetFlags(const JetVector& jets, JVFUncertaintyTool* jvfTool, SusyNtSys sys, AnalysisType anaType, SsPassFlags f)
{
  int njCl = numberOfCLJets(jets, jvfTool, sys, anaType);
  int njB  = numberOfCBJets(jets);
  int njF  = numberOfFJets (jets);
  f.bjveto = njB  == 0;
  f.fjveto = njF  == 0;
  f.ge1j   = njCl >= 1;
  f.eq1j   = njCl == 1;
  f.ge2j   = njCl >= 2;
  return f;
}
//-----------------------------------------
void SusySelection::resetAllCounters()
{
  for(int w=0; w<kWeightTypesN; ++w){// Loop over weight types
    n_readin          [w] = 0;
    n_pass_Grl        [w] = 0;
    n_pass_LarErr     [w] = 0;
    n_pass_TileErr    [w] = 0;
    n_pass_TTCVeto    [w] = 0;
    n_pass_GoodVtx    [w] = 0;
    n_pass_TileTrip   [w] = 0;
    n_pass_LAr        [w] = 0;
    n_pass_BadJet     [w] = 0;
    n_pass_FEBCut     [w] = 0;
    n_pass_BadMuon    [w] = 0;
    n_pass_Cosmic     [w] = 0;
    n_pass_hfor       [w] = 0;
    n_pass_HttVeto    [w] = 0;
    n_pass_ge2l       [w] = 0;
    n_pass_eq2l       [w] = 0;
    n_pass_mll        [w] = 0;
    n_pass_signalLep  [w] = 0;
    for(int i=0; i<ET_N; ++i){ // loop over weight x channel.
      // per-SR counters
      n_pass_SR6sign[i][w] = n_pass_SR6flav[i][w] = n_pass_SR6metr[i][w] = 0;
      n_pass_SR6ge1j[i][w] = n_pass_SR6ge2j[i][w] = n_pass_SR6eq2j[i][w] = 0;
      n_pass_SR6eq2jNfv[i][w] = n_pass_SR6ge2jNfv[i][w] = n_pass_SR6[i][w] = 0;
      n_pass_SR6DrllMax     [i][w] = n_pass_SR6PtllMin     [i][w] = 0;
      n_pass_SR6MllMax      [i][w] = n_pass_SR6METRel      [i][w] = 0;
      n_pass_SR6MtLlmetMin  [i][w] = n_pass_SR6MtMinlmetMin[i][w] = 0;
      n_pass_SR6ZtautauVeto [i][w] = 0;

      n_pass_SR7sign[i][w] = n_pass_SR7flav[i][w] = n_pass_SR7metr[i][w] = 0;
      n_pass_SR7ge1j[i][w] = n_pass_SR7ge2j[i][w] = n_pass_SR7eq2j[i][w] = 0;
      n_pass_SR7eq2jNfv[i][w] = n_pass_SR7ge2jNfv[i][w] = n_pass_SR7[i][w] = 0;
      n_pass_SR7DrllMax     [i][w] = n_pass_SR7PtllMin     [i][w] = 0;
      n_pass_SR7MllMax      [i][w] = n_pass_SR7METRel      [i][w] = 0;
      n_pass_SR7MtLlmetMin  [i][w] = n_pass_SR7MtMinlmetMin[i][w] = 0;
      n_pass_SR7ZtautauVeto [i][w] = 0;

      n_pass_SR8sign[i][w] = n_pass_SR8flav[i][w] = n_pass_SR8metr[i][w] = 0;
      n_pass_SR8ge1j[i][w] = n_pass_SR8ge2j[i][w] = n_pass_SR8eq2j[i][w] = 0;
      n_pass_SR8eq2jNfv[i][w] = n_pass_SR8ge2jNfv[i][w] = n_pass_SR8[i][w] = 0;

      n_pass_SR9sign[i][w] = n_pass_SR9flav[i][w] = n_pass_SR9metr[i][w] = 0;
      n_pass_SR9ge1j[i][w] = n_pass_SR9ge2j[i][w] = n_pass_SR9eq2j[i][w] = 0;
      n_pass_SR9eq2jNfv[i][w] = n_pass_SR9ge2jNfv[i][w] = n_pass_SR9[i][w] = 0;

      n_pass_flavor         [i][w] = 0;
      n_pass_os             [i][w] = 0;
      n_pass_ss             [i][w] = 0;
      n_pass_tr2L           [i][w] = 0;
      n_pass_tr2LMatch      [i][w] = 0;
      n_pass_mcTrue2l       [i][w] = 0;
      n_pass_category       [i][w] = 0;
      n_pass_nSigLep        [i][w] = 0;
      n_pass_tauVeto        [i][w] = 0;
      n_pass_mllMin         [i][w] = 0;
      n_pass_fjVeto         [i][w] = 0;
      n_pass_bjVeto         [i][w] = 0;
      n_pass_ge1j           [i][w] = 0;
      n_pass_eq1j           [i][w] = 0;
      n_pass_ge2j           [i][w] = 0;
      n_pass_lepPt          [i][w] = 0;
      n_pass_mllZveto       [i][w] = 0;
      n_pass_mWwt           [i][w] = 0;
      n_pass_ht             [i][w] = 0;
      n_pass_metRel         [i][w] = 0;
      n_pass_3rdLep         [i][w] = 0;
      n_pass_eq1jlepPt      [i][w] = 0;
      n_pass_eq1jmllZveto   [i][w] = 0;
      n_pass_eq1jDetall     [i][w] = 0;
      n_pass_eq1jMtMax      [i][w] = 0;
      n_pass_eq1jMlj        [i][w] = 0;
      n_pass_eq1jht         [i][w] = 0;
      n_pass_eq1jmetRel     [i][w] = 0;
      n_pass_eq1jmWwt       [i][w] = 0;
      n_pass_ge2jlepPt      [i][w] = 0;
      n_pass_ge2jmllZveto   [i][w] = 0;
      n_pass_ge2jDetall     [i][w] = 0;
      n_pass_ge2jMtMax      [i][w] = 0;
      n_pass_ge2jMljj       [i][w] = 0;
      n_pass_ge2jht         [i][w] = 0;
      n_pass_ge2jmetRel     [i][w] = 0;
      n_pass_ge2jmWwt       [i][w] = 0;
      n_pass_cr1jzv         [i][w] = 0;
      n_pass_cr1jfake       [i][w] = 0;
      n_pass_cr1jzvfake     [i][w] = 0;
      n_pass_cr2jzv         [i][w] = 0;
      n_pass_cr2jfake       [i][w] = 0;
      n_pass_cr2jzvfake     [i][w] = 0;

      n_pass_ewkSs          [i][w] = 0;
      n_pass_ewkSsLoose     [i][w] = 0;
      n_pass_ewkSsLea       [i][w] = 0;
    } // end for(i)
  } // end for(w)
}
//-----------------------------------------
void SusySelection::initChargeFlipTool()
{
  char* rcdir = getenv("ROOTCOREDIR");
  if(!rcdir){
    if(m_dbg) cout<<"invalid ROOTCOREDIR, cannot initialize chargeFlipTool"<<endl;
    return;
  }
  string chargeFlipInput(rcdir);
  //  chargeFlipInput += "/../ChargeFlip/data/d0_new2d_chargeflip_map.root";
  //  chargeFlipInput += "/../ChargeFlip/data/d0_new2d_chargeflip_map_scale_last_ptbin.root"; // scaled with wwjj
  chargeFlipInput += "/../ChargeFlip/data/d0_new2d_chargeflip_map_scale_with_mc_last_ptbin.root"; // scaled with Emma's mc map
  m_chargeFlip = new chargeFlip(chargeFlipInput);
  if(m_dbg) m_chargeFlip->printSettings();
}
//-----------------------------------------
bool SusySelection::initMcWeighter(TTree *tree)
{
    bool success=false;
    if(tree){
        string xsecDir = gSystem->ExpandPathName("$ROOTCOREBIN/data/SUSYTools/mc12_8TeV/");
        m_mcWeighter = new MCWeighter(tree, xsecDir);
        bool isPmssmSample(contains(sampleName(), "Herwigpp_UEEE3_CTEQ6L1_DGnoSL_TB10"));
        if(isPmssmSample) m_mcWeighter->setLabelBinCounter("Initial").clearAndRebuildSumwMap(m_tree);
        if(m_dbg) cout<<"SusySelection: MCWeighter has been initialized"<<endl;
    } else {
        cout<<"SusySelection::initMcWeighter: error, invalid input tree, cannot initialize Mcweighter"<<endl;
    }
    return success;
}
//-----------------------------------------
LeptonVector SusySelection::getAnyElOrMu(SusyNtObject &susyNt/*, SusyNtSys sys*/)
{
    // DG 2013-12-02:
    // todo1 : re-implement with std algo
    // todo2 : re-implement with syst
    float minPt = 6.0;
    LeptonVector leptons;
    for(uint ie=0; ie<susyNt.ele()->size(); ++ie){
        if(Electron* e = & susyNt.ele()->at(ie)){ //e->setState(sys);
            if(e->Pt()>minPt) leptons.push_back(static_cast<Lepton*>(e));
        }
    }
    for(uint im=0; im<susyNt.muo()->size(); ++im){
        if(Muon* m = & susyNt.muo()->at(im)){ //m->setState(sys);
            if(m->Pt(),minPt) leptons.push_back(static_cast<Lepton*>(m));
        }
    }
    return leptons;
}
//-----------------------------------------
bool SusySelection::passCrWhZVfakeEe(const susy::wh::kin::DilepVars &v)
{
    return (v.isEe
            && fabs(v.mll - 91.2)>10.0
            && v.metrel > 40.0
            && ((v.numCentralLightJets==1 && v.mlj  > 90.0)
                ||
                (v.numCentralLightJets >1 && v.mljj >120.0)));
}
//-----------------------------------------
bool SusySelection::passCrWhZVfakeEm(const susy::wh::kin::DilepVars &v)
{
    return (v.isEm
            && v.pt0 > 30.0
            && v.pt1 > 30.0
            && ((v.numCentralLightJets==1 && v.mlj  > 90.0)
                ||
                (v.numCentralLightJets >1 && v.mljj >120.0)));
}
//-----------------------------------------
bool SusySelection::passCrWhfakeEm  (const susy::wh::kin::DilepVars &v)
{
    return (v.isEm
            && v.pt0 > 30.0
            && v.pt1 < 30.0 // orthogonal to WhZVfake1jem
            && ((v.numCentralLightJets==1 && v.mlj  > 90.0)
                ||
                (v.numCentralLightJets >1 && v.mljj >120.0)));
}
//-----------------------------------------
bool SusySelection::passCrWhZVMm    (const susy::wh::kin::DilepVars &v)
{
    return (v.isMm
            && v.pt0 > 30.0
            && v.pt1 > 30.0
            && ((v.numCentralLightJets==1 && v.mlj  > 90.0)
                ||
                (v.numCentralLightJets >1 && v.mljj >120.0)));
}
//-----------------------------------------
bool SusySelection::passCrWhfakeMm  (const susy::wh::kin::DilepVars &v)
{
    return (v.isMm
            // && v.pt0 > 30.0 // ??
            && v.pt1 < 30.0
            && ((v.numCentralLightJets==1 && v.mlj  > 90.0)
                ||
                (v.numCentralLightJets >1 && v.mljj >120.0)));
}
//-----------------------------------------
bool SusySelection::passCrWhZVfake(const susy::wh::kin::DilepVars &v)
{
    return (SusySelection::passCrWhZVfakeEe(v)
            ||
            SusySelection::passCrWhZVfakeEm(v));
}
//-----------------------------------------
bool SusySelection::passCrWhfake(const susy::wh::kin::DilepVars &v)
{
    return (SusySelection::passCrWhfakeEm(v)
            ||
            SusySelection::passCrWhfakeMm(v));
}
//-----------------------------------------
bool SusySelection::passCrWhZV(const susy::wh::kin::DilepVars &v)
{
    return SusySelection::passCrWhZVMm(v);
}
//-----------------------------------------
DiLepEvtType vars2type(const susy::wh::kin::DilepVars &v)
{
    DiLepEvtType et = ET_Unknown;
    if     (v.isEe) et = ET_ee;
    else if(v.isEm) et = ET_em;
    else if(v.isMm) et = ET_mm;
    return et;
}
//-----------------------------------------
bool SusySelection::passAndIncrementCrWhZVfake(const susy::wh::kin::DilepVars &v)
{
    bool pass(passCrWhZVfake(v));
    if(pass) {
        DiLepEvtType ll = vars2type(v);
        float* counters = v.numCentralLightJets==1 ? n_pass_cr1jzvfake[ll] : n_pass_cr2jzvfake[ll];
        increment(counters, m_weightComponents);
    }
    return pass;
}
//-----------------------------------------
bool SusySelection::passAndIncrementCrWhfake(const susy::wh::kin::DilepVars &v)
{
    bool pass(passCrWhfake(v));
    if(pass) {
        DiLepEvtType ll = vars2type(v);
        float* counters = v.numCentralLightJets==1 ? n_pass_cr1jfake[ll] : n_pass_cr2jfake[ll];
        increment(counters, m_weightComponents);
    }
    return pass;
}
//-----------------------------------------
bool SusySelection::passAndIncrementCrWhZV(const susy::wh::kin::DilepVars &v)
{
    bool pass(passCrWhZV(v));
    if(pass) {
        DiLepEvtType ll = vars2type(v);
        float* counters = v.numCentralLightJets==1 ? n_pass_cr1jzv[ll] : n_pass_cr2jzv[ll];
        increment(counters, m_weightComponents);
    }
    return pass;
}
//-----------------------------------------
bool SusySelection::passSrWh1j(const susy::wh::kin::DilepVars &v, SsPassFlags &f)
{
    bool notApplied(true), pass(false);
    if(v.numCentralLightJets==1){
        f.veto3rdL = v.l3veto;
        if(v.isMm) {
            f.lepPt   = (v.pt0 > 30.0 && v.pt1 > 20.0);
            f.zllVeto = notApplied;
            f.dEtall  = v.detall <   1.5;
            f.maxMt   = v.mtmax()> 100.0;
            f.mljj    = v.mlj    <  90.0;
            f.ht      = v.ht     > 200.0;
            f.metrel  = notApplied;
            f.mtllmet = notApplied;
        } else if(v.isEm) {
            f.lepPt   = (v.pt0 > 30.0 && v.pt1 > 30.0);
            f.zllVeto = notApplied;
            f.dEtall  = v.detall <   1.5;
            f.maxMt   = v.mtmax()> 110.0;
            f.mljj    = v.mlj    <  90.0;
            f.ht      = notApplied;
            f.metrel  = notApplied;
            f.mtllmet = v.mtllmet> 120.0;
        } else if(v.isEe) {
            f.lepPt   = (v.pt0 > 30.0 && v.pt1 > 20.0);
            f.zllVeto = fabs(v.mll-91.2) > 10.0;
            f.dEtall  = notApplied;
            f.maxMt   = notApplied;
            f.mljj    = v.mlj    <  90.0;
            f.ht      = v.ht     > 200.0;
            f.metrel  = v.metrel >  55.0;
            f.mtllmet = notApplied;
        }
        pass = f.passKinCriteria();
    }
    return pass;
}
//-----------------------------------------
bool SusySelection::passSrWh1j(const susy::wh::kin::DilepVars &v)
{
    SsPassFlags f;
    return SusySelection::passSrWh1j(v, f);
}
//-----------------------------------------
bool SusySelection::passSrWh2j(const susy::wh::kin::DilepVars &v, SsPassFlags &f)
{
    bool notApplied(true), pass(false);
    if(v.numCentralLightJets>1 && v.numCentralLightJets<4){
        f.veto3rdL = v.l3veto;
        if(v.isMm) {
            f.lepPt = (v.pt0 > 30.0 && v.pt1 > 30.0);
            f.zllVeto = notApplied;
            f.dEtall  = v.detall <   1.5;
            f.maxMt   = notApplied;
            f.mljj    = v.mljj   < 120.0;
            f.ht      = v.ht     > 220.0;
            f.metrel  = notApplied;
            f.mtllmet = notApplied;
        } else if(v.isEm) {
            f.lepPt   = (v.pt0 > 30.0 && v.pt1 > 30.0);
            f.zllVeto = notApplied;
            f.dEtall  = v.detall <   1.5;
            f.maxMt   = notApplied;
            f.mljj    = v.mljj   < 120.0;
            f.ht      = notApplied;
            f.metrel  = notApplied;
            f.mtllmet = v.mtllmet> 110.0;
        } else if(v.isEe) {
            f.lepPt   = (v.pt0 > 30.0 && v.pt1 > 20.0);
            f.zllVeto = fabs(v.mll-91.2) > 10.0;
            f.dEtall  = notApplied;
            f.maxMt   = v.mtmax()> 100.0;
            f.mljj    = v.mljj   < 120.0;
            f.ht      = notApplied;
            f.metrel  = v.metrel >  30.0;
            f.mtllmet = notApplied;
        }
        pass = f.passKinCriteria();
    }
    return pass;
}
//-----------------------------------------
bool SusySelection::passSrWh2j(const susy::wh::kin::DilepVars &v)
{
    SsPassFlags f;
    return SusySelection::passSrWh2j(v, f);
}
//-----------------------------------------
bool SusySelection::isEventForHft(const susy::wh::kin::DilepVars &vars, const SsPassFlags &flags)
{
    // temporary change: save all same-sign events, DG 2014-04-02
    return flags.sameSign;
/*
    const susy::wh::kin::DilepVars &v = vars;
    SsPassFlags f = flags;
    bool onejet(v.numCentralLightJets==1);
    if(onejet) passSrWh1j(v, f);
    else       passSrWh2j(v, f);
    bool dropThisRequirement = true;
    if(v.isEe) {
        if(onejet) { f.mljj = f.ht = dropThisRequirement; }
        else       { f.mljj = f.maxMt = dropThisRequirement; }
    } else if(v.isEm) {
        if(onejet) { f.mljj = f.mtllmet = dropThisRequirement; }
        else       { f.mljj = f.mtllmet = dropThisRequirement; }
    } else if(v.isMm) {
        if(onejet) { f.mljj = f.ht = dropThisRequirement; }
        else       { f.mljj = f.ht = dropThisRequirement; }
    }
    return f.passKinCriteria();
*/
}
//-----------------------------------------
