#!/bin/bash
# Generate the examples section for docs/index.html
# Usage: ./scripts/generate_examples_index.sh [docs_dir]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCS_DIR=${1:-../SWFRecompDocs/docs}
EXAMPLES_DIR="${DOCS_DIR}/examples"
INDEX_FILE="${DOCS_DIR}/index.html"

# Load exclude list from shared config file
EXCLUDE_CONFIG="${SCRIPT_DIR}/excluded_tests.conf"
EXCLUDE_TESTS=()

if [ -f "$EXCLUDE_CONFIG" ]; then
    # Read exclude list (skip comments and empty lines, keep full line with reason)
    while IFS= read -r line || [ -n "$line" ]; do
        # Skip comments and empty lines
        [[ "$line" =~ ^#.*$ ]] && continue
        [[ -z "$line" ]] && continue
        EXCLUDE_TESTS+=("$line")
    done < "$EXCLUDE_CONFIG"
fi

if [ ! -d "$EXAMPLES_DIR" ]; then
    echo "Error: Examples directory not found: $EXAMPLES_DIR"
    exit 1
fi

# Get list of examples (directories in examples/)
EXAMPLES=($(cd "$EXAMPLES_DIR" && find . -maxdepth 1 -type d ! -name '.' -exec basename {} \; | sort))

if [ ${#EXAMPLES[@]} -eq 0 ]; then
    echo "No examples found in $EXAMPLES_DIR"
    exit 0
fi

echo "Found ${#EXAMPLES[@]} examples: ${EXAMPLES[*]}"

# Generate demo cards HTML
DEMO_CARDS=""
for example in "${EXAMPLES[@]}"; do
    # Create a human-readable title (replace underscores with spaces, capitalize)
    TITLE=$(echo "$example" | sed 's/_/ /g' | sed 's/\b\(.\)/\u\1/g')

    # Determine description based on test name
    DESCRIPTION="Flash SWF test demonstrating "
    case "$example" in
        trace*)
            DESCRIPTION+="console output with the <code>trace()</code> function."
            ;;
        add_floats*)
            DESCRIPTION+="floating-point arithmetic addition operations."
            ;;
        add_strings*)
            DESCRIPTION+="string concatenation with the <code>add</code> operator."
            ;;
        *float*)
            DESCRIPTION+="floating-point arithmetic operations."
            ;;
        *string*)
            DESCRIPTION+="string manipulation and operations."
            ;;
        *var*)
            DESCRIPTION+="variable storage and retrieval with ActionScript."
            ;;
        *)
            DESCRIPTION+="ActionScript bytecode recompilation to WebAssembly."
            ;;
    esac

    DEMO_CARDS+="
                <div class=\"demo-card\">
                    <h3>$example</h3>
                    <p>$DESCRIPTION</p>
                    <a href=\"examples/$example/\" class=\"demo-link\">View Demo →</a>
                </div>
"
done

# Generate excluded tests section
EXCLUDED_SECTION=""
if [ ${#EXCLUDE_TESTS[@]} -gt 0 ]; then
    EXCLUDED_SECTION="
            <section>
                <h2>Excluded Tests</h2>
                <p>The following tests are currently excluded from the live demos due to known issues:</p>

                <div style=\"margin: 20px 0;\">
"

    for exclude_entry in "${EXCLUDE_TESTS[@]}"; do
        # Split on first colon to get test name and reason
        test_name="${exclude_entry%%:*}"
        reason="${exclude_entry#*:}"

        EXCLUDED_SECTION+="
                    <div style=\"background: #1f1f1f; padding: 15px; border-radius: 8px; margin: 10px 0; border-left: 4px solid #ff9800;\">
                        <h3 style=\"color: #ff9800; margin: 0 0 8px 0; font-size: 1.1em;\">$test_name</h3>
                        <p style=\"color: #b0b0b0; margin: 0; font-size: 0.95em;\">$reason</p>
                    </div>
"
    done

    EXCLUDED_SECTION+="
                </div>
            </section>
"
fi

# Read the current index.html
if [ ! -f "$INDEX_FILE" ]; then
    echo "Error: Index file not found: $INDEX_FILE"
    exit 1
fi

# Create a temporary file with updated content
TEMP_FILE=$(mktemp)

# Use awk to replace the demo-section content and add excluded section
awk -v demos="$DEMO_CARDS" -v excluded="$EXCLUDED_SECTION" '
    BEGIN { in_demo_section = 0; in_excluded_section = 0; skip_next_section = 0 }

    # Start of demo section
    /<section class="demo-section">/ {
        in_demo_section = 1
        print
        print "                <h2>Live Demos</h2>"
        print ""
        print demos
        next
    }

    # End of demo section
    in_demo_section && /<\/section>/ {
        in_demo_section = 0
        skip_next_section = 1
        print "                <p style=\"margin-top: 20px; color: #666;\">"
        print "                    More complex demos with graphics rendering coming soon!"
        print "                </p>"
        print
        # Insert excluded section after demo section
        if (excluded != "") {
            print excluded
        }
        next
    }

    # Check if next section is old excluded section - if so, skip it
    /<section>/ && skip_next_section {
        skip_next_section = 0
        # Peek at next line
        getline next_line
        if (next_line ~ /<h2>Excluded Tests<\/h2>/) {
            in_excluded_section = 1
            next
        } else {
            # Not excluded section, print normally
            print
            print next_line
            next
        }
    }

    # Skip everything inside old excluded section
    in_excluded_section {
        if (/<\/section>/) {
            in_excluded_section = 0
        }
        next
    }

    # Skip lines inside sections we are replacing
    in_demo_section { next }

    # Print all other lines
    { print }
' "$INDEX_FILE" > "$TEMP_FILE"

# Replace the original file
mv "$TEMP_FILE" "$INDEX_FILE"

echo "✅ Updated $INDEX_FILE with ${#EXAMPLES[@]} examples"
echo ""
echo "Examples included:"
for example in "${EXAMPLES[@]}"; do
    echo "  - $example"
done
