#!/usr/bin/env bash
# Smoke test for exfiltration pipeline (requires running server + implant)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"

echo "[1/3] Running unit tests..."
cmake -S "${ROOT}" -B "${BUILD}" -DBUILD_TESTING=ON
cmake --build "${BUILD}" --target exfil_tests
"${BUILD}/bin/Common/tests/exfil_tests"

echo "[2/3] Build artifacts..."
cmake --build "${BUILD}" --target server client implant

echo "[3/3] Manual integration checklist:"
cat <<'EOF'
  1. Start server:  ./build/bin/Server/server
  2. Start client:  ./build/bin/Client/client
  3. Start implant: ./build/bin/Implant/implant
  4. From Connections menu: send Loot / Screenshot / Keylogger
  5. Open Artifacts tab and click Refresh
  6. Terminal: download <file> and verify local artifact entry
EOF

echo "Smoke test complete."
