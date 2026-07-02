#!/usr/bin/env bash
# Smoke test for metamorphic implant packer
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
TEMPLATE="${BUILD}/bin/Client/implant"

echo "[1/5] Checking clang..."
if ! command -v clang >/dev/null 2>&1; then
  echo "FAIL: clang is required for DecryptorCodegen"
  exit 1
fi

echo "[2/5] Running unit tests..."
cmake -S "${ROOT}" -B "${BUILD}" -DBUILD_TESTING=ON
cmake --build "${BUILD}" --target metamorphic_tests metamorphic_pack_smoke
"${BUILD}/bin/Common/tests/metamorphic_tests"
"${BUILD}/bin/Client/metamorphic_pack_smoke" "${TEMPLATE}"

echo "[3/5] Build artifacts..."
cmake --build "${BUILD}" --target implant client

echo "[4/5] Verifying template sections..."
readelf -S "${TEMPLATE}" | rg '\.mx_(config|text|stub)' >/dev/null

echo "[5/5] Manual integration checklist:"
cat <<'EOF'
  1. Start server:  ./build/bin/Server/server
  2. Build packed implant from client Builder tab (or use metamorphic_pack_smoke outputs)
  3. Run packed implant: /tmp/malvex_metamorphic_smoke/implant_a
  4. Confirm victim check-in in client Connections tab
  5. Build a second implant and verify sha256sum differs
  6. See Documentation/GDB_METAMORPHIC.md for stub decrypt breakpoints
EOF

echo "Smoke test complete."
