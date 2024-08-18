#ifndef UTILITIES_H
#define UTILITIES_H

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

extern  std::vector<std::string> colors;        //Color code for different FunCell nodes
extern  std::string input_pin_color;            //Pre-defined color-code for the input-pin cell
extern  std::string output_pin_color;           //Pre-defined color-code for the output-pin cell
extern  std::string unused_cell_color;          //Pre-defined color-code for the unused cell
extern  std::map<std::string, std::string> funCellMapping;  //Key-> Device-model node-name :: Value-> Mapped application name.

/**
 * @brief Removes the curly brackets '{' and '}' from the given string. 
 * The kernels present in CGRA-ME have {} included in the node names, which hinders dot visualization.
 * 
 * @param input The input string from which curly brackets should be removed.
 * @return A new string with the curly brackets removed. If no curly brackets 
 * are found, the original string is returned unchanged.
 */
std::string removeCurlyBrackets(const std::string& input);

/**
 * @brief Changes the delimiters in the given string from "." to "_" (neato dot format does not support "." delimiter in the node name)
 * 
 * @param gNames The input string to modify.
 * @return A new string with updated delimiters.
 */
std::string gNames_deliemter_changes(std::string gNames);

/**
 * @brief Removes a specified substring from the original string.
 * 
 * @param original_string The original string.
 * @param toRemove The substring to remove.
 * @return A new string with the substring removed.
 */
std::string string_remover(std::string original_string, std::string toRemove);

/**
 * @brief Prints routing information to a mapping-output file.
 * 
 * @param gNumber A boost node id from the device-model graph.
 * @param y A boost node id from the application graph.
 * @param positionedOutputFile The positioned-output dot file stream (this dot-file contains actual co-ordinates of the node cells).
 * @param unpositionedOutputFile The unpositioned-output dot file stream (this dot-file does not contain any co-ordinates of the node cells).
 */
void printRoutingResults(int y, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile);

/**
 * @brief Prints placement information to a mapping-output file.
 *
 * @param gNumber A boost node id from the device-model graph.
 * @param gName An integer used in the output.
 * @param positionedOutputFile The positioned-output dot file stream (this dot-file contains actual co-ordinates of the node cells).
 * @param unpositionedOutputFile The unpositioned-output dot file stream (this dot-file does not contain any co-ordinates of the node cells).
 * @param hConfig A map containing node configuration details of device-model graph.
 */
void printPlacementResults(int gNumber, std::string gName, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile, std::map<int, NodeConfig> *gConfig);

/**
 * @brief Prints mapping results in neato format: First displays the layout and then shows connections between the nodes.
 * 
 * @param H A pointer to the application graph.
 * @param G A pointer to the device-model graph.
 * @param hConfig A map containing node configuration details of application graph.
 * @param hConfig A map containing node configuration details of device-model graph.
 */
void printMappedResults(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig , std::map<int, NodeConfig> *gConfig);

/**
 * @brief Prints vertex models of the application graph's nodes.
 * 
 * @param H A pointer to the application graph.
 * @param G A pointer to the device-model graph.
 */
void printVertexModels(DirectedGraph *H, DirectedGraph *G);

/**
 * @brief Prints the device model cell name corresponding to a given boost id number from device model graph.
 * 
 * @param n The boost integer id.
 */
void printName(int n);

#endif