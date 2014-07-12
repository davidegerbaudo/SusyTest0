#include "SusyTest0/JesSystematicBreakdown.h"

using namespace susy::wh;

std::map<susy::wh::JesSystematicComponents, std::string> susy::wh::jesSystematicComponentsNames()
{
    std::map<susy::wh::JesSystematicComponents, std::string> names;
    names[EffectiveNP_1_Up                       ] = "JES_EFFNP_1_UP"      ;
    names[EffectiveNP_1_Down                     ] = "JES_EFFNP_1_DN"      ;
    names[EffectiveNP_2_Up                       ] = "JES_EFFNP_2_UP"      ;
    names[EffectiveNP_2_Down                     ] = "JES_EFFNP_2_DN"      ;
    names[EffectiveNP_3_Up                       ] = "JES_EFFNP_3_UP"      ;
    names[EffectiveNP_3_Down                     ] = "JES_EFFNP_3_DN"      ;
    names[EffectiveNP_4_Up                       ] = "JES_EFFNP_4_UP"      ;
    names[EffectiveNP_4_Down                     ] = "JES_EFFNP_4_DN"      ;
    names[EffectiveNP_5_Up                       ] = "JES_EFFNP_5_UP"      ;
    names[EffectiveNP_5_Down                     ] = "JES_EFFNP_5_DN"      ;
    names[EffectiveNP_6_Up                       ] = "JES_EFFNP_6_UP"      ;
    names[EffectiveNP_6_Down                     ] = "JES_EFFNP_6_DN"      ;
    names[EtaIntercalibration_Modelling_Up       ] = "JES_ETAIC_MOD_UP"    ;
    names[EtaIntercalibration_Modelling_Down     ] = "JES_ETAIC_MOD_DN"    ;
    names[EtaIntercalibration_StatAndMethod_Up   ] = "JES_ETAIC_SAM_UP"    ;
    names[EtaIntercalibration_StatAndMethod_Down ] = "JES_ETAIC_SAM_DN"    ;
    names[SingleParticle_HighPt_Up               ] = "JES_SINGLE_UP"       ;
    names[SingleParticle_HighPt_Down             ] = "JES_SINGLE_DN"       ;
    names[RelativeNonClosure_Pythia8_Up          ] = "JES_CLOSURE_PY8_UP"  ;
    names[RelativeNonClosure_Pythia8_Down        ] = "JES_CLOSURE_PY8_DN"  ;
    names[PileupOffsetTermMuUp                   ] = "JES_PU_OFF_MU_UP"    ;
    names[PileupOffsetTermMuDown                 ] = "JES_PU_OFF_MU_DN"    ;
    names[PileupOffsetTermNPVUp                  ] = "JES_PU_OFF_NPV_UP"   ;
    names[PileupOffsetTermNPVDown                ] = "JES_PU_OFF_NPV_DN"   ;
    names[PileupPtTermUp                         ] = "JES_PU_PT_UP"        ;
    names[PileupPtTermDown                       ] = "JES_PU_PT_DN"        ;
    names[PileupRhoTopologyUp                    ] = "JES_PURHO_TOPO_UP"   ;
    names[PileupRhoTopologyDown                  ] = "JES_PURHO_TOPO_DN"   ;
    names[CloseByUp                              ] = "JES_CLOSEBY_UP"      ;
    names[CloseByDown                            ] = "JES_CLOSEBY_DN"      ;
    names[FlavorCompUncertUp                     ] = "JES_FLAV_COMP_UP"    ;
    names[FlavorCompUncertDown                   ] = "JES_FLAV_COMP_DN"    ;
    names[FlavorResponseUncertUp                 ] = "JES_FLAV_RES_UP"     ;
    names[FlavorResponseUncertDown               ] = "JES_FLAV_RES_DN"     ;
    names[BJesUp                                 ] = "JES_BJES_UP"         ;
    names[BJesDown                               ] = "JES_BJES_DN"         ;
    return names; 
}
//-----------------------------------------
std::string susy::wh::jesSystematicComponents2str(const JesSystematicComponents v)
{
    std::map<susy::wh::JesSystematicComponents, std::string> names = susy::wh::jesSystematicComponentsNames();
    return names.find(v)->second;
}
