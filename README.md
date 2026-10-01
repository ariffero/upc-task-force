# UPC task force - Vertexing efficiency
This repository contains the macros use to process the data for the study of the vertexing efficiency in UPC. It contains both the configurables and scripts needed to run the O2Physics task locally, and some macros for post processing.

## Content
### O2Physics task
- `run_upc_rho.sh` and `run_upc_jpsi`: script used to run the task `upcTrackVertexingQA.cxx`. They require a configurables, called `config_upc_rho.json` and `config_upc_jpsi.json`, respectively.
- `config_upc.json` configurable needed to run the task `upcTrackVertexingQA.cxx`. It allows to set some limits for some kinematic variables related to the pair or to the tracks. It has few process functions: more process function can be enabled at the same time, but not if they refer to different particles (i.e. $J/\psi$ and $\rho^0$) or if they can run only on data or both data and MC. Process functions availble:
  1. `processRhoCand`: process data asking for a collision before looping on tracks, fill histograms considering the process $\rho^0 \rightarrow \pi\pi$;
  2. `processJpsiCand`: process data asking for a collision before looping on tracks, fill histograms considering the process $J/\psi \rightarrow \mu\mu$;
  3. `processRhoCandMcInfo`: same as `processRhoCand`, but add some info on gen MC;
  4. `processJpsiCandMcInfo`, same as `processJpsiCand`, but add some info on gen MC;
  5. `processRhoTracksBeforeGrouping`: process tracks from $\rho^0 \rightarrow \pi\pi$ without requiring them to be matched to a collision;
  6. `processJpsiTracksBeforeGrouping`: process tracks from $J/\psi \rightarrow \mu\mu$ without requiring them to be matched to a collision;
  7. `processRhoTracksBeforeGroupingMcInfo`: same as `processRhoTracksBeforeGrouping` but add some info on gen MC;
  8. `processJpsiTracksBeforeGroupingMcInfo`: same as `processJpsiTracksBeforeGrouping` but add some info on gen MC;
  9. `processMcGenRho`: process generated MC info (both collisions and tracks) from $\rho^0 \rightarrow \pi\pi$;
  10. `processMcGenJpsi`: process generated MC info (both collisions and tracks) from $J/\psi \rightarrow \mu\mu$

### Post processing
- `compareHisto.cpp`: macro used to compare the results from the `upcTrackVertexingQA.cxx`, considering all tracks or only the one surviving after the match with a collision. It has three arguments, the first refers to the particle and the type of comparison, and can be `"rhoAss"` or `"jpsiAss"`, to compare all tracks with the ones associated to a collision, or `"rhoMC"` or `"jpsiMC"`, for comparing gen and reco distributions. The second is a boolean that needs to be set on `true` to normalize the distributions, otherwise they will be plotted with their own normalization. The third is a bool set to `true` if we are analyzing MC.