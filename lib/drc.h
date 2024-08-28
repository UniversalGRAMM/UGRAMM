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


//------------ Example ----//
// /**
//  * @brief Prints vertex models of the application graph's nodes.
//  * 
//  * @param H A pointer to the application graph.
//  * @param G A pointer to the device-model graph.
//  * @param hConfig A map containing node configuration details of device-model graph.
//  */
// void printVertexModels(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig);

//------------ The following sections is for DRC Rules check for Device Model Graph -----------//
/**
 * @brief Input Pins in the Device Model Graph must have one fanout edges that is connected to the PE
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_VerifyPinNodeFanOut(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


//------------ The following sections is for DRC Rules check for Application Model Graph -----------//


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