/**
 * Plugin: scan_yarax
 * Purpose: Run yara-x rules against raw pages
 * Reference: https://virustotal.github.io/yara-x/
 **/

#include <iostream>

#include "config.h"
#include "be20_api/scanner_params.h"

#ifdef HAVE_YARAX
extern "C" {
#include <yara_x.h>

void scan_yarax(scanner_params &sp);
}

auto YARA_RULE = R"(
rule test_rule {
  strings:
    $a = "test"
  condition:
    $a
}
)";

using rules_ptr = std::unique_ptr<YRX_RULES, decltype(&yrx_rules_destroy)>;

rules_ptr& getYaraRules() {
  static rules_ptr rules(nullptr, yrx_rules_destroy);
  return rules;
}

struct YaraCallbackData {
  feature_recorder &Recorder;
  pos0_t PagePos;
  size_t Offset;
};

void yara_callback(const YRX_RULE* rule, void* userData) {
  const uint8_t* ruleID = nullptr;
  size_t   idLen = 0;
  if (yrx_rule_identifier(rule, &ruleID, &idLen) != SUCCESS) {
    std::cerr << "Failed to get rule ID in yara_callback" << std::endl;
    return;
  }
  std::string_view ruleIDView(reinterpret_cast<const char*>(ruleID), idLen);
  const YaraCallbackData* data = static_cast<YaraCallbackData*>(userData);
  data->Recorder.write(data->PagePos.shift(data->Offset), std::string(ruleIDView), "");
}

void scan_yarax(scanner_params &sp) {
  sp.check_version();
  if (sp.phase == scanner_params::PHASE_INIT){
    sp.info->set_name("yarax");
    sp.info->author          = "Jon Stewart";
    sp.info->description     = "Scans for yara-x rule matches in raw pages";
    sp.info->scanner_version = "1.0";
    sp.info->feature_defs.push_back(feature_recorder_def("yara-x"));
    return;
  }
  else if (sp.phase == scanner_params::PHASE_INIT2) {
    YRX_RULES* rules = nullptr;
    YRX_RESULT result = yrx_compile(YARA_RULE, &rules);
    if (result != SUCCESS) {
      std::cerr << "Failed to compile yara-x rule: " << yrx_last_error() << std::endl;
      return;
    }
    else {
      getYaraRules().reset(rules);
    }
  }
  else if (sp.phase == scanner_params::PHASE_SCAN) {
    rules_ptr& rules = getYaraRules();
    if (!rules) {
      return;
    }
    YRX_SCANNER* scannerRawPtr = nullptr;
    YRX_RESULT result = yrx_scanner_create(rules.get(), &scannerRawPtr);
    if (result != SUCCESS) {
      std::cerr << "Failed to create yara-x scanner: " << yrx_last_error() << std::endl;
      return;
    }
    std::unique_ptr<YRX_SCANNER, decltype(&yrx_scanner_destroy)> scanner(scannerRawPtr, yrx_scanner_destroy);

    YaraCallbackData callbackData{sp.named_feature_recorder("yara-x"), sp.sbuf->pos0, 0};

    result = yrx_scanner_on_matching_rule(scanner.get(), yara_callback, &callbackData);
    if (result != SUCCESS) {
      std::cerr << "Failed to set yara-x callback: " << yrx_last_error() << std::endl;
      return;
    }

    // Iterate the sbuf 4KB at a time and scan these pages individually
    const sbuf_t *sbuf = sp.sbuf;
    const size_t blockSize = 4096;
    const uint8_t* curBlock = sbuf->get_buf();
    const uint8_t* end = curBlock + sbuf->pagesize;
    while (curBlock < end) {
      const size_t len = std::min(blockSize, static_cast<size_t>(end - curBlock));
      result = yrx_scanner_scan(scanner.get(), curBlock, len);
      if (result != SUCCESS) {
        std::cerr << "Failed to scan yara-x: " << yrx_last_error() << std::endl;
        return;
      }
      curBlock += len;
      callbackData.Offset += len;
    }
  }
}
#endif

