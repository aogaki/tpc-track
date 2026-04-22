#ifndef frameGUI_hpp
#define frameGUI_hpp 1

#include <iostream>

#include <RQ_OBJECT.h>
#include <TGButton.h>
#include <TGClient.h>
#include <TGFrame.h>

class frameGUI {
    RQ_OBJECT("MyMainFrame")

  public:
    void printConsole(const TString printed_str);
    void generateFrame(const TGWindow *p, UInt_t w, UInt_t h);

  private:
    TGMainFrame *fMain;
};

#endif