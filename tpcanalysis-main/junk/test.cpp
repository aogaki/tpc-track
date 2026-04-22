#include <cstdio>
#include <iostream>
#include <limits>
#include <random>

#include "include/ErrorCodesMap.hpp"
#include "src/convertHitData.cpp"
#include "src/convertUVW.cpp"
#include "src/convertXYZ.cpp"
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

// From GET
/* #include "dict/include/GDataSample.h"
#include "dict/include/GDataChannel.h"
#include "dict/include/GDataFrame.h" */
#include "dict/include/GFrameHeader.h"

#include "dict/src/GDataChannel.cpp"
#include "dict/src/GDataFrame.cpp"
#include "dict/src/GDataSample.cpp"
#include "dict/src/GFrameHeader.cpp"

std::vector<hitPoints> generateHitData() {

    std::vector<hitPoints> hit_vect;
    hitPoints loc_hit;

    loc_hit.peak_x = 290;
    loc_hit.peak_y = 100;
    loc_hit.entry_nr = 66;
    loc_hit.base_line = 50;
    loc_hit.fwhm = 5;

    loc_hit.plane = 0;

    double j = 1;

    for (auto i = 67; i > 6; i -= 2) {

        loc_hit.peak_x = j;
        j += 9;

        loc_hit.strip = i;

        hit_vect.push_back(loc_hit);
    }

    /* for(auto i = 1; i < 513; i += 15){

        loc_hit.peak_x = i;
        loc_hit.strip = 31;
        hit_vect.push_back(loc_hit);

    } */

    loc_hit.plane = 1;
    j = 1;

    for (auto i = 62; i < 93; i++) {

        loc_hit.peak_x = j;
        j += 9;

        loc_hit.strip = i;

        hit_vect.push_back(loc_hit);
    }

    /* for(auto i = 1; i < 513; i += 15){

        loc_hit.peak_x = i;
        loc_hit.strip = 65;
        hit_vect.push_back(loc_hit);

    } */

    loc_hit.plane = 2;

    j = 1;

    for (auto i = 1; i < 31; i++) {

        loc_hit.peak_x = j;
        j += 9;

        loc_hit.strip = i;

        hit_vect.push_back(loc_hit);
    }

    /*     for(auto i = 1; i < 513; i += 15){

            loc_hit.peak_x = i;
            loc_hit.strip = 30;
            hit_vect.push_back(loc_hit);

        } */

    return hit_vect;
}

std::vector<dataUVW> generateUVW() {

    std::vector<dataUVW> data_vec;
    dataUVW loc_data;

    std::mt19937 engine;
    std::random_device seed_gen;
    engine.seed(seed_gen());

    std::uniform_real_distribution<> noise_gen(40, 45);
    std::uniform_real_distribution<> peak_gen(150, 200);

    loc_data.entry_nr = 66;

    // U plane
    for (auto i = 1; i < 73; i++) {

        loc_data.plane_val = 0;
        loc_data.strip_nr = i;

        for (auto j = 0; j < 512; j++) {

            double loc_signal_el = noise_gen(engine);

            if (j > 290 && j < 300 && i > 10 && i < 70) {

                loc_signal_el += peak_gen(engine);
            }

            if (j > 400 && j < 410 && i > 40 && i < 50) {

                loc_signal_el += peak_gen(engine);
            }

            loc_data.signal_val.push_back(loc_signal_el);
        }

        data_vec.push_back(loc_data);

        std::vector<double>().swap(loc_data.signal_val);
    }

    // V plane
    for (auto i = 1; i < 93; i++) {

        loc_data.plane_val = 1;
        loc_data.strip_nr = i;

        for (auto j = 0; j < 512; j++) {

            double loc_signal_el = noise_gen(engine);

            if (j > 290 && j < 300 && i > 10 && i < 70) {

                loc_signal_el += peak_gen(engine);
            }

            if (j > 400 && j < 410 && i > 40 && i < 50) {

                loc_signal_el += peak_gen(engine);
            }

            loc_data.signal_val.push_back(loc_signal_el);
        }

        data_vec.push_back(loc_data);

        std::vector<double>().swap(loc_data.signal_val);
    }

    // W plane
    for (auto i = 1; i < 93; i++) {

        loc_data.plane_val = 2;
        loc_data.strip_nr = i;

        for (auto j = 0; j < 512; j++) {
            ;
            double loc_signal_el = noise_gen(engine);

            if (j > 290 && j < 300 && i > 10 && i < 70) {

                loc_signal_el += peak_gen(engine);
            }

            if (j > 400 && j < 410 && i > 40 && i < 50) {

                loc_signal_el += peak_gen(engine);
            }

            loc_data.signal_val.push_back(loc_signal_el);
        }

        data_vec.push_back(loc_data);

        std::vector<double>().swap(loc_data.signal_val);
    }

    return data_vec;
}

void test_openFile() {

    auto noFile = "./rootdata/stefan23214124.root";
    auto goodFile = "./rootdata/stefan2.root";
    loadData no_data_loader(noFile, "tree_data");

    auto testNoFile = no_data_loader.openFile();

    /* if(testNoFile.get() != nullptr) {
        std::cerr << "Error: loadData try to read empty file, but not return
    error\n";
    } */

    loadData good_data_loader(goodFile, "tree_data");

    auto testGoodFile = good_data_loader.openFile();

    /*     if(testGoodFile.get() == nullptr){
            std::cerr << "Error opening good file\n";
        } */
}

void test_readData() {

    auto noFile = "./rootdata/stefan23214124.root";
    auto goodFile = "./rootdata/data.root";

    auto badTree = "bad_tree_name";
    auto goodTree = "tree";

    loadData noFileNoTree(noFile, badTree);
    loadData goodFileBadTree(goodFile, badTree);
    loadData goodFileGoodTree(goodFile, goodTree);

    auto testNoFile = noFileNoTree.openFile();
    auto testBadTree = goodFileBadTree.openFile();
    auto testGoodTree = goodFileGoodTree.openFile();

    auto err = noFileNoTree.readData();

    if (err != -2)
        std::cerr << "Error: file not existing did not return good error code: "
                  << err << std::endl;

    err = goodFileBadTree.readData();

    if (err != -1)
        std::cerr << "Error: bad tree returned bad error code: " << err
                  << std::endl;

    err = goodFileGoodTree.readData();

    if (err != 0)
        std::cerr << "Error: good tree returned bad code: " << err << std::endl;
}

void test_decodeData() {

    auto noFile = "./rootdata/stefan23214124.root";
    auto goodFile = "./rootdata/data2.root";

    auto badTree = "bad_tree_name";
    auto goodTree = "tree";

    loadData noFileNoTree(noFile, badTree);
    loadData goodFileBadTree(goodFile, badTree);
    loadData goodFileGoodTree(goodFile, goodTree);

    auto testNoFile = noFileNoTree.openFile();
    auto testBadTree = goodFileBadTree.openFile();
    auto testGoodTree = goodFileGoodTree.openFile();

    auto err = noFileNoTree.readData();

    err = goodFileBadTree.readData();

    err = goodFileGoodTree.readData();

    generalDataStorage nofile_data_container;
    generalDataStorage bad_data_container;
    generalDataStorage good_data_container;

    err = noFileNoTree.decodeData(210);
    nofile_data_container.root_raw_data = noFileNoTree.returnRawData();

    if (err != -2)
        std::cerr << "Error: file not existing did not return good error code: "
                  << err << std::endl;

    err = goodFileBadTree.decodeData(210);
    bad_data_container.root_raw_data = goodFileBadTree.returnRawData();

    if (err != -1)
        std::cerr << "Error: bad tree returned bad error code: " << err
                  << std::endl;

    err = goodFileGoodTree.decodeData(210);
    good_data_container.root_raw_data = goodFileGoodTree.returnRawData();

    if (err != 0)
        std::cerr << "Error: good tree returned bad code: " << err << std::endl;
    else
        std::cout << "Success! Size of vector is "
                  << good_data_container.root_raw_data.size() << std::endl;

    for (auto i : good_data_container.root_raw_data) {

        std::cout << "Channel is " << i.ch_nr << " chip is " << i.chip_nr
                  << " signal value size is " << i.signal_val.size()
                  << std::endl;
    }
}

void test_loadData() {

    /*     test_openFile();

        test_readData(); */

    test_decodeData();
}

void test_convertUVW() {

    auto goodFile = "./rootdata/data.root";

    auto goodTree = "tree";

    generalDataStorage data_container;

    loadData good_data(goodFile, goodTree);

    auto returned_file = good_data.openFile();
    auto err = good_data.readData();
    err = good_data.decodeData(210);
    data_container.root_raw_data = good_data.returnRawData();

    convertUVW loc_converter(data_container.root_raw_data);

    err = loc_converter.openSpecFile();

    if (err != 0)
        return;

    err = loc_converter.makeConversion();

    for (auto i : data_container.uvw_data) {

        std::cout << "Signal value size is " << i.signal_val.size()
                  << " decoded plane value is " << i.plane_val
                  << " decoded strip nr is " << i.strip_nr << std::endl;
    }
}

void test_viewdata(double peak_th = 50) {

    generalDataStorage data_container;

    auto goodFile = "./rootdata/data2.root";

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto returned_file = good_data.openFile();
    auto err = good_data.readData();
    err = good_data.decodeData(210);
    data_container.root_raw_data = good_data.returnRawData();
    data_container.n_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw(data_container.root_raw_data);

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    err = loc_conv_uvw.makeConversion();
    if (err != 0)
        std::cout << "Make conversion error code " << err << std::endl;

    err = loc_conv_uvw.substractBl();

    if (err != 0)
        std::cout << "Substractbl error code " << err << std::endl;

    data_container.uvw_data = loc_conv_uvw.returnDataUVW();

    TCanvas *print_canv = new TCanvas("Test_canvas", "Test_canvas", 1000, 1000);

    TH2D *raw_hist = new TH2D("my_hist", "hist", 512, 1, 513, 300, 1, 301);
    // raw_hist->SetDirectory(0);

    // err = loc_conv_uvw.getHitInfo(data_container.root_raw_data,
    // data_container.hit_data, data_container.raw_hist_container);

    convertHitData loc_convert_hit(data_container.uvw_data);

    err = loc_convert_hit.getHitInfo(peak_th);
    if (err != 0)
        std::cout << "Error get hit info code " << err << std::endl;

    data_container.hit_data = loc_convert_hit.returnHitData();
    data_container.raw_hist_container = loc_convert_hit.returnHistData();

    std::cout << "Hit data size " << data_container.hit_data.size()
              << std::endl;
    std::cout << "Hist data size " << data_container.raw_hist_container.size()
              << std::endl;

    int bin = 0;
    int strip = 1;

    /*     for(auto iter : data_container.root_raw_data){

            for(auto sig_iter : iter.signal_val){
                raw_hist->SetBinContent(++bin, strip, sig_iter);


            }






            bin = 0;
            strip++;
        } */

    print_canv->Print("test_hist01.pdf[");

    // ignore Info level messages
    // if you don't do this you get spammed with useless messages
    gErrorIgnoreLevel = kWarning;

    // draw 2d hists
    /*     raw_hist->Draw("COLZ");
        print_canv->Print("test_hist01.pdf", "Test title"); */

    std::map<int, TH2D *> map_u;
    std::map<int, TH2D *> map_v;
    std::map<int, TH2D *> map_w;

    // make map for hists for each entry
    for (auto i_entries = 0; i_entries < data_container.n_entries;
         i_entries++) {

        TH2D *u_hists = new TH2D(Form("u_hists_%d", i_entries), "u_hist", 512,
                                 1, 513, 100, 1, 101);
        TH2D *v_hists = new TH2D(Form("v_hists_%d", i_entries), "v_hist", 512,
                                 1, 513, 100, 1, 101);
        TH2D *w_hists = new TH2D(Form("w_hists_%d", i_entries), "w_hist", 512,
                                 1, 513, 100, 1, 101);

        map_u.insert({210 + i_entries, u_hists});
        map_v.insert({210 + i_entries, v_hists});
        map_w.insert({210 + i_entries, w_hists});
    }

    for (auto iter : data_container.uvw_data) {

        bin = 0;

        if (iter.plane_val == 0) {

            for (auto sig_iter : iter.signal_val) {

                map_u.at(iter.entry_nr)
                    ->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 1) {

            for (auto sig_iter : iter.signal_val) {

                map_v.at(iter.entry_nr)
                    ->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 2) {

            for (auto sig_iter : iter.signal_val) {

                map_w.at(iter.entry_nr)
                    ->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }
    }

    for (auto i_entries = 0; i_entries < 1 /* data_container.n_entries */;
         i_entries++) {

        TH2D *u_hists = map_u.at(210 + i_entries);
        TH2D *v_hists = map_v.at(210 + i_entries);
        TH2D *w_hists = map_w.at(210 + i_entries);

        u_hists->Draw("COLZ");
        print_canv->Print("test_hist01.pdf", "Test title");

        v_hists->Draw("COLZ");
        print_canv->Print("test_hist01.pdf", "Test title");

        w_hists->Draw("COLZ");
        print_canv->Print("test_hist01.pdf", "Test title");
    }

    for (auto hist_iter : data_container.raw_hist_container) {

        hist_iter->Draw();
        print_canv->Print("test_hist01.pdf", "Test title");
    }

    // reset message level
    gErrorIgnoreLevel = kPrint;
    print_canv->Print("test_hist01.pdf]");

    print_canv->Close();

    /*     for(auto hit_iter : data_container.hit_data){

            std::cout<<"\n\n\n\n"<<hit_iter.npeaks<<" at plane
       "<<hit_iter.plane<<" and strip "<<hit_iter.strip<<"\n"; for(auto p_iter :
       hit_iter.peaks_info){ std::cout<<"X is "<<p_iter.peak_x<<" Y is
       "<<p_iter.peak_y<<" fwhm is "<<p_iter.fwhm<<"\n";
            }

        }
     */
}

void test_hitdata() {

    generalDataStorage data_container;
    int err;

    // test error code -3
    std::vector<dataUVW> empty_data;
    convertHitData bad_hit_data(empty_data);

    err = bad_hit_data.getHitInfo();

    if (err != -3)
        std::cout << "Error hit data returned bad error code: " << err
                  << std::endl;

    data_container.uvw_data = generateUVW();

    convertHitData loc_convert_hit(data_container.uvw_data);

    err = loc_convert_hit.getHitInfo(100);
    if (err != 0)
        std::cout << "Error get hit info code " << err << std::endl;

    data_container.hit_data = loc_convert_hit.returnHitData();
    data_container.raw_hist_container = loc_convert_hit.returnHistData();

    std::cout << "Hit data size " << data_container.hit_data.size()
              << std::endl;
    std::cout << "Hist data size " << data_container.raw_hist_container.size()
              << std::endl;

    auto loc_canv = new TCanvas("xy format v2", "Peaks in XY v2");
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(3, 2, 0.01, 0.01);
    loc_pad->Draw();

    TH2D *u_hists =
        new TH2D(Form("u_hists_%d", 66), "u_hist", 512, 1, 513, 100, 1, 101);
    TH2D *v_hists =
        new TH2D(Form("v_hists_%d", 66), "v_hist", 512, 1, 513, 100, 1, 101);
    TH2D *w_hists =
        new TH2D(Form("w_hists_%d", 66), "w_hist", 512, 1, 513, 100, 1, 101);

    int bin = 0;
    int strip = 1;

    for (auto iter : data_container.uvw_data) {

        bin = 0;

        if (iter.plane_val == 0) {

            for (auto sig_iter : iter.signal_val) {

                u_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 1) {

            for (auto sig_iter : iter.signal_val) {

                v_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 2) {

            for (auto sig_iter : iter.signal_val) {

                w_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }
    }

    loc_pad->cd(1);
    u_hists->Draw("COLZ");
    loc_pad->cd(2);
    v_hists->Draw("COLZ");
    loc_pad->cd(3);
    w_hists->Draw("COLZ");

    std::vector<double> x_u;
    std::vector<double> y_u;

    std::vector<double> x_v;
    std::vector<double> y_v;

    std::vector<double> x_w;
    std::vector<double> y_w;

    for (auto hit_iter : data_container.hit_data) {

        if (hit_iter.plane == 0) {

            x_u.push_back(hit_iter.peak_x);
            y_u.push_back(hit_iter.strip);

        } else if (hit_iter.plane == 1) {

            x_v.push_back(hit_iter.peak_x);
            y_v.push_back(hit_iter.strip);

        } else if (hit_iter.plane == 2) {

            x_w.push_back(hit_iter.peak_x);
            y_w.push_back(hit_iter.strip);
        }
    }

    auto u_graph = new TGraph(x_u.size(), x_u.data(), y_u.data());
    TAxis *u_axis = u_graph->GetXaxis();
    u_axis->SetLimits(-1, 512);
    u_graph->SetMarkerColor(kBlue);
    u_graph->SetMarkerStyle(kFullCircle);
    loc_pad->cd(4);
    u_graph->Draw("AP");

    auto v_graph = new TGraph(x_v.size(), x_v.data(), y_v.data());
    TAxis *v_axis = v_graph->GetXaxis();
    v_axis->SetLimits(-1, 512);
    v_graph->SetMarkerColor(kBlue);
    v_graph->SetMarkerStyle(kFullCircle);
    loc_pad->cd(5);
    v_graph->Draw("AP");

    auto w_graph = new TGraph(x_w.size(), x_w.data(), y_w.data());
    TAxis *w_axis = w_graph->GetXaxis();
    w_axis->SetLimits(-1, 512);
    w_graph->SetMarkerColor(kBlue);
    w_graph->SetMarkerStyle(kFullCircle);
    loc_pad->cd(6);
    w_graph->Draw("AP");

    loc_canv->Update();
}

void test_convertXYZ(int entry_nr = 210, double peak_th = 50) {

    generalDataStorage data_container;

    auto goodFile = "./rootdata/data2.root";

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto returned_file = good_data.openFile();
    auto err = good_data.readData();
    err = good_data.decodeData(entry_nr);
    data_container.root_raw_data = good_data.returnRawData();

    convertUVW loc_conv_uvw(data_container.root_raw_data);

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    err = loc_conv_uvw.makeConversion();
    if (err != 0)
        std::cout << "Make conversion error code " << err << std::endl;

    err = loc_conv_uvw.substractBl();

    if (err != 0)
        std::cout << "Substractbl error code " << err << std::endl;

    err = loc_conv_uvw.drawChargeHist();

    if (err != 0)
        std::cout << "Draw Charge Hist error code " << err << std::endl;

    data_container.uvw_data = loc_conv_uvw.returnDataUVW();

    convertHitData loc_convert_hit(data_container.uvw_data);

    err = loc_convert_hit.getHitInfo(peak_th);
    if (err != 0)
        std::cout << "Error get hit info code " << err << std::endl;

    data_container.hit_data = loc_convert_hit.returnHitData();
    data_container.raw_hist_container = loc_convert_hit.returnHistData();

    std::cout << "Hit data size " << data_container.hit_data.size()
              << std::endl;
    std::cout << "Hist data size " << data_container.raw_hist_container.size()
              << std::endl;

    convertXYZ loc_conv_xyz(data_container.hit_data);

    err = loc_conv_xyz.makeConversionXYZ();

    data_container.xyz_data = loc_conv_xyz.returnXYZ();

    std::vector<double> x, y, z;

    for (auto point_iter : data_container.xyz_data) {

        x.push_back(point_iter.data_x);
        y.push_back(point_iter.data_y);
        z.push_back(point_iter.data_z);
    }

    auto loc_canv = new TCanvas("xy format v2", "Peaks in XY v2");
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(3, 3, 0.01, 0.01);
    loc_pad->Draw();

    auto p_graph = new TGraph(x.size(), x.data(), y.data());
    p_graph->GetXaxis()->SetLimits(-10, 150);
    p_graph->SetMarkerColor(kBlue);
    p_graph->SetMarkerStyle(kFullCircle);
    p_graph->SetTitle("XY coordinates projection; X axis; Y axis");
    loc_pad->cd(1);
    p_graph->Draw("AP");
    // loc_canv->Update();
    // loc_canv->Print("peaksxyv2.png");

    auto p_graph3d = new TGraph2D(x.size(), x.data(), y.data(), z.data());
    p_graph3d->SetMarkerColor(kRed);
    p_graph3d->SetMarkerStyle(kFullCircle);
    p_graph3d->SetTitle(
        "Reconstructed data in XYZ coordinates; X axis; Y axis; Z axis");
    loc_pad->cd(2);
    p_graph3d->Draw("P0");
    // loc_canv->Update();
    // loc_canv->Print("peaksxyz.png");

    TH2D *u_hists = new TH2D(Form("u_hists_%d", entry_nr), "u_hist", 512, 1,
                             513, 100, 1, 101);
    TH2D *v_hists = new TH2D(Form("v_hists_%d", entry_nr), "v_hist", 512, 1,
                             513, 100, 1, 101);
    TH2D *w_hists = new TH2D(Form("w_hists_%d", entry_nr), "w_hist", 512, 1,
                             513, 100, 1, 101);

    int bin = 0;
    int strip = 1;

    for (auto iter : data_container.uvw_data) {

        bin = 0;

        if (iter.plane_val == 0) {

            for (auto sig_iter : iter.signal_val) {

                u_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 1) {

            for (auto sig_iter : iter.signal_val) {

                v_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 2) {

            for (auto sig_iter : iter.signal_val) {

                w_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }
    }

    loc_pad->cd(4);
    u_hists->Draw("COLZ");
    loc_pad->cd(5);
    v_hists->Draw("COLZ");
    loc_pad->cd(6);
    w_hists->Draw("COLZ");

    std::vector<double> x_u;
    std::vector<double> y_u;

    std::vector<double> x_v;
    std::vector<double> y_v;

    std::vector<double> x_w;
    std::vector<double> y_w;

    for (auto hit_iter : data_container.hit_data) {

        if (hit_iter.plane == 0) {

            x_u.push_back(hit_iter.peak_x);
            y_u.push_back(hit_iter.strip);

        } else if (hit_iter.plane == 1) {

            x_v.push_back(hit_iter.peak_x);
            y_v.push_back(hit_iter.strip);

        } else if (hit_iter.plane == 2) {

            x_w.push_back(hit_iter.peak_x);
            y_w.push_back(hit_iter.strip);
        }
    }

    auto u_graph = new TGraph(x_u.size(), x_u.data(), y_u.data());
    u_graph->GetXaxis()->SetLimits(-1, 512);
    u_graph->GetHistogram()->SetMaximum(73);
    u_graph->GetHistogram()->SetMinimum(0);
    u_graph->SetMarkerColor(kBlue);
    u_graph->SetMarkerStyle(kFullCircle);
    u_graph->SetTitle("Hits detected on U plane; Time; Strip");
    loc_pad->cd(7);
    u_graph->Draw("AP");

    auto v_graph = new TGraph(x_v.size(), x_v.data(), y_v.data());
    v_graph->GetXaxis()->SetLimits(-1, 512);
    v_graph->GetHistogram()->SetMaximum(93);
    v_graph->GetHistogram()->SetMinimum(0);
    v_graph->SetMarkerColor(kBlue);
    v_graph->SetMarkerStyle(kFullCircle);
    v_graph->SetTitle("Hits detected on V plane; Time; Strip");
    loc_pad->cd(8);
    v_graph->Draw("AP");

    auto w_graph = new TGraph(x_w.size(), x_w.data(), y_w.data());
    w_graph->GetXaxis()->SetLimits(-1, 512);
    w_graph->GetHistogram()->SetMaximum(93);
    w_graph->GetHistogram()->SetMinimum(0);
    w_graph->SetMarkerColor(kBlue);
    w_graph->SetMarkerStyle(kFullCircle);
    w_graph->SetTitle("Hits detected on W plane; Time; Strip");
    loc_pad->cd(9);
    w_graph->Draw("AP");
    loc_pad->Modified();
    loc_pad->Update();

    loc_canv->Update();

    sleep(20);
}

void test_convert_multiple_entries() {

    std::vector<generalDataStorage> data_cont_vect;

    auto goodFile = "./rootdata/data2.root";

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto returned_file = good_data.openFile();
    auto err = good_data.readData();

    std::cout << "Tree has " << good_data.returnNEntries()
              << " entries.\n\n\n\n";

    for (auto i = 210; i < 300; i++) {

        generalDataStorage loc_data_storage;

        err = good_data.decodeData(i);

        loc_data_storage.root_raw_data = good_data.returnRawData();

        convertUVW loc_conv_uvw(loc_data_storage.root_raw_data);

        err = loc_conv_uvw.openSpecFile();

        if (err != 0)
            return;

        err = loc_conv_uvw.makeConversion();
        if (err != 0)
            std::cout << "Make conversion error code " << err << std::endl;

        err = loc_conv_uvw.substractBl();

        if (err != 0)
            std::cout << "Substractbl error code " << err << std::endl;

        loc_data_storage.uvw_data = loc_conv_uvw.returnDataUVW();

        convertHitData loc_convert_hit(loc_data_storage.uvw_data);

        err = loc_convert_hit.getHitInfo();
        if (err != 0)
            std::cout << "Error get hit info code " << err << std::endl;

        loc_data_storage.hit_data = loc_convert_hit.returnHitData();
        loc_data_storage.raw_hist_container = loc_convert_hit.returnHistData();

        convertXYZ loc_conv_xyz(loc_data_storage.hit_data);

        err = loc_conv_xyz.makeConversionXYZ();

        loc_data_storage.xyz_data = loc_conv_xyz.returnXYZ();

        data_cont_vect.push_back(loc_data_storage);
    }

    auto loc_canv = new TCanvas("xy format v2", "Peaks in XY v2");

    loc_canv->Print("xyz_data.pdf[");
    gErrorIgnoreLevel = kWarning;

    /*     auto p_graph3d = new TGraph2D(nr_points, x, y, z);
        p_graph3d->SetMarkerColor(kBlue);
        p_graph3d->SetMarkerStyle(kFullCircle);
        p_graph3d->Draw("P0");
        loc_canv->Update(); */

    for (auto loc_data_storage : data_cont_vect) {

        std::vector<double> x, y, z;

        for (auto point_iter : loc_data_storage.xyz_data) {

            x.push_back(point_iter.data_x);
            y.push_back(point_iter.data_y);
            z.push_back(point_iter.data_z);
        }

        auto p_graph3d = new TGraph2D(
            Form("Graph%d", loc_data_storage.root_raw_data.at(0).entry_nr),
            Form("Graph for %d", loc_data_storage.root_raw_data.at(0).entry_nr),
            x.size(), x.data(), y.data(), z.data());
        p_graph3d->SetMarkerColor(kBlue);
        p_graph3d->SetMarkerStyle(kFullCircle);
        p_graph3d->Draw("P0");
        loc_canv->Print("xyz_data.pdf", "titlu");
    }

    gErrorIgnoreLevel = kPrint;
    loc_canv->Print("xyz_data.pdf]");
}

void test_unitXYZ() {

    generalDataStorage data_container;
    int err;

    std::vector<hitPoints> empty_vect;

    convertXYZ bad_data_xyz(empty_vect);

    err = bad_data_xyz.makeConversionXYZ();

    if (err != -3)
        std::cout << "Error bad return code: " << err << std::endl;

    data_container.hit_data = generateHitData();

    convertXYZ loc_conv_xyz(data_container.hit_data);

    err = loc_conv_xyz.makeConversionXYZ();

    data_container.xyz_data = loc_conv_xyz.returnXYZ();

    std::vector<double> x, y, z;

    for (auto point_iter : data_container.xyz_data) {

        x.push_back(point_iter.data_x);
        y.push_back(point_iter.data_y);
        z.push_back(point_iter.data_z);
    }

    auto loc_canv = new TCanvas("xy format v2", "Peaks in XY v2");
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(3, 2, 0.01, 0.01);
    loc_pad->Draw();

    auto p_graph = new TGraph(x.size(), x.data(), y.data());
    TAxis *px_axis = p_graph->GetXaxis();
    px_axis->SetLimits(-1, 150);
    p_graph->GetHistogram()->SetMaximum(150);
    p_graph->GetHistogram()->SetMinimum(-1);
    p_graph->SetMarkerColor(kBlue);
    p_graph->SetMarkerStyle(kFullCircle);
    p_graph->SetTitle("XY coordinates projection; X axis; Y axis");
    loc_pad->cd(1);
    p_graph->Draw("AP");

    auto p_graph3d = new TGraph2D(x.size(), x.data(), y.data(), z.data());
    TAxis *px_axis3d = p_graph3d->GetXaxis();
    TAxis *py_axis3d = p_graph3d->GetYaxis();
    px_axis3d->SetRangeUser(-1, 150);
    py_axis3d->SetRangeUser(-1, 150);
    p_graph3d->SetMarkerColor(kRed);
    p_graph3d->SetMarkerStyle(kFullCircle);
    p_graph3d->SetTitle(
        "Reconstructed data in XYZ coordinates; X axis; Y axis; Z axis");
    loc_pad->cd(2);
    p_graph3d->Draw("P0");

    std::vector<double> x_u;
    std::vector<double> y_u;

    std::vector<double> x_v;
    std::vector<double> y_v;

    std::vector<double> x_w;
    std::vector<double> y_w;

    for (auto hit_iter : data_container.hit_data) {

        if (hit_iter.plane == 0) {

            x_u.push_back(hit_iter.peak_x);
            y_u.push_back(hit_iter.strip);

        } else if (hit_iter.plane == 1) {

            x_v.push_back(hit_iter.peak_x);
            y_v.push_back(hit_iter.strip);

        } else if (hit_iter.plane == 2) {

            x_w.push_back(hit_iter.peak_x);
            y_w.push_back(hit_iter.strip);
        }
    }

    auto u_graph = new TGraph(x_u.size(), x_u.data(), y_u.data());
    TAxis *u_axis = u_graph->GetXaxis();
    u_axis->SetLimits(-1, 512);
    u_graph->GetHistogram()->SetMaximum(73);
    u_graph->GetHistogram()->SetMinimum(0);
    u_graph->SetMarkerColor(kBlue);
    u_graph->SetMarkerStyle(kFullCircle);
    u_graph->SetTitle("Hits detected on U plane; Time; Strip");
    loc_pad->cd(4);
    u_graph->Draw("AP");

    auto v_graph = new TGraph(x_v.size(), x_v.data(), y_v.data());
    TAxis *v_axis = v_graph->GetXaxis();
    v_axis->SetLimits(-1, 512);
    v_graph->GetHistogram()->SetMaximum(93);
    v_graph->GetHistogram()->SetMinimum(0);
    v_graph->SetMarkerColor(kBlue);
    v_graph->SetMarkerStyle(kFullCircle);
    v_graph->SetTitle("Hits detected on V plane; Time; Strip");
    loc_pad->cd(5);
    v_graph->Draw("AP");

    auto w_graph = new TGraph(x_w.size(), x_w.data(), y_w.data());
    TAxis *w_axis = w_graph->GetXaxis();
    w_axis->SetLimits(-1, 512);
    w_graph->GetHistogram()->SetMaximum(93);
    w_graph->GetHistogram()->SetMinimum(0);
    w_graph->SetMarkerColor(kBlue);
    w_graph->SetMarkerStyle(kFullCircle);
    w_graph->SetTitle("Hits detected on W plane; Time; Strip");
    loc_pad->cd(6);
    w_graph->Draw("AP");

    loc_canv->Update();
}

void test_convert_to_csv(int entry_nr = 210) {

    generalDataStorage data_container;

    auto goodFile = "./rootdata/data2.root";

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto returned_file = good_data.openFile();
    auto err = good_data.readData();
    err = good_data.decodeData(entry_nr);
    data_container.root_raw_data = good_data.returnRawData();

    convertUVW loc_conv_uvw(data_container.root_raw_data);

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    err = loc_conv_uvw.makeConversion();
    if (err != 0)
        std::cout << "Make conversion error code " << err << std::endl;

    err = loc_conv_uvw.convertToCSV(
        "/media/gant/Expansion/tpcanalcsv/testcsv.csv");
    if (err != 0)
        std::cout << "Write to csv error code " << err << std::endl;
}

void test(TString lin_arg) {

    /*     auto getLib = "dict/build/libMyLib.so";
        gSystem->Load(getLib); */

    // test_loadData();
    // test_convertUVW();
    // test_viewdata();

    // 271 100 sau 210 50
    // test_convertXYZ(210);

    // test_convert_multiple_entries();
    // test_convert_to_csv();

    // test_hitdata();
    // test_unitXYZ();
}