//=======================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                           //
// file : utilities.cp                                                   //
// description: contains functions needed for printing and visualization //
//=======================================================================//

#include "../lib/GRAMM.h"
#include "../lib/drc.h"
#include "../lib/utilities.h"

//------------ The following sections is for DRC Rules check for Device Model Graph -----------//
void deviceModelDRC_VerifyPinNodes(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected){
  // Iterate over all the nodes of the device model graph and assign the type of nodes:
  for (int i = 0; i < num_vertices(*G); i++){
    //Checking if all of the inputPins have a fanout of 1 and connected to a FuncCell
    out_edge_iterator eo, eo_end;
    boost::tie(eo, eo_end) = out_edges(i, *G);
    for (; eo != eo_end; eo++){
      //Check if the node in the device model graph is an input PinCell
      if (((*gConfig)[i].Cell == "PINCELL") && (*gConfig)[i].Type == "IN"){
        //Verifying that all input PinCell node has a out degree of 1
        if (boost::out_degree(i, *G) != 1){
          GRAMM->error("[DRC Error] {} input pin node can not have a fanout", gNames[i]);
          *errorDetected  = true;
        } 
        //Verifying that all input PinCell fanout edge is connected to a funcCell
        if (!((*gConfig)[boost::target(*eo, *G)].Cell == "FUNCCELL")){
          GRAMM->error("[DRC Error] {} input pin node fanout is not connected to a FuncCell", gNames[i]);
          *errorDetected  = true;
        } 
      }
    }
    //Checking if all of the outputPins have a fanin of 1 and connected to a FuncCell
    in_edge_iterator ei, ei_end;
    boost::tie(ei, ei_end) = in_edges(i, *G);
    for (; ei != ei_end; ei++){
      //Check if the node in the device model graph is an output PinCell
      if (((*gConfig)[i].Cell == "PINCELL") && (*gConfig)[i].Type == "OUT"){
        //Verifying that all output PinCell node has a in degree of 1
        if (boost::in_degree(i, *G) != 1){
          GRAMM->error("[DRC Error] {} output pin node can not have a fanin", gNames[i]);
          *errorDetected  = true;
        }
        //Verifying that all output PinCell fanin edge is connected to a funcCell
        if (!((*gConfig)[boost::source(*ei, *G)].Cell == "FUNCCELL")){
          GRAMM->error("[DRC Error] {} output pin node fanin is not connected to a FuncCell", gNames[i]);
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
        GRAMM->error("[DRC Error] {} node is floating in the device model graph and is not connected to any other nodes", gNames[i]);
        *errorDetected  = true;
      } 
  }
}


void deviceModelDRC_CheckDeviceModelWeaklyConnected(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected){

  UnDirectedGraph G_undirected;
  boost::copy_graph(*G, G_undirected);

  // Vector to store the component number of each vertex 
  std::vector<int> component(boost::num_vertices(G_undirected));

  // Get the number of sub-graphs in the device model graph. If the number of sub-graph is more that 1,
  // then the graph is disconnected. 
  int num_components = boost::connected_components(G_undirected, &component[0]);

  if (num_components > 1) {
        GRAMM->error("[DRC Error] Device model graph is disconnect. There is {} numbers of differents independant graph within the provided Device Model", num_components);
        *errorDetected  = true;
  }
}

void deviceModelDRC_CheckFuncCellConnectivity(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected){

  // Get the weight map for modifying weights
  auto weight_map = boost::get(&EdgeProperty::weight, *G);

  // Iterate over the edges and set all weights to 1
  edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = boost::edges(*G); ei != ei_end; ++ei) {
      boost::put(weight_map, *ei, 1); // Set each edge's weight to 1
  }


  vertex_iterator vi, vi_end;
  std::vector<vertex_descriptor> funcCells_list;

  // Find all FuncCell nodes
  for (boost::tie(vi, vi_end) = vertices(*G); vi != vi_end; ++vi) {
      if ((*gConfig)[*vi].Cell == "FUNCCELL") {
          funcCells_list.push_back(*vi);
      }
  }

  // Iterate over each FuncCell node to check connectivity with others
  for (vertex_descriptor source_vertex : funcCells_list) {
    std::vector<int> distances(boost::num_vertices(*G), std::numeric_limits<int>::max());
    std::vector<int> predecessors(boost::num_vertices(*G), -1);

    // Run Dijkstra's algorithm with default edge weights (all edges treated equally)
    boost::dijkstra_shortest_paths(*G, source_vertex,
        boost::distance_map(boost::make_iterator_property_map(distances.begin(), boost::get(boost::vertex_index, *G)))
        .predecessor_map(boost::make_iterator_property_map(predecessors.begin(), boost::get(boost::vertex_index, *G)))
        .weight_map(weight_map));

    // Check if all other FuncCell nodes are reachable
    for (vertex_descriptor target_vertex : funcCells_list) {
        if (target_vertex != source_vertex && distances[target_vertex] == std::numeric_limits<int>::max()) {
            GRAMM->warn("[DRC Warning] Device model graph is disconnect. There is no routable path from FuncCell {} to FuncCell {}", gNames[source_vertex], gNames[target_vertex]);
        }
    }
  }
}

void deviceModelDRC_CheckDeviceModelAttributes(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected){
  for (int i = 0; i < num_vertices(*G); i++){
    vertex_descriptor v = vertex(i, *G);

    //Check if the G_Name attribute in the device model graph
    std::string G_Name = boost::get(&DotVertex::G_Name, *G, v);
    if (G_Name.empty()){
      GRAMM->error("[DRC Error] Device model graph have verticies that does not have a G_Name attribute");
      *errorDetected  = true;
    }


    //Check if the G_NodeType attribute in the device model graph
    std::string G_NodeCell = boost::get(&DotVertex::G_NodeType, *G, v);
    if (G_NodeCell.empty()){
      GRAMM->error("[DRC Error] Vertex (G_Name) {} in device model graph does not have a G_NodeCell attribute", G_Name);
      *errorDetected  = true;
    }


    //Check if the G_NodeType attribute in the device model graph.
    std::string G_NodeType = boost::get(&DotVertex::G_NodeType, *G, v);
    if (G_NodeType.empty()){
      GRAMM->error("[DRC Error] Vertex (G_Name) {} in device model graph does not have a G_NodeType attribute", G_Name);
      *errorDetected  = true;
    }

    //Check if the G_VisualX attribute in the device model graph.
    std::string G_VisualX = boost::get(&DotVertex::G_VisualX, *G, v);
    if (G_VisualX.empty()){
      GRAMM->warn("[DRC Warning] Vertex (G_Name) {} in device model graph does not have an optional G_VisualX attribute", G_Name);
    }

    //Check if the G_VisualY attribute in the device model graph.
    std::string G_VisualY = boost::get(&DotVertex::G_VisualY, *G, v);
    if (G_VisualY.empty()){
      GRAMM->warn("[DRC Warning] Vertex (G_Name) {} in device model graph does not have an optional G_VisualY attribute", G_Name);
    }

  }
}


//------------ The following sections is for DRC Rules check for Application Model Graph -----------//
void applicationGraphDRC_CheckFloatingNodes(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected){
  // Iterate over all the nodes of the device model graph and assign the type of nodes:
  for (int i = 0; i < num_vertices(*H); i++){
    // Check for floating nodes as it will have no output and input edges
    if (boost::in_degree(i, *H) == 0 && boost::out_degree(i, *H) == 0 && hNames[i] != "NULL"){
      GRAMM->error("[DRC Error] {} node is floating in the application DFG and is not connected to any other nodes", hNames[i]);
      *errorDetected  = true;
    } 
  }
}


void applicationGraphDRC_CheckPinNames(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected){
  // Iterate over all the nodes of the device model graph and assign the type of nodes:
  for (int i = 0; i < num_vertices(*H); i++){
    vertex_descriptor v = vertex(i, *H);

    //Get the output edges for the application DFG nodes
    out_edge_iterator eo, eo_end;
    boost::tie(eo, eo_end) = out_edges(v, *H);
    for (; eo != eo_end; eo++){

      auto it_inPin = std::find(inPin.begin(), inPin.end(), boost::get(&EdgeProperty::H_LoadPin, *H, *eo));
      if (it_inPin == inPin.end()){
        GRAMM->error("[DRC Error] load pin attribute {} for edge {} -> {} is not defined in inPin vector seen in GRAMM.cpp", boost::get(&EdgeProperty::H_LoadPin, *H, *eo), hNames[boost::source(*eo, *H)], hNames[boost::target(*eo, *H)]);
        *errorDetected  = true;
      }

      auto it_outPin = std::find(outPin.begin(), outPin.end(), boost::get(&EdgeProperty::H_DriverPin, *H, *eo));
      if (it_outPin == outPin.end()){
        GRAMM->error("[DRC Error] driver pin attribute {} for edge {} -> {} is not defined in outPin vector seen in GRAMM.cpp", boost::get(&EdgeProperty::H_DriverPin, *H, *eo), hNames[boost::source(*eo, *H)], hNames[boost::target(*eo, *H)]);
        *errorDetected  = true;
      }
    }
  }
}


void applicationGraphDRC_CheckApplicationDFGWeaklyConnected(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected){

  UnDirectedGraph H_undirected;
  boost::copy_graph(*H, H_undirected);


  // Vector to store the component number of each vertex 
  std::vector<int> component(boost::num_vertices(H_undirected));

  // Get the number of sub-graphs in the device model graph. If the number of sub-graph is more that 1,
  // then the graph is disconnected. 
  int num_components = boost::connected_components(H_undirected, &component[0]);

  if (num_components > 1) {
        GRAMM->error("[DRC Error] Application DFG is disconnect. There is {} numbers of differents independant graph within the provided DFG", num_components);
        *errorDetected  = true;
  }
}

void applicationGraphDRC_CheckDeviceModelAttributes(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected){
  for (int i = 0; i < num_vertices(*H); i++){
    vertex_descriptor v = vertex(i, *H);


    //Check if the name attribute in the application DFG
    std::string H_Name = boost::get(&DotVertex::H_Name, *H, v);
    if (H_Name.empty()){
      GRAMM->error("[DRC Error] Vertex {} in application DFG does not have a H_Name attribute", H_Name);
      *errorDetected  = true;
    }

    //Check if the opcode attribute in the application DFG
    std::string H_Opcode = boost::get(&DotVertex::H_Opcode, *H, v);
    if (H_Opcode.empty()){
      GRAMM->error("[DRC Error] Vertex {} in application DFG does not have a opcode attribute", H_Name);
      *errorDetected  = true;
    }

    //Check if the loadPin and driverPin attribute is in the application DFG edges
    out_edge_iterator eo, eo_end;
    boost::tie(eo, eo_end) = out_edges(v, *H);
    for (; eo != eo_end; eo++){
      //Check if the loadPin attribute is in the application DFG edges
      std::string H_LoadPin = boost::get(&EdgeProperty::H_LoadPin, *H, *eo);
      if (H_LoadPin.empty()){
        GRAMM->error("[DRC Error] Edge {} -> {} in application DFG does not have a loadPin attribute", boost::get(&DotVertex::H_Name, *H, boost::source(*eo, *H)), boost::get(&DotVertex::H_Name, *H, boost::target(*eo, *H)));
        *errorDetected  = true;
      }

      //Check if the driverPin attribute is in the application DFG edges
      std::string H_DriverPin = boost::get(&EdgeProperty::H_DriverPin, *H, *eo);
      if (H_DriverPin.empty()){
        GRAMM->error("[DRC Error] Edge {} -> {} in application DFG does not have a driverPin attribute", boost::get(&DotVertex::H_Name, *H, boost::source(*eo, *H)), boost::get(&DotVertex::H_Name, *H, boost::target(*eo, *H)));
        *errorDetected  = true;
      }
    }

    //Check if the latency attribute in the application DFG.
    std::string H_Latency = boost::get(&DotVertex::H_Latency, *H, v);
    if (H_Latency.empty()){
      GRAMM->warn("[DRC Warning] Vertex {} in application DFG does not have an optional latency attribute", H_Name);
    }

    //Check if the placementX attribute in the application DFG.
    std::string H_PlacementX = boost::get(&DotVertex::H_PlacementX, *H, v);
    if (H_PlacementX.empty()){
      GRAMM->warn("[DRC Warning] Vertex {} in application DFG does not have an optional placementX attribute", H_Name);
    }

    //Check if the placementY attribute in the application DFG.
    std::string H_PlacementY = boost::get(&DotVertex::H_PlacementY, *H, v);
    if (H_PlacementY.empty()){
      GRAMM->warn("[DRC Warning] Vertex {} in application DFG does not have an optional placementY attribute", H_Name);
    }

  }
}

//------------ The following sections is the functions that runs all DRC rules -----------//
double runDRC(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig){
  
  //--------------- Starting timestamp -------------------------//
  struct timeval drcTime;
  gettimeofday(&drcTime, NULL);
  uint64_t startDRC = drcTime.tv_sec * (uint64_t)1000000 + drcTime.tv_usec;
  
  //-------------- Run all the DRC rule check function, and if an error is detected raise the errorDetected flag ---------//
  bool errorDetected = false;


  GRAMM->info("--------------------------------------------------");
  GRAMM->info("Executing DRC Rules Check");
  GRAMM->info("--------------------------------------------------");

  //--------- Running DRC for the Device Model Graph -----------//
  //------ Please add any Device Model Graph DRC Rule Check Functions Below ------//
  deviceModelDRC_VerifyPinNodes(G, gConfig, &errorDetected);
  deviceModelDRC_CheckFloatingNodes(G, gConfig, &errorDetected);
  deviceModelDRC_CheckDeviceModelWeaklyConnected(G, gConfig, &errorDetected);
  deviceModelDRC_CheckFuncCellConnectivity(G, gConfig, &errorDetected);
  deviceModelDRC_CheckDeviceModelAttributes(G, gConfig, &errorDetected);

  //--------- Running DRC for the Application Model Graph -----------//
  //------ Please add any Application Model Graph DRC Rule Check Functions Below ------//
  applicationGraphDRC_CheckFloatingNodes(H, hConfig, &errorDetected);
  applicationGraphDRC_CheckPinNames(H, hConfig, &errorDetected);
  //applicationGraphDRC_CheckApplicationDFGWeaklyConnected(H, hConfig, &errorDetected);
  applicationGraphDRC_CheckDeviceModelAttributes(H, hConfig, &errorDetected);

  //--------------- get elapsed time -------------------------//
  gettimeofday(&drcTime, NULL);
  uint64_t endDRC = drcTime.tv_sec * (uint64_t)1000000 + drcTime.tv_usec;
  uint64_t elapsedDRC = endDRC - startDRC;
  double secondsDRC = static_cast<double>(elapsedDRC) / 1000000.0;

  //--------- Error Check -----------//
  if (errorDetected){
    GRAMM->info("--------------------------------------------------");
    GRAMM->info ("DRC Error Detected --- Please fix all the errors above");
    GRAMM->info("--------------------------------------------------");
    GRAMM->info("Total time taken for [DRC] :: {} Seconds", secondsDRC);
    exit(-1);
  } else {
    GRAMM->info("--------------------------------------------------");
    GRAMM->info ("DRC Passed --- Continueing to GRAMM Mapping");
    GRAMM->info("--------------------------------------------------");
  }
  return secondsDRC;
}