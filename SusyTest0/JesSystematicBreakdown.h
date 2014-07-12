// emacs -*- C++ -*-
#ifndef SUSY_WH_JESSYSTEMATICBREAKDOWN_H
#define SUSY_WH_JESSYSTEMATICBREAKDOWN_H

#include <string>
#include <map>

namespace susy
{
namespace wh
{
/*!

  An enum to loop and compute on the fly the JES syst broken down in 18 (x2) components.

  For details, see SUSYObjDef::ApplyJetSystematics() in
  SUSYPhys/SUSYTools/tags/SUSYTools-00-03-14/Root/SUSYObjDef.cxx

  There are 36 systematic variations; all of them have symmetric
  up/down variations.  Note: the original SUSYObjDef implementation
  had special cases for atlasfast samples.  I have dropped them for
  now since we are not using them.

  davide.gerbaudo@gmail.com
  Jul 2014
*/

enum JesSystematicComponents {
    EffectiveNP_1_Up=1 // note to self: 0 is reserved for NtSys_NOM
    ,EffectiveNP_1_Down
    ,EffectiveNP_2_Up
    ,EffectiveNP_2_Down
    ,EffectiveNP_3_Up
    ,EffectiveNP_3_Down
    ,EffectiveNP_4_Up
    ,EffectiveNP_4_Down
    ,EffectiveNP_5_Up
    ,EffectiveNP_5_Down
    ,EffectiveNP_6_Up
    ,EffectiveNP_6_Down
    ,EtaIntercalibration_Modelling_Up
    ,EtaIntercalibration_Modelling_Down
    ,EtaIntercalibration_StatAndMethod_Up
    ,EtaIntercalibration_StatAndMethod_Down
    ,SingleParticle_HighPt_Up
    ,SingleParticle_HighPt_Down
    ,RelativeNonClosure_Pythia8_Up
    ,RelativeNonClosure_Pythia8_Down
    ,PileupOffsetTermMuUp
    ,PileupOffsetTermMuDown
    ,PileupOffsetTermNPVUp
    ,PileupOffsetTermNPVDown
    ,PileupPtTermUp
    ,PileupPtTermDown
    ,PileupRhoTopologyUp
    ,PileupRhoTopologyDown
    ,CloseByUp
    ,CloseByDown
    ,FlavorCompUncertUp
    ,FlavorCompUncertDown
    ,FlavorResponseUncertUp
    ,FlavorResponseUncertDown
    ,BJesUp
    ,BJesDown
};
/// provide shorter names following our Histfitter convention (all caps etc.)
std::map<susy::wh::JesSystematicComponents, std::string> jesSystematicComponentsNames();
/// conversion trhough const-access to the map of names
/**
   This is a safety net against enum vs. names mismatch
 */
std::string jesSystematicComponents2str(const JesSystematicComponents v);
} // namespace wh
} // namespace susy

#endif // end include guard
