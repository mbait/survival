#!/usr/bin/env bash
# Decide whether the CI lint job should run against the full source tree
# or only the C/C++ files that changed in this push / pull request.
#
# Inputs (env):
#   GITHUB_EVENT_NAME  push | pull_request
#   PR_BASE_SHA        for pull_request events
#   EVENT_BEFORE       for push events ('0000...' on a new branch)
#   GITHUB_OUTPUT      file path appended to (when run under GH Actions)
#
# Outputs (to $GITHUB_OUTPUT + stderr):
#   mode               all | none | some
#   changed_files      space-separated, only when mode=some
#   cpp_files          space-separated changed .cpp, only when mode=some
#   any_header_changed true | false, only when mode=some
#
# "all" — full-tree lint. Picked when the diff range can't be determined
# (new branch, shallow clone, missing base ref) or when one of the lint
# config files itself changed (clang-format / clang-tidy rules,
# CMakeLists, presets, this script, the workflow).
# "none" — no C/C++ source or header changed; lint can be skipped.
# "some" — at least one .cpp/.h changed; lint only those.

set -euo pipefail

emit() {
    if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
        printf '%s=%s\n' "$1" "$2" >> "$GITHUB_OUTPUT"
    fi
    printf '%s=%s\n' "$1" "$2" >&2
}

event_name="${GITHUB_EVENT_NAME:-push}"
base=""

case "$event_name" in
    pull_request)
        base="${PR_BASE_SHA:-}"
        ;;
    *)
        before="${EVENT_BEFORE:-}"
        if [[ -n "$before" && "$before" != "0000000000000000000000000000000000000000" ]]; then
            base="$before"
        fi
        ;;
esac

if [[ -z "$base" ]] || ! git cat-file -e "$base" 2>/dev/null; then
    echo "no usable base ref ($event_name, base=$base): lint everything" >&2
    emit mode all
    exit 0
fi

# Anything in this list bumps the scope to "all" — touching the rule set
# or the configure step affects every TU's lint outcome.
config_paths=(
    .clang-format
    .clang-tidy
    .github/workflows/ci.yml
    CMakeLists.txt
    CMakePresets.json
    scripts/ci-lint-scope.sh
)
config_changed=$(
    git diff --name-only --diff-filter=ACMR "$base"..HEAD -- "${config_paths[@]}" || true
)
if [[ -n "$config_changed" ]]; then
    echo "lint config touched ($(echo "$config_changed" | tr '\n' ' ')): lint everything" >&2
    emit mode all
    exit 0
fi

# Restrict to in-tree project files; drop anything under build/ or vendored
# dirs that .gitignore would have excluded but could still appear via add.
cpp_changed=$(
    git diff --name-only --diff-filter=ACMR "$base"..HEAD -- '*.cpp' \
    | grep -vE '^(build|\.git|d3dg|d3dg2|a|Debug|Release|builds)/' || true
)
header_changed=$(
    git diff --name-only --diff-filter=ACMR "$base"..HEAD -- '*.h' \
    | grep -vE '^(build|\.git|d3dg|d3dg2|a|Debug|Release|builds)/' || true
)

if [[ -z "$cpp_changed" && -z "$header_changed" ]]; then
    echo "no C/C++ changes in $base..HEAD: skip lint" >&2
    emit mode none
    exit 0
fi

# Concatenate via printf so tabs/newlines collapse to single spaces.
all_changed=$(printf '%s\n%s\n' "$cpp_changed" "$header_changed" \
    | grep -vE '^\s*$' | tr '\n' ' ')

emit mode some
emit changed_files "$all_changed"
emit cpp_files "$(printf '%s\n' "$cpp_changed" | tr '\n' ' ')"
if [[ -n "$header_changed" ]]; then
    emit any_header_changed true
else
    emit any_header_changed false
fi
