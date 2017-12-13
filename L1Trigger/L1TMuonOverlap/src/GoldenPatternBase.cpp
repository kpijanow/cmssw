/*
 * GoldenPatternBase.cpp
 *
 *  Created on: Oct 3, 2017
 *      Author: kbunkow
 */


#include "L1Trigger/L1TMuonOverlap/interface/GoldenPatternBase.h"

GoldenPatternBase::GoldenPatternBase(const Key & aKey) : theKey(aKey), myOmtfConfig(0) {
  //std::cout<<__FUNCTION__<<":"<<__LINE__<<std::endl;
}

GoldenPatternBase::GoldenPatternBase(const Key& aKey, const OMTFConfiguration * omtfConfig) : theKey(aKey), myOmtfConfig(omtfConfig),
results(boost::extents[myOmtfConfig->processorCnt()][myOmtfConfig->nTestRefHits()]) {
  //std::cout<<__FUNCTION__<<":"<<__LINE__<<std::endl;
  for(unsigned int iProc = 0; iProc < results.size(); iProc++) {
    for(unsigned int iTestRefHit = 0; iTestRefHit < results[iProc].size(); iTestRefHit++) {
      results[iProc][iTestRefHit].init(omtfConfig);
    }
  }
}

void GoldenPatternBase::setConfig(const OMTFConfiguration * omtfConfig) {
  myOmtfConfig = omtfConfig;
  results.resize(boost::extents[myOmtfConfig->processorCnt()][myOmtfConfig->nTestRefHits()]);
  for(unsigned int iProc = 0; iProc < results.size(); iProc++) {
    for(unsigned int iTestRefHit = 0; iTestRefHit < results[iProc].size(); iTestRefHit++) {
      results[iProc][iTestRefHit].init(omtfConfig);
    }
  }
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////
GoldenPatternResult::LayerResult GoldenPatternBase::process1Layer1RefLayer(unsigned int iRefLayer,
    unsigned int iLayer,
    const int phiRefHit,
    const OMTFinput::vector1D & layerHits,
    int refLayerPhiB)
{
  //if (this->getDistPhiBitShift(iLayer, iRefLayer) != 0) std::cout<<__FUNCTION__<<":"<<__LINE__<<key()<<this->getDistPhiBitShift(iLayer, iRefLayer)<<std::endl;
  GoldenPatternResult::LayerResult aResult(0, 0, 0); //0, 0

  int phiMean = this->meanDistPhiValue(iLayer, iRefLayer, refLayerPhiB);
  int phiDistMin = myOmtfConfig->nPdfBins(); //1<<(myOmtfConfig->nPdfAddrBits()); //"infinite" value for the beginning
  ///Select hit closest to the mean of probability
  ///distribution in given layer
  for(auto itHit: layerHits){
    if(itHit >= (int)myOmtfConfig->nPhiBins())
      continue;  //empty itHits are marked with nPhiBins() in OMTFProcessor::restrictInput

    int phiDist = itHit-phiMean-phiRefHit;
    //if (this->getDistPhiBitShift(iLayer, iRefLayer) != 0) std::cout<<__FUNCTION__<<":"<<__LINE__<<" itHit "<<itHit<<" phiMean "<<phiMean<<" phiRefHit "<<phiRefHit<<" phiDist "<<phiDist<<std::endl;
    phiDist = phiDist >> this->getDistPhiBitShift(iLayer, iRefLayer); //if the shift is done here, it means that the phiMean in the xml should be the same as without shift
    //if (this->getDistPhiBitShift(iLayer, iRefLayer) != 0) std::cout<<__FUNCTION__<<":"<<__LINE__<<" phiDist "<<phiDist<<std::endl;
    if(abs(phiDist) < abs(phiDistMin))
      phiDistMin = phiDist;
  }

  ///Check if phiDistMin is within pdf range -63 +63
  ///in firmware here the arithmetic "value and sign" is used, therefore the range is -63 +63, and not -64 +63
  if(abs(phiDistMin) > ( (1<<(myOmtfConfig->nPdfAddrBits()-1)) -1) ) {
    return GoldenPatternResult::LayerResult(this->pdfValue(iLayer, iRefLayer, 0), false, 0);
    //in some algorithms versions with thresholds we use the bin 0 to store the pdf value returned when there was no hit.
    //in the version without thresholds, the value in the bin 0 should be 0
  }

  ///Shift phidist, so 0 is at the middle of the range
  phiDistMin += 1<<(myOmtfConfig->nPdfAddrBits()-1);
  //if (this->getDistPhiBitShift(iLayer, iRefLayer) != 0) std::cout<<__FUNCTION__<<":"<<__LINE__<<" phiDistMin "<<phiDistMin<<std::endl;
  omtfPdfValueType pdfVal = this->pdfValue(iLayer, iRefLayer, phiDistMin);
  if(pdfVal <= 0)
    return GoldenPatternResult::LayerResult(this->pdfValue(iLayer, iRefLayer, 0), false, phiDistMin); //the pdf[0] needed in some versions of algorithm with threshold
  return GoldenPatternResult::LayerResult(pdfVal, true, phiDistMin);
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////
void GoldenPatternBase::finalise(unsigned int procIndx) {
  for(auto& result : getResults()[procIndx]) {
    result.finalise();
  }
}
