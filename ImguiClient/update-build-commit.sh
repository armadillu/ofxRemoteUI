#!/bin/sh
# Embed the addon repository revision in the client, updating only on change.
set -eu
client_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$client_dir/.." && pwd -P)
git_root=$(git -C "$repo_dir" rev-parse --show-toplevel 2>/dev/null || true)
commit=unknown
if [ "$git_root" = "$repo_dir" ]; then
    commit=$(git -C "$repo_dir" rev-parse --verify HEAD 2>/dev/null || printf unknown)
fi
header="$client_dir/src/BuildCommit.h"
content="#define OFX_REMOTEUI_BUILD_COMMIT \"$commit\""
if [ ! -f "$header" ] || [ "$(cat "$header")" != "$content" ]; then
    printf '%s\n' "$content" > "$header"
fi
