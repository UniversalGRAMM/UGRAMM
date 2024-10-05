//===========================================================================//
// Universal GRAaph Minor Mapping (UGRAMM) method for CGRA                   //
// file : utilities.h                                                        //
// description: contains variables and function declaration used for utility //
//===========================================================================//

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

#define FunCell_Visual_Enable   1
#define PinCell_Visual_Enable   1
#define RouteCell_Visual_Enable 0

extern  std::vector<std::string> colors;        //Color code for different FunCell nodes
extern  std::string input_pin_color;            //Pre-defined color-code for the input-pin cell
extern  std::string output_pin_color;           //Pre-defined color-code for the output-pin cell
extern  std::string unused_cell_color;          //Pre-defined color-code for the unused cell
extern  std::map<std::string, std::string> funCellMapping;  //Key-> Device-model node-name :: Value-> Mapped application name.


//------------------------------------------------------------------------------------//
//------------------- [Utilities] String manipulation operations ---------------------//
//------------------------------------------------------------------------------------//

/**
 * Removes the curly brackets '{' and '}' from the given string. 
 * The kernels present in CGRA-ME have {} included in the node names, which hinders dot visualization.
 * 
 * @param input The input string from which curly brackets should be removed.
 * @return A new string with the curly brackets removed. If no curly brackets 
 * are found, the original string is returned unchanged.
 */
std::string removeCurlyBrackets(const std::string& input);

/**
 * Changes the delimiters in the given string from "." to "_" (neato dot format does not support "." delimiter in the node name)
 * 
 * @param gNames The input string to modify.
 * @return A new string with updated delimiters.
 */
std::string gNames_deliemter_changes(std::string gNames);

/**
 * Removes a specified substring from the original string.
 * 
 * @param original_string The original string.
 * @param toRemove The substring to remove.
 * @return A new string with the substring removed.
 */
std::string string_remover(std::string original_string, std::string toRemove);

/**
 * Converts all keys and values of the parsed JSON object to uppercase for normalization.
 * 
 * @param j Reference to the JSON object to be normalized.
 */
void jsonUppercase(json& j);

//------------------------------------------------------------------------------------//
//---------------------- [Utilities] PRAGMA related functions ------------------------//
//------------------------------------------------------------------------------------//

/**
 * Reads a multi-line comment containing PRAGMA from the given file.
 * 
 * @param inputFile Reference to the input file stream from which to read the comment section.
 * @return std::string Returns the content of the multi-line comment as a string.
 */
std::string readCommentSection(std::ifstream &inputFile);

/**
 * Checks whether placement is required for the given opcode based on the information given in the JSON.
 * In JSON, we can either have opcode("Reg") or nodeType("ALU/Memport") to be skipped.
 * 
 * @param hOpcode The opcode from the application graph to check.
 * @param jsonParsed Reference to the parsed JSON object containing node type information.
 * @return bool Returns true if placement is required, false otherwise.
 */
bool skipPlacement(std::string hOpcode, json& jsonParsed);

/**
 * Checks whether locking is required for the given hNamed based on the information given in the JSON.
 * In JSON, we define the locking code as "hName::gName"
 *
 * @param HID Current HID of the application graph 
 * @param jsonParsed Reference to the parsed JSON object containing node type information.
 * @param jsonLockNode Retrieved full string of the lock node from the JSON file only is a lock exist for a specific node in application graph
 * @return bool Returns true if locking is required, false otherwise.
*/ 
bool getLockPE(int HID, json &jsonParsed, std::string& jsonLockNode);

/**
 * Parses PRAGMA vectors from the comment section of the device model graph.
 * 
 * This function extracts and parses vectors of strings associated with the given keyword 
 * from the comment section of the device model graph, and stores them in the provided 
 * ugrammConfig map.
 * 
 * @param commentSection The comment section containing PRAGMA directives.
 * @param keyword The keyword to search for within the comment section.
 * @param ugrammConfig Reference to the map where parsed vectors of strings will be stored.
 */
void parseVectorofStrings(std::string commentSection, std::string keyword, std::map<std::string, std::vector<std::string>> &ugrammConfig);

/**
 * Validates PRAGMA directives read from the application graph.
 * 
 * @param commentSection The comment section containing PRAGMA directives.
 * @param keyword The keyword to search for within the comment section.
 * @param Type Reference to the vector in which keyword will be searched.
 * @return bool Returns true if the validation is successful, false otherwise.
 */
bool checkVectorofStrings(std::string commentSection, std::string keyword, std::vector<std::string> &Type);

/**
 * Reads, checks, and stores PRAGMA directives from the device model file.
 * 
 * @param deviceModelFile Reference to the input file stream for the device model.
 * @param ugrammConfig Reference to the map where PRAGMA directives will be stored.
 */
void readDeviceModelPragma(std::ifstream &deviceModelFile, std::map<std::string, std::vector<std::string>> &ugrammConfig);

/**
 * Reads, checks, and stores PRAGMA directives from the application graph file.
 * 
 * @param applicationGraphFile Reference to the input file stream for the application graph.
 * @param ugrammConfig Reference to the map where PRAGMA directives will be stored.
 */
void readApplicationGraphPragma(std::ifstream &applicationGraphFile, std::map<std::string, std::vector<std::string>> &ugrammConfig);

/**
 * This function gets the device model node type that is best suited for the application
 * graph opcode
 * 
 * @param hOpcode The opcode required by the application node. [FMUL, FADD, INPUT, OUTPUT]
 * @param nodeType Passes the nodeType string for the device model graph
 * @return bool Returns true if the suitable node type in the device model graph is found, false otherwise.
 */
bool getDeviceModelNodeType(const std::string &hOpcode, std::string &nodeType);


//------------------------------------------------------------------------------------//
//---------------------- [Utilities] Printing and visualization ----------------------//
//------------------------------------------------------------------------------------//

/**
 * Prints routing information to a mapping-output file.
 * 
 * @param gNumber A boost node id from the device-model graph.
 * @param y A boost node id from the application graph.
 * @param positionedOutputFile The positioned-output dot file stream (this dot-file contains actual co-ordinates of the node cells).
 * @param unpositionedOutputFile The unpositioned-output dot file stream (this dot-file does not contain any co-ordinates of the node cells).
 * @param hConfig A map containing node configuration details of device-model graph.
 */
void printRoutingResults(int y, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile, std::map<int, NodeConfig> *hConfig);

/**
 * Connects associated pins to the specified FunCell in the device model graph.
 * 
 * @param gNumber A boost node id from the device-model graph.
 * @param FunCellName The name of the FunCell to connect.
 * @param G Pointer to the directed graph representing the device model.
 * @param positionedOutputFile Output file stream for positioned connections.
 * @param unpositionedOutputFile Output file stream for unpositioned connections.
 */
void mandatoryFunCellConnections(int gNumber, std::string FunCellName, DirectedGraph *G, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile);

/**
 * Prints placement information to a mapping-output file.
 *
 * @param gNumber A boost node id from the device-model graph.
 * @param gName An integer used in the output.
 * @param G A pointer to the device-model graph.
 * @param positionedOutputFile The positioned-output dot file stream (this dot-file contains actual co-ordinates of the node cells).
 * @param unpositionedOutputFile The unpositioned-output dot file stream (this dot-file does not contain any co-ordinates of the node cells).
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param ugrammConfig A map containing parsed pragma and config related information.
 */
void printPlacementResults(int gNumber, std::string gName, DirectedGraph *G, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile, std::map<int, NodeConfig> *gConfig, std::map<std::string, std::vector<std::string>> &ugrammConfig);

/**
 * Prints mapping results in neato format: First displays the layout and then shows connections between the nodes.
 * 
 * @param H A pointer to the application graph.
 * @param G A pointer to the device-model graph.
 * @param hConfig A map containing node configuration details of application graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param ugrammConfig A map containing parsed pragma and config related information.
 */
void printMappedResults(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig , std::map<int, NodeConfig> *gConfig, std::map<std::string, std::vector<std::string>> &ugrammConfig);

/**
 * Prints the device model cell name corresponding to a given boost id number from device model graph.
 * 
 * @param n The boost integer id.
 */
void printName(int n);

/**
 * Prints vertex models of the application graph's nodes.
 * 
 * @param H A pointer to the application graph.
 * @param G A pointer to the device-model graph.
 * @param hConfig A map containing node configuration details of device-model graph.
 * @param ugrammConfig A map containing parsed pragma and config related information.
 * @param invUsers A map of hIDs to mapped gIDs.
 */
void printVertexModels(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<std::string, std::vector<std::string>> &ugrammConfig, std::map<int, int> &invUsers);


/**
 * Prints the routing details for the given signal.
 * 
 * @param signal The signal for which to print routing details.
 */
void printRouting(int signal);

//------------------------------------------------------------------------------------//
//-------------------------------- [Utilities] File read -----------------------------//
//------------------------------------------------------------------------------------//

/**
 * Reads a DOT file and stores attributes into the device model graph.
 * 
 * @param G Pointer to the directed graph representing the device model.
 * @param gConfig Pointer to the map where node attributes will be stored.
 */
void readDeviceModel(DirectedGraph *G, std::map<int, NodeConfig> *gConfig);

/**
 * Reads a DOT file and stores attributes into the application graph.
 * 
 * @param H Pointer to the directed graph representing the application graph.
 * @param hConfig Pointer to the map where node attributes will be stored.
 */
void readApplicationGraph(DirectedGraph *H, std::map<int, NodeConfig> *hConfig);

#endif