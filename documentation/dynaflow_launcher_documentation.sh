#!/bin/bash

# Except where otherwise noted, content in this documentation is Copyright (c)
# 2022, RTE (http://www.rte-france.com) and licensed under a
# CC-BY-4.0 (https://creativecommons.org/licenses/by/4.0/)
# license. All rights reserved.

mkdir -p dynaflowLauncherDocumentation

folders=(introduction installation configuringDynaflowLauncher functionalDoc advancedDoc)

pdflatex_options="-halt-on-error -interaction=nonstopmode"
output_file=DynaflowLauncherDocumentation.tex

echo '%% Except where otherwise noted, content in this documentation is Copyright (c)
%% 2022, RTE (http://www.rte-france.com) and licensed under a
%% CC-BY-4.0 (https://creativecommons.org/licenses/by/4.0/)
%% license. All rights reserved.

\documentclass[a4paper, 12pt]{report}
\usepackage{etex}

% Latex setup
\input{../latex_setup.tex}
\usepackage{minitoc}
\usepackage[titletoc]{appendix}
\usepackage{pdfpages}

\begin{document}

\title{\huge{Dynaflow-launcher Documentation} \\ \LARGE{v1.4.1} \\
\includegraphics[width=0.5\textwidth]{../resources/dynaflowLauncher.png}}
\date\today

\maketitle
\dominitoc
\tableofcontents

\listoffigures \mtcaddchapter % Avoid problems with minitoc numbering' > dynaflowLauncherDocumentation/$output_file

for folder in ${folders[*]}; do
  latex_files=$(find $folder -name "*.tex")
  for file in ${latex_files[*]}; do
    echo $file
    sed -n '/tableofcontents/,/end{document}/p' $file | tail -n +2 | head -n -1 >> dynaflowLauncherDocumentation/$output_file
  done
done

#sed -i '/chapter{/a\\\minitoc' dynaflowLauncherDocumentation/$output_file
#sed -i '/bibliography{/d' dynaflowLauncherDocumentation/$output_file
#sed -i '/bibliographystyle{/d' dynaflowLauncherDocumentation/$output_file
#sed -i '/vspace{0.6cm} % vspace only for DynawoAlgorithmsInputFiles standalone doc/d' dynaflowLauncherDocumentation/$output_file

#echo "\bibliography{../resources/dynaflowLauncherDocumentation}" >> dynaflowLauncherDocumentation/$output_file
#echo "\bibliographystyle{abbrv}" >> dynaflowLauncherDocumentation/$output_file
#echo "" >> dynaflowLauncherDocumentation/$output_file

echo "\begin{appendices}" >> dynaflowLauncherDocumentation/$output_file

licenses_folders=(licenses/dynawo licenses/dynawo-algorithms licenses/dynaflow-launcher licenses/dynaflow-launcher-documentation licenses/mpich)

# Latex compile
for folder in ${licenses_folders[*]}; do
  latex_files=$(find $folder -name "*.tex")
  for file in ${latex_files[*]}; do
    #echo $(basename $file)
    (cd $folder; sed -i'' '2i\\usepackage{nopageno}\' $(basename $file); pdflatex --jobname=$(basename ${file%.tex})-no-numbering $pdflatex_options $(basename $file))
    sed -i'' '/\usepackage{nopageno}/d' $file
  done
done

license_name=('\Dynawo' '\Dynawo-algorithms' 'Dynaflow-launcher' 'Dynaflow-launcher documentation' 'mpich')

i=0
j=1
for name in "${license_name[@]}"; do
  echo "\chapter[${name/\\/} License]{$name License}" >> dynaflowLauncherDocumentation/$output_file
  if [ ! -z "$(echo "$name" | grep jQuery)" ]; then
    echo "\includepdf[pages=-,pagecommand=\thispagestyle{plain}]{../${licenses_folders[$i]}/license-$j-no-numbering.pdf}" >> dynaflowLauncherDocumentation/$output_file
    if (( $j < 2 )); then
      (( j+=1 ))
    else
      (( i+=1 ))
    fi
  else
    echo "\includepdf[pages=-,pagecommand=\thispagestyle{plain}]{../${licenses_folders[$i]}/license-no-numbering.pdf}" >> dynaflowLauncherDocumentation/$output_file
    (( i+=1 ))
  fi
  echo "" >> dynaflowLauncherDocumentation/$output_file
done

echo "\end{appendices}" >> dynaflowLauncherDocumentation/$output_file

echo "\end{document}" >> dynaflowLauncherDocumentation/$output_file

(cd dynaflowLauncherDocumentation; pdflatex $pdflatex_options $output_file; bibtex ${output_file%.tex}; pdflatex $pdflatex_options $output_file; pdflatex $pdflatex_options $output_file)
