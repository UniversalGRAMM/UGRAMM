//=======================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                           //
// file : utilities.cp                                                   //
// description: contains functions needed for printing, visualization    //
//              pragma etc.                                              //
//=======================================================================//

#include "../lib/GRAMM.h"
#include "../lib/utilities.h"
#include "../lib/routing.h"

std::vector<std::string> colors = {
    "#FFA500", // Orange (Replaced Light Pink)
    "#FFFFE0", // Light Yellow
    "#D8BFD8", // Thistle (Lighter Purple)
    "#D2B48C", // Tan (Lighter Brown)
    "#AFEEEE", // Light Turquoise
    "#E0FFFF", // Light Cyan
    "#FFDAB9", // Peach Puff (Lighter Orange)
    "#00FF00", // Lime (Replaced Pink)
    "#AFEEEE", // Light Turquoise
    "#DDA0DD"  // Plum (Lighter Indigo)
}; // Color code for different FunCells

std::string input_pin_color = "#ADD8E6";           // Pre-defined color-code for the input-pin cell ( Light Blue)
std::string output_pin_color = "#FFB6C1";          // Pre-defined color-code for the output-pin cell (Lighter Red/Pink)
std::string unused_cell_color = "#A9A9A9";         // Pre-defined color-code for the unused cell
std::map<std::string, std::string> funCellMapping; // Key-> Device-model node-name :: Value-> Mapped application name.

//------------------------------------------------------------------------------------//
//------------------- [Utilities] String manipulation operations ---------------------//
//------------------------------------------------------------------------------------//

/**
 * Removes the curly brackets '{' and '}' from the given string.
 * The kernels present in CGRA-ME have {} included in the node names, which hinders dot visualization.
 */
std::string removeCurlyBrackets(const std::string &input)
{
  std::string result = input;

  // Find and remove the opening curly bracket
  size_t pos = result.find('{');
  if (pos != std::string::npos)
  {
    result.erase(pos, 1);
  }

  // Find and remove the closing curly bracket
  pos = result.find('}');
  if (pos != std::string::npos)
  {
    result.erase(pos, 1);
  }

  return result;
}

/**
 * Changes the delimiters in the given string from "." to "_" (neato dot format does not support "." delimiter in the node name)
 */
std::string gNames_deliemter_changes(std::string gNames)
{
  std::string modified_string;

  // Iterate through each character in the input string
  for (char c : gNames)
  {
    if (c == '.')
    {
      // Replace '.' with '_'
      modified_string += '_';
    }
    else
    {
      // Keep the character as it is
      modified_string += c;
    }
  }

  return modified_string;
}

/**
 * Removes a specified substring from the original string.
 */
std::string string_remover(std::string original_string, std::string toRemove)
{
  std::string modified_string;

  // Getting the position of the sub_string which needs to be removed:
  size_t pos = original_string.find(toRemove);

  if (pos != std::string::npos)
  {
    modified_string = original_string.substr(0, pos);
  }
  else
  {
    modified_string = original_string;
  }

  return modified_string;
}

/**
 * Converts all keys and values of the parsed JSON object to uppercase for normalization.
 */
void jsonUppercase(json &j)
{
  json result;

  for (auto &[key, value] : j.items())
  {
    std::string upperKey = boost::to_upper_copy(key);
    auto upperValue = value;
    json upperArray = json::array();
    if (value.is_array())
    { // Case1: value is an array.
      for (auto &el : upperValue)
      {
        std::string upperValue = el.get<std::string>();
        std::transform(upperValue.begin(), upperValue.end(), upperValue.begin(), ::toupper);
        upperArray.push_back(upperValue);
      }
    }
    result[upperKey] = upperArray;
  }
  j = result;
}

//------------------------------------------------------------------------------------//
//---------------------- [Utilities] PRAGMA related functions ------------------------//
//------------------------------------------------------------------------------------//

/**
 * Reads a multi-line comment containing PRAGMA from the given file.
 */
std::string readCommentSection(std::ifstream &inputFile)
{
  std::string line;
  std::string commentSection = {};
  bool comment_started = false;

  while (std::getline(inputFile, line))
  {
    size_t startPos = line.find("/*");
    if (startPos != std::string::npos)
    {
      comment_started = true;
    }
    else if (comment_started == true)
    {
      size_t endPos = line.find("*/");
      if (endPos != std::string::npos)
      {
        return commentSection;
      }
      else
      {
        std::string upperCaseline = boost::to_upper_copy(line);
        commentSection += upperCaseline + "\n";
      }
    }
  }
  return commentSection;
}

/**
 * Checks whether placement is required for the given opcode based on the information given in the JSON.
 * In JSON, we can either have opcode("Reg") or nodeType("ALU/Memport") to be skipped.
*/ 
bool doPlacement(std::string hOpcode, json &jsonParsed)
{
  if (jsonParsed["SKIP-PLACEMENT"].empty())
  {
    return false;
  }
  else
  {
    // First case: Opcode itself is mentioned in the json file to be skipped.
    if (std::find(jsonParsed["SKIP-PLACEMENT"].begin(), jsonParsed["SKIP-PLACEMENT"].end(), hOpcode) != jsonParsed["SKIP-PLACEMENT"].end())
    { 
      GRAMM->trace(" [Case-1] : Skipping found Opcode {} in JSON",  hOpcode);
      return true;
    }

    // Second case: NodeType to be skipped is given in the JSON file:
    std::vector<std::string> gTypeSupported;

    for (const auto &pair : GrammConfig)
    {
      if (std::find(pair.second.begin(), pair.second.end(), hOpcode) != pair.second.end())
        gTypeSupported.push_back(pair.first);
    }

    for (const auto &value : gTypeSupported)
    { 
      if (std::find(jsonParsed["SKIP-PLACEMENT"].begin(), jsonParsed["SKIP-PLACEMENT"].end(), value) != jsonParsed["SKIP-PLACEMENT"].end())
      {
        GRAMM->trace(" [Case-2] : Skipping found NodeType {} in JSON",  value);
        return true;
      }
    }
    return false;
  }
}

/**
 * Parses PRAGMA vectors from the comment section of the device model graph.
 * 
 * This function extracts and parses vectors of strings associated with the given keyword 
 * from the comment section of the device model graph, and stores them in the provided 
 * GrammConfig map.
 */
void parseVectorofStrings(std::string commentSection, std::string keyword, std::map<std::string, std::vector<std::string>> &GrammConfig)
{
  std::istringstream stream(commentSection);
  std::string line;

  while (std::getline(stream, line))
  {
    size_t keywordPos = line.find(keyword);
    if (keywordPos != std::string::npos) // Keyword is found
    {
      size_t startBracket = line.find("{");
      size_t endBracket = line.find("}");
      if ((startBracket != std::string::npos) & (endBracket != std::string::npos))
      {
        std::string content = line.substr(startBracket + 1, endBracket - startBracket - 1);
        std::istringstream content_stream(content);
        std::string token;
        std::string FunCellName;
        int token_count = 0;
        while (std::getline(content_stream, token, ','))
        {
          // Remove leading and/or trailing whitespaces:
          token.erase(0, token.find_first_not_of(" \t\n\r"));
          token.erase(token.find_last_not_of(" \t\n\r") + 1);
          if (token_count == 0)
          {
            FunCellName = token;
            GrammConfig[token] = {};
          }
          GrammConfig[FunCellName].push_back(token);
          token_count++;
        }
      }
    }
  }
}

/**
 * Validates PRAGMA directives read from the application graph.
 */
bool checkVectorofStrings(std::string commentSection, std::string keyword, std::vector<std::string> &Type)
{
  std::istringstream stream(commentSection);
  std::string line;

  while (std::getline(stream, line))
  {
    size_t keywordPos = line.find(keyword);
    size_t PlacementLinePos = line.find("[PLACEMENT]");
    // Confirming this is not a Placement line:
    if ((keywordPos != std::string::npos) && (PlacementLinePos == std::string::npos))
    {
      size_t startBracket = line.find("{");
      size_t endBracket = line.find("}");
      if ((startBracket != std::string::npos) & (endBracket != std::string::npos))
      {
        std::string content = line.substr(startBracket + 1, endBracket - startBracket - 1);
        std::istringstream content_stream(content);
        std::string token;
        int token_counter = 0;
        while (std::getline(content_stream, token, ','))
        {
          // Remove leading and/or trailing whitespaces:
          token.erase(0, token.find_first_not_of(" \t\n\r"));
          token.erase(token.find_last_not_of(" \t\n\r") + 1);

          if (token_counter == 0)
          {
            if (token != keyword)
            {
              GRAMM->error("FunCell {} from application graph not found in the device model pragma", token);
              return false;
            }
          }
          else
          {
            // Checking the above token present in the device model description.
            auto it = std::find(Type.begin(), Type.end(), token);
            if (it != Type.end())
            { // If iterator is not at the end, the string is present
              GRAMM->info("[PASSED] The token {} found in {}", token, keyword);
            }
            else
            {
              GRAMM->error("[FAILED] The token {} not found in {}", token, keyword);
              return false;
            }
          }
          token_counter++;
        }
      }
      return true;
    }
  }
  GRAMM->error("FunCell {} from device-model not found in the application graph pragma", keyword);
  return false;
}

/**
 * Reads, checks, and stores PRAGMA directives from the device model file.
*/
void readDeviceModelPragma(std::ifstream &deviceModelFile, std::map<std::string, std::vector<std::string>> &GrammConfig)
{
  std::string commentSection = readCommentSection(deviceModelFile);
  parseVectorofStrings(commentSection, "[SUPPORTEDOPS]", GrammConfig);

  // OB for debug
  GRAMM->trace(" Device model pragma READ from the dot file :: {}", commentSection);

  GRAMM->info("Parsed Device-Model Pragma: ");
  for (const auto &pair : GrammConfig)
  {
    std::cout << "[" << pair.first << "] :: ";

    // Iterate over the vector and print elements
    for (size_t i = 0; i < pair.second.size(); ++i)
    {
      std::cout << pair.second[i];
      if (i != pair.second.size() - 1)
      {
        std::cout << " :: "; // Add " :: " only if it's not the last element
      }
    }
    std::cout << std::endl;
  }
}

/**
 * Reads, checks, and stores PRAGMA directives from the application graph file.
*/ 
void readApplicationGraphPragma(std::ifstream &applicationGraphFile, std::map<std::string, std::vector<std::string>> &GrammConfig)
{

  std::string commentSection = readCommentSection(applicationGraphFile);

  // OB for debug
  GRAMM->trace(" Application graph pragma READ from the dot file :: {}", commentSection);

  for (const auto pair : GrammConfig)
  {
    GRAMM->info("Checking compatibility of SupportedOps of [{}]", pair.first);
    bool status = checkVectorofStrings(commentSection, pair.first, GrammConfig[pair.first]);
    if (status == false)
    {
      GRAMM->error("FATAL ERROR -- application pragma are not compatiable with the device model pragma's");
      exit(1);
    }
  }
}

//------------------------------------------------------------------------------------//
//---------------------- [Utilities] Printing and visualization ----------------------//
//------------------------------------------------------------------------------------//

/**
 * Prints routing information to a mapping-output file.
 */ 
void printRoutingResults(int y, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile, std::map<int, NodeConfig> *hConfig)
{

  struct RoutingTree *RT = &((*Trees)[y]);

  if (RT->nodes.size() == 0)
    return;

  int n = RT->nodes.front();
  int orign = n;

  std::list<int>::iterator it = RT->nodes.begin();

  for (; it != RT->nodes.end(); it++)
  {
    int m = *it;

    if (m == orign)
      continue;

    if (boost::algorithm::contains(gNames[RT->parent[m]], "outPin"))
    {
      //  This else if loop is hit when the source is outPin and the while loop below traces the connection from the outPin and exits when found valid inPin.
      //  ex: outPin -> switchblock -> switchblock_pe_input -> pe_inPin
      //      This loop will trace the above connection and show a connection between outPin -> pe_inPin
      int current_sink = *it;
      while (it != RT->nodes.end())
      {
        if (boost::algorithm::contains(gNames[current_sink], "inPin"))
        {
          positionedOutputFile << gNames_deliemter_changes(gNames[RT->parent[m]]) << " -> " << gNames_deliemter_changes(gNames[current_sink]) << "\n";
          unpositionedOutputFile << gNames_deliemter_changes(gNames[RT->parent[m]]) << " -> " << gNames_deliemter_changes(gNames[current_sink]) << "\n";
        }
        it++;
        current_sink = *it;
      }
      break; // As the above while loop traces till the end of the connection, we have break out of the for loop iterator!!
    }
  }
}

/**
 * Connects associated pins to the specified FunCell in the device model graph.
*/ 
void mandatoryFunCellConnections(int gNumber, std::string FunCellName, DirectedGraph *G, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile)
{

  vertex_descriptor yD = vertex(gNumber, *G); // Vertex Descriptor for FunCell

  //--------------------------
  // Output pin-connection:
  //--------------------------
  out_edge_iterator eo, eo_end;
  boost::tie(eo, eo_end) = out_edges(yD, *G);
  int selectedCellOutputPin = target(*eo, *G);

  if (((*Users)[selectedCellOutputPin].size() >= 1))
  {
    positionedOutputFile << FunCellName + "_" + funCellMapping[gNames[gNumber]] << " -> " << gNames_deliemter_changes(gNames[selectedCellOutputPin]) << "\n";
    unpositionedOutputFile << FunCellName + "_" + funCellMapping[gNames[gNumber]] << " -> " << gNames_deliemter_changes(gNames[selectedCellOutputPin]) << "\n";
  }

  //--------------------------
  // Input Pin-connection:
  //--------------------------
  in_edge_iterator ei, ei_end;
  boost::tie(ei, ei_end) = in_edges(yD, *G);
  for (; ei != ei_end; ei++)
  {
    int source_id = source(*ei, *G);
    if (((*Users)[source_id].size() >= 1))
    {
      positionedOutputFile << gNames_deliemter_changes(gNames[source_id]) << " -> " << FunCellName + "_" + funCellMapping[gNames[gNumber]] << "\n";
      unpositionedOutputFile << gNames_deliemter_changes(gNames[source_id]) << " -> " << FunCellName + "_" + funCellMapping[gNames[gNumber]] << "\n";
    }
  }
}

/**
 * Prints placement information to a mapping-output file.
*/
void printPlacementResults(int gNumber, std::string gName, DirectedGraph *G, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile, std::map<int, NodeConfig> *gConfig, std::map<std::string, std::vector<std::string>> &GrammConfig)
{
  int scale = 6;
  float G_VisualX = boost::get(&DotVertex::G_VisualX, *G, gNumber) * scale;
  float G_VisualY = boost::get(&DotVertex::G_VisualY, *G, gNumber) * scale;
  // std::cout << "The X,Y location of " << gName << "is " << G_VisualX << " , " << G_VisualY << "\n";

  // Use for deciding the color of the FunCell based on the opcode
  // int opcode_gNumber = (*gConfig)[gNumber].opcode;
  int opcode_gNumber = 0;
  for (const auto &pair : GrammConfig)
  {
    if (pair.first == (*gConfig)[gNumber].Type)
      break;
    else
      opcode_gNumber++;
  }
  std::string modified_name = gNames_deliemter_changes(gName); // Modified combined string

  // OB Debug: std::cout << gNames_deliemter_changes(gName) << " " << (*Users)[gNumber].size() << std::endl;

  if ((*gConfig)[gNumber].Cell == "FUNCCELL")
  {
    if (((*Users)[gNumber].size() >= 1))
    {
      positionedOutputFile << modified_name + "_" + funCellMapping[gName] << " [shape=\"rectangle\" width=0.5 fontsize=12 fillcolor=\"" << colors[opcode_gNumber] << "\" pos=\"" << G_VisualX << "," << G_VisualY << "!\"]\n";
      unpositionedOutputFile << modified_name + "_" + funCellMapping[gName] << " [shape=\"rectangle\" width=0.5 fontsize=12 fillcolor=\"" << colors[opcode_gNumber] << "\"]\n";
      // If FuncCell is used, then we have to do mandatary connections of FunCell to the input and output pins that are used.
      mandatoryFunCellConnections(gNumber, modified_name, G, positionedOutputFile, unpositionedOutputFile);
    }
    else
    {
      positionedOutputFile << modified_name << " [shape=\"rectangle\" width=0.5 fontsize=12 fillcolor=\"" << unused_cell_color << "\" pos=\"" << G_VisualX << "," << G_VisualY << "!\"]\n";
    }
  }
  else if ((*gConfig)[gNumber].Cell == "PINCELL")
  {
    if (((*Users)[gNumber].size() >= 1))
    {
      if ((*gConfig)[gNumber].Type == "IN")
      {
        positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << input_pin_color << "\" pos=\"" << G_VisualX << "," << G_VisualY << "!\"]\n";
        unpositionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << input_pin_color << "\"]\n";
      }
      else
      {
        positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << output_pin_color << "\" pos=\"" << G_VisualX << "," << G_VisualY << "!\"]\n";
        unpositionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << output_pin_color << "\"]\n";
      }
    }
    else
    {
      positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << unused_cell_color << "\" pos=\"" << G_VisualX << "," << G_VisualY << "!\"]\n";
    }
  }
}

/**
 * Prints mapping results in neato format: First displays the layout and then shows connections between the nodes.
 */
void printMappedResults(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig, std::map<std::string, std::vector<std::string>> &GrammConfig)
{

  // Output stream for storing successful mapping: The positioned-output dot file stream (this contains actual co-ordinates of the node cells).
  std::ofstream positionedOutputFile;
  positionedOutputFile.open("positioned_dot_output.dot");
  GRAMM->info("Writing the positioned mapping output in file 'positioned_dot_output.dot'");

  // Output stream for storing successful mapping:
  std::ofstream unpositionedOutputFile;
  unpositionedOutputFile.open("unpositioned_dot_output.dot");
  GRAMM->info("Writing the unpositioned mapping output in file 'unpositioned_dot_output.dot'");

  // Printing the start of the dot file:
  positionedOutputFile << "digraph {\ngraph [bgcolor=lightgray];\n node [style=filled, fontname=\"times-bold\", penwidth=2];\n edge [penwidth=4]; \n splines=ortho;\n";
  unpositionedOutputFile << "digraph {\ngraph [bgcolor=lightgray];\n node [style=filled, fontname=\"times-bold\", penwidth=2];\n edge [penwidth=4]; \n splines=true; rankdir=TB;\n";

  //---------------------------------------------------
  // Adding nodes of kernel into simplified_file_output:
  //---------------------------------------------------
  unpositionedOutputFile << "subgraph cluster_1 {\n label = \"Input Kernel\"; fontsize = 40; style=dashed; \n edge [minlen=3]\n";
  for (int i = 0; i < num_vertices(*H); i++)
  {
    if (doPlacement((*hConfig)[i].Opcode, jsonParsed))
      continue;
    unpositionedOutputFile << hNames[i] << ";\n";
  }

  std::pair<edge_iterator, edge_iterator> edge_pair = edges(*H);
  for (edge_iterator e_it = edge_pair.first; e_it != edge_pair.second; ++e_it)
  {
    vertex_descriptor u = source(*e_it, *H);
    vertex_descriptor v = target(*e_it, *H);

    if (doPlacement((*hConfig)[u].Opcode, jsonParsed))
      continue;

    unpositionedOutputFile << "  " << hNames[u] << " -> " << hNames[v] << ";\n";
  }

  unpositionedOutputFile << "}\n";

  unpositionedOutputFile << "subgraph cluster_0 {\n label = \"GRAMM mapping output\"; fontsize = 40; style=dashed;\n";

  //------------------------
  // Draw/Print placement:
  //------------------------
  for (auto hElement : gNames)
  {
    int gNumber = hElement.first;

    if (((*gConfig)[gNumber].Cell == "FUNCCELL") && (std::find(GrammConfig["PLACEMENT"].begin(), GrammConfig["PLACEMENT"].end(), (*gConfig)[gNumber].Type) == GrammConfig["PLACEMENT"].end()))
      continue;

    if ((FunCell_Visual_Enable & ((*gConfig)[gNumber].Cell == "FUNCCELL")) || (PinCell_Visual_Enable & ((*gConfig)[gNumber].Cell == "PINCELL")) || (RouteCell_Visual_Enable & ((*gConfig)[gNumber].Cell == "ROUTECELL")))
    {
      std::string gName = hElement.second;
      printPlacementResults(gNumber, gName, G, positionedOutputFile, unpositionedOutputFile, gConfig, GrammConfig);
    }
  }

  //------------------------
  // Draw/Print routing:
  //------------------------
  for (int i = 0; i < num_vertices(*H); i++)
  {
    if (doPlacement((*hConfig)[i].Opcode, jsonParsed))
      continue;
    printRoutingResults(i, positionedOutputFile, unpositionedOutputFile, hConfig);
  }

  positionedOutputFile << "}\n";   // End of digraph
  unpositionedOutputFile << "}\n"; // End of second cluster
  unpositionedOutputFile << "}\n"; // End of digraph
}

/**
 * Prints the device model cell name corresponding to a given boost id number from device model graph.
 */
void printName(int n)
{
  GRAMM->debug("{}", gNames[n]);
}

/**
 * Prints vertex models of the application graph's nodes.
 */
void printVertexModels(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<std::string, std::vector<std::string>> &GrammConfig, std::map<int, int> &invUsers)
{
  for (int i = 0; i < num_vertices(*H); i++)
  {

    if (doPlacement((*hConfig)[i].Opcode, jsonParsed))
      continue;

    struct RoutingTree *RT = &((*Trees)[i]);

    GRAMM->info("** routing for {}'s output pin :: ", hNames[i]);
    std::list<int>::iterator it = RT->nodes.begin();

    //Checking for the elements with zero vertex model size (for the nodes without any fanout)
    if(RT->nodes.size() == 0)
    {
      GRAMM->info("\t Empty vertex model (no-fanouts for the node)");
      funCellMapping[gNames[invUsers[i]]] = hNames[i];
    }
   
    while (it != RT->nodes.end())
    {
      int n = *it;
      GRAMM->info("\t {} \t {}", n, gNames[n]);
      if (it == RT->nodes.begin())
      {
        // The begining node in the resource tree will be always outPin for the FunCell
        //   - Finding the FuncCell based on this outPin ID.
        //   - finding ID for alu_x_y from this: "alu_x_y.outPinA"
        int FunCellLoc = findFunCellFromOutputPin(*it, G);

        // For concatinating the mapped applicationNodeID name in device model cell
        funCellMapping[gNames[FunCellLoc]] = hNames[i];
      }

      it++;
    }
  }
}

/**
 * Prints the routing details for the given signal.
*/ 
void printRouting(int signal)
{

  struct RoutingTree *RT = &((*Trees)[signal]);
  GRAMM->debug("** routing for i: {} {} ", signal, hNames[signal]);

  std::list<int>::iterator it = RT->nodes.begin();
  for (; it != RT->nodes.end(); it++)
  {
    GRAMM->debug("\t {} \t {}", *it, gNames[*it]);
  }
}

//------------------------------------------------------------------------------------//
//-------------------------------- [Utilities] File read -----------------------------//
//------------------------------------------------------------------------------------//

/**
 * Reads a DOT file and stores attributes into the device model graph.
 */
void readDeviceModel(DirectedGraph *G, std::map<int, NodeConfig> *gConfig)
{
  //--------------------------------------------------------------------
  // Omkar:  Reading device model dot file instead.
  //--------------------------------------------------------------------

  // Iterate over all the nodes of the device model graph and assign the type of nodes:
  for (int i = 0; i < num_vertices(*G); i++)
  {
    vertex_descriptor v = vertex(i, *G);

    // Contains the Node type of Device Model Graph (FuncCell, RouteCell, PinCell)
    std::string arch_NodeCell = boost::get(&DotVertex::G_NodeCell, *G, v);
    std::string upperCaseNodeCell = boost::to_upper_copy(arch_NodeCell);
    (*gConfig)[i].Cell = upperCaseNodeCell;

    // Contains the Opcode of the NodeType (For example "ALU" for NodeType "FuncCell")
    std::string arch_NodeType = boost::get(&DotVertex::G_NodeType, *G, v);
    std::string upperCaseType = boost::to_upper_copy(arch_NodeType);
    (*gConfig)[i].Type = upperCaseType;

    // Contains the node name
    std::string arch_NodeName = boost::get(&DotVertex::G_Name, *G, v);
    gNames[i] = arch_NodeName;

    // Obtaining the loadPin name for PinCell type:
    if ((*gConfig)[i].Cell == "PINCELL")
    {
      size_t pos = gNames[i].find_last_of('.');
      if (pos != std::string::npos)
      {
        // GRAMM->info("loadPin for {} is {}", gNames[i], gNames[i].substr(pos + 1));
        (*gConfig)[i].loadPin = gNames[i].substr(pos + 1);
      }
    }

    GRAMM->trace("[G] arch_NodeName {} :: arch_NodeCell {} :: arch_NodeType {}", arch_NodeName, upperCaseNodeCell, upperCaseType);
  }
}

/**
 * Reads a DOT file and stores attributes into the application graph.
*/ 
void readApplicationGraph(DirectedGraph *H, std::map<int, NodeConfig> *hConfig)
{
  /*
    Application DFG is inputed in GRAMM as a directed graph using the Neato Dot file convenction. In neeto,
    we have defined the vertices (also refered to as nodes) and edges with attributes (also refered to as properies).
    For instance, a node in GRAMM will have a attribite to define the opcode or the edges may have the attribute to
    define the selective pins for an PE.
  */

  for (int i = 0; i < num_vertices(*H); i++)
  {
    vertex_descriptor v = vertex(i, *H);

    // Fetching node name from the application-graph:
    std::string name = boost::get(&DotVertex::name, *H, v);
    hNames[i] = removeCurlyBrackets(name); // Removing if there are any curly brackets from the hNames.

    // Following characters are not supported in the neato visualizer: "|, =, ."
    std::replace(hNames[i].begin(), hNames[i].end(), '|', '_');
    std::replace(hNames[i].begin(), hNames[i].end(), '=', '_');
    std::replace(hNames[i].begin(), hNames[i].end(), '.', '_');

    // Fetching opcode from the application-graph:
    // Contains the Opcode of the operation (ex: FMUL, FADD, INPUT, OUTPUT etc.)
    std::string applicationOpcode = boost::get(&DotVertex::opcode, *H, v);
    std::string upperCaseOpcode = boost::to_upper_copy(applicationOpcode);
    (*hConfig)[i].Opcode = upperCaseOpcode;

    GRAMM->trace("[H] name {} :: applicationOpcode {} ", name, upperCaseOpcode);
  }
}
