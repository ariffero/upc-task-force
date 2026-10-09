#include <iostream>
#include <string>
#include <vector>

#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TMath.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TH1.h"
#include "TH1D.h"
#include "TH2.h"

using namespace std;

string dataT = "";
bool appNorm = false;

namespace Folders {
  static const std::string Coll          = "upc-track-vertexing-q-a/Coll/";
  static const std::string Trk           = "upc-track-vertexing-q-a/Trk/";
  static const std::string TrkColl       = "upc-track-vertexing-q-a/TrkColl/";
  static const std::string Cand          = "upc-track-vertexing-q-a/Cand/";
  static const std::string CollGen       = "upc-track-vertexing-q-a/McGen/Coll/";
  static const std::string PartGen       = "upc-track-vertexing-q-a/McGen/Part/";
  static const std::string MotherPartGen = "upc-track-vertexing-q-a/McGen/MotherPart/";
}

namespace Labels {
  static const std::string GoodTracks    = "Good tracks";
  static const std::string AssocTracks   = "Assoc. tracks";
  static const std::string MCGenParts    = "MC Gen Particles";
  static const std::string MCRecoCands   = "MC Reco Candidates";
}

int plotColors[] = {kRed+1, kAzure+2, kBlack, kGreen+2};

void normalize(TH1 *h){
  h->Scale(1/h->Integral());
  h->GetYaxis()->SetTitle(Form("%s (normalized)",h->GetYaxis()->GetTitle()));
}

double extractMax(std::vector<TH1*> histos){
  double maxVal = 0;
  for (auto h : histos) {
    maxVal = TMath::Max(maxVal, h->GetMaximum());
  }
  return maxVal;
}

TCanvas *superimposeNHistos(TFile *f, string nameHisto, std::vector<std::string> folders,
                            std::vector<std::string> labels, string comparisonType, string dataFileName,
                            string addId = "", bool log = false) {
  // Get the histograms
  std::vector<TH1*> histos;
  for (const auto &folder : folders) {
    TH1 *h = (TH1*) f->Get(Form("%s/%s", folder.c_str(), nameHisto.c_str()));
    if (h) histos.push_back(h);
  }
  if (histos.empty()) return nullptr;

  // Normalize the histograms if needed
  if(appNorm){
    for (auto h : histos) normalize(h);
  }

  // Set maximum and minimum for the histograms
  double maxY = extractMax(histos);
  for (auto h : histos) {
    h->SetMaximum(1.2*maxY);
    if(!log) h->SetMinimum(0.);
  }

  nameHisto = nameHisto + addId;

  // Format histos
  for (int i = 0; i < histos.size(); i++) {
    TH1 *h = histos[i];
    h->SetLineWidth(2);
    h->SetLineColor(plotColors[i]);
    // dimension of labels and titles
    h->GetXaxis()->SetLabelSize(0.05);
    h->GetYaxis()->SetLabelSize(0.05);
    h->GetXaxis()->SetTitleSize(0.05);
    h->GetYaxis()->SetTitleSize(0.05);
    h->GetXaxis()->SetTitleOffset(1.1);
  }

  // Draw canvas
  TCanvas *c = new TCanvas(Form("superimposed:%s",nameHisto.c_str()),nameHisto.c_str(),1920,1080);
  gStyle->SetGridColor(kGray+3);

  TPad *pad1 = new TPad("pad1", "top",    0, 0.30, 1, 1.00);  // top: 70% of height
  TPad *pad2 = new TPad("pad2", "bottom", 0, 0.00, 1, 0.30);  // bottom: 30% of height

  pad1->SetBottomMargin(0.02);
  pad2->SetTopMargin(0.02);
  pad2->SetBottomMargin(0.30);   // room for the x-axis labels/title

  pad1->Draw();
  pad2->Draw();

  pad1->cd();
  gPad->SetLogy(log);
  gPad->SetTicks();
  gPad->SetGrid();

  TLegend *legend = new TLegend(0.65,0.77,0.98,0.94);
  legend->SetBorderSize(0);

  for (int i = 0; i < histos.size(); i++) {
    TH1 *h = histos[i];
    // Remove axis labels and titles for the top pad
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetXaxis()->SetTitleSize(0.0);

    // add legend
    legend->AddEntry(h,labels[i].c_str(),"l");
    histos[i]->Draw(i == 0 ? "" : "same");
  }
  legend->Draw();

  // Lower pad for ratio plot
  pad2->cd();
  gPad->SetTicks();
  gPad->SetGrid();

  double scale = 0.70 / 0.30;   // top pad height / bottom pad height
  TH1 *h1 = histos[0];

  for (int i = 1; i < histos.size(); i++) {
    TH1 *hRatio = (TH1*) histos[i]->Clone(Form("hRatio_%d", i));
    hRatio->GetXaxis()->SetLabelSize(0.045 * scale);
    hRatio->GetXaxis()->SetTitleSize(0.05  * scale);
    hRatio->GetXaxis()->SetTitleOffset(1.0 / scale * 2.5);

    hRatio->GetYaxis()->SetLabelSize(0.045 * scale);
    hRatio->GetYaxis()->SetTitleSize(0.05  * scale);
    hRatio->GetYaxis()->SetTitleOffset(1.0 / scale);
    hRatio->GetYaxis()->SetNdivisions(505);

    hRatio->GetYaxis()->SetTitle("Ratio");

    hRatio->Divide(histos[i],h1,1,1,"B");
    hRatio->SetMaximum(TMath::Min(1.2*hRatio->GetMaximum(), 1.1));
    hRatio->SetLineColor(plotColors[i]);
    hRatio->SetLineWidth(2);
    hRatio->Draw(i == 1 ? "" : "same");
  }

  // save the results
  if(!appNorm){
    gSystem->mkdir(Form("plots_%s",comparisonType.c_str()));
    c->SaveAs(Form("plots_%s/%s%s.png",comparisonType.c_str(),nameHisto.c_str(),dataT.c_str()));
    c->SaveAs(Form("plots_%s/%s%s.pdf",comparisonType.c_str(),nameHisto.c_str(),dataT.c_str()));
  }else{
    gSystem->mkdir(Form("plots_%s_norm",comparisonType.c_str()));
    c->SaveAs(Form("plots_%s_norm/%s%s.png",comparisonType.c_str(),nameHisto.c_str(),dataT.c_str()));
    c->SaveAs(Form("plots_%s_norm/%s%s.pdf",comparisonType.c_str(),nameHisto.c_str(),dataT.c_str()));
  }

  //print normalization
  for (int i = 0; i < histos.size(); i++) {
    cout<<"Number of entries: "<< histos[i]->GetEntries() << "; Integral: "<< histos[i]->Integral(1, histos[i]->GetXaxis()->GetNbins()) << endl;
  }
  return c;
}

TCanvas *computeEff(TFile *f, string nameHisto, string folder1, string folder2, string label1, string label2, string comparisonType, string dataFileName, string addId = ""){
  // get the histograms
  TH1* h1 = (TH2*) f->Get(Form("%s/%s", folder1.c_str(), nameHisto.c_str()));
  TH1* h2 = (TH2*) f->Get(Form("%s/%s", folder2.c_str(), nameHisto.c_str())); 

  nameHisto = nameHisto + addId;

  TCanvas *c = new TCanvas(Form("efficiency:%s",nameHisto.c_str()),nameHisto.c_str(),1920,1080);
  TH1D *hRatio = (TH1D*)h1->Clone("Ratio reco/gen");
  hRatio->SetTitle("Efficiency");
  hRatio->Divide(h1,h2,1,1,"B");
  hRatio->Draw("colz");

  if(!appNorm){
    gSystem->mkdir(Form("plots_%s",comparisonType.c_str()));
    c->SaveAs(Form("plots_%s/%s%s.png",comparisonType.c_str(),nameHisto.c_str(),dataT.c_str()));
    c->SaveAs(Form("plots_%s/%s%s.pdf",comparisonType.c_str(),nameHisto.c_str(),dataT.c_str()));
  }else{
    gSystem->mkdir(Form("plots_%s_norm",comparisonType.c_str()));
    c->SaveAs(Form("plots_%s_norm/%s%s.png",comparisonType.c_str(),nameHisto.c_str(),dataT.c_str()));
    c->SaveAs(Form("plots_%s_norm/%s%s.pdf",comparisonType.c_str(),nameHisto.c_str(),dataT.c_str()));
  }

  return c;
}

void compareHisto(string comparisonType = "rhoAll", bool normalize = false){

  appNorm = normalize;

  //do not show the pop up
  gROOT->SetBatch(kTRUE);

  // file that will contain the control plots
  string dataFileName = "";

  // no stat box shown
  gStyle->SetOptStat(0);
  gROOT->ForceStyle();

  std::vector<std::string> trackFolders = {Folders::Trk, Folders::TrkColl, Folders::PartGen};
  std::vector<std::string> collFolders = {Folders::CollGen, Folders::Coll};
  std::vector<std::string> candFolders = {Folders::MotherPartGen, Folders::Cand};

  std::vector<std::string> trackLabels = {Labels::GoodTracks, Labels::AssocTracks, Labels::MCGenParts};
  std::vector<std::string> mcLabels = { Labels::MCGenParts, Labels::MCRecoCands};

  // get input files
  TFile *file = NULL; 

  if (comparisonType == "rhoAll") {
    dataFileName = "compRhoAll.root";
    file = new TFile("AnalysisResults_rho_mc.root");
  }
  else if (comparisonType == "jpsiAll") {
    dataFileName = "compJpsiAll.root";
    file = new TFile("AnalysisResults_jpsi_mc.root");
  }
  else{
    cerr << "Wrong comparison type!" << endl;
    return;
  }
  
  // compare the histograms
  TFile *dataFile = new TFile(dataFileName.c_str(),"recreate");

  superimposeNHistos(file, "hPt", trackFolders, trackLabels, comparisonType, dataFileName)->Write();
  superimposeNHistos(file, "hEta", trackFolders, trackLabels, comparisonType, dataFileName)->Write();

  if (comparisonType == "rhoAll" || comparisonType == "jpsiAll") {
    superimposeNHistos(file, "hVtxX", collFolders, mcLabels, comparisonType, dataFileName)->Write();
    superimposeNHistos(file, "hVtxY", collFolders, mcLabels, comparisonType, dataFileName)->Write();
    superimposeNHistos(file, "hVtxZ", collFolders, mcLabels, comparisonType, dataFileName)->Write();

    superimposeNHistos(file, "hPt", candFolders, mcLabels,comparisonType, dataFileName, "Cand", true)->Write();
    superimposeNHistos(file,"hRapidity", candFolders, mcLabels,comparisonType, dataFileName, "Cand")->Write();

    superimposeNHistos(file,"hChi2NCl", trackFolders, trackLabels,comparisonType, dataFileName)->Write();
    superimposeNHistos(file,"hHasIts", trackFolders, trackLabels,comparisonType, dataFileName)->Write();
    superimposeNHistos(file,"hIsPvContrib", trackFolders, trackLabels,comparisonType, dataFileName)->Write();
    superimposeNHistos(file,"hTpcNClsFound", trackFolders, trackLabels,comparisonType, dataFileName)->Write();
    superimposeNHistos(file,"hItsChi2NCl", trackFolders, trackLabels,comparisonType, dataFileName)->Write();
    superimposeNHistos(file,"hItsNCls", trackFolders, trackLabels,comparisonType, dataFileName)->Write();
    superimposeNHistos(file,"hItsNClsInnerBarrel", trackFolders, trackLabels,comparisonType, dataFileName)->Write();
    superimposeNHistos(file,"hDcaXY", trackFolders, trackLabels,comparisonType, dataFileName,"",true)->Write();
    superimposeNHistos(file,"hDcaZ", trackFolders, trackLabels,comparisonType, dataFileName,"",true)->Write();
  }
  dataFile->Close();
}