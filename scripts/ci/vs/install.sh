#!/usr/bin/env bash
set -e

echo "Executing ci/vs/install.sh"

export OF_ROOT="$APPVEYOR_BUILD_FOLDER/../openFrameworks"
export OF_ADDONS=$OF_ROOT/addons

pwd;
echo "OF_ADDONS: $OF_ADDONS"

ADDONS="local-projects/ofxLibWebsockets"

cd $OF_ADDONS

for ADDON in $ADDONS
do
  echo "Cloning addon '$ADDON' to " `pwd`
  git clone --depth=1  https://github.com/$ADDON.git
done

cd -