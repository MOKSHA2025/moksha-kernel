#!/bin/bash

REPO_DIR="$HOME/moksha-kernel"
TOPIC="Moksha-Master-Core"

cd "$REPO_DIR" || exit 1

# Send Watcher Startup Alert
curl -s \
  -H "Title: 👁️ Moksha Auto-Watcher Online" \
  -H "Priority: max" \
  -H "Tags: eye,shield" \
  -d "Real-time GitHub auto-sync watcher daemon is now running in background." \
  "https://ntfy.sh/$TOPIC" > /dev/null 2>&1 &

echo "[WATCHER] Background Auto-Watcher Daemon active on $REPO_DIR"

while true; do
    # Check git status for any untracked or modified files
    if [[ -n $(git status --porcelain) ]]; then
        echo "[WATCHER] File change detected! Triggering sync.sh..."
        bash "$REPO_DIR/sync.sh"
    fi
    sleep 3
done
