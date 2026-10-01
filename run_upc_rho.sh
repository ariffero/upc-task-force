#!/bin/bash

OPTIONS=(
  --configuration json://config_upc_rho.json
  --shm-segment-size 10000000000
  -b
  --readers 4
  --time-limit 500
  --aod-file /data/ariffero/upc-task-force/kCohRhoToPiWithCont/AO2D.root
)

echo "options: ${OPTIONS[@]}"

o2-analysis-ud-upc-track-vertexing-qa ${OPTIONS[@]} | \
o2-analysis-event-selection-service ${OPTIONS[@]} | \
o2-analysis-trackselection ${OPTIONS[@]} | \
o2-analysis-pid-tpc-service ${OPTIONS[@]} | \
o2-analysis-propagationservice ${OPTIONS[@]} --fairmq-ipc-prefix .

mv AnalysisResults.root AnalysisResults_rho_mc.root

rm localhost*
rm core_dump*
