#ifndef FinalNewFake_h
#define FinalNewFake_h


/////////////////////////////////////////////////////////
// Format the histograms to be used as input to the    //
// matrix method. Currently following the 2-lep        //
// procedure outlined in the conf note. Reason being   //
// that the # of true fakes in the signal region makes //
// it difficult to extract reliable %. Maybe updated   //
/////////////////////////////////////////////////////////

#include "SusyTest0/FakePlotting.h"
#include "SusyTest0/SusyAnaDefsMatt.h"
#include "TParameter.h"
#include <string>

// --------- Scale Factors -----------//
const float el_qcdSF  = 0.73345;
const float mu_qcdSF  = 0.79059;
const float el_convSF = 1.24359;
const float el_realSF = 0.99729;
const float mu_realSF = 0.99719;


// --------- Systematic Errors -----------//

			    
// HF/LF error for electrons
const float el_HFLFerr = 0.05; //%
const float mu_HFLFerr = 0.0;  //%

// Data/MC errors
// These are effectively the sf error
// Right now taking the Pt variation into
// account
const float el_datamc = 0.20; 
const float mu_datamc = 0.05; 

// Region errors
const float el_region = 0.05; 
const float mu_region = 0.10; 

// Electron real error (no time to split up)
const float el_real_up = 0.01;
const float el_real_dn = 0.02;
const float mu_real_up = 0.00;
const float mu_real_dn = 0.02;


// ------------------------- //
// Enum for the fake process
// ------------------------- //
enum FakeProcess
{
  FP_ttbar=0,
  FP_Wjet,
  FP_Zjet,
  FP_dib,
  FP_bbbar,
  FP_N
};

class FinalNewFake : public FakePlotting
{

 public:

  // Constructor - Destructor
  FinalNewFake(string outputname="FinalNewFake_out.root");
  ~FinalNewFake();
  void initIoFiles();
  // 
  // Methods to contstruct the rates
  //
  
  // Build Rates
  void buildRates();
  void buildMuonRateSR();     //!< build muon SR rate
  void buildElectronRateSR(); //!< build electron rate using signal region composition
  void buildSystematics();

  // Grab rates from files
  TH1* getRealEff(string lep, FakeProcess process);
  TH1* getFakeRate(string lep, FakeProcess process, bool isConv);

  // Construct systematic shifts
  pair< TParameter<double>, TParameter<double> > getRealSys(string lep);
  TParameter<double> getHFLFSys(string lep);
  TParameter<double> getDataMCSys(string lep);
  TParameter<double> getRegionSys(string lep);
  TH1* getMetRelSys(string lep);
  TH1* getEtaSys(string lep);

  void getFakePercentages(string lep, vector<double> &frac_qcd, vector<double> &frac_conv, string cr);
  void getRealPercentages(string lep, vector<double> &frac, string cr);		      

  // Kept in case we go back to scaling
  void scale(TH1* &h, float sf);

  //Get final rates given the percentages and the corresponding rates
  TH1* getFinalRate(vector<TH1*> rates_qcd, vector<TH1*> rates_conv,
                    vector<double> percent_qcd, vector<double> percent_conv);
  TH1* getFinalRate(vector<TH1*> rates, vector<double> percent);   
  // Dump the rates to plots for the note
  void dumpPlot(TH1* hist);
  void dumpStat(TH1* hist); //!< dump histoname and max error deviation
  struct RegionLeptonFakeorreal { //!< configuration at each step
    SignalRegion reg;
    bool isEle, isFake;
    RegionLeptonFakeorreal(SignalRegion r, bool ele, bool fake) :
      reg(r), isEle(ele), isFake(fake) {}
    string str() {
      return string(SRNames[reg] + (isEle ? " el":" mu") + (isFake ? " fake_rate":" real_eff"));
    }
  };
  typedef RegionLeptonFakeorreal rlf_t;
  typedef const vector<TH1*> cvecth1p_t;
  typedef const vector<double> cvecd_t;
  bool combineWriteAndPlot(rlf_t cfg, cvecth1p_t &histos, cvecd_t &weights);
  //! expanded version of combineWriteAndPlot, used to combine qcd and conv for electrons
  bool combineWriteAndPlot(rlf_t cfg,
                           cvecth1p_t &histos1, cvecth1p_t &histos2,
                           cvecd_t &weights1, cvecd_t &weights2);
    
  //
  // Miscellaneous
  //

  FinalNewFake& setTag(const std::string &name);
  FinalNewFake& setInputDir(const std::string &dir);
  FinalNewFake& setOuputFilename(const std::string &name);
  FinalNewFake& setOuputPlotdir(const std::string &name);
  void writeToOutputFile(TH1* h);
  void writeToOutputFile(TParameter<double>  param);
  void writeFileAndClose();
  

 protected:
  
  File m_mc;            // Average rate
  File m_Wjet;          // File for Wjet
  File m_Zjet;         // File for Zjet
  File m_diboson;       // File for diboson
  File m_ttbar;         // File for ttbar
  File m_bbbar;         // FIle for bbbar

  File m_files[FP_N];   // Vector for all files

  TFile* m_outfile;     // file for the output

  std::string m_tag;
  std::string m_inputdir;
  std::string m_outputfname;
  std::string m_outputplotdir;
};

#endif
