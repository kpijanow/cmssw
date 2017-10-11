#ifndef OMTF_OMTFSorterWithThreshold_H
#define OMTF_OMTFSorterWithThreshold_H

#include <L1Trigger/L1TMuonOverlap/interface/SorterBase.h>
#include <L1Trigger/L1TMuonOverlap/interface/GoldenPattern.h>
#include <vector>

class OMTFSorterWithThreshold: public SorterBase<GoldenPatternWithThresh> {
public:
  virtual ~OMTFSorterWithThreshold() {}

  ///Sort results from a single reference hit.
  ///Select candidate with highest number of hit layers
  ///Then select a candidate with largest likelihood value and given charge
  ///as we allow two candidates with opposite charge from single 10deg region
  virtual AlgoMuon sortRefHitResults(unsigned int iRefHit, const std::vector< std::shared_ptr<GoldenPatternWithThresh> >& gPatterns,
				int charge=0);
};

#endif
