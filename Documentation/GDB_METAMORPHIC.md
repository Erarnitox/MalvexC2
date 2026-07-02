# GDB Debugging — Metamorphic Implant Packer

## Packed implant: break on stub entry

```bash
gdb -ex 'break main' \
    -ex run --args /tmp/malvex_metamorphic_smoke/implant_a
```

After the stub passes startup checks, inspect boot metadata:

```
p *(metamorphic::BootParams*)mx_stub_region
x/16bx mx_stub_region
```

## Break after config decrypt

```bash
gdb -ex 'break metamorphic::decrypt_buffer' \
    -ex run --args /tmp/malvex_metamorphic_smoke/implant_a
```

Inside GDB:

```
finish
x/s envelope.payload_section_name
x/s envelope.implant.server_url
```

## Break on payload handoff (`mx_main`)

The stub decrypts `.mx_text` **in place** at its linked VMA (not a separate mmap copy),
then jumps to `mx_main` at the original section address. This preserves RIP-relative
addressing required by the payload code.

```bash
gdb -ex 'break mx_main' \
    -ex run --args /tmp/malvex_metamorphic_smoke/implant_a
```

If the symbol is stripped in the packed binary, break on `decrypt_to_executable`
in `StubMain.cpp` and single-step through the function pointer call.

## Inspect per-build bytecode

```bash
gdb -ex 'break metamorphic::TransformEngine::decrypt' \
    -ex run --args /tmp/malvex_metamorphic_smoke/implant_a
```

Compare bytecode at `mx_stub_region + sizeof(BootParams)` between two packed builds.

## Compare two packed builds

```bash
readelf -S /tmp/malvex_metamorphic_smoke/implant_a
readelf -S /tmp/malvex_metamorphic_smoke/implant_b
sha256sum /tmp/malvex_metamorphic_smoke/implant_*
objdump -d -j .mx_stub /tmp/malvex_metamorphic_smoke/implant_a | head
```

Section names `.mx_config` / `.mx_text` must be absent; randomized `.mx_<hex>` names
and different stub decryptor bytes confirm metamorphic output.

## Useful settings

```
set logging on
set logging file /tmp/malvex-metamorphic-gdb.log
set pagination off
set disassemble-next-line on
```

## Development mode (`MALVEX_PACKED=0`)

Rebuild implant with `-DMALVEX_PACKED=OFF` to run `mx_main` directly without packing:

```bash
cmake -S . -B build -DMALVEX_PACKED=OFF
cmake --build build --target implant
gdb -ex 'break mx_main' -ex run --args ./build/bin/Implant/implant
```
