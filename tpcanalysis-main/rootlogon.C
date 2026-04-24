// Auto-loaded by ROOT when launched from this directory. Sets up everything
// that run_mini.cpp / verify_plot.cpp need so the user can just do:
//
//   root -b -q 'run_mini.cpp("path.root")'
//   root -b -q 'run_mini.cpp+O("path.root")'
//
// without any -e flags, ROOT_INCLUDE_PATH, or LD_LIBRARY_PATH.
void rootlogon() {
    // Include path for our tpctrack/*.hpp headers. gSystem is for the
    // rootcling + ACLiC compile flags; gInterpreter is what cling itself
    // uses to resolve quoted #include in interpreter mode. Both are needed.
    gSystem->AddIncludePath(" -I../include ");
    gInterpreter->AddIncludePath("../include");

    // Link path + -lMyLib for ACLiC's final dylib link. The built libMyLib
    // has an absolute install_name baked in, so dlopen finds it at runtime.
    gSystem->AddLinkedLibs("-Ldict/build -lMyLib");
}
