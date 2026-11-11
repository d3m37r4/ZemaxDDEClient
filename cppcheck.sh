#!/bin/bash
# Static analysis with cppcheck.
#
# The same command runs locally and in CI, so both see identical findings.
# Findings are compared against a checked-in baseline: the check fails only on
# NEW findings, so the existing noise never blocks the pipeline while any
# regression is caught immediately.
#
#   ./cppcheck.sh --check           fail when new findings appear (CI mode)
#   ./cppcheck.sh --update-baseline accept the current findings as the baseline
#
# The baseline depends on the analyzer version (MSYS2 ships 2.19.x). After a
# toolchain upgrade it has to be regenerated deliberately.

set -u

MODE="${1:---check}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR" || exit 1

SRC_DIRS=(src/app src/dde src/gui src/logger)
SUPPRESSIONS="ci/cppcheck-suppressions.txt"
BASELINE="ci/cppcheck-baseline.txt"
OUT_DIR="build-audit"
RAW_REPORT="$OUT_DIR/cppcheck-report.txt"
FINDINGS="$OUT_DIR/cppcheck-findings.txt"
NEW_FINDINGS="$OUT_DIR/cppcheck-new.txt"

mkdir -p "$OUT_DIR"

if [ ! -f "$SUPPRESSIONS" ]; then
    echo "Missing suppression list: $SUPPRESSIONS" >&2
    exit 2
fi

cppcheck \
    --enable=warning,style,performance,portability \
    --inconclusive \
    --check-level=exhaustive \
    --std=c++23 \
    --platform=win64 \
    --library=windows \
    --suppressions-list="$SUPPRESSIONS" \
    --suppress=missingIncludeSystem \
    --suppress=unusedFunction \
    --template='{file}:{line}:{severity}:{id}' \
    -j "$(nproc)" \
    -I src/app -I src/dde -I src/gui -I src/logger -I src/assets \
    -I src/lib/imgui -I src/lib/implot -I src/lib/implot3d \
    -I src/lib/json/single_include \
    "${SRC_DIRS[@]}" > "$RAW_REPORT" 2>&1
CPPCHECK_STATUS=$?

if [ "$CPPCHECK_STATUS" -ne 0 ]; then
    echo "cppcheck failed with exit code $CPPCHECK_STATUS" >&2
    tail -n 30 "$RAW_REPORT" >&2
    exit "$CPPCHECK_STATUS"
fi

# Keep only finding lines: "<file>:<line>:<severity>:<id>". The message text is
# excluded on purpose - it changes between analyzer versions and would break
# the baseline comparison for no reason.
LC_ALL=C grep -E '^[^:]+:[0-9]+:[a-z]+:[A-Za-z0-9_]+$' "$RAW_REPORT" \
    | sed -e 's|^\./||' -e 's|\\|/|g' \
    | LC_ALL=C sort -u > "$FINDINGS"

FOUND=$(wc -l < "$FINDINGS")
echo "cppcheck: $FOUND distinct findings in ${SRC_DIRS[*]}"

case "$MODE" in
    --update-baseline)
        cp "$FINDINGS" "$BASELINE"
        echo "Baseline updated: $BASELINE ($FOUND findings)"
        ;;
    --check)
        if [ ! -f "$BASELINE" ]; then
            echo "Missing baseline: $BASELINE (run ./cppcheck.sh --update-baseline)" >&2
            exit 2
        fi
        LC_ALL=C sort -u "$BASELINE" > "$OUT_DIR/baseline.sorted"
        LC_ALL=C comm -13 "$OUT_DIR/baseline.sorted" "$FINDINGS" > "$NEW_FINDINGS"
        NEW=$(wc -l < "$NEW_FINDINGS")
        if [ "$NEW" -gt 0 ]; then
            echo "New cppcheck findings ($NEW):"
            cat "$NEW_FINDINGS"
            exit 1
        fi
        echo "No new cppcheck findings."
        ;;
    *)
        echo "Usage: $0 [--check|--update-baseline]" >&2
        exit 2
        ;;
esac
