#!/bin/sh

# Script to setup an area to run SusyTest0 for the ss2l analysis
#
# Main steps:
# - source this script
#   > source setup_area_analysis.sh 2>&1 | tee setup_area_analysis.log
# - get input file from NtMaker
# - run
#    localSetupROOT --rootVersion 5.34.18-x86_64-slc6-gcc4.7
#    cd analysis
#    source RootCore/scripts/setup.sh
#    SusySel -i susyNt.root -s mc12_8TeV.177501.Herwigpp_UEEE3_CTEQ6L1_sM_wA_noslep_notauhad_WH_2Lep_1 2>&1  | tee SusySel.log
#
# davide.gerbaudo@gmail.com
# Feb 2015

PROD_DIR="analysis"

echo "Starting                          -- `date`"

mkdir -p ${PROD_DIR}
cd    ${PROD_DIR}

# see list of tags from
#  https://github.com/gerbaudo/SusyTest0/blob/e9421d4360991eee69d4b9aec94d11dad2229207/doc/tags.txt

SVN_INST=svn+ssh://svn.cern.ch/reps/atlasinst/Institutes
SVN_PHYS=svn+ssh://svn.cern.ch/reps/atlasphys/Physics
SVN_PANA=svn+ssh://svn.cern.ch/reps/atlasoff/PhysicsAnalysis
SVN_PREC=svn+ssh://svn.cern.ch/reps/atlasoff/Reconstruction
# atlas packages
svn co ${SVN_PREC}/Jet/JetResolution/tags/JetResolution-02-00-02                                            JetResolution
svn co ${SVN_PREC}/Jet/JetUncertainties/tags/JetUncertainties-00-08-06                                      JetUncertainties
svn co ${SVN_PREC}/Jet/JetAnalysisTools/ApplyJetResolutionSmearing/tags/ApplyJetResolutionSmearing-00-01-02 ApplyJetResolutionSmearing
svn co ${SVN_PANA}/AnalysisCommon/ReweightUtils/tags/ReweightUtils-00-02-13                                 ReweightUtils
svn co ${SVN_PANA}/D3PDTools/RootCore/tags/RootCore-00-02-99                                                RootCore
# minimal CalibrationDataInterface
BTAG_URL=${SVN_PANA}/JetTagging/JetTagPerformanceCalibration/CalibrationDataInterface/tags/CalibrationDataInterface-00-03-06
svn co \
 ${BTAG_URL}/cmt \
 ${BTAG_URL}/CalibrationDataInterface \
 ${BTAG_URL}/Root \
 ${BTAG_URL}/src \
 CalibrationDataInterface

# minimal SUSYTools
SUSY_URL="${SVN_PANA}/SUSYPhys/SUSYTools/tags/SUSYTools-00-03-14"
mkdir -p SUSYTools/SUSYTools SUSYTools/Root SUSYTools/cmt SUSYTools/data
svn export ${SUSY_URL}/SUSYTools/SUSYCrossSection.h SUSYTools/SUSYTools/SUSYCrossSection.h
svn export ${SUSY_URL}/SUSYTools/BTagCalib.h SUSYTools/SUSYTools/BTagCalib.h
svn export ${SUSY_URL}/Root/SUSYCrossSection.cxx SUSYTools/Root/SUSYCrossSection.cxx
svn export ${SUSY_URL}/Root/BTagCalib.cxx SUSYTools/Root/BTagCalib.cxx
svn export ${SUSY_URL}/cmt/Makefile.RootCore SUSYTools/cmt/Makefile.RootCore
svn co ${SUSY_URL}/data SUSYTools/data
sed -i "s/^PACKAGE_DEP.*/PACKAGE_DEP = CalibrationDataInterface/" SUSYTools/cmt/Makefile.RootCore

# susy packages
svn co ${SVN_PHYS}/SUSY/Analyses/WeakProduction/ChargeFlip/tags/ChargeFlip-00-00-19                ChargeFlip
svn co ${SVN_PHYS}/SUSY/Analyses/WeakProduction/DGTriggerReweight/tags/DGTriggerReweight-00-00-29  DGTriggerReweight
svn co ${SVN_PHYS}/SUSY/Analyses/WeakProduction/HistFitterTree/tags/HistFitterTree-00-00-33        HistFitterTree
svn co ${SVN_PHYS}/SUSY/Analyses/WeakProduction/LeptonTruthTools/tags/LeptonTruthTools-00-01-07    LeptonTruthTools
svn co ${SVN_PHYS}/SUSY/Analyses/WeakProduction/Mt2/tags/Mt2-00-00-01                              Mt2

svn co ${SVN_INST}/UCIrvine/mrelich/SusyXSReader/trunk                                             SusyXSReader

# my packages
wget --no-verbose https://github.com/gerbaudo/DileptonMatrixMethod/archive/SusyMatrixMethod-00-01-05.tar.gz
tar xzf SusyMatrixMethod-00-01-05.tar.gz
mv DileptonMatrixMethod-SusyMatrixMethod-00-01-05 SusyMatrixMethod

wget --no-verbose https://github.com/gerbaudo/SusyNtuple/archive/SusyNtuple-00-01-07-01.tar.gz
tar xzf SusyNtuple-00-01-07-01.tar.gz
mv SusyNtuple-SusyNtuple-00-01-07-01 SusyNtuple

# git clone git@github.com:gerbaudo/SusyTest0.git
git clone https://github.com/gerbaudo/SusyTest0.git
cd SusyTest0
git checkout -b v0.4.3 v0.4.3
cd -

setupATLAS
localSetupROOT --rootVersion 5.34.18-x86_64-slc6-gcc4.7
# Configure RootCore
cd RootCore
./configure
source scripts/setup.sh
cd ..

# Compile everything
$ROOTCOREDIR/scripts/build.sh


