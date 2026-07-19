#!/bin/bash
# etc/CONFIGURE_WINDOWS_MSYS2.bash
# Configure MSYS2/MinGW environment for be20_api build
# See: https://www.msys2.org/

OS_NAME=msys
MAKE_CONCURRENCY=-j2
MPKGS=""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

. ./paths.bash 2>/dev/null || true

echo "******************************************************************"
echo "Configuring Windows/MSYS2 environment to compile be20_api"
echo "******************************************************************"

# Ensure MSYS2 is updated

# Install required packages
if [ $? != 0 ]; then
  echo "Could not install some of the packages. Will not proceed."
  exit 1
fi
