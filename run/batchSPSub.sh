#!/bin/bash

#PBS -q atlas
#PBS -l nodes=1:ppn=1

cd $PBS_O_WORKDIR

#run job
SusyPlot -f filelist/${inp}.txt -s ${out} ${opt} > batchLog/${out}.susyplot.log
#SusySel -f filelist/${inp}.txt -s ${out} ${opt} > batchLog/${out}.susyplot.log
