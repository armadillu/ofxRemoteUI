#!/usr/bin/env bash
set -e

echo "Executing ci/linux/install.sh"

export OF_ROOT=~/openFrameworks
export OF_ADDONS=$OF_ROOT/addons

ADDONS=""

cd $OF_ADDONS

for ADDON in $ADDONS
do
  echo "Cloning addon '$ADDON' to " `pwd`
  git clone --depth=1 --branch=$OF_BRANCH https://github.com/$ADDON.git
done

cd -