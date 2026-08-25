#!/bin/bash
REPO_DIR="$HOME/moksha-kernel"
TOPIC="Moksha-Master-Core"

cd "$REPO_DIR" || exit 1

if [[ -n $(git status --porcelain) ]]; then
    echo "🔄 Auto-Syncing with GitHub..."
    TIMESTAMP=$(date "+%Y-%m-%d %H:%M:%S")
    git add . > /dev/null 2>&1
    git commit -m "auto-sync: $TIMESTAMP" > /dev/null 2>&1
    if git push origin main > /dev/null 2>&1; then
        echo "✅ Synced with GitHub successfully."
        curl -s \
          -H "Title: 🐙 GitHub Auto-Sync Complete" \
          -H "Priority: max" \
          -H "Tags: git,rocket" \
          -d "Changes synced to origin/main at $TIMESTAMP" \
          "https://ntfy.sh/$TOPIC" > /dev/null 2>&1 &
    fi
fi
