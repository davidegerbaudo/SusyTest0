#!/bin/env python

# Sanity checks on the hft trees: yields and basic plots
#
# todo:
# - combine errors
# - nicefy plots
# davide.gerbaudo@gmail.com
# March 2014

import collections
import datetime
import math
import optparse
import os
import pprint
from rootUtils import (drawAtlasLabel
                       ,getBinContents
                       ,getMinMax
                       ,graphWithPoissonError
                       ,increaseAxisFont
                       ,topRightLegend
                       ,importRoot
                       ,integralAndError
                       ,setWhPlotStyle
                       ,setAtlasStyle
                       )
r = importRoot()
from utils import (first
                   ,getCommandOutput
                   ,mkdirIfNeeded
                   ,filterWithRegexp
                   ,remove_duplicates
                   ,sortedAs
                   )

import SampleUtils
from CutflowTable import CutflowTable
import systUtils

usage="""

This code is used either (1) to fill the histos, or (2) to make plots
and tables. The output of (1) is used as input of (2).

Required inputs: HFT trees produced with `submitJobs.py ... --with-hft`
See cmd/hft.txt for details.

Example usage ('fill' mode):
%prog \\
 --syst EES_Z_UP
 --input-gen  out/susyplot/merged/ \\
 --input-fake out/fakepred/merged/ \\
 --output-dir     out/hft/         \\
 --verbose
 >& log/hft/check_hft_trees_fill.log

Example usage ('plot' mode):
%prog \\
 --syst ANY                 \\
 --input-dir out/hft/       \\
 --output-dir out/hft/plots \\
 --verbose                  \\
 >& log/hft/check_hft_trees_plot.log"""

def main() :
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-b', '--batch',  action='store_true', default=False, help='submit to batch (used in fill mode)')
    parser.add_option('-f', '--input-fake', help='location hft trees for fake')
    parser.add_option('-g', '--input-gen', help='location hft trees for everything else')
    parser.add_option('-i', '--input-dir')
    parser.add_option('-o', '--output-dir')
    parser.add_option('-s', '--syst', help="variations to process (default all). Give a comma-sep list or say 'weight', 'object', or 'fake'")
    parser.add_option('-e', '--exclude', help="skip some systematics, example 'EL_FR_.*'")
    parser.add_option('-v', '--verbose', action='store_true', default=False)
    parser.add_option('-l', '--list-systematics', action='store_true', default=False, help='list what is already in output_dir')
    parser.add_option('-L', '--list-all-systematics', action='store_true', default=False, help='list all possible systematics')

    (opts, args) = parser.parse_args()
    if opts.list_all_systematics :
        print "All systematics:\n\t%s"%'\n\t'.join(systUtils.getAllVariations())
        return
    if opts.list_systematics :
        print listExistingSyst(opts.input_dir)
        return
    inGenSpecified, inDirSpecified = opts.input_gen!=None, opts.input_dir!=None
    eitherMode = inGenSpecified != inDirSpecified
    if not eitherMode : parser.error("Run either in 'fill' or 'plot' mode")
    mode = 'fill' if inGenSpecified else 'plot' if inDirSpecified else None
    requiredOptions = (['input_fake', 'input_gen', 'output_dir'] if mode=='fill' else ['input_dir', 'output_dir'])
    allOptions = [x.dest for x in parser._get_all_options()[1:]]
    def optIsNotSpecified(o) : return not hasattr(opts, o) or getattr(opts,o) is None
    if any(optIsNotSpecified(o) for o in requiredOptions) :
        parser.error('Missing required option\n'+'\n'.join(["%s : %s"%(o, getattr(opts, o)) for o in requiredOptions]))
    if opts.verbose : print '\nUsing the following options:\n'+'\n'.join("%s : %s"%(o, str(getattr(opts, o))) for o in allOptions)

    if   mode=='fill' : runFill(opts)
    elif mode=='plot' : runPlot(opts)

def runFill(opts) :
    batchMode    = opts.batch
    inputFakeDir = opts.input_fake
    inputGenDir  = opts.input_gen
    outputDir    = opts.output_dir
    sysOption    = opts.syst
    excludedSyst = opts.exclude
    verbose      = opts.verbose

    if verbose : print "filling histos"
    mkdirIfNeeded(outputDir)
    systematics = ['NOM']
    anySys = sysOption==None
    if sysOption=='fake'   or anySys : systematics += systUtils.fakeSystVariations()
    if sysOption=='object' or anySys : systematics += systUtils.mcObjectVariations()
    if sysOption=='weight' or anySys : systematics += systUtils.mcWeightVariations()
    if sysOption and sysOption.count(',') : systematics = [s for s in systUtils.getAllVariations() if s in sysOption.split(',')]
    elif sysOption in systUtils.getAllVariations() : systematics = [sysOption]
    elif not anySys and len(systematics)==1 and sysOption!='NOM' : raise ValueError("Invalid syst %s"%str(sysOption))
    if excludedSyst : systematics = [s for s in systematics if s not in filterWithRegexp(systematics, excludedSyst)]

    if verbose : print "about to loop over these systematics:\n %s"%str(systematics)
    for syst in systematics :
        if batchMode :
            newOptions  = " --input-gen %s" % opts.input_gen
            newOptions += " --input-fake %s" % opts.input_fake
            newOptions += " --output-dir %s" % opts.output_dir
            newOptions += " --verbose %s" % opts.verbose
            newOptions += " --syst %s" % syst
            template = 'batch/templates/check_hft_fill.sh.template'
            script = "batch/hft_%s.sh"%syst
            scriptFile = open(script, 'w')
            scriptFile.write(open(template).read().replace('${opt}', newOptions))
            scriptFile.close()
            cmd = "qsub -q atlas"
            cmd += " -j oe -V " # join stdout/stderr, export env
            cmd += " -N %(jobname)s "
            cmd += " -o %(outlog)s "
            cmd += " %(scripname)s"
            cmd = cmd % {'jobname' : 'fill_'+syst,
                         'outlog' : 'log/hft/fill_'+syst+'.log',
                         'scripname' : script,
                         'options' : newOptions
                         }
            if verbose : print cmd
            out = getCommandOutput(cmd)
            if verbose : print out['stdout']
            if out['stderr'] : print  out['stderr']
            continue
        if verbose : print '---- filling ',syst
        samplesPerGroup = allSamplesAllGroups()
        [s.setSyst(syst) for g, samples in samplesPerGroup.iteritems() for s in samples]
        counters, histos = countAndFillHistos(samplesPerGroup=samplesPerGroup, syst=syst, verbose=verbose, outdir=outputDir)
        printCounters(counters)
        saveHistos(samplesPerGroup, histos, outputDir, verbose)

def runPlot(opts) :
    inputDir     = opts.input_dir
    outputDir    = opts.output_dir
    sysOption    = opts.syst
    excludedSyst = opts.exclude
    verbose      = opts.verbose
    mkdirIfNeeded(outputDir)
    buildTotBkg = systUtils.buildTotBackgroundHisto
    buildStat = systUtils.buildStatisticalErrorBand
    buildSyst = systUtils.buildSystematicErrorBand

    groups = allGroups()
    selections = allRegions()
    variables = variablesToPlot()
    for group in groups :
        group.setHistosDir(inputDir)
        group.exploreAvailableSystematics(verbose)
        group.filterAndDropSystematics(sysOption, excludedSyst, verbose)

    mkdirIfNeeded(outputDir)
    systematics = ['NOM']
    anySys = sysOption==None
    if sysOption=='fake'   or anySys : systematics += systUtils.fakeSystVariations()
    if sysOption=='object' or anySys : systematics += systUtils.mcObjectVariations()
    if sysOption=='weight' or anySys : systematics += systUtils.mcWeightVariations()
    if sysOption and sysOption.count(',') : systematics = [s for s in systUtils.getAllVariations() if s in sysOption.split(',')]
    elif sysOption in systUtils.getAllVariations() : systematics = [sysOption]
    if not anySys and len(systematics)==1 and sysOption!='NOM' : raise ValueError("Invalid syst %s"%str(sysOption))
    if excludedSyst : systematics = [s for s in systematics if s not in filterWithRegexp(systematics, excludedSyst)]
    if verbose : print "using the following systematics : %s"%str(systematics)

    fakeSystematics = [s for s in systematics if s in systUtils.fakeSystVariations()]
    mcSystematics = [s for s in systematics if s in systUtils.mcObjectVariations() + systUtils.mcWeightVariations()]

    simBkgs = [g for g in groups if g.isMcBkg]
    data, fake, signal = findByName(groups, 'data'), findByName(groups, 'fake'), findByName(groups, 'signal')

    for sel in selections :
        if verbose : print '-- plotting ',sel
        for var in variables :
            if verbose : print '---- plotting ',var
            for g in groups : g.setSystNominal()
            nominalHistoData    = data.getHistogram(variable=var, selection=sel, cacheIt=True)
            nominalHistoSign    = signal.getHistogram(variable=var, selection=sel, cacheIt=True)
            nominalHistoFakeBkg = fake.getHistogram(variable=var, selection=sel, cacheIt=True)
            nominalHistosSimBkg = dict([(g.name, g.getHistogram(variable=var, selection=sel, cacheIt=True)) for g in simBkgs])
            nominalHistosBkg    = dict([('fake', nominalHistoFakeBkg)] + [(g, h) for g, h in nominalHistosSimBkg.iteritems()])
            nominalHistoTotBkg  = buildTotBkg(histoFakeBkg=nominalHistoFakeBkg, histosSimBkgs=nominalHistosSimBkg)
            statErrBand = buildStat(nominalHistoTotBkg)
            systErrBand = buildSyst(fake=fake, simBkgs=simBkgs, variable=var, selection=sel,
                                    fakeVariations=fakeSystematics, mcVariations=mcSystematics, verbose=verbose)

            plotHistos(histoData=nominalHistoData, histoSignal=nominalHistoSign, histoTotBkg=nominalHistoTotBkg,
                       histosBkg=nominalHistosBkg,
                       statErrBand=statErrBand, systErrBand=systErrBand,
                       canvasName=(sel+'_'+var), outdir=outputDir, verbose=verbose)
    for group in groups :
        summary = group.variationsSummary()
        for selection, summarySel in summary.iteritems() :
            colW = str(12)
            header = ' '.join([('%'+colW+'s')%colName for colName in ['variation', 'yield', 'delta[%]']])
            lineTemplate = '%(sys)'+colW+'s'+'%(counts)'+colW+'s'+'%(delta)'+colW+'s'
            print "---- summary of variations for %s ----" % group.name
            print "---             %s                 ---" % selection
            print header
            print '\n'.join(lineTemplate%{'sys':s,
                                          'counts':(("%.3f"%c) if type(c) is float else (str(c)+str(type(c)))),
                                          'delta' :(("%.3f"%d) if type(d) is float else '--' if d==None else (str(d)+str(type(d)))) }
                            for s,c,d in summarySel)

def countAndFillHistos(samplesPerGroup={}, syst='', verbose=False, outdir='./') :

    selections = allRegions()
    variables = variablesToPlot()

    mcGroups, fakeGroups = mcDatasetids().keys(), ['fake']
    objVariations, weightVariations, fakeVariations = systUtils.mcObjectVariations(), systUtils.mcWeightVariations(), systUtils.fakeSystVariations()
    def groupIsRelevantForSys(g, s) :
        isRelevant = (s=='NOM' or (g in mcGroups and s in objVariations+weightVariations) or (g in fakeGroups and s in fakeVariations))
        if verbose and not isRelevant : print "skipping %s for %s"%(g, s)
        return isRelevant
    def dropIrrelevantGroupsForThisSys(groups, sys) : return dict((g, samples) for g, samples in groups.iteritems() if groupIsRelevantForSys(g, syst))
    def dropSamplesWithoutTree(samples) : return [s for s in samples if s.hasInputHftTree(msg='Warning! ')]
    def dropGroupsWithoutSamples(groups) : return dict((g, samples) for g, samples in groups.iteritems() if len(samples))
    samplesPerGroup = dropIrrelevantGroupsForThisSys(samplesPerGroup, syst)
    samplesPerGroup = dict((g, dropSamplesWithoutTree(samples)) for g, samples in samplesPerGroup.iteritems())
    samplesPerGroup = dropGroupsWithoutSamples(samplesPerGroup)

    groups = samplesPerGroup.keys()
    counters = bookCounters(groups, selections)
    histos = bookHistos(variables, groups, selections)
    for group, samplesGroup in samplesPerGroup.iteritems() :
        logLine = "---->"
        if verbose : print 1*' ',group
        histosGroup = histos  [group]
        countsGroup = counters[group]
        for sample in samplesGroup :
            if verbose : logLine +=" %s"%sample.name
            fillAndCount(histosGroup, countsGroup, sample)
        if verbose : print logLine
    if verbose : print 'done'
    return counters, histos

def printCounters(counters):
    countTotalBkg(counters)
    blindGroups   = [g for g in counters.keys() if g!='data']
    unblindGroups = [g for g in counters.keys()]
    tableSr  = CutflowTable(samples=blindGroups,   selections=signalRegions(), countsSampleSel=counters)
    tablePre = CutflowTable(samples=blindGroups,   selections=controlRegions(), countsSampleSel=counters)
    tableBld = CutflowTable(samples=unblindGroups, selections=blindRegions(), countsSampleSel=counters)
    for table in [tableSr, tablePre, tableBld] : table.nDecimal = 6
    print 4*'-',' sig regions ',4*'-'
    print tableSr.csv()
    print 4*'-',' pre regions ',4*'-'
    print tablePre.csv()
    print 4*'-',' blind regions ',4*'-'
    print tableBld.csv()
#___________________________________________________________
class BaseSampleGroup(object) :
    def __init__(self, name) :
        self.name = name
        self.setSystNominal()
        self.varCounts = collections.defaultdict(dict)
    @property
    def label(self) : return self.groupname if hasattr(self, 'groupname') else self.name
    @property
    def isFake(self) : return self.label=='fake'
    @property
    def isData(self) : return self.label=='data'
    @property
    def isMc(self) : return not (self.isFake or self.isData)
    @property
    def isSignal(self) : return self.label=='signal'
    @property
    def isMcBkg(self) : return self.isMc and not self.isSignal
    def isNeededForSys(self, sys) :
        return (sys=='NOM'
                or (self.isMc and sys in systUtils.mcWeightVariations())
                or (self.isMc and sys in systUtils.mcObjectVariations())
                or (self.isFake and sys in systUtils.fakeSystVariations()))
    def setSystNominal(self) : return self.setSyst()
    def setSyst(self, sys='NOM') :
        nominal = 'NOM' # do we have differnt names for nom (mc vs fake)?
        self.isObjSys    = sys in systUtils.mcObjectVariations()
        self.isWeightSys = sys in systUtils.mcWeightVariations()
        self.isFakeSys   = sys in systUtils.fakeSystVariations()
        def nameObjectSys(s) : return s if self.isMc else nominal
        def nameWeightSys(s) : return s if self.isMc else nominal
        def nameFakeSys(s) : return s if self.isFake else nominal
        def identity(s) : return s
        sysNameFunc = nameObjectSys if self.isObjSys else nameWeightSys if self.isWeightSys else nameFakeSys if self.isFakeSys else identity
        self.syst = sysNameFunc(sys)
        return self
    def logVariation(self, sys='', selection='', counts=0.0) :
        "log this systematic variation and internally store it as [selection][sys]"
        self.varCounts[selection][sys] = counts
        return self
    def variationsSummary(self) :
        summaries = {} # one summary for each selection
        for selection, sysCounts in self.varCounts.iteritems() :
            nominalCount = sysCounts['NOM']
            summaries[selection] = [(sys, sysCount, (100.0*(sysCount-nominalCount)/nominalCount) if nominalCount else None)
                                    for sys, sysCount in sortedAs(sysCounts, systUtils.getAllVariations())]
        return summaries

def findByName(bsgs=[], name='') : return [b for b in bsgs if b.name==name][0]
#___________________________________________________________
class Sample(BaseSampleGroup) :
    def __init__(self, name, groupname) :
        super(Sample, self).__init__(name) # this is either the name (for data and fake) or the dsid (for mc)
        self.groupname = groupname
        self.setHftInputDir()
    def setHftInputDir(self, dir='') :
        useDefaults = not dir
        defaultDir = 'out/fakepred' if self.isFake else 'out/susyplot'
        self.hftInputDir = defaultDir if useDefaults else dir
        return self
    @property
    def weightLeafname(self) :
        leafname = 'eventweight'
        if  self.isWeightSys : leafname += " * %s"%systUtils.mcWeightBranchname(self.syst)
        return leafname
    @property
    def filenameHftTree(self) :
        def dataFilename(sample, dir, sys) : return "%(dir)s/%(sys)s_%(sam)s.PhysCont.root" % {'dir':dir, 'sam':sample, 'sys':sys}
        def fakeFilename(sample, dir, sys) : return "%(dir)s/%(sys)s_fake.%(sam)s.PhysCont.root" % {'dir':dir, 'sam':sample, 'sys':sys}
        def mcFilename  (sample, dir, sys) : return "%(dir)s/%(sys)s_%(dsid)s.root" % {'dir':dir, 'sys':sys, 'dsid':sample}
        fnameFunc = dataFilename if self.isData else fakeFilename if self.isFake else mcFilename
        sys = (self.syst
               if (self.isMc and self.isObjSys or self.isFake and self.isFakeSys)
               else 'NOM')
        return fnameFunc(self.name, self.hftInputDir, sys)
    @property
    def hftTreename(self) :
        def dataTreename(samplename) : return "id_%(s)s.PhysCont" % {'s' : samplename}
        def fakeTreename(samplename) : return "id_fake.%(s)s.PhysCont"%{'s':samplename}
        def mcTreename(dsid=123456) :  return "id_%d"%dsid
        getTreename = dataTreename if self.isData else fakeTreename if self.isFake else mcTreename
        return getTreename(self.name)
    def hasInputHftFile(self, msg) :
        filename = self.filenameHftTree
        isThere = os.path.exists(filename)
        if not isThere : print msg+"%s %s missing : %s"%(self.groupname, self.name, filename)
        return isThere
    def hasInputHftTree(self, msg='') :
        treeIsThere = False
        if self.hasInputHftFile(msg) :
            filename, treename = self.filenameHftTree, self.hftTreename
            inputFile = r.TFile.Open(filename) if self.hasInputHftFile(msg) else None
            if inputFile :
                if inputFile.Get(treename) : treeIsThere = True
                else : print msg+"%s %s missing tree '%s' from %s"%(self.groupname, self.name, treename, filename)
            inputFile.Close()
        return treeIsThere
    def group(self) :
        return Group(self.groupname).setSyst(self.syst)
#___________________________________________________________
class Group(BaseSampleGroup) :
    def __init__(self, name) :
        super(Group, self).__init__(name)
        self.setSyst()
        self.setHistosDir()
        self._histoCache = collections.defaultdict(dict) # [syst][histoname]
    def setHistosDir(self, dir='') :
        self.histosDir = dir if dir else 'out/hft'
        return self
    @property
    def filenameHisto(self) :
        "file containig the histograms for the current syst"
        def dataFilename(group, dir, sys) : return "%(dir)s/%(sys)s_%(gr)s.PhysCont.root" % {'dir':dir, 'gr':group, 'sys':sys}
        def fakeFilename(group, dir, sys) : return "%(dir)s/%(sys)s_fake.%(gr)s.PhysCont.root" % {'dir':dir, 'gr':group, 'sys':sys}
        def mcFilename  (group, dir, sys) : return "%(dir)s/%(sys)s_%(gr)s.root" % {'dir':dir, 'sys':sys, 'gr':group}
        fnameFunc = dataFilename if self.isData else fakeFilename if self.isFake else mcFilename
        return fnameFunc(self.name, self.histosDir, self.syst)
    def exploreAvailableSystematics(self, verbose=False) :
        systs = ['NOM']
        if self.isFake :
            systs += systUtils.fakeSystVariations()
        elif self.isMc :
            systs += systUtils.mcObjectVariations()
            systs += systUtils.mcWeightVariations()
        self.systematics = []
        for sys in systs :
            self.setSyst(sys)
            if os.path.exists(self.filenameHisto) :
                self.systematics.append(sys)
        if verbose : print "%s : found %d variations : %s"%(self.name, len(self.systematics), str(self.systematics))
    def filterAndDropSystematics(self, include='.*', exclude=None, verbose=False) :
        nBefore = len(self.systematics)
        anyFilter = include or exclude
        toBeExcluded = filter(self,systematics, exclude) if exclude else []
        systs = ['NOM'] if 'NOM' in self.systematics else []
        if include : systs += filterWithRegexp(self.systematics, include)
        if exclude : systs  = [s for s in systs if toBeExcluded and s not in toBeExcluded]
        self.systematics = systs if anyFilter else self.systematics
        self.systematics = remove_duplicates(self.systematics)
        nAfter = len(self.systematics)
        if verbose : print "%s : dropped %d systematics, left with %s"%(self.name, nBefore-nAfter, str(self.systematics))
        assert self.systematics.count('NOM')==1 or not nBefore, "%s : 'NOM' required %s"%(self.name, str(self.systematics))
    def getHistogram(self, variable, selection, cacheIt=False) :
        hname = histoName(sample=self.name, selection=selection, variable=variable)
        histo = None
        try :
            histo = self._histoCache[self.syst][hname]
        except KeyError :
            file = r.TFile.Open(self.filenameHisto)
            if not file : print "missing file %s"%self.filenameHisto
            hname = histoName(sample=self.name, selection=selection, variable=variable)
            histo = file.Get(hname)
            if not histo : print "%s : cannot get histo %s"%(self.name, hname)
            elif cacheIt :
                histo.SetDirectory(0)
                self._histoCache[self.syst][hname] = histo
            else :
                histo.SetDirectory(0)
                file.Close()
        if variable=='onebin' and histo : self.logVariation(self.syst, selection, histo.Integral(0, -1))
        return histo
    def getBinContents(self, variable, selection) :
        return getBinContents(self.getHistogram)
#___________________________________________________________
def allGroups(noData=False, noSignal=True) :
    return ([k for k in mcDatasetids().keys() if k!='signal' or not noSignal]
            + ([] if noData else ['data'])
            + ['fake']
            )
def llPairs() : return ['ee', 'em', 'mm']
def njetSelections() : return ['1jet', '23jets']
def signalRegions() :
    return ["%(ll)sSR%(nj)s"%{'ll':ll, 'nj':nj} for ll in llPairs() for nj in njetSelections()]
def controlRegions() :
    return ['pre'+r for r in signalRegions()]
def blindRegions() :
    "blind regions, where the mlj/mljj cut has been reversed"
    return [blindRegionFromAnyRegion(r) for r in signalRegions()]
def allRegions() :
    return signalRegions() + controlRegions() + blindRegions()
def blindRegionFromAnyRegion(sr) :
    "given any selection region, provide the corresponding blind region"
    if   'bld' in sr : return sr
    elif 'pre' in sr : return sr.replace('pre', 'bld')
    else             : return 'bld'+sr
def signalRegionFromAnyRegion(sr) :
    "given any selection region, provide the corresponding signal region"
    return sr.replace('pre','').replace('bld','')
def selectionFormulas(sel) :
    ee, em, mm = 'isEE', 'isEMU', 'isMUMU'
    pt32  = '(lept1Pt>30000.0 && lept2Pt>20000.0)'
    pt33  = '(lept1Pt>30000.0 && lept2Pt>30000.0)'
    j1    = '(L2nCentralLightJets==1)'
    j23   = '(L2nCentralLightJets==2 || L2nCentralLightJets==3)'
    vetoZ = '(L2Mll<(91200.0-10000.0) || L2Mll>(91200.0+10000.))'
    dEll  = '(TMath::Abs(deltaEtaLl)<1.5)'
    ss    = '(!isOS || L2qFlipWeight!=1.0)' # ssOrQflip
    mlj1  = 'mlj < 90000.0'
    mlj2  = 'mljj<120000.0'
    formulas = {
        'eeSR1jet'   : '('+ee+' && '+ss+' && '+j1 +' && '+pt32+' && '+vetoZ+' && '+mlj1+' && L2METrel>55000.0 && Ht>200000.0)',
        'eeSR23jets' : '('+ee+' && '+ss+' && '+j23+' && '+pt32+' && '+vetoZ+' && '+mlj2+' && L2METrel>30000.0 &&                mtmax>100000.0)',
        'mmSR1jet'   : '('+mm+' && '+ss+' && '+j1 +' && '+pt32+' && '+dEll +' && '+mlj1+' &&                     Ht>200000.0 && mtmax>100000.0)',
        'mmSR23jets' : '('+mm+' && '+ss+' && '+j23+' && '+pt33+' && '+dEll +' && '+mlj2+' &&                     Ht>220000.0)',
        'emSR1jet'   : '('+em+' && '+ss+' && '+j1 +' && '+pt33+' && '+dEll +' && '+mlj1+' && mtllmet>120000.0 &&                mtmax>110000.0)',
        'emSR23jets' : '('+em+' && '+ss+' && '+j23+' && '+pt33+' && '+dEll +' && '+mlj2+' && mtllmet>110000.0 )',
        }
    for f in formulas.keys() :
        formulas['pre'+f] = formulas[f].replace(mlj1, '1').replace(mlj2, '1')
    mlj1Not, mlj2Not = mlj1.replace('<','>'), mlj2.replace('<','>')
    for f in formulas.keys() :
        formulas['bld'+f] = formulas[f].replace(mlj1, mlj1Not).replace(mlj2, mlj2Not)
    return formulas[sel]

def fillAndCount(histos, counters, sample, blind=True) :
    group    = sample.group
    filename = sample.filenameHftTree
    treename = sample.hftTreename
    file = r.TFile.Open(filename)
    tree = file.Get(treename)
    selections = allRegions()
    selWeights = dict((s, r.TTreeFormula(s, selectionFormulas(s), tree)) for s in selections)
    weightFormula = r.TTreeFormula('weightFormula', sample.weightLeafname, tree)
    for ev in tree :
        weight = weightFormula.EvalInstance()
        passSels = dict((s, selWeights[s].EvalInstance()) for s in selections)
        for sel in selections : counters[sel] += (weight if passSels[sel] else 0.0)
        for sel in selections :
            fillHisto = passSels[sel]
            if blind and sample.isData :
                if sel in signalRegions() : fillHisto = False
                else : fillHisto = passSels[blindRegionFromAnyRegion(sel)] and not passSels[signalRegionFromAnyRegion(sel)]
            oneJet = tree.L2nCentralLightJets==1
            mev2gev = 1.0e-3
            mljj = mev2gev*(tree.mlj if oneJet else tree.mljj)
            if fillHisto :
                histos[sel]['mljj'  ].Fill(mljj, weight)
                histos[sel]['onebin'].Fill(1.0,  weight)
            # checks
            if (False and fillHisto
                and sel in signalRegions()
                and sample.isFake ) :
                channel = 'ee' if tree.isEE else 'mm' if tree.isMUMU else 'em'
                print "ev %d run %d channel %s sel %s weight %f"%(tree.runNumber, tree.eventNumber, channel, sel, weight)
    file.Close()

def dataSampleNames() :
    return ["period%(period)s.physics_%(stream)s"%{'period':p, 'stream':s}
            for p in ['A','B','C','D','E','G','H','I','J','L']
            for s in ['Egamma','Muons']]
def mcDatasetids() :
    "encode the grouping we use to make HFT plots; from HistFitter_TreeCreator.py"
    return {
        'Higgs' : [160155, 160205, 160255, 160305, 160505, 160555,
                   160655, 160705, 160755, 160805, 161005, 161055,
                   161105, 161155, 161305, 161555, 161566, 161577,
                   161675, 161686, 161697, 161708, 161719, 161730,
                   161805, 167418, 169072],
        'Top' : [110001, 108346, 119353, 174830, 174831, 119355,
                   174832, 174833, 179991, 179992, 119583, 169704,
                   169705, 169706, 158344],
        'WW' : [177997, 183734, 183736, 183738, 169471, 169472,
                169473, 169474, 169475, 169476, 169477, 169478,
                169479, 126988, 126989, 167006],
        'ZV' : [179974],
#                 , 179975, 183585, 183587, 183589, 183591,
#                 183735, 183737, 183739, 167007, 177999, 183586,
#                 183588, 183590, 126894, 179396, 167008],
        'Zjets' : [178354, 178355, 178356, 178357, 178358, 178359,
                   178360, 178361, 178362, 178363, 178364, 178365,
                   178366, 178367, 178368, 178369, 178370, 178371,
                   178372, 178373, 178374, 178375, 178376, 178377,
                   178378, 178379, 178380, 178381, 178382, 178383,
                   178384, 178385, 178386, 178387, 178388, 178389,
                   178390, 178391, 178392, 178393, 178394, 178395,
                   178396, 178397, 178398, 178399, 178400, 178401,
                   178402, 178403, 178404, 178405, 178406, 178407,
                   117650, 117651, 117652, 117653, 117654, 117655,
                   110805, 110806, 110807, 110808, 110817, 110818,
                   110819, 110820, 117660, 117661, 117662, 117663,
                   117664, 117665, 110809, 110810, 110811, 110812,
                   110821, 110822, 110823, 110824, 117670, 117671,
                   117672, 117673, 117674, 117675, 110813, 110814,
                   110815, 110816, 110825, 110826, 110827, 110828],
        'signal' : [177501]
#                     , 177502, 177503, 177504, 177505, 177506,
#                     177507, 177508, 177509, 177510, 177511, 177512,
#                     177513, 177514, 177515, 177516, 177517, 177518,
#                     177519, 177520, 177521, 177522, 177523, 177524,
#                     177525, 177526]
        }
def allSamplesAllGroups() :
    asg = dict( [(group, [Sample(groupname=group, name=dsid) for dsid in dsids]) for group, dsids in mcDatasetids().iteritems()]
               +[('data', [Sample(groupname='data', name=s) for s in dataSampleNames()])]
               +[('fake', [Sample(groupname='fake', name=s) for s in dataSampleNames()])])
    return asg
def allGroups() :
    return [Group(g) for g in mcDatasetids().keys()+['data']+['fake']]

def stackedGroups() :
    return [g for g in allSamplesAllGroups().keys() if g not in ['data', 'signal']]

def variablesToPlot() :
    return ['onebin','mljj']
    return ['pt0','pt1','mll','mtmin','mtmax','mtllmet','ht','metrel','dphill','detall',
            'mt2j','mljj','dphijj','detajj']
def histoName(sample, selection, variable) : return "h_%s_%s_%s"%(variable, sample, selection)
def bookHistos(variables, samples, selections) :
    "book a dict of histograms with keys [sample][selection][var]"
    def histo(variable, sam, sel) :
        twopi = +2.0*math.pi
        mljjLab = 'm_{lj}' if '1j' in sel else 'm_{ljj}'
        h = None
        if   v=='onebin'  : h = r.TH1F(histoName(sam, sel, 'onebin' ), ';; entries',                             1, 0.5,   1.5)
        elif v=='pt0'     : h = r.TH1F(histoName(sam, sel, 'pt0'    ), ';p_{T,l0} [GeV]; entries/bin',          25, 0.0, 250.0)
        elif v=='pt1'     : h = r.TH1F(histoName(sam, sel, 'pt1'    ), ';p_{T,l1} [GeV]; entries/bin',          25, 0.0, 250.0)
        elif v=='mll'     : h = r.TH1F(histoName(sam, sel, 'mll'    ), ';m_{l0,l1} [GeV]; entries/bin',         25, 0.0, 250.0)
        elif v=='mtmin'   : h = r.TH1F(histoName(sam, sel, 'mtmin'  ), ';m_{T,min}(l, MET) [GeV]; entries/bin', 25, 0.0, 400.0)
        elif v=='mtmax'   : h = r.TH1F(histoName(sam, sel, 'mtmax'  ), ';m_{T,max}(l, MET) [GeV]; entries/bin', 25, 0.0, 400.0)
        elif v=='mtllmet' : h = r.TH1F(histoName(sam, sel, 'mtllmet'), ';m_{T}(l+l, MET) [GeV]; entries/bin',   25, 0.0, 600.0)
        elif v=='ht'      : h = r.TH1F(histoName(sam, sel, 'ht'     ), ';H_{T} [GeV]; entries/bin',             25, 0.0, 800.0)
        elif v=='metrel'  : h = r.TH1F(histoName(sam, sel, 'metrel' ), ';MET_{rel} [GeV]; entries/bin',         25, 0.0, 300.0)
        elif v=='dphill'  : h = r.TH1F(histoName(sam, sel, 'dphill' ), ';#Delta#phi(l, l) [rad]; entries/bin',  25, 0.0, twopi)
        elif v=='detall'  : h = r.TH1F(histoName(sam, sel, 'detall' ), ';#Delta#eta(l, l); entries/bin',        25, 0.0, +3.0 )
        elif v=='mt2j'    : h = r.TH1F(histoName(sam, sel, 'mt2j'   ), ';m^{J}_{T2} [GeV]; entries/bin',        25, 0.0, 500.0)
        elif v=='mljj'    : h = r.TH1F(histoName(sam, sel, 'mljj'   ), ';'+mljjLab+' [GeV]; entries/30GeV',     15, 0.0, 450.0)
        elif v=='dphijj'  : h = r.TH1F(histoName(sam, sel, 'dphijj' ), ';#Delta#phi(j, j) [rad]; entries/bin',  25, 0.0, twopi)
        elif v=='detajj'  : h = r.TH1F(histoName(sam, sel, 'detajj' ), ';#Delta#eta(j, j); entries/bin',        25, 0.0, +3.0 )
        else : print "unknown variable %s"%v
        h.Sumw2()
        h.SetDirectory(0)
        return h
    return dict([(sam, dict([(sel, dict([(v, histo(v, sam, sel)) for v in variables]))
                         for sel in selections]))
                 for sam in samples])
def bookCounters(samples, selections) :
    "book a dict of counters with keys [sample][selection]"
    return dict((s, dict((sel, 0.0) for sel in selections)) for s in samples)
def countTotalBkg(counters={'sample' : {'sel':0.0}}) :
    backgrounds = [g for g in counters.keys() if g!='signal' and g!='data']
    selections = first(counters).keys()
    counters['totBkg'] = dict((s, sum(counters[b][s] for b in backgrounds)) for s in selections)
def getGroupColor(g) :
    oldColors = SampleUtils.colors
    newColors = [('signal',r.kMagenta), ('WW',r.kAzure-9), ('Higgs',r.kYellow-9)]
    colors = dict((g,c) for g,c in [(k,v) for k,v in oldColors.iteritems()] + newColors)
    def hftGroup2stdGroup(_) :
        fromTo = {'Zjets':'zjets', 'Top':'ttbar', 'ZV':'diboson',}
        return fromTo[_] if _ in fromTo else _
    g = hftGroup2stdGroup(g)
    return colors[g]

def plotHistos(histoData=None, histoSignal=None, histoTotBkg=None, histosBkg={},
               statErrBand=None, systErrBand=None, # these are TGraphAsymmErrors
               canvasName='canvas', outdir='./', verbose=False,
               drawStatErr=False, drawSystErr=False,
               drawYieldAndError=False) :
    "Note: blinding can be required for only a subrange of the histo, so it is taken care of when filling"
    #setWhPlotStyle()
    setAtlasStyle()
    padMaster = histoData
    if verbose : print "plotting ",padMaster.GetName()
    can = r.TCanvas(canvasName, padMaster.GetTitle(), 800, 600)
    can.cd()
    can._hists = [padMaster]
    padMaster.Draw('axis')
    can.Update() # necessary to fool root's dumb object ownership of the stack
    stack = r.THStack('stack_'+padMaster.GetName(), '')
    r.SetOwnership(stack, False)
    can._hists.append(stack)
    leg = topRightLegend(can, 0.225, 0.325)
    can._leg = leg
    leg.SetBorderSize(0)
    leg._reversedEntries = []
    for group, histo in sortedAs(histosBkg, stackedGroups()) :
        histo.SetFillColor(getGroupColor(group))
        histo.SetLineWidth(2)
        histo.SetLineColor(r.kBlack)
        stack.Add(histo)
        can._hists.append(histo)
        leg._reversedEntries.append((histo, group, 'F'))
    for h, g, o in leg._reversedEntries[::-1] : leg.AddEntry(h, g, o) # stack goes b-t, legend goes t-b
    stack.Draw('hist same')
    histoData.SetMarkerStyle(r.kFullCircle)
    histoData.SetLineWidth(2)
    dataGraph = graphWithPoissonError(histoData)
    dataGraph.Draw('same p')
    if histoSignal :
        histoSignal.SetLineColor(getGroupColor('signal'))
        histoSignal.SetLineWidth(2)
        histoSignal.Draw('histo same')
        leg.AddEntry(histoSignal, '(m_{C1},m_{N1})=(130, 0)GeV', 'l')
    if statErrBand and drawStatErr :
        statErrBand.SetFillStyle(3006)
        statErrBand.Draw('E2 same')
        leg.AddEntry(statErrBand, 'stat', 'f')
    if systErrBand and drawSystErr :
        systErrBand.SetFillStyle(3007)
        systErrBand.Draw('E2 same')
        leg.AddEntry(systErrBand, 'syst', 'f')
    totErrBand = systUtils.addErrorBandsInQuadrature(statErrBand, systErrBand)
    if totErrBand :
        totErrBand.Draw('E2 same')
        totErrBand.SetFillStyle(3005)
        leg.AddEntry(totErrBand, 'stat+syst', 'f')
    leg.Draw('same')
    can.Update()
    tex = r.TLatex()
    tex.SetTextSize(0.5 * tex.GetTextSize())
    tex.SetNDC(True)
    label  = "%s tot bkg : "%(can.GetName())
    label += "%.3f #pm %.3f (stat)"%(integralAndError(histoTotBkg))
    if systErrBand :
        sysUp, sysDo = systUtils.totalUpDownVariation(systErrBand)
        label += "#pm #splitline{%.3f}{%.3f} (syst)"%(sysUp, sysDo)
    if drawYieldAndError : tex.DrawLatex(0.10, 0.95, label)
    drawAtlasLabel(can, xpos=0.125, align=13)
    yMin, yMax = getMinMax([histoData, dataGraph, histoTotBkg, histoSignal, totErrBand])
    padMaster.SetMinimum(0.0)
    padMaster.SetMaximum(1.1 * yMax)
    increaseAxisFont(padMaster.GetXaxis())
    increaseAxisFont(padMaster.GetYaxis())
    can.RedrawAxis()
    can.Update() # force stack to create padMaster
    for ext in ['png','eps'] : can.SaveAs(outdir+'/'+can.GetName()+'.'+ext)

def listExistingSyst(dir) :
    print "listing systematics from ",dir
    print "...not implemented..."
def saveHistos(samplesPerGroup={}, histosPerGroup={}, outdir='./', verbose=False) :
    for groupname, histos in histosPerGroup.iteritems() :
        group = first(samplesPerGroup[groupname]).group().setHistosDir(outdir)
        outFilename = group.filenameHisto
        if verbose : print "creating file %s"%outFilename
        file = r.TFile.Open(outFilename, 'recreate')
        file.cd()
        for g, histosPerSel in histosPerGroup.iteritems() :
            for sel, histos in histosPerSel.iteritems() :
                for var, h in histos.iteritems() : h.Write()
        file.Close()


if __name__=='__main__' :
    main()
