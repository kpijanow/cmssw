#ifndef OMTF_GoldenPatternWithStat_H
#define OMTF_GoldenPatternWithStat_H

#include <vector>
#include <ostream>

#include "TH1I.h"

#include "L1Trigger/L1TMuonOverlap/interface/OMTFinput.h"
#include "L1Trigger/L1TMuonOverlap/interface/GoldenPattern.h"

class OMTFConfiguration;

//////////////////////////////////
// Golden Pattern
//////////////////////////////////

class GoldenPatternWithStat : public GoldenPatternWithThresh {
public:
  static const unsigned int STAT_BINS = 4;
  typedef boost::multi_array<float, 4> StatArrayType;
  //GoldenPatternWithStat(const Key & aKey) : GoldenPattern(aKey) {}

  GoldenPatternWithStat(const Key & aKey, unsigned int nLayers, unsigned int nRefLayers, unsigned int nPdfAddrBits);

  GoldenPatternWithStat(const Key & aKey, const OMTFConfiguration* omtfConfig);

  virtual ~GoldenPatternWithStat() {};

  virtual void updateStat(unsigned int iLayer, unsigned int iRefLayer, unsigned int iBin, unsigned int what, double value);

  //virtual void updatePdfs(double learingRate);

  friend std::ostream & operator << (std::ostream &out, const GoldenPatternWithStat & aPattern);

  friend class PatternOptimizer;

  void init();

private:
  StatArrayType statisitics;

  ///the vector index is the muon pt_code
  ///the histogram bin is the value of the pdfSum (or product) for the muons of given pt_code
  std::vector<TH1I> gpProbabilityStat; //TODO maybe better is to have just TH2I
};
//////////////////////////////////
//////////////////////////////////
#endif 
