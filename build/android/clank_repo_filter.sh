#!/bin/bash

# This script is intended to be run by repo forall like:
#   repo forall -c clank_repo_filter.sh
# For each repository, if it is a clank-specific repository then
# it prints out the project name.
if [ "$REPO_REMOTE" == "clank" ]; then
  echo "$REPO_PROJECT"
fi
