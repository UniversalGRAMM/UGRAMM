//=======================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                           //
// file : utilities.cp                                                   //
// description: contains functions needed for printing and visualization //
//=======================================================================//

#include "../lib/GRAMM.h"
#include "../lib/utilities.h"

std::vector<std::string> colors = {
    "#FFFFE0", // Light Yellow
    "#AFEEEE", // Light Turquoise
    "#D2B48C", // Tan (Lighter Brown)
    "#E0FFFF", // Light Cyan
    "#FFA500", // Orange (Replaced Light Pink)
    "#FFDAB9", // Peach Puff (Lighter Orange)
    "#D8BFD8", // Thistle (Lighter Purple)
    "#00FF00", // Lime (Replaced Pink)
    "#AFEEEE", // Light Turquoise
    "#DDA0DD"  // Plum (Lighter Indigo)
}; // Color code for different FunCells

std::string input_pin_color = "#ADD8E6";           // Pre-defined color-code for the input-pin cell ( Light Blue)
std::string output_pin_color = "#FFB6C1";          // Pre-defined color-code for the output-pin cell (Lighter Red/Pink)
std::string unused_cell_color = "#A9A9A9";         // Pre-defined color-code for the unused cell
std::map<std::string, std::string> funCellMapping; // Key-> Device-model node-name :: Value-> Mapped application name.

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

std::string readCommentSection(std::ifstream &deviceModelFile)
{
  std::string line;
  std::string commentSection = {};
  bool comment_started = false;

  while (std::getline(deviceModelFile, line))
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

bool checkVectorofStrings(std::string commentSection, std::string keyword, std::vector<std::string> &Type)
{
  std::istringstream stream(commentSection);
  std::string line;

  while (std::getline(stream, line))
  {
    size_t keywordPos = line.find(keyword);
    if (keywordPos != std::string::npos)
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

void readApplicationGraphPragma(std::ifstream &applicationGraphFile, std::map<std::string, std::vector<std::string>> GrammConfig)
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

void printPlacementResults(int gNumber, std::string gName, DirectedGraph *G, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile, std::map<int, NodeConfig> *gConfig)
{
  int scale = 6;
  float G_VisualX = boost::get(&DotVertex::G_VisualX, *G, gNumber) * scale;
  float G_VisualY = boost::get(&DotVertex::G_VisualY, *G, gNumber) * scale;
  // std::cout << "The X,Y location of " << gName << "is " << G_VisualX << " , " << G_VisualY << "\n";

  // Use for deciding the color of the FunCell based on the opcode
  // int opcode_gNumber = (*gConfig)[gNumber].opcode;
  int opcode_gNumber = 2;
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

void printMappedResults(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig)
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
    unpositionedOutputFile << removeCurlyBrackets(hNames[i]) << ";\n";
  }

  std::pair<edge_iterator, edge_iterator> edge_pair = edges(*H);
  for (edge_iterator e_it = edge_pair.first; e_it != edge_pair.second; ++e_it)
  {
    vertex_descriptor u = source(*e_it, *H);
    vertex_descriptor v = target(*e_it, *H);

    unpositionedOutputFile << "  " << removeCurlyBrackets(hNames[u]) << " -> " << removeCurlyBrackets(hNames[v]) << ";\n";
  }

  unpositionedOutputFile << "}\n";

  unpositionedOutputFile << "subgraph cluster_0 {\n label = \"GRAMM mapping output\"; fontsize = 40; style=dashed;\n";

  //------------------------
  // Draw/Print placement:
  //------------------------
  for (auto hElement : gNames)
  {
    int gNumber = hElement.first;
    if ((FunCell_Visual_Enable & ((*gConfig)[gNumber].Cell == "FUNCCELL")) || (PinCell_Visual_Enable & ((*gConfig)[gNumber].Cell == "PINCELL")) || (RouteCell_Visual_Enable & ((*gConfig)[gNumber].Cell == "ROUTECELL")))
    {
      std::string gName = hElement.second;
      printPlacementResults(gNumber, gName, G, positionedOutputFile, unpositionedOutputFile, gConfig);
    }
  }

  //------------------------
  // Draw/Print routing:
  //------------------------
  for (int i = 0; i < num_vertices(*H); i++)
  {
    printRoutingResults(i, positionedOutputFile, unpositionedOutputFile, hConfig);
  }

  positionedOutputFile << "}\n";   // End of digraph
  unpositionedOutputFile << "}\n"; // End of second cluster
  unpositionedOutputFile << "}\n"; // End of digraph
}

void printName(int n)
{
  GRAMM->debug("{}", gNames[n]);
}

/*
  Description: For the given outputPin (signal), finds the associated FunCell node from the deviceModel.
  Ex: FunCell(signal) -> outPin (selectedCellOutputPin) [Walking uphill --- finding the source of this edge]
*/
int findFunCellFromOutputPin(int signal, DirectedGraph *G)
{
  vertex_descriptor signalVertex = vertex(signal, *G);
  in_edge_iterator ei, ei_end;
  boost::tie(ei, ei_end) = in_edges(signal, *G);
  int selectedCellFunCell = source(*ei, *G);

  return selectedCellFunCell;
}

void ripup(int signal, std::list<int> *nodes)
{
  std::list<int>::iterator it = (*nodes).begin();
  struct RoutingTree *RT = &((*Trees)[signal]);

  for (; it != (*nodes).end(); it++)
  {
    int delNode = *it;
    RT->nodes.remove(delNode);

    if (RT->parent.count(delNode))
    {
      RT->children[RT->parent[delNode]].remove(delNode);
      RT->parent.erase(delNode);
    }
    (*Users)[delNode].remove(signal);
  }
}

void ripUpRouting(int signal, DirectedGraph *G)
{
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int> toDel;
  toDel.clear();

  std::list<int>::iterator it = RT->nodes.begin();
  int driverFuncell = findFunCellFromOutputPin(*it, G);
  toDel.push_back(driverFuncell);
  for (; it != RT->nodes.end(); it++)
  {
    toDel.push_back(*it);
    //    std::cout << "RIPUP ";
    //    printName(*it);
  }

  ripup(signal, &toDel);
}

void printVertexModels(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig)
{
  for (int i = 0; i < num_vertices(*H); i++)
  {
    struct RoutingTree *RT = &((*Trees)[i]);

    GRAMM->info("** routing for {}'s output pin :: ", hNames[i]);
    std::list<int>::iterator it = RT->nodes.begin();

    //------------------------- Corner case: -------------------------------//
    // Checking whether there Fan-outs for the current application node:
    out_edge_iterator eo, eo_end;
    boost::tie(eo, eo_end) = out_edges(i, *H);
    if (eo == eo_end)
    {
      GRAMM->info("\t Empty vertex model (no-fanouts for the node)");

      //Finding FunCell location from the driver(outPin)
      int FunCellLoc = findFunCellFromOutputPin(*it, G);

      //For concatinating the mapped applicationNodeID name in device model cell
      funCellMapping[gNames[FunCellLoc]] = hNames[i];

      //Removing the members of vertex model if any:
      ripUpRouting(i, G); 

      //Since ripUp will remove the Users history as well for this node:
      (*Users)[FunCellLoc].push_back(i);            
      continue;
    }
    //-----------------------------------------------------------------------//

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

        //For concatinating the mapped applicationNodeID name in device model cell
        funCellMapping[gNames[FunCellLoc]] = hNames[i];   
      }

      it++;
    }
  }
}

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