#include "../include/frameGUI.hpp"

void frameGUI::generateFrame(const TGWindow *p, UInt_t w, UInt_t h) {

    TGMainFrame *fMain = new TGMainFrame(gClient->GetRoot(), 200, 200);

    TGHorizontalFrame *hframe = new TGHorizontalFrame(fMain, 200, 40);

    TGTextButton *print = new TGTextButton(hframe, "&Print");
    print->Connect("Clicked()", "frameGUI", this, "printConsole(=\"Hello\")");
    hframe->AddFrame(print, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

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

void frameGUI::printConsole(const TString printed_str) {

    std::cout << "String: \n" << printed_str << std::endl;
}
