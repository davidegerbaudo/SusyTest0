#!/bin/env python

# produce significance plots
#
# Inputs:
# - the pickle files with the number of signal and background events
#   (produced with makeCutflowTable.py)
#
# davide.gerbaudo@gmail.com
# Jan 2013

import collections, optparse, sys
from rootUtils import importRoot
r = importRoot()
from PickleUtils import readFromPickle
from SampleUtils import ModeAWhDbPar, ModeAWhDbReqid

#########
# default parameters [begin]
defaultSigPickle   = 'counts_signal.pkl'
defaultBkgPickle   = 'counts_backgr.pkl'
defaultSigScale    = 1.0
# default parameters [end]
#########

parser = optparse.OptionParser()
parser.add_option("-s", "--sig-file", dest="sig", default=defaultSigPickle,
                  help="file with signal counts, default : %s" % defaultSigPickle)
parser.add_option("-b", "--bkg-file", dest="bkg", default=defaultBkgPickle,
                  help="file with background counts, default : %s" % defaultBkgPickle)
parser.add_option("-S", "--scale-sig", dest="sigScale", default=defaultSigScale, type='float',
                  help="scale the signal yield by this factor (default %.1f)" % defaultSigScale)
parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False,
                  help="print more details about what is going on")
(options, args) = parser.parse_args()
sigInputFname   = options.sig
bkgInputFname   = options.bkg
sigScale        = options.sigScale
verbose         = options.verbose

countsSigSampleSel = readFromPickle(sigInputFname)
countsBkgSampleSel = readFromPickle(bkgInputFname)
interestingSelections = ["sr%d"%i for i in range(6,9+1)]
countBkgTot = collections.defaultdict(float)
for sample, countsSel in countsBkgSampleSel.iteritems() :
    if sample in ['data', 'totbkg'] : continue
    for sel, counts in countsSel.iteritems() :
        if sel not in interestingSelections : continue
        print 'adding '+sample+' to '+sel+' ('+str(counts)+')'
        countBkgTot[sel] += counts
print countBkgTot

reqDb = ModeAWhDbReqid()
parDb = ModeAWhDbPar()

def selIsRelevant(sel) : return sel.startswith('sr')
def selIsFinal(sel) : return sel in interestingSelections

mc1Range = {'min': min(parDb.allMc1()), 'max' : max(parDb.allMc1())}
mn1Range = {'min': min(parDb.allMn1()), 'max' : max(parDb.allMn1())}

histos = dict()
backgroundRelErr = 0.2
for sample, countsSel in countsSigSampleSel.iteritems() :
    mc1, mn1 = parDb.mc1Mn1ByReqid(reqDb.reqidBySample(sample))
    print "%s (%.1f, %.1f) " % (sample, mc1, mn1)
    for sel, counts in sorted(countsSel.iteritems()) :
        if not selIsFinal(sel) : continue
        histo = histos[sel] if sel in histos else r.TH2F(sel+'_zn',
                                                         sel+" Z_{n} (#deltab=%.2f);mc_{1};mn_{1}"%backgroundRelErr,
                                                         50, float(mc1Range['min']), float(mc1Range['max']),
                                                         50, float(mn1Range['min']), float(mn1Range['max']))
        if sel not in histos : histos[sel] = histo
        sig, bkg, dBkg = counts, countBkgTot[sel], backgroundRelErr
        zn = r.RooStats.NumberCountingUtils.BinomialExpZ(sigScale*sig, bkg, dBkg)
        histo.Fill(mc1, mn1, zn)
        print "%s : %.2f / %.2f -> %.2f"%(sel, sig, bkg, zn)

r.gStyle.SetPaintTextFormat('.2f')
for s, h in histos.iteritems() :
    c = r.TCanvas(s, s, 800, 600)
    c.cd()
    h.SetStats(0)
    h.SetMarkerSize(1.75*h.GetMarkerSize())
    hOnlyPos = h.Clone(h.GetName()+'OnlyPos')
    for i in range(hOnlyPos.GetNbinsX()+1) :
        for j in range(hOnlyPos.GetNbinsY()+1) :
            if hOnlyPos.GetBinContent(i,j)<0. :
                hOnlyPos.SetBinContent(i,j,0.)
                hOnlyPos.SetBinError(i,j,0.)
    h.Draw('text')
    hOnlyPos.Draw('colz same') #contz
    h.Draw('text same')
    def writeScale(can, scale, font='') :
        tex = r.TLatex(0.0, 0.0, '')
        tex.SetNDC()
        if font : tex.SetTextFont(font)
        tex.SetTextAlign(31)
        tex.DrawLatex(1.0-can.GetTopMargin(), 1.0-can.GetRightMargin(), "Signal: x %.1f"%scale)
        can.Update()
    if sigScale != defaultSigScale : writeScale(c, sigScale, h.GetTitleFont())
    c.SaveAs('signif_'+s+'.png')



