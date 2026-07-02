# GDB Debugging — Exfiltration Pipeline

## Server: break on result storage

```bash
gdb -ex 'break ResultService::store_beacon_result' \
    -ex run --args ./build/bin/Server/server
```

Inside GDB after a beacon check-in:

```
bt full
print beacon_result.command_uid
print beacon_result.kind
print beacon_result.chunk_total
```

## Implant: break on command dispatch

```bash
gdb -ex 'break CommandDispatcher::dispatch' \
    -ex run --args ./build/bin/Implant/implant
```

## Client: break on result fetch

```bash
gdb -ex 'break RestGateway::fetch_results' \
    -ex run --args ./build/bin/Client/client
```

## Useful settings

```
set logging on
set logging file /tmp/malvex-gdb.log
set pagination off
```

## Chunked download verification

1. Set breakpoint on `BeaconState::enqueue_chunks`
2. Queue `download` for a file larger than 512 KB
3. Confirm multiple chunks are queued and sent across beacon cycles
4. On server, break on `chunk_assembler::assemble` to verify reassembly
