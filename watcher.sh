#!/bin/bash
REPO_DIR="$HOME/moksha-kernel"
cd "$REPO_DIR" || exit 1

while true; do
    if [[ -n $(git status --porcelain) ]]; then
        bash "$REPO_DIR/sync.sh"
    fi
    sleep 2
done
