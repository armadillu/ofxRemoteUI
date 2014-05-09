#/bin/bash

echo "path=$INFOPLIST_PATH"

gitCommitNum=$(git rev-list HEAD --count)
gitBranch=$(git rev-parse --abbrev-ref HEAD)
gitCommitHash=$(git rev-parse --verify HEAD --short)

if [ -z "$gitCommitNum" ]; then
gitCommitNum="Unknown Version"
fi

if [ -z "$gitBranch" ]; then
gitBranch="Unknown Branch"
fi

if [ -z "$gitCommitHash" ]; then
gitCommitHash="Unknown Commit"
fi


defaults write "$INFOPLIST_PATH" "CFBundleShortVersionString" "Git Commit $gitCommitNum"
defaults write "$INFOPLIST_PATH" "CFBundleVersion" "$gitBranch - $gitCommitHash"
