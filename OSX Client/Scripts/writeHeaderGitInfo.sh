#/bin/bash


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

echo "#define GIT_COMMIT_NUMBER @\"$gitCommitNum\"" > $SRCROOT/src/GitCommitNumber.h

