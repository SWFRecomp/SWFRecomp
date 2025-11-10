#!/bin/bash
# Add config.toml to all test directories that have test.swf but no config.toml

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SWFRECOMP_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TESTS_DIR="${SWFRECOMP_ROOT}/tests"

cd "${TESTS_DIR}"

# Find all directories with test.swf
COUNT=0
SKIPPED=0

for test_dir in */; do
    test_dir="${test_dir%/}"  # Remove trailing slash

    # Check if test.swf exists
    if [ ! -f "${test_dir}/test.swf" ]; then
        continue
    fi

    # Check if config.toml already exists
    if [ -f "${test_dir}/config.toml" ]; then
        echo "✓ ${test_dir} already has config.toml"
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    # Create config.toml
    cat > "${test_dir}/config.toml" << 'EOF'
[input]
path_to_swf = "test.swf"
output_tags_folder = "RecompiledTags"
output_scripts_folder = "RecompiledScripts"
EOF

    echo "✅ Created config.toml for ${test_dir}"
    COUNT=$((COUNT + 1))
done

echo ""
echo "Summary:"
echo "  Created: ${COUNT} config.toml files"
echo "  Skipped: ${SKIPPED} (already exist)"
echo "  Total tests: $((COUNT + SKIPPED))"
