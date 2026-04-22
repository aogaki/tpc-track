#include <csignal>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

#include "TApplication.h"
#include "TFile.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TTree.h"

#include "../include/loadData.hpp"

// From GET
#include "dict/include/GDataChannel.h"
#include "dict/include/GDataFrame.h"
#include "dict/include/GDataSample.h"
#include "dict/include/GFrameHeader.h"

void alarm_handl(int sig_num) {
    std::cout
        << "Program is taking too long\n############\nClosing.\n############"
        << std::endl;
    _exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {

    // Load file
    TApplication rApp("Root app", &argc, argv);

    /* auto getLib = "dict/build/libMyLib.so";
    gSystem->Load(getLib); */

    // loadData("a", "b");
}