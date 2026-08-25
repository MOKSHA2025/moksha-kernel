#!/bin/bash

# Target Repo Directory
REPO_DIR="$HOME/moksha-kernel"
TOPIC="Moksha-Master-Core"

cd "$REPO_DIR" || exit 1

# Check if there are changes
if [[ -n $(git status --porcelain) ]]; then
    TIMESTAMP=$(date "+%Y-%m-%d %H:%M:%S")
    
    # Stage and Commit
    git add .
    git commit -m "auto-sync: core state updated at $TIMESTAMP" > /dev/null 2>&1
    
    # Push to GitHub
    if git push origin main > /dev/null 2>&1; then
        # Push notification on success
        curl -s \
          -H "Title: 🐙 GitHub Auto-Sync Complete" \
          -H "Priority: max" \
          -H "Tags: git,octocat,rocket" \
          -d "All updates successfully committed & pushed to origin/main at $TIMESTAMP" \
          "https://ntfy.sh/$TOPIC" > /dev/null 2>&1 &
        echo "[SYNC] Successfully pushed updates to GitHub."
    else
        # Push notification on failure
        curl -s \
          -H "Title: ⚠️ GitHub Sync Failed" \
          -H "Priority: max" \
          -H "Tags: warning,x" \
          -d "Failed to push updates to GitHub. Check network or credentials." \
          "https://ntfy.sh/$TOPIC" > /dev/null 2>&1 &
        echo "[SYNC ERROR] Failed to push to GitHub."
    fi
else
    echo "[SYNC] No changes detected. Working tree clean."
fi
