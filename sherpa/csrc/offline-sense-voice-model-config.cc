// sherpa/csrc/offline-sense-voice-model-config.cc
//
// Copyright (c)  2025  Xiaomi Corporation

#include "sherpa/csrc/offline-sense-voice-model-config.h"

#include "sherpa/csrc/file-utils.h"
#include "sherpa/csrc/macros.h"

namespace sherpa {

void OfflineSenseVoiceModelConfig::Register(ParseOptions *po) {
  po->Register("sense-voice-model", &model, "Path to model.pt of SenseVoice.");
  po->Register(
      "sense-voice-language", &language,
      "Valid values: auto, zh, en, ja, ko, yue. If left empty, auto is used");
  po->Register(
      "sense-voice-use-itn", &use_itn,
      "True to enable inverse text normalization. False to disable it.");
}

bool OfflineSenseVoiceModelConfig::Validate() const {
  if (!FileExists(model)) {
    SHERPA_LOGE("SenseVoice model '%s' does not exist", model.c_str());
    return false;
  }

  if (!language.empty()) {
    if (language != "auto" && language != "zh" && language != "en" &&
        language != "ja" && language != "ko" && language != "yue") {
      SHERPA_LOGE(
          "Invalid sense-voice-language: '%s'. Valid values are: auto, zh, en, "
          "ja, ko, yue. Or you can leave it empty to use 'auto'",
          language.c_str());

      return false;
    }
  }

  return true;
}

std::string OfflineSenseVoiceModelConfig::ToString() const {
  std::ostringstream os;

  os << "OfflineSenseVoiceModelConfig(";
  os << "model=\"" << model << "\", ";
  os << "language=\"" << language << "\", ";
  os << "use_itn=" << (use_itn ? "True" : "False") << ")";

  return os.str();
}

}  // namespace sherpa
