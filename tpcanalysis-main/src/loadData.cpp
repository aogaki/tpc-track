#include "../include/loadData.hpp"

/**
 * @brief Construct a new load Data::load Data object
 *
 * @param filename name of the file to be opened
 * @param treename name of the tree in the file
 */
loadData::loadData(const TString filename, const TString treename) {

    m_filename = filename;
    m_treename = treename;
    m_data_branch = new GDataFrame();
}

loadData::~loadData() {

    m_roottree->ResetBranchAddresses();

    if (m_roottree) {

        delete (m_roottree);
        m_roottree = nullptr;
    }

    // Having issues with this or m_data_branch->Reset().
    // I get really weird segmentation faults. Will leave it like this for now.
    /* if(m_data_branch){

        delete(m_data_branch);
        m_data_branch = nullptr;

    } */
}

/**
 * @brief Checks if the file exists and opens it.
 *
 * @return std::shared_ptr<TFile> On success returns pointer to file. If file
 * isn't opened returns nullptr.
 */
int loadData::openFile() {

    m_rootfile = std::make_shared<TFile>(m_filename, "READ");

    if (m_rootfile->IsOpen())
        return 0;

    return -1;
}

/**
 * @brief Reads the tree from the file.
 *
 * @return int error codes
 */
int loadData::readData() {

    if (m_rootfile->IsZombie()) {

        std::cerr << "Error: file is not open." << std::endl;
        return -1;
    }

    if (!(m_rootfile->GetListOfKeys()->Contains(m_treename))) {

        std::cerr << "Error: file does not contain tree " << m_treename.Data()
                  << std::endl;
        return -2;
    }

    m_roottree = m_rootfile->Get<TTree>(m_treename);

    m_roottree->SetBranchAddress("GDataFrame", &m_data_branch);

    return 0;
}

/**
 * @brief Decodes the data from an entry in the tree using the dictionary and
 * saves it in a rawData vector. If multiple entries are to be read then you
 * should call this function multiple times with the number of the entry you
 * want. You must call the returnRawData() function before decoding the next
 * entry, otherwise you will overwrite the current entry. The code should look
 * something like: for(...){
 *
 *  err = decodeData(i);
 *  loc_data = returnRawData();
 *
 * }
 *
 *
 * @param entryNr the number of the entry from the tree to be decoded
 * @param remove_fpn If set to true, the Fixed Pattern Noise will be removed
 * from all of the channels. For now this setting makes things worse.
 * @return int error codes
 */
int loadData::decodeData(const int entryNr, const bool remove_fpn) {

    if (m_rootfile->IsZombie()) {

        std::cerr << "Error: file is not open." << std::endl;
        return -1;
    }

    if (!(m_rootfile->GetListOfKeys()->Contains(m_treename))) {

        std::cerr << "Error: file does not contain tree " << m_treename.Data()
                  << std::endl;
        return -2;
    }

    // Clear the rawData vector so we don't collect old data. I think it's
    // better to use clear since the size should always be the same anyway so we
    // don't lose anything. The memory freeing and realocation is slow.
    // std::vector<rawData>().swap(m_root_raw_data);
    m_root_raw_data.clear();

    rawData loc_data;

    m_roottree->GetEntry(entryNr);

    TIter channelIT((TCollection *) m_data_branch->GetChannels());
    GDataChannel *channel = nullptr;
    while ((channel = (GDataChannel *) channelIT.Next())) {
        TIter sampleIT((TCollection *) &channel->fSamples);
        GDataSample *sample = nullptr;

        loc_data.chip_nr = channel->fAgetIdx;
        loc_data.ch_nr = channel->fChanIdx;
        loc_data.asad_id = m_data_branch->fHeader.fAsadIdx;
        loc_data.event_id = m_data_branch->fHeader.fEventIdx;
        loc_data.entry_nr = entryNr;

        while ((sample = (GDataSample *) sampleIT.Next())) {

            loc_data.signal_val.push_back(sample->fValue);
        }

        m_root_raw_data.push_back(loc_data);
        // No need to reset the memory since the size is the same anyway.
        loc_data.signal_val.clear();
        // std::vector<double>().swap(loc_data.signal_val);
    }

    if (remove_fpn) {
        removeFPN();
    }

    return 0;
}

/**
 * @brief
 *
 */
void loadData::removeFPN() {

    // The FPN channels used for noise removal.
    static const std::set<int> channels_of_interest = {11, 22, 45, 56};

    std::multimap<int, std::reference_wrapper<std::vector<double>>>
        fpn_signals_by_chip;

    std::multimap<int, std::reference_wrapper<std::vector<double>>>
        all_signals_by_chip;

    std::unordered_set<int> map_keys;

    for (auto &data : m_root_raw_data) {

        if (channels_of_interest.count(data.ch_nr) > 0) {
            fpn_signals_by_chip.insert(
                {data.chip_nr, std::ref(data.signal_val)});
        }

        all_signals_by_chip.insert({data.chip_nr, std::ref(data.signal_val)});

        map_keys.insert(data.chip_nr);
    }

    std::vector<std::array<double, 512>> mean_fpn;
    mean_fpn.resize(map_keys.size(), {0});

    for (const auto &key : map_keys) {

        if (fpn_signals_by_chip.find(key) == fpn_signals_by_chip.end())
            continue;

        auto itr1 = fpn_signals_by_chip.lower_bound(key);
        auto itr2 = fpn_signals_by_chip.upper_bound(key);

        int fpn_size = 0;

        while (itr1 != itr2) {

            const auto &signal_vec = (*itr1).second.get();

            for (std::size_t i = 0; i < mean_fpn[key].size(); i++) {

                mean_fpn[key][i] += signal_vec[i];
            }

            fpn_size++;

            itr1++;
        }

        std::transform(
            mean_fpn[key].begin(), mean_fpn[key].end(), mean_fpn[key].begin(),
            [fpn_size](const double &sig_el) { return sig_el / fpn_size; });
    }

    for (const auto &key : map_keys) {

        auto itr1 = all_signals_by_chip.lower_bound(key);
        auto itr2 = all_signals_by_chip.upper_bound(key);

        while (itr1 != itr2) {

            auto &signal_vec = (*itr1).second.get();

            for (auto i = 0; i < signal_vec.size(); i++) {

                signal_vec[i] -= mean_fpn[key][i];

                if (signal_vec[i] < 10) {

                    signal_vec[i] = 10;
                }
            }

            itr1++;
        }
    }
}
/**
 * @brief Returns the raw data vector. To be called only after the decodeData
 * function.
 *
 * @return std::vector<rawData> the vector
 */
std::vector<rawData> loadData::returnRawData() const { return m_root_raw_data; }

/**
 * @brief Returns the number of entries. To be called only after the ReadData()
 * function.
 *
 * @return Long64_t the number of entries taken from the file
 */
Long64_t loadData::returnNEntries() const { return m_roottree->GetEntries(); }