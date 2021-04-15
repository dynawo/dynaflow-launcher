#!/bin/bash
#
# Copyright (c) 2021, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

set_commit_hook() {
  hook_file_msg='#!'"/bin/bash
$DFL_HOME/scripts/hooks/commit_hook.sh"' $1'
  if [ -f "$DFL_HOME/.git/hooks/commit-msg" ]; then
    current_file=$(cat $DFL_HOME/.git/hooks/commit-msg)
    if [ "$hook_file_msg" != "$current_file" ]; then
      echo "$hook_file_msg" > $DFL_HOME/.git/hooks/commit-msg || error_exit "You need to set commit-msg in .git/hooks."
    fi
    if [ ! -x "$DFL_HOME/.git/hooks/commit-msg" ]; then
      chmod +x $DFL_HOME/.git/hooks/commit-msg || error_exit "commit-msg in .git/hooks needs to be executable."
    fi
  else
    if [ -d "$DFL_HOME/.git" ]; then
      echo "$hook_file_msg" > $DFL_HOME/.git/hooks/commit-msg || error_exit "You need to set commit-msg in .git/hooks."
      chmod +x $DFL_HOME/.git/hooks/commit-msg || error_exit "commit-msg in .git/hooks needs to be executable."
    fi
  fi

  hook_file_master='#!'"/bin/bash
# Avoid committing in master
branch=\"\$(git rev-parse --abbrev-ref HEAD)\"
if [ \"\$branch\" = \"master\" ]; then
  echo \"You can't commit directly to master branch.\"
  exit 1
fi
whitespace_present=no
for file in \$(git diff-index --name-status --cached HEAD | grep -v \"^D\" | grep -v \".*.patch\" | grep -v \".*.png\" | grep -v \"ModelicaCompiler/test\" | grep -v \"reference\" | cut -c3-); do
  if [ ! -z \"\$(git grep --cached \"^[[:space:]]\+\$\" \$file)\" ]; then
    sed -i 's/^[[:space:]]*\$//' \$file
    whitespace_present=yes
  fi
  if [ ! -z \"\$(git grep --cached \"[[:space:]]\+\$\" \$file)\" ]; then
    sed -i 's/[[:space:]]*\$//' \$file
    whitespace_present=yes
  fi
  if [ -n \"\$(git show :\$file | tail -c1)\" ]; then
    if [ -n \"\$(tail -c1 \$file)\" ]; then
      echo >> \$file
    fi
    whitespace_present=yes
  fi
done
tab_present=no
files=()
for file in \$(git diff-index --name-status --cached HEAD | grep -v \"^D\" | grep -v \".*.patch\" | grep -v \".*.png\" | grep -v \"Makefile\" | grep -v \"ModelicaCompiler/test\" | grep -v \"reference\" | cut -c3-); do
  if [ ! -z \"\$(git grep --cached \"$(printf '\t')\" \$file)\" ]; then
    tab_present=yes
    files=(\${files[@]} \$file)
  fi
done
if [ \"\$whitespace_present\" = \"yes\" ]; then
  echo \"Whitespace problems has been corrected on your files. You need to add them to the index and relaunch commit again.\"
  echo
fi
if [ \"\$tab_present\" = \"yes\" ]; then
  echo \"Following files contain tab character, you need to replace them by spaces:\"
  for file in \"\${files[@]}\"; do
    echo \$file
  done
  echo
fi
if [[ \"\$whitespace_present\" == \"yes\" || \"\$tab_present\" == \"yes\" ]]; then
  exit 1
fi
git diff-index --check --cached HEAD -- ':(exclude)*/reference/*' ':(exclude)*.patch'"
  if [ -f "$DFL_HOME/.git/hooks/pre-commit" ]; then
    current_file=$(cat $DFL_HOME/.git/hooks/pre-commit)
    if [ "$hook_file_master" != "$current_file" ]; then
      echo "$hook_file_master" > $DFL_HOME/.git/hooks/pre-commit
    fi
    if [ ! -x "$DFL_HOME/.git/hooks/pre-commit" ]; then
      chmod +x $DFL_HOME/.git/hooks/pre-commit
    fi
  else
    if [ -d "$DFL_HOME/.git" ]; then
      echo "$hook_file_master" > $DFL_HOME/.git/hooks/pre-commit
      chmod +x $DFL_HOME/.git/hooks/pre-commit
    fi
  fi

  if [ -e "$DFL_HOME/.git" ]; then
    if [ -z "$(git config --get core.commentchar 2> /dev/null)" ] || [ $(git config --get core.commentchar 2> /dev/null) = "#" ]; then
      git config core.commentchar % || error_exit "You need to change git config commentchar from # to %."
    fi
  fi
}

#################################
########### Main script #########
#################################

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export DFL_HOME=$HERE/..
set_commit_hook
