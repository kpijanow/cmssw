#include <L1Trigger/L1TMuonOverlap/interface/GoldenPatternResult.h>
#include <iostream>
#include <ostream>
#include <iomanip>
#include <cmath>

#include "L1Trigger/L1TMuonOverlap/interface/OMTFConfiguration.h"

////////////////////////////////////////////
////////////////////////////////////////////
int GoldenPatternResult::finalizeFunction = 0;

////////////////////////////////////////////
////////////////////////////////////////////
GoldenPatternResult::GoldenPatternResult(const OMTFConfiguration * omtfConfig):  valid(false), myOmtfConfig(omtfConfig) {
  if(myOmtfConfig)
    reset();
}

////////////////////////////////////////////
////////////////////////////////////////////
/*void GoldenPatternResult::configure(const OMTFConfiguration * omtfConfig) {
  myOmtfConfig = omtfConfig;
  assert(myOmtfConfig != 0);
  reset();
}*/
////////////////////////////////////////////
////////////////////////////////////////////

void GoldenPatternResult::set(int refLayer_, unsigned int phi, unsigned int eta, unsigned int refHitPhi,
    unsigned int iLayer, GoldenPatternResult::LayerResult layerResult) {
  if( isValid() && this->refLayer != refLayer_) {
    std::cout<<__FUNCTION__<<" "<<__LINE__<<" this->refLayer "<<this->refLayer<<" refLayer_ "<<refLayer_<<std::endl;
  }
  assert( !isValid() || this->refLayer == refLayer_);

  refLayer = refLayer_;
  this->phi = phi;
  this->eta = eta;
  this->refHitPhi = refHitPhi;
  pdfWeights.at(iLayer) = layerResult.pdfVal;
  if(layerResult.valid) {
    firedLayerBits |= (1<< iLayer);
    hitPdfBins[iLayer] = layerResult.pdfBin;
  }
  /*if(layerResult.valid || layerResult.pdfVal)
    std::cout<<__FUNCTION__<<" "<<__LINE__<<" iLayer "<<iLayer<<" refLayer "<<refLayer<<" pdfBin "<<layerResult.pdfBin<<" val "<<layerResult.pdfVal<<" valid "<<layerResult.valid<<std::endl;
 */ //pdfWeightSum += pdfVal; - this cannot be done here, because the pdfVal for the banding layer must be added only
  //if hit in the corresponding phi layer was accpeted (i.e. its pdfVal > 0. therefore it is done in finalise()
}


/*void GoldenPatternResult::setRefPhiRHits(unsigned int iRefLayer, int iRefPhiRHit) {
  refHitPhi = iRefPhiRHit;
}
////////////////////////////////////////////
////////////////////////////////////////////

void GoldenPatternResult::addResult(unsigned int iRefLayer,
			   unsigned int iLayer,
			   unsigned int val,
			   int iRefPhi, 
			   int iRefEta){

  refLayerResults[iRefLayer].phi = iRefPhi;
  refLayerResults[iRefLayer].eta = iRefEta;
  refLayerResults[iRefLayer].pdfWeights[iLayer] = val;

}*/
////////////////////////////////////////////
////////////////////////////////////////////
void GoldenPatternResult::reset() {
  valid = false;
  refLayer = -1;
  pdfWeights.assign(myOmtfConfig->nLayers(), 0);
  hitPdfBins.assign(myOmtfConfig->nLayers(), 0);
  phi = 1024;
  eta = 1024;
  pdfWeightSum = 0;
  firedLayerCnt = 0;
  firedLayerBits = 0;
  refHitPhi = 1024;
}

/*void GoldenPatternResult::clear() {
  if(refLayerResults.size() == 0)
    refLayerResults.assign(myOmtfConfig->nRefLayers(), RefLayerResult());
  for (auto& reflayerRes: refLayerResults) {
    reflayerRes.reset();
  }
  results1D.assign(myOmtfConfig->nRefLayers(),0);
  hits1D.assign(myOmtfConfig->nRefLayers(),0);
  results.assign(myOmtfConfig->nLayers(),results1D);
  refPhi1D.assign(myOmtfConfig->nRefLayers(),1024);
  refEta1D.assign(myOmtfConfig->nRefLayers(),1024);
  hitsBits.assign(myOmtfConfig->nRefLayers(),0);  
  refPhiRHit1D.assign(myOmtfConfig->nRefLayers(),1024);
}*/
////////////////////////////////////////////
////////////////////////////////////////////
//default version
void GoldenPatternResult::finalise0() {
  for(unsigned int iLogicLayer=0; iLogicLayer < pdfWeights.size(); ++iLogicLayer) {
    unsigned int connectedLayer = myOmtfConfig->getLogicToLogic().at(iLogicLayer);
    //here we require that in case of the DT layers, both phi and phiB is fired
    if(firedLayerBits & (1<<connectedLayer) ) {
      if( firedLayerBits & (1<<iLogicLayer) ) {//now in the GoldenPattern::process1Layer1RefLayer the pdf bin 0 is returned when the layer is not fired, so this is 'if' is to assured that this pdf val is ont added here
        pdfWeightSum += pdfWeights[iLogicLayer];
        if(!myOmtfConfig->getBendingLayers().count(iLogicLayer)) //in DT case, the phi and phiB layers are threaded as one, so the firedLayerCnt is increased only for the phi layer
          firedLayerCnt++;
      }
    }
    else {
      firedLayerBits &= ~(1<<iLogicLayer);
    }
  }

  valid = true;
  //by default result becomes valid here, but can be overwritten later
}

////////////////////////////////////////////
////////////////////////////////////////////
//for the algo version with thresholds
void GoldenPatternResult::finalise1() {
  for(unsigned int iLogicLayer=0; iLogicLayer < pdfWeights.size(); ++iLogicLayer) {
    //in this version we do not require that both phi and phiB is fired (non-zero), we thread them just independent
    //TODO check if it affects performance
    pdfWeightSum += pdfWeights[iLogicLayer];
    firedLayerCnt += ( (firedLayerBits & (1<<iLogicLayer)) != 0 );
  }

  valid = true;
  //by default result becomes valid here, but can be overwritten later
}

////////////////////////////////////////////
////////////////////////////////////////////
/*bool GoldenPatternResults::empty() const{

  unsigned int nHits = 0;
  for(unsigned int iRefLayer=0; iRefLayer<myOmtfConfig->nRefLayers(); ++iRefLayer){
    nHits+=hits1D[iRefLayer];
  }      
  return (nHits==0);
}*/
////////////////////////////////////////////
////////////////////////////////////////////
std::ostream & operator << (std::ostream &out, const GoldenPatternResult & gpResult) {
  for(unsigned int iLogicLayer=0; iLogicLayer < gpResult.getPdfWeights().size(); ++iLogicLayer){
    out<<" layer: "<<iLogicLayer<<" pdfBin: "<<std::setw(3)<<gpResult.hitPdfBins[iLogicLayer]<<" pdfVal: "<<std::setw(3)<<gpResult.getPdfWeights()[iLogicLayer]<<std::endl;
  }

  out<<"  refLayer: ";
  out << gpResult.getRefLayer()<<"\t";

  out<<" Sum over layers: ";
  out<<gpResult.getPdfWeigtSum()<<"\t";

  out<<" Number of hits: ";
  out << gpResult.getFiredLayerCnt()<<"\t";
  out<<std::endl;


  return out;
}
////////////////////////////////////////////
////////////////////////////////////////////
