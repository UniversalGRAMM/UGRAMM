#ifndef DRC_H
#define DRC_H

#include <boost/config.hpp>
#include <iostream>
#include <fstream>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>
#include <queue>
#include <map>
#include <list>
#include <bitset>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/copy.hpp>



//------------ The following sections is for DRC Rules check for Device Model Graph -----------//
/**
 * @brief Input Pins in the Device Model Graph must have one fanout edges that is connected to the PE
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_VerifyPinNodeFanOut(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);

/**
 * @brief Output Pins in the Device Model Graph must have one fanin edges that is connected to its neighnor node. In Riken aarchitecture, output pins are connected to either PE or constant node.
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_VerifyPinNodeFanIn(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


/**
 * @brief All functional cell, regardless of their operand, must be atleast connected to a input or output pin 
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_CheckPinsExistBeforeAndAfterFuncCell(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


/**
 * @brief Device model graphs should not have any floating nodes
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_CheckFloatingNodes(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


/**
 * @brief  I/O and LS ports must have one input and one output pin
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_CheckPinsIO(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


/**
 * @brief  Check if the device model graph is weekly conencted
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_CheckDeviceModelWeeklyConnected(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


/**
 * @brief  Check if FuncCells in the Device Model Graph is connected to other FuncCell. If any two FuncCell does not have a path, this means that the graph is disjointed.
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_CheckFuncCellConnectivity(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


//------------ The following sections is for DRC Rules check for Application Model Graph -----------//
/**
 * @brief Application graph should not have any floating nodes
 * 
 * @param H A pointer to the device-model graph.
 * @param hConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void applicationGraphDRC_CheckFloatingNodes(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected);


/**
 * @brief Pin names used in the application DFG must follow the names defined in inPin  and outPin vectors in GRAMM.cpp
 * 
 * @param H A pointer to the device-model graph.
 * @param hConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void applicationGraphDRC_CheckPinNames(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected);


/**
 * @brief Check if the application DFG is weekly conencted
 * 
 * @param H A pointer to the device-model graph.
 * @param hConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void applicationGraphDRC_CheckApplicationDFGWeeklyConnected(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected);




//------------ The following sections is for DRC Rules check for Application Model Graph -----------//
/**
 * @brief Main DRC function that run through all DRC rules check for both H and G graph seen above
 * 
 * @param H A pointer to the application-model graph.
 * @param G A pointer to the device-model graph.
 * @param hConfig A map containing node configuration details of application-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 */
void runDRC(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig);

#endif