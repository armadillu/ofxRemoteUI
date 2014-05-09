#/bin/bash


gitCommitNum=$(git rev-list HEAD --count)
gitBranch=$(git rev-parse --abbrev-ref HEAD)
gitCommitHash=$(git rev-parse --verify HEAD --short)
gitIsDirty=$([[ $(git diff --shortstat 2> /dev/null | tail -n1) != "" ]] && echo " *Dirty!")

if [ -z "$gitCommitNum" ]; then
gitCommitNum="Unknown Version"
fi

if [ -z "$gitBranch" ]; then
gitBranch="Unknown Branch"
fi

if [ -z "$gitCommitHash" ]; then
gitCommitHash="Unknown Commit"
fi


defaults write "${TARGET_BUILD_DIR}/$INFOPLIST_PATH" "CFBundleShortVersionString" "Git Commit # $gitCommitNum" 
defaults write "${TARGET_BUILD_DIR}/$INFOPLIST_PATH" "CFBundleVersion" "$gitBranch - $gitCommitHash $gitIsDirty"

echo 'GIT_COMMIT_NUMBER is '$gitCommitNum
echo 'git Commit Hash is '$gitCommitHash
