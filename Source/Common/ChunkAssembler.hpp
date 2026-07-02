#pragma once

#include "ExfilEnvelope.hpp"
#include "ResultDAO.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace chunk_assembler {

[[nodiscard]] inline bool is_complete(const std::vector<ResultDAO>& chunks, int chunk_total) {
    if (chunk_total <= 1) {
        return !chunks.empty();
    }

    std::map<int, bool> seen;
    for (const auto& chunk : chunks) {
        if (chunk.chunk_index >= 0 && chunk.chunk_index < chunk_total) {
            seen[chunk.chunk_index] = true;
        }
    }

    if (static_cast<int>(seen.size()) != chunk_total) {
        return false;
    }

    for (int i = 0; i < chunk_total; ++i) {
        if (!seen.contains(i)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] inline std::optional<ResultDAO> assemble(
    const std::vector<ResultDAO>& chunks,
    int chunk_total) {
    if (chunks.empty()) {
        return std::nullopt;
    }

    if (chunk_total <= 1) {
        return chunks.front();
    }

    if (!is_complete(chunks, chunk_total)) {
        return std::nullopt;
    }

    std::map<int, std::string> ordered;
    for (const auto& chunk : chunks) {
        ordered[chunk.chunk_index] = chunk.data;
    }

    ResultDAO assembled = chunks.front();
    assembled.data.clear();
    for (int i = 0; i < chunk_total; ++i) {
        assembled.data += ordered.at(i);
    }
    assembled.chunk_index = -1;
    assembled.chunk_total = 1;
    assembled.status = exfil::kStatusSuccess;

    return assembled;
}

} // namespace chunk_assembler
