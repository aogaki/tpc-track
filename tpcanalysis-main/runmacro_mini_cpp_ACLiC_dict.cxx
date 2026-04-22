// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME dIUsersdIaogakidIWorkSpacedItpcmItrackdItpcanalysismImaindIrunmacro_mini_cpp_ACLiC_dict
#define R__NO_DEPRECATION

/*******************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define G__DICTIONARY
#include "ROOT/RConfig.hxx"
#include "TClass.h"
#include "TDictAttributeMap.h"
#include "TInterpreter.h"
#include "TROOT.h"
#include "TBuffer.h"
#include "TMemberInspector.h"
#include "TInterpreter.h"
#include "TVirtualMutex.h"
#include "TError.h"

#ifndef G__ROOT
#define G__ROOT
#endif

#include "RtypesImp.h"
#include "TIsAProxy.h"
#include "TFileMergeInfo.h"
#include <algorithm>
#include "TCollectionProxyInfo.h"
/*******************************************************************/

#include "TDataMember.h"

// The generated code does not explicitly qualify STL entities
namespace std {} using namespace std;

// Header files passed as explicit arguments
#include "/Users/aogaki/WorkSpace/tpc-track/tpcanalysis-main/./runmacro_mini.cpp"

// Header files passed via #pragma extra_include

namespace ROOT {
   // Registration Schema evolution read functions
   int RecordReadRules_runmacro_mini_cpp_ACLiC_dict() {
      return 0;
   }
   static int _R__UNIQUE_DICT_(ReadRules_runmacro_mini_cpp_ACLiC_dict) = RecordReadRules_runmacro_mini_cpp_ACLiC_dict();R__UseDummy(_R__UNIQUE_DICT_(ReadRules_runmacro_mini_cpp_ACLiC_dict));
} // namespace ROOT
namespace {
  void TriggerDictionaryInitialization_runmacro_mini_cpp_ACLiC_dict_Impl() {
    static const char* headers[] = {
"./runmacro_mini.cpp",
nullptr
    };
    static const char* includePaths[] = {
"/opt/ROOT/include",
"/opt/ROOT/etc/",
"/opt/ROOT/etc//cling",
"/opt/ROOT/etc//cling/plugins/include",
"/opt/ROOT/include/",
"/opt/ROOT/include",
"/opt/ROOT/include/",
"/Users/aogaki/WorkSpace/tpc-track/tpcanalysis-main/",
nullptr
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "runmacro_mini_cpp_ACLiC_dict dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_AutoLoading_Map;
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "runmacro_mini_cpp_ACLiC_dict dictionary payload"

#ifndef __ACLIC__
  #define __ACLIC__ 1
#endif

#define _BACKWARD_BACKWARD_WARNING_H
// Inline headers
#include "./runmacro_mini.cpp"

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[] = {
"drawUVWimage_mini", payloadCode, "@",
"mass_create_clean_images_mini", payloadCode, "@",
"runmacro_mini", payloadCode, "@",
"viewUVWdata_mini", payloadCode, "@",
nullptr
};
    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("runmacro_mini_cpp_ACLiC_dict",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_runmacro_mini_cpp_ACLiC_dict_Impl, {}, classesHeaders, /*hasCxxModule*/false);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_runmacro_mini_cpp_ACLiC_dict_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_runmacro_mini_cpp_ACLiC_dict() {
  TriggerDictionaryInitialization_runmacro_mini_cpp_ACLiC_dict_Impl();
}
