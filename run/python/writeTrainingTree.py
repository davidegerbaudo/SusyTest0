#!/bin/env python

# compute the variables needed to train a multivariate discriminant, and save them to a tree
#
# davide.gerbaudo@gmail.com
# Jan 2014

import array
import glob
import os
import ROOT as r
r.gROOT.SetStyle('Plain')
r.gROOT.SetBatch(True)                     # no windows popping up
r.PyConfig.IgnoreCommandLineOptions = True # don't let root steal our cmd-line options
from utils import first
from rootUtils import drawLegendWithDictKeys
from SampleUtils import colors

rootcoredir = os.environ['ROOTCOREDIR']
r.gROOT.LoadMacro(rootcoredir+'/scripts/load_packages.C+')
r.load_packages()
tlv = r.TLorentzVector

treename = 'SusySel'
tag = 'Jan_06'
basedir = '/gdata/atlas/gerbaudo/wh/Susy2013_Nt_01_04_dev/SusyTest0/run/out/susysel/'
samples = ['McAtNloJimmy_CT10_ttbar_LeptonFilter','Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_1_']
filenames = dict((s, glob.glob(basedir+'/'+s+'*'+tag+'.root')) for s in samples)
assert all(len(v)==1 for v in filenames.values()),"ambiguous filenames\n%s"%str(filenames)
filenames = dict((s,v[0]) for s,v in filenames.iteritems())

def FourMom2TLorentzVector(fm) :
    l = tlv()
    l.SetPxPyPzE(fm.px, fm.py, fm.pz, fm.E)
    return l

fm2tlv = FourMom2TLorentzVector # move to module
mt2 = r.mt2_bisect.mt2()
def computeMt2(a, b, met, zeroMass, lspMass) :
    pa    = array.array( 'd', [0.0 if zeroMass else a.M(), a.Px(), a.Py() ] )
    pb    = array.array( 'd', [0.0 if zeroMass else b.M(), b.Px(), b.Py() ] )
    pmiss = array.array( 'd', [0.0, met.Px(), met.Py() ] )
    mt2.set_momenta(pa, pb, pmiss)
    mt2.set_mn(lspMass)
    return mt2.get_mt2()
def computeMt2j(l0, l1, j0, j1, met, zeroMass=False, lspMass=0.0) :
    "As described in CMS-SUS-13-017"
    mt2_00 = computeMt2(l0+j0, l1+j1, met, zeroMass, lspMass)
    mt2_01 = computeMt2(l1+j0, l0+j1, met, zeroMass, lspMass)
    return min([mt2_00, mt2_01])
def computeMljj(l0, l1, j0, j1) :
    "Todo: extened combinatorics to N_j>2; good enough for now (we have few cases with >=3j)"
    jj = j0+j1
    dr0, dr1 = jj.DeltaR(l0), jj.DeltaR(l1)
    return (jj+l0).M() if dr0<dr1 else (jj+l1).M()

hs_mt2j = dict()
hs_mljj = dict()
hs_slpt = dict()

r.gROOT.ProcessLine("struct vars { \
float pt0; \
float pt1; \
float mll; \
};")
vars = r.vars()

def createOutTree(filenames) :
  
  for sample, filename in filenames.iteritems() :
      print sample
      outFile = r.TFile.Open('/tmp/'+sample+'.root', 'recreate')
      outTree = r.TTree("training","Training tree")
      outTree.Branch('vars', vars, 'pt0/F:pt1:mll')
      outTree.SetDirectory(outFile)
      file = r.TFile.Open(filename)
      tree = file.Get(treename)
      print "processing %s (%d entries)"%(sample, tree.GetEntries())
      iEvent = 0
      for event in tree :
          l0 = fm2tlv(event.l0)
          l1 = fm2tlv(event.l1)
          met = fm2tlv(event.met)
          jets = [fm2tlv(j) for j in event.jets]
          lepts = [fm2tlv(l) for l in event.lepts]
          pars = event.pars
          if len(jets)<2 : continue
          j0, j1 = jets[0], jets[1]
          vars.pt0 = l0.Pt()
          vars.pt1 = l1.Pt()
          vars.mll = (l0+l1).M()
          outTree.Fill()
          iEvent += 1
          if iEvent  > 10 : continue
  
      print "filled ",outTree.GetEntries()," entries"
      outFile.Write()
      outFile.Close()

def train(sigFiles=[], bkgFiles=[]) :
    bkgTree, sigTree = r.TChain('training'), r.TChain('training')
    for b in bkgFiles : bkgTree.Add(b)
    for s in sigFiles : sigTree.Add(s)
    r.TMVA.Tools.Instance()
    fileOut = r.TFile("testmva.root","RECREATE")
    factory = r.TMVA.Factory("TMVAClassification", fileOut,
                             ":".join(["!V",
                                       "!Silent",
                                       "Color",
                                       "DrawProgressBar",
                                       "Transformations=I;D;P;G,D",
                                       "AnalysisType=Classification"]
                                      ))
    factory.AddVariable('pt0', 'F')
    factory.AddVariable('pt1', 'F')
    factory.AddVariable('mll', 'F')
    sigCut = r.TCut("")
    bkgCut = r.TCut("")
    factory.AddSignalTree    (sigTree)
    factory.AddBackgroundTree(bkgTree)
    factory.SetBackgroundWeightExpression("")
    factory.SetSignalWeightExpression("")
    factory.PrepareTrainingAndTestTree(sigCut, bkgCut,
                                       ":".join(["nTrain_Signal=0",
                                                 "nTrain_Background=0",
                                                 "SplitMode=Random",
                                                 "NormMode=NumEvents",
                                                 "V"
                                                 ]))
    method = factory.BookMethod(r.TMVA.Types.kBDT, "BDT",
                                ":".join(["!H",
                                          "V",
                                          "NTrees=200",
                                          "nEventsMin=50",
                                          "MaxDepth=3",
                                          "BoostType=AdaBoost",
                                          "AdaBoostBeta=0.5",
                                          "SeparationType=GiniIndex",
                                          "nCuts=20",
                                          "PruneMethod=NoPruning",
                                          ]))

    factory.TrainAllMethods()
    factory.TestAllMethods()
    factory.EvaluateAllMethods()
    
    
#createOutTree(filenames)

train(sigFiles=['/tmp/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_1_.root'],
      bkgFiles=['/tmp/McAtNloJimmy_CT10_ttbar_LeptonFilter.root'])





