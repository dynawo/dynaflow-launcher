#!/bin/bash

# Except where otherwise noted, content in this documentation is Copyright (c)
# 2022, RTE (http://www.rte-france.com) and licensed under a
# CC-BY-4.0 (https://creativecommons.org/licenses/by/4.0/)
# license. All rights reserved.

folders=(introduction installation configuringDynaflowLauncher functionalDoc advancedDoc
licenses/dynawo licenses/dynawo-algorithms licenses/dynaflow-launcher licenses/dynaflow-launcher-documentation licenses/mpich)
pdflatex_options="-halt-on-error -interaction=nonstopmode"

for folder in ${folders[*]}; do
  latex_files=$(find $folder -name "*.tex")
  for file in ${latex_files[*]}; do
    (cd $folder; pdflatex $pdflatex_options $(basename $file))
    if [ ! -z "$(grep bibliography $file)" ]; then
      (cd $folder; bibtex $(basename ${file%.tex}); pdflatex $pdflatex_options $(basename $file); pdflatex $pdflatex_options $(basename $file))
    else
      (cd $folder; pdflatex $pdflatex_options $(basename $file))
    fi
  done
done
