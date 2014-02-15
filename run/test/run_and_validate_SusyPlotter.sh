#!/bin/sh

if [ $# -eq 0 ]
then
    echo "Specify either ref or new"
fi
MODE=$1

REF_ROO=/tmp/gerbaudo/WZ_SusyPlot_ref.root
REF_LOG=/tmp/gerbaudo/WZ_SusyPlot_ref.log
NEW_ROO=/tmp/gerbaudo/WZ_SusyPlot_new.root
NEW_LOG=/tmp/gerbaudo/WZ_SusyPlot_new.log

if [ "$MODE" == "ref" ]
then
    echo "---running ref---"
    SusyPlot \
        -n 300000 \
        -i filelist/Sherpa_CT10_lllnu_WZ.txt \
        -o ${REF_ROO} \
        -s Sherpa_CT10_lllnu_WZ  \
        >& ${REF_LOG}
    ./python/cat_ROOT_file.py ${REF_ROO} >& ${REF_ROO}.txt 
fi

if [ "$MODE" == "new" ]
then
    echo "---running new---"
    SusyPlot \
        -n 300000 \
        -i filelist/Sherpa_CT10_lllnu_WZ.txt \
        -o ${NEW_ROO} \
        -s Sherpa_CT10_lllnu_WZ  \
        >& ${NEW_LOG}
    ./python/cat_ROOT_file.py ${NEW_ROO} >& ${NEW_ROO}.txt     
    echo "---diff log files---"
    diff ${REF_LOG} ${NEW_LOG}    
    echo "---diff root files---"
    ./python/diff_ROOT_files.py ${REF_ROO} ${NEW_ROO}
    echo "diff ${REF_ROO}.txt ${NEW_ROO}.txt"
    diff ${REF_ROO}.txt ${NEW_ROO}.txt
fi

echo "---done---"

