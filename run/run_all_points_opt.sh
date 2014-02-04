

# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_1_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_2_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_3_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_4_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_5_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_6_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_7_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_8_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_9_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_10_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_11_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_12_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_13_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_14_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_15_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_16_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_17_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_18_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_19_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_20_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_21_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_22_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_23_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_25_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_26_Jan_11.root
# out/susysel/Herwigpp_sM_wA_noslep_notauhad_WH_2Lep_27_Jan_11.root
# 

for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 25 26 27 
do
  mkdir -p plots_opt_take1_${i}
done

time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_1/summary.txt   --plotdir plots_opt_take1_1  --signal-regexp 'WH_2Lep_1$'   | tee plots_opt_take1_1/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_2/summary.txt   --plotdir plots_opt_take1_2  --signal-regexp 'WH_2Lep_2$'   | tee plots_opt_take1_2/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_3/summary.txt   --plotdir plots_opt_take1_3  --signal-regexp 'WH_2Lep_3$'   | tee plots_opt_take1_3/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_4/summary.txt   --plotdir plots_opt_take1_4  --signal-regexp 'WH_2Lep_4$'   | tee plots_opt_take1_4/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_5/summary.txt   --plotdir plots_opt_take1_5  --signal-regexp 'WH_2Lep_5$'   | tee plots_opt_take1_5/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_6/summary.txt   --plotdir plots_opt_take1_6  --signal-regexp 'WH_2Lep_6$'   | tee plots_opt_take1_6/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_7/summary.txt   --plotdir plots_opt_take1_7  --signal-regexp 'WH_2Lep_7$'   | tee plots_opt_take1_7/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_8/summary.txt   --plotdir plots_opt_take1_8  --signal-regexp 'WH_2Lep_8$'   | tee plots_opt_take1_8/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_9/summary.txt   --plotdir plots_opt_take1_9  --signal-regexp 'WH_2Lep_9$'   | tee plots_opt_take1_9/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_10/summary.txt  --plotdir plots_opt_take1_10 --signal-regexp 'WH_2Lep_10$'  | tee plots_opt_take1_10/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_11/summary.txt  --plotdir plots_opt_take1_11 --signal-regexp 'WH_2Lep_11$'  | tee plots_opt_take1_11/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_12/summary.txt  --plotdir plots_opt_take1_12 --signal-regexp 'WH_2Lep_12$'  | tee plots_opt_take1_12/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_13/summary.txt  --plotdir plots_opt_take1_13 --signal-regexp 'WH_2Lep_13$'  | tee plots_opt_take1_13/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_14/summary.txt  --plotdir plots_opt_take1_14 --signal-regexp 'WH_2Lep_14$'  | tee plots_opt_take1_14/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_15/summary.txt  --plotdir plots_opt_take1_15 --signal-regexp 'WH_2Lep_15$'  | tee plots_opt_take1_15/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_16/summary.txt  --plotdir plots_opt_take1_16 --signal-regexp 'WH_2Lep_16$'  | tee plots_opt_take1_16/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_17/summary.txt  --plotdir plots_opt_take1_17 --signal-regexp 'WH_2Lep_17$'  | tee plots_opt_take1_17/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_18/summary.txt  --plotdir plots_opt_take1_18 --signal-regexp 'WH_2Lep_18$'  | tee plots_opt_take1_18/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_19/summary.txt  --plotdir plots_opt_take1_19 --signal-regexp 'WH_2Lep_19$'  | tee plots_opt_take1_19/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_20/summary.txt  --plotdir plots_opt_take1_20 --signal-regexp 'WH_2Lep_20$'  | tee plots_opt_take1_20/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_21/summary.txt  --plotdir plots_opt_take1_21 --signal-regexp 'WH_2Lep_21$'  | tee plots_opt_take1_21/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_22/summary.txt  --plotdir plots_opt_take1_22 --signal-regexp 'WH_2Lep_22$'  | tee plots_opt_take1_22/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_23/summary.txt  --plotdir plots_opt_take1_23 --signal-regexp 'WH_2Lep_23$'  | tee plots_opt_take1_23/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_25/summary.txt  --plotdir plots_opt_take1_25 --signal-regexp 'WH_2Lep_25$'  | tee plots_opt_take1_25/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_26/summary.txt  --plotdir plots_opt_take1_26 --signal-regexp 'WH_2Lep_26$'  | tee plots_opt_take1_26/log.txt
time python/optimizeSelection.py -v -d out/susysel/merged/ --summary plots_opt_take1_27/summary.txt  --plotdir plots_opt_take1_27 --signal-regexp 'WH_2Lep_27$'  | tee plots_opt_take1_27/log.txt

