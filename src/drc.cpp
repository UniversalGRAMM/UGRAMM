//=======================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                           //
// file : utilities.cp                                                   //
// description: contains functions needed for printing and visualization //
//=======================================================================//

#include "../lib/GRAMM.h"
#include "../lib/drc.h"
#include "../lib/utilities.h"

//------------ The following sections is for DRC Rules check for Device Model Graph -----------//
void deviceModelDRC_VerifyPinNodeFanOut(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected){
  // Iterate over all the nodes of the device model graph and assign the type of nodes:
  for (int i = 0; i < num_vertices(*G); i++){
    //Check if the node in the device model graph is an input PinCell
    if (((*gConfig)[i].type == PinCell) && (*gConfig)[i].opcode == in){
      //Verifying that all input PinCell node has a out degree of 1
      if (boost::out_degree(i, *G) != 1){
        GRAMM->error("DRC Error - {} node can not have a fanout", gNames[i]);
        *errorDetected  = true;
      } else {
        GRAMM->info("DRC Info - {} node has one fanout as required", gNames[i]);
      }
    }

  }
}

//------------ The following sections is for DRC Rules check for Application Model Graph -----------//


//------------ The following sections is for DRC Rules check for Application Model Graph -----------//
void runDRC(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig){
  // Run all the DRC rule check function, and if an error is detected, 
  // raise the errorDetected flag
  bool errorDetected = false;


  GRAMM->info("--------------------------------------------------");
  GRAMM->info("Executing DRC Rules Check");
  GRAMM->info("--------------------------------------------------");

  //--------- Running DRC for the Device Model Graph -----------//
  //------ Please add any Device Model Graph DRC Rule Check Functions Below ------//
  deviceModelDRC_VerifyPinNodeFanOut(G, gConfig, &errorDetected);

  //--------- Running DRC for the Application Model Graph -----------//
  //------ Please add any Application Model Graph DRC Rule Check Functions Below ------//


  //--------- Error Check -----------//
  if (errorDetected){
    GRAMM->error ("DRC Error Detected - Please fix all the errors above");
    //std::cout << "DRC Error Detected - Please fix all the errors above\n";
    exit(-1);
  } else {
    GRAMM->info ("DRC Passed");
  }
}