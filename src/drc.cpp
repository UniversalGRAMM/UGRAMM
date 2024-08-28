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
        GRAMM->error("DRC Error - {} input pin node can not have a fanout", gNames[i]);
        *errorDetected  = true;
      } 
    }

  }
}

void deviceModelDRC_VerifyPinNodeFanIn(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected){
  // Iterate over all the nodes of the device model graph and assign the type of nodes:
  for (int i = 0; i < num_vertices(*G); i++){
    //Check if the node in the device model graph is an input PinCell
    if (((*gConfig)[i].type == PinCell) && (*gConfig)[i].opcode == out){
      //Verifying that all input PinCell node has a out degree of 1
      if (boost::in_degree(i, *G) != 1){
        GRAMM->error("DRC Error - {} ouput pin node can not have a fanin", gNames[i]);
        *errorDetected  = true;
      } 
    }
  }
}

void deviceModelDRC_CheckPinsExistBeforeAndAfterFuncCell(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected){
  // Iterate over all the nodes of the device model graph and assign the type of nodes:
  for (int i = 0; i < num_vertices(*G); i++){
    vertex_descriptor v = vertex(i, *G);

    // Obtaining the iterator that will iterator over all the output edges of the node
    out_edge_iterator eo, eo_end;
    boost::tie(eo, eo_end) = out_edges(v, *G);
    for (; eo != eo_end; eo++){
      // Checking if the out edges of the input pin cells are connected to a functional unit
      if ((*gConfig)[boost::source(*eo, *G)].type == PinCell && (*gConfig)[boost::source(*eo, *G)].opcode == in){
        if (!((*gConfig)[boost::target(*eo, *G)].type == FuncCell)){
          GRAMM->error("DRC Error - {} node is not a FuncCell, hence should not be input edge connected from {} input PinCell", gNames[boost::target(*eo, *G)], gNames[boost::source(*eo, *G)]);
          *errorDetected  = true;
        }
      }

      // Checking if the out edges of the functional cells are connected to a output pin cell
      if ((*gConfig)[boost::source(*eo, *G)].type == FuncCell){
        if (!((*gConfig)[boost::target(*eo, *G)].type == PinCell && (*gConfig)[boost::target(*eo, *G)].opcode == out)){
          GRAMM->error("DRC Error - {} node is not a output PinCell, hence should not be input edge connected from {} FuncCell", gNames[boost::target(*eo, *G)], gNames[boost::source(*eo, *G)]);
          *errorDetected  = true;
        }
      }

    }
  }
}

void deviceModelDRC_CheckFloatingNodes(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected){
  // Iterate over all the nodes of the device model graph and assign the type of nodes:
  for (int i = 0; i < num_vertices(*G); i++){
    // Check for floating nodes as it will have no output and input edges
    if (boost::in_degree(i, *G) == 0 && boost::out_degree(i, *G) == 0){
        GRAMM->error("DRC Error - {} node is floating in the device model graph and is not connected", gNames[i]);
        *errorDetected  = true;
      } 
  }
}


void deviceModelDRC_CheckPinsIO(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected){
  // Iterate over all the nodes of the device model graph and assign the type of nodes:
  for (int i = 0; i < num_vertices(*G); i++){
    if (((*gConfig)[i].type == FuncCell) && (*gConfig)[i].opcode == memport){
      // Check for IO or Memport nodes as it will have one input and one output edge
      if (boost::in_degree(i, *G) != 1){
          GRAMM->error("DRC Error - {} IO or Memory port has multiple input edges. These ports can only have one input edge comming from an input pin ", gNames[i]);
          *errorDetected  = true;
      }
      if (boost::out_degree(i, *G) != 1){
          GRAMM->error("DRC Error - {} IO or Memory port has multiple output edges. These ports can only have one output edge going to an output pin ", gNames[i]);
          *errorDetected  = true;
      }
      // Check to see in the input edge to a IO or Memport node is comming from an input PinCell
      in_edge_iterator ei, ei_end;
      boost::tie(ei, ei_end) = in_edges(i, *G);
      for (; ei != ei_end; ei++){
        if (!((*gConfig)[boost::source(*ei, *G)].type == PinCell && (*gConfig)[boost::source(*ei, *G)].opcode == in)){
          GRAMM->error("DRC Error - {} IO or Memory port is not connected to an input PinCell", gNames[i]);
          *errorDetected  = true;
        }
      }

      // Check to see in the input edge to a IO or Memport node is going to an out PinCell
      out_edge_iterator eo, eo_end;
      boost::tie(eo, eo_end) = out_edges(i, *G);
      for (; eo != eo_end; eo++){
        if (!((*gConfig)[boost::target(*eo, *G)].type == PinCell && (*gConfig)[boost::target(*eo, *G)].opcode == out)){
          GRAMM->error("DRC Error - {} IO or Memory port is not connected to an output PinCell", gNames[i]);
          *errorDetected  = true;
        }
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
  deviceModelDRC_VerifyPinNodeFanIn(G, gConfig, &errorDetected);
  deviceModelDRC_CheckPinsExistBeforeAndAfterFuncCell(G, gConfig, &errorDetected);
  deviceModelDRC_CheckFloatingNodes(G, gConfig, &errorDetected);
  deviceModelDRC_CheckPinsIO(G, gConfig, &errorDetected);

  //--------- Running DRC for the Application Model Graph -----------//
  //------ Please add any Application Model Graph DRC Rule Check Functions Below ------//


  //--------- Error Check -----------//
  if (errorDetected){
    GRAMM->info("--------------------------------------------------");
    GRAMM->info ("DRC Error Detected --- Please fix all the errors above");
    GRAMM->info("--------------------------------------------------");
    //std::cout << "DRC Error Detected - Please fix all the errors above\n";
    exit(-1);
  } else {
    GRAMM->info("--------------------------------------------------");
    GRAMM->info ("DRC Passed --- Continueing to GRAMM Mapping");
    GRAMM->info("--------------------------------------------------");
  }
}