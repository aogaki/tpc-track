#include <cstdio>
#include <glob.h>
#include <iostream>
#include <limits>
#include <map>
#include <random>

#include "TString.h"
#include "include/ErrorCodesMap.hpp"
#include "src/cleanUVW.cpp"
#include "src/convertUVW_elitpc.cpp"
#include "src/loadData.cpp"

#include "include/generalDataStorage.hpp"

#include "TCanvas.h"
#include "TColor.h"
#include "TError.h"
#include "TF1.h"
#include "TH2D.h"
#include "TMarker.h"
#include "TROOT.h"
#include "TSpectrum.h"
#include "TStyle.h"
#include "TSystem.h"
#include <RQ_OBJECT.h>
#include <TGButton.h>
#include <TGClient.h>
#include <TGFrame.h>
#include <TGTextEntry.h>

#include "dict/src/GDataChannel.cpp"
#include "dict/src/GDataFrame.cpp"
#include "dict/src/GDataSample.cpp"
#include "dict/src/GFrameHeader.cpp"

void printConsole(const char *printed_str) {

    TString loc_string(printed_str);

    std::cout << "String: \n" << loc_string << std::endl;
}

TString getBoxText(TGTextEntry *entry_box) {

    TString box_string(entry_box->GetText());
    return box_string;
}

// https://root.cern.ch/root/htmldoc/guides/users-guide/WritingGUI.html
void macroGUI() {

    TGMainFrame *fMain = new TGMainFrame(gClient->GetRoot(), 200, 200);

    TGHorizontalFrame *hframe = new TGHorizontalFrame(fMain, 200, 40);

    // Text input box
    TGTextEntry *entry_box = new TGTextEntry(hframe, new TGTextBuffer(100));
    entry_box->SetToolTipText("This is a text entry widget");
    hframe->AddFrame(entry_box, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    // The button that prints to console.
    TGTextButton *print = new TGTextButton(hframe, "&Print");
    print->Connect(
        "Clicked()", 0, 0,
        Form("printConsole(=\"Button: " + getBoxText(entry_box) + "\")"));
    hframe->AddFrame(print, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    // The exit button.
    TGTextButton *exit =
        new TGTextButton(hframe, "&Exit", "gApplication->Terminate(0)");
    hframe->AddFrame(exit, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    fMain->AddFrame(hframe, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

    // Set a name to the main frame
    fMain->SetWindowName("Simple Example");

    // Map all subwindows of main frame
    fMain->MapSubwindows();

    // Initialize the layout algorithm
    fMain->Resize(fMain->GetDefaultSize());

    // Map main frame
    fMain->MapWindow();
}