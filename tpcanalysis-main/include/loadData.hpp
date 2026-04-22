#ifndef loadData_hpp
#define loadData_hpp 1

#include <iostream>
#include <set>
#include <vector>

#include "rawData.hpp"

#include "TFile.h"
#include "TString.h"
#include "TTree.h"
#include <TROOT.h>
#include <TSystem.h>

// From GET
#include "../dict/include/GDataChannel.h"
#include "../dict/include/GDataFrame.h"
#include "../dict/include/GDataSample.h"
#include "../dict/include/GFrameHeader.h"

using namespace GET;

/**
 * @brief Reads the specified TTree from the specified '.root' file.
 * Can only decode one entry at a time.
 *
 */
class loadData {
  public:
    loadData(){};
    ~loadData();

    loadData(const TString filename, const TString treename);

    int openFile();

    int readData();

    int decodeData(const int entryNr, const bool remove_fpn = false);

    std::vector<rawData> returnRawData() const;

    Long64_t returnNEntries() const;

  private:
    /// @brief The name of the file to be opened.
    TString m_filename;

    /// @brief The name of the TTree in the file.
    TString m_treename;

    /// @brief The file containg the data.
    std::shared_ptr<TFile> m_rootfile;

    /// @brief The TTree read from the file.
    TTree *m_roottree;

    /// @brief The raw data taken from the TTree. Conatins only 1 entry.
    std::vector<rawData> m_root_raw_data;

    /// @brief The number of entries in the TTree.
    Long64_t m_nEntries = 0;

    /// @brief Used for TTree::SetBranchAddress. Stores the actual data after
    /// using m_roottree->GetEntry(entryNr).
    GDataFrame *m_data_branch;

    void removeFPN();
};

#endif