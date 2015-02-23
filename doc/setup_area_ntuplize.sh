#!/bin/sh

# Script to setup an area where we compile the packages needed to run NtMaker
#
# Requirements:
# - access to the svn.cern.ch repositories (with kerb auth)
# - access to github
# - one NTUP_SUSY input file (the example below is *NTUP_SUSY*_p1512/)
#
# Main steps:
# - source this script
#   > source setup_area.sh
# - this will create a directory `prod` with all the packages, and compile them
# - run NtMaker
#   > localSetupROOT --rootVersion 5.34.18-x86_64-slc6-gcc4.7
#   > source RootCore/scripts/setup.sh
#   > NtMaker -f sM_wA_noslep_notauhad_WH_2Lep_1.txt -s mc12_8TeV.177501.Herwigpp_UEEE3_CTEQ6L1_sM_wA_noslep_notauhad_WH_2Lep_1 --saveTruth --filterOff 2>&1 | tee ntmaker.log
#
# davide.gerbaudo@gmail.com
# Mar 2013

PROD_DIR="prod"

echo "Starting                          -- `date`"

mkdir -p ${PROD_DIR}
cd    ${PROD_DIR}

# tag used to produce n0150
wget --no-verbose https://github.com/gerbaudo/SusyNtuple/archive/SusyNtuple-00-01-06.tar.gz
wget --no-verbose https://github.com/gerbaudo/SusyCommon/archive/SusyCommon-00-01-04.tar.gz
tar xzf SusyNtuple-00-01-06.tar.gz ; mv SusyNtuple-SusyNtuple-00-01-06 SusyNtuple
tar xzf SusyCommon-00-01-04.tar.gz ; mv SusyCommon-SusyCommon-00-01-04 SusyCommon

# git clone git@github.com:gerbaudo/SusyNtuple.git SusyNtuple
# cd SusyNtuple; git checkout SusyNtuple-00-01-06; cd -
# git clone git@github.com:gerbaudo/SusyCommon.git SusyCommon
# cd SusyCommon; git checkout SusyCommon-00-01-04; cd -


# susy packages
SVN_PHYS=svn+ssh://svn.cern.ch/reps/atlasphys/Physics
svn co ${SVN_PHYS}/SUSY/Analyses/WeakProduction/MultiLep/tags/MultiLep-01-06-04                             MultiLep

# package list from
# https://twiki.cern.ch/twiki/bin/viewauth/AtlasProtected/UCISusyNtuples
# SUSYTools-00-03-14 MultiLep-01-06-04 SusyNtuple-00-01-06 SusyCommon-00-01-04

sed -i -e '/asetup/s/^/#/' MultiLep/installscripts/install_script.sh # forget about asetup, we just need root
localSetupROOT --rootVersion 5.34.18-x86_64-slc6-gcc4.7
# the option below is the one needed for submit.py (see output of localSetupROOT)
# --rootVer=5.34/18 --cmtConfig=x86_64-slc6-gcc47-opt

source MultiLep/installscripts/install_script.sh

# echo "to fix SUSYTools-00-03-21 you need to" # tmp DG
# echo "sed -i  '/PACKAGE\_DEP/ s/$/ PhotonEfficiencyCorrection/' SUSYTools/cmt/Makefile.RootCore"

echo "Done compiling                    -- `date`"

