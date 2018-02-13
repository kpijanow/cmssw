/*
 * PatternOptimizer.h
 *
 *  Created on: Oct 12, 2017
 *      Author: kbunkow
 */

#ifndef OMTF_PATTERNOPTIMIZER_H_
#define OMTF_PATTERNOPTIMIZER_H_

#include <functional>

#include "L1Trigger/L1TMuonOverlap/interface/IOMTFEmulationObserver.h"
#include "L1Trigger/L1TMuonOverlap/interface/GoldenPatternWithStat.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "SimDataFormats/Track/interface/SimTrackContainer.h"

#include "TH1I.h"

class PatternOptimizer: public IOMTFEmulationObserver {
public:
  PatternOptimizer(const edm::ParameterSet& edmCfg, const OMTFConfiguration* omtfConfig, std::vector<std::shared_ptr<GoldenPatternWithStat> >& gps);
  virtual ~PatternOptimizer();

  virtual void observeProcesorEmulation(unsigned int iProcessor, l1t::tftype mtfType,  const OMTFinput &input,
      const std::vector<AlgoMuon>& algoCandidates,
      std::vector<AlgoMuon>& gbCandidates,
      const std::vector<l1t::RegionalMuonCand> & candMuons);

  virtual void observeEventBegin(const edm::Event& iEvent);

  virtual void observeEventEnd(const edm::Event& iEvent);

  virtual void endJob();

  const SimTrack* findSimMuon(const edm::Event &event, const SimTrack* previous = 0);

  static const unsigned int whatExptVal = 0;
  static const unsigned int whatExptNorm = 1;
  static const unsigned int whatOmtfVal = 2;
  static const unsigned int whatOmtfNorm = 3;
private:
  //candidate found by omtf in a given event
  AlgoMuon omtfCand;
  unsigned int candProcIndx;

  GoldenPatternResult omtfResult;
  GoldenPatternResult exptResult;

  unsigned int exptPatNum;

  unsigned int selectedPatNum;

  unsigned int currnetPtBatchPatNum; //for threshold finding

  edm::ParameterSet edmCfg;
  //edm::Handle<edm::SimTrackContainer> simTks;

  const OMTFConfiguration* omtfConfig;
  std::vector<std::shared_ptr<GoldenPatternWithStat> > goldenPatterns;

  const SimTrack* simMuon;

  TH1I* simMuPt;
  TH1I* simMuFoundByOmtfPt;

  //std::vector<TH2I*> gpExpt_gpOmtf;
  //std::vector<TH1F*> gpEff;


  //double ptRangeFrom = 0;
  //double ptRangeTo = 0;

  unsigned int ptCut = 37;

  std::vector<double> rateWeights;

  std::vector<int> patternPtCodes; //continous ptCode 1...31 (liek in the old PAC)

  void initRateWeights();

  std::function<void (GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp)> updateStatFunc;

  std::function<void (GoldenPatternWithStat* gp, unsigned int& iLayer, unsigned int& iRefLayer, double& learingRate) > updatePdfsFunc;

  void updateStatForAllGps(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp);

  void updateStat(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp, double delta, double norm);

  enum SecondCloserStatIndx {
    goodBigger,
    goodSmaller,
    badBigger,
    badSmaller
  };
  void updateStatCloseResults(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp);
  void updatePdfCloseResults();

  void updateStatCollectProb(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp);
  void calulateProb();


  void calculateThresholds(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp);
  void calculateThresholds(double targetEff);


  void tuneClassProb(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp);
  void tuneClassProb(double targetEff);

  //void updateStatPtDiff_1(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp);
  void updateStatVoter_1(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp);

  void updateStatPtDiff2_1(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp);

  void updateStatPtLogDiff_1(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp);
  void updateStatPtLogDiff_2(GoldenPatternWithStat* omtfCandGp, GoldenPatternWithStat* exptCandGp);

  void updatePdfsMean_1(GoldenPatternWithStat* gp, unsigned int& iLayer, unsigned int& iRefLayer, double& learingRate);
  void updatePdfsMean_2(GoldenPatternWithStat* gp, unsigned int& iLayer, unsigned int& iRefLayer, double& learingRate);
  void updatePdfsVoter_1(GoldenPatternWithStat* gp, unsigned int& iLayer, unsigned int& iRefLayer, double& learingRate);

  void savePatternsInRoot(std::string rootFileName);

  void modifyPatterns();
  void modifyPatterns1(double step);

  void printPatterns();
};

#endif /* OMTF_PATTERNOPTIMIZER_H_ */
