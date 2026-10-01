#include <Riostream.h>
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLatex.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TStopwatch.h"
#include "TMath.h"
#include "TF1.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TPaletteAxis.h"
#include "TSystem.h"
#include "TDatabasePDG.h"
#include <string>
#include "TROOT.h"
#include "algorithm"

string dataT = "";
bool appNorm = false;
bool isMC = false;

gROOT->ForceStyle();
gStyle->SetOptStat(0);

void normalize(TH1 *h){
  h->Scale(1/h->Integral());
  h->GetYaxis()->SetTitle(Form("%s (normalized)",h->GetYaxis()->GetTitle()));
}

TCanvas *superimposeHisto(TFile *f, string nameHisto, string folder1, string folder2, string label1, string label2, string comparisonType, string dataFileName, string addId = "", bool log = false){
  
  // get the histograms
  TH1* h1 = (TH1*) f->Get(Form("%s/%s", folder1.c_str(), nameHisto.c_str()));
  TH1* h2 = (TH1*) f->Get(Form("%s/%s", folder2.c_str(), nameHisto.c_str())); 

  nameHisto = nameHisto + addId;

  //normalize the histos
  if(appNorm){
    normalize(h1);
    normalize(h2);
  }

  // set maximum
  float maxY = 1.2*TMath::Max(h1->GetMaximum(),h2->GetMaximum());
  h1->SetMaximum(maxY);
  if(!log) h1->SetMinimum(0.);

  // dimension of labels and titles
  h1->GetXaxis()->SetLabelSize(0.05);
  h1->GetYaxis()->SetLabelSize(0.05);
  h1->GetXaxis()->SetTitleSize(0.05);
  h1->GetYaxis()->SetTitleSize(0.05);
  h1->GetXaxis()->SetTitleOffset(1.1);

  h1->SetLineWidth(2);
  h2->SetLineWidth(2);

  h1->SetLineColor(kRed+1);
  h2->SetLineColor(kAzure+2);
    
  cout<<h1->GetXaxis()->GetNbins()<<" "<<h2->GetXaxis()->GetNbins()<<endl;
  // draw in canvas
  TCanvas *c = new TCanvas(Form("superimposed:%s",nameHisto.c_str()),nameHisto.c_str(),1920,1080);

  gStyle->SetGridColor(kGray+3);

  // TPad(name, title, xlow, ylow, xup, yup)
  TPad *pad1 = new TPad("pad1", "top",    0, 0.30, 1, 1.00);  // top: 70% of height
  TPad *pad2 = new TPad("pad2", "bottom", 0, 0.00, 1, 0.30);  // bottom: 30% of height

  // margins
  pad1->SetBottomMargin(0.02);
  pad2->SetTopMargin(0.02);
  pad2->SetBottomMargin(0.30);   // room for the x-axis labels/title

  pad1->Draw();
  pad2->Draw();

  pad1->cd();
  gPad->SetLogy(log);
  gPad->SetTicks();
  gPad->SetGrid();

  h1->Draw();
  h2->Draw("same");

  h1->GetXaxis()->SetLabelSize(0.0);
  h1->GetXaxis()->SetTitleSize(0.0);

  // add legend
  TLegend *legend = new TLegend(0.7,0.77,0.98,0.94);
  legend->SetBorderSize(0);
  legend->AddEntry(h1,label1.c_str(),"l");
  legend->AddEntry(h2,label2.c_str(),"l");
  legend->Draw();

  pad2->cd();
  gPad->SetTicks();
  gPad->SetGrid();

  TH1D *hRatio = (TH1D*)h1->Clone("Ratio");
  //hRatio->SetTitle("Ratio");
  hRatio->Divide(h2,h1,1,1,"B");
  hRatio->GetYaxis()->SetNdivisions(505);

  double scale = 0.70 / 0.30;   // top pad height / bottom pad height

  hRatio->GetXaxis()->SetLabelSize(0.045 * scale);
  hRatio->GetXaxis()->SetTitleSize(0.05  * scale);
  hRatio->GetXaxis()->SetTitleOffset(1.0 / scale * 2.5);

  hRatio->GetYaxis()->SetLabelSize(0.045 * scale);
  hRatio->GetYaxis()->SetTitleSize(0.05  * scale);
  hRatio->GetYaxis()->SetTitleOffset(1.0 / scale);

  if(isMC) hRatio->SetMaximum(TMath::Min(1.2*hRatio->GetMaximum(),1.));
  hRatio->GetYaxis()->SetTitle("Ratio");

  hRatio->Draw();

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
  cout<<h1->Integral()<<"\t"<<h2->Integral()<<endl;

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

void compareHisto(string comparisonType = "rho", bool normalize = false, bool isMc = true){
  
  appNorm = normalize;
  isMC = isMc;

  //do not show the pop up
  gROOT->SetBatch(kTRUE);

  // file that will contain the control plots
  string dataFileName = "";

  // no stat box shown
  gStyle->SetOptStat(0);

  // get input files
  TFile *file = NULL; 
  string label1 = "";
  string label2 = "";
  string folder1 = "";
  string folder2 = "";
  string folder3 = "";
  string folder4 = "";
  string folder5 = "";
  string folder6 = "";

  if(comparisonType == "rhoAss"){
    dataFileName = "compRhoAss.root";
    if(appNorm) dataFileName = "compRhoAssNorm.root";
    file = new TFile("AnalysisResults_rho_mc.root");
    label1 = "All good tracks";
    label2 = "Good tracks ass to collision";
    dataT = "_rhoAss";
    folder1 = "upc-track-vertexing-q-a/Trk";
    folder2 = "upc-track-vertexing-q-a/TrkColl";
  }
  else if(comparisonType == "jpsiAss"){
    dataFileName = "compJpsiAss.root";
    if(appNorm) dataFileName = "compJpsiAssNorm.root";
    file = new TFile("AnalysisResults_jpsi_mc.root");
    label1 = "All good tracks";
    label2 = "Good tracks ass to collision";
    dataT = "_jpsiAss";
    folder1 = "upc-track-vertexing-q-a/Trk";
    folder2 = "upc-track-vertexing-q-a/TrkColl";
  }
  else if(comparisonType == "rhoMC"){
    isMc = true;
    dataFileName = "compRhoMC.root";
    if(appNorm) dataFileName = "compRhoMCNorm.root";
    file = new TFile("AnalysisResults_rho_mc.root");
    label1 = "Generated MC";
    label2 = "Reconstructed MC";
    dataT = "_rhoMC";
    // track distrib
    folder1 = "upc-track-vertexing-q-a/McGen/Part/";
    folder2 = "upc-track-vertexing-q-a/TrkColl/";
    // coll info
    folder3 = "upc-track-vertexing-q-a/McGen/Coll/";
    folder4 = "upc-track-vertexing-q-a/Coll/";
    // cand info
    folder5 = "upc-track-vertexing-q-a/McGen/MotherPart/";
    folder6 = "upc-track-vertexing-q-a/Cand/";
  }
  else if(comparisonType == "jpsiMC"){
    isMc = true;
    dataFileName = "compJpsiMC.root";
    if(appNorm) dataFileName = "compJpsiMCNorm.root";
    file = new TFile("AnalysisResults_jpsi_mc.root");
    label1 = "Generated MC";
    label2 = "Reconstructed MC";
    dataT = "_jpsiMC";
    // track distrib
    folder1 = "upc-track-vertexing-q-a/McGen/Part/";
    folder2 = "upc-track-vertexing-q-a/TrkColl/";
    // coll info
    folder3 = "upc-track-vertexing-q-a/McGen/Coll/";
    folder4 = "upc-track-vertexing-q-a/Coll/";
    // cand info
    folder5 = "upc-track-vertexing-q-a/McGen/MotherPart/";
    folder6 = "upc-track-vertexing-q-a/Cand/";
  }
  else{
    cout<<"Wrong comparison type!"<<endl;
    return;
  }
  
  // compare the histograms
  TFile *dataFile = new TFile(dataFileName.c_str(),"recreate");

  
  superimposeHisto(file,"hPt",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
  superimposeHisto(file,"hEta",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();

  if(comparisonType == "rhoMC" || comparisonType == "jpsiMC"){
    superimposeHisto(file,"hVtxX",folder3,folder4,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
    superimposeHisto(file,"hVtxY",folder3,folder4,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
    superimposeHisto(file,"hVtxZ",folder3,folder4,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
    
    superimposeHisto(file,"hPt",folder5,folder6,label1.c_str(),label2.c_str(),comparisonType, dataFileName,"Cand",true)->Write();
    superimposeHisto(file,"hRapidity",folder5,folder6,label1.c_str(),label2.c_str(),comparisonType, dataFileName,"Cand")->Write();

    //superimposeHisto(file,"hMotherPdg",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();

    computeEff(file,"hPtVsRapidity",folder6,folder5,label2.c_str(),label1.c_str(),comparisonType, dataFileName,"Cand")->Write();
  }
  if(comparisonType == "rhoAss" || comparisonType == "jpsiAss"){
    superimposeHisto(file,"hChi2NCl",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
    superimposeHisto(file,"hHasIts",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
    superimposeHisto(file,"hIsPvContrib",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
    superimposeHisto(file,"hTpcNClsFound",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
    superimposeHisto(file,"hItsChi2NCl",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
    superimposeHisto(file,"hItsNCls",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
    superimposeHisto(file,"hItsNClsInnerBarrel",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName)->Write();
    superimposeHisto(file,"hDcaXY",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName,"",true)->Write();
    superimposeHisto(file,"hDcaZ",folder1,folder2,label1.c_str(),label2.c_str(),comparisonType, dataFileName,"",true)->Write();
  }
  dataFile->Close();
}