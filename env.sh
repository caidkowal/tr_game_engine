#!/bin/bash
# Source this file to set up the environment

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export TREASURE_RUNNER_ASSETS="${REPO_ROOT}/assets"
export LD_LIBRARY_PATH="${REPO_ROOT}/dist${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export PYTHONPATH="${REPO_ROOT}/python${PYTHONPATH:+:${PYTHONPATH}}"
