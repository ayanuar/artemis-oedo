/**
 * @file   TSRPPACPlaneProcessor.cc
 * @brief  Processor for SR-PPAC Plane
 *
 * @date   Created       : 2019-02-07 03:40:03 DST
 *         Last Modified : 2019-02-18 21:44:59 JST (ota)
 * @author Shoichiro Masuoka <masuoka@cns.s.u-tokyo.ac.jp>
 *
 *    (C) 2019 Shoichiro Masuoka
 */

#include "TSRPPACPlaneProcessor.h"
#include "TTimingChargeData.h"
#include <TMath.h>
#include <TClonesArray.h>
#include <TObjArray.h>
#include "TSRPPACParameter.h"

#include <iostream>
using art::TSRPPACPlaneProcessor;

ClassImp(TSRPPACPlaneProcessor)

TSRPPACPlaneProcessor::TSRPPACPlaneProcessor() : fOutput(NULL)
{
   RegisterInputCollection("InputCollection",  
                           "input collection",
                           fNameInput, "srppacx_cal",
                           &fInput, TClonesArray::Class_Name(),
                           art::TTimingChargeData::Class_Name());
   // RegisterInputCollection("InputCollectionY", 
   //                         "input collection (Y direction)",
   //                         fNameInputY, "srppacy_cal",
   //                         &fInputY, TClonesArray::ClassName(),
   //                         art::TTimingChargeData::ClassName());
   // RegsterInputCollection("InputCollectionAnode",
   //                        "Input collection (Anode)",
   //                        &fInputA, TClonesArray::ClassName(),
   //                        art::TTimingChargeData::ClassName());

   RegisterOutputCollection("OutputCollection","output collection",
                            fNameOutput,"srppac",
                            &fOutput, TClonesArray::Class_Name(),
                            art::TSRPPACPlaneData::Class_Name());
   RegisterOptionalInputInfo("SRPPACParameter",
                             "name of SR-PPAC parameter",
                             fParameterName,TString("prm:srppac"),
                             &fParameter, TClonesArray::Class_Name(),
                             art::TSRPPACParameter::Class_Name());
   RegisterOptionalParameter("Verbose", "verbose level (0: quiet)",
                             fVerbose, 0);
   RegisterProcessorParameter("nStrip", "Number of strip",fNStrip,(Int_t)0);
   RegisterProcessorParameter("stripWidth", "strip width (mm)", fStripWidth, (Double_t)0.);
   RegisterProcessorParameter("center", "center of ID", fCenter,(Double_t)0.);
   RegisterProcessorParameter("detoffset", "detector offset", fDetOffset, (Double_t)0.);
   RegisterProcessorParameter("z", "z postion from a focul plane (mm)", fZ, (Double_t)0.);
   RegisterProcessorParameter("turned","1: detector is turned", fTurned, (Bool_t)kFALSE);

   
}

TSRPPACPlaneProcessor::~TSRPPACPlaneProcessor()
{
   delete fOutput;
}

void TSRPPACPlaneProcessor::Init(TEventCollection *col)
{
   // TSRPPACParameter *prm = dynamic_cast<TSRPPACParameter*>((*fParameter)->FindObject(fParameterName));
   // if (!prm) {
   //    SetStateError(TString::Format("No such parameter '%s' is found",
   //                                  fParameterName.Data()));
   //    return;
   // }
   
}

void TSRPPACPlaneProcessor::Process()
{
   TTimingChargeData::SetSortType(TTimingChargeData::kCharge);
   TTimingChargeData::SetSortOrder(TTimingChargeData::kDESC);
   (*fInput)->Sort();
   Bool_t kProcessed = kFALSE;
   const Int_t nData = (*fInput)->GetEntriesFast();
  
   Int_t id[nData];
   Double_t charge[nData];
   Double_t timing[nData];
   
   for (Int_t iData = 0; iData != nData; ++iData) {
      const TTimingChargeData *const inData = static_cast<TTimingChargeData*>((*fInput)->At(iData));
      id[iData] = inData->GetID();
      charge[iData] = inData->GetCharge();
      timing[iData] = inData->GetTiming();
 
   }

   Double_t position;
   // TSRPPACParameter *prm = static_cast<TSRPPACParameter*>(fParameter);
   // const Double_t turned = (prm->IsTurned()) ? -1. : 1.;
   const Double_t turned = (fTurned) ? -1. : 1.;   
//   Double_t cdiff = charge[0]-charge[1];
   Double_t cdiff = (charge[0]-charge[1])/(charge[0]+charge[1]);
   Double_t dxValue;
   if (nData < 2) {
      if (fVerbose) std::cout << "Multiplicity is" << nData << ", cannot process" << std::endl;
      kProcessed = kFALSE;
      position = TMath::QuietNaN();
   }
   else if (id[1] == (id[0] + 1)) { // beam must go through the young direction;
      kProcessed = kTRUE;
	  
	  dxValue = cdiff/12.0;
	  position = turned*((id[0] - fCenter - dxValue + 0.5)*fStripWidth - fDetOffset);      // Dozono 2019.2.16 13:25
      if (fVerbose) {
         std::cout << "center = " << fCenter << ", cdiff = " << cdiff
                   << ", strip width = " << fStripWidth << ", offset = " << fDetOffset << std::endl;
      }
   }
   else if (id[1] == (id[0] - 1)) { // beam must go through the old direction;
      kProcessed = kTRUE;
	  dxValue = cdiff/12.0;
      position = turned*((id[0] - fCenter + dxValue - 0.5)*fStripWidth - fDetOffset); // Dozono 2019.2.16 13:25
      if (fVerbose) {
         std::cout << "center = " << fCenter << ", cdiff = " << cdiff
                   << ", strip width = " << fStripWidth << ", offset = " << fDetOffset << std::endl;
      }
   }
   else {
      if (fVerbose) std::cout << "Strip #0 is " << id[0] << ", Strip #1 is " << id[1] << ", cannot process" << std::endl;
      kProcessed = kFALSE;
      position = TMath::QuietNaN();
   }
   
   TSRPPACPlaneData *outData = NULL;
   for (Int_t iData = 0; iData != nData; ++iData) {
      outData = static_cast<TSRPPACPlaneData*>(fOutput->ConstructedAt(iData));
      outData->SetID(id[iData]);
      outData->SetCharge(charge[iData]);
      outData->SetTiming(timing[iData]);
      outData->SetPosition(position);
      outData->SetProcessed(kProcessed);      
   }

   return;
}
