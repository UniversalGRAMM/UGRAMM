//=======================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                           //
// file : utilities.cp                                                   //
// description: contains functions needed for printing and visualization //
//=======================================================================//

#include "../lib/GRAMM.h"
#include "../lib/utilities.h"

#define DEBUG 0

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

void printRoutingResults(int y, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile, std::map<int, NodeConfig> *hConfig)
{

  struct RoutingTree *RT = &((*Trees)[y]);

  if (RT->nodes.size() == 0)
    return;

  int n = RT->nodes.front();
  int orign = n;

  if (DEBUG)
    std::cout << "For hNames[y] :: " << hNames[y] << " :: gNames[n] :: " << gNames[n] << "\n";

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
  float G_VisualX = std::stof(boost::get(&DotVertex::G_VisualX, *G, gNumber)) * scale;
  float G_VisualY = std::stof(boost::get(&DotVertex::G_VisualY, *G, gNumber)) * scale;
  // std::cout << "The X,Y location of " << gName << "is " << G_VisualX << " , " << G_VisualY << "\n";

  int opcode_gNumber = (*gConfig)[gNumber].opcode;             // Use for deciding the color of the FunCell based on the opcode
  std::string modified_name = gNames_deliemter_changes(gName); // Modified combined string

  //OB Debug: std::cout << gNames_deliemter_changes(gName) << " " << (*Users)[gNumber].size() << std::endl;

  if ((*gConfig)[gNumber].type == FuncCell)
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
  else if ((*gConfig)[gNumber].type == PinCell)
  {
    if (((*Users)[gNumber].size() >= 1))
    {
      if ((*gConfig)[gNumber].opcode == in)
      {
        positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << input_pin_color << "\" pos=\"" << G_VisualX << "," << G_VisualY << "!\"]\n";
        unpositionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << input_pin_color  << "\"]\n";
      }
      else
      {
        positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << output_pin_color << "\" pos=\"" << G_VisualX << "," << G_VisualY << "!\"]\n";
        unpositionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << output_pin_color  << "\"]\n";
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
    if ((FunCell_Visual_Enable & ((*gConfig)[gNumber].type == FuncCell)) || (PinCell_Visual_Enable & ((*gConfig)[gNumber].type == PinCell)) || (RouteCell_Visual_Enable & ((*gConfig)[gNumber].type == RouteCell)))
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

void printVertexModels(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig)
{
  for (int i = 0; i < num_vertices(*H); i++)
  {
    struct RoutingTree *RT = &((*Trees)[i]);

    GRAMM->info("** routing for {}'s output pin :: ", hNames[i]);

    std::list<int>::iterator it = RT->nodes.begin();

    while (it != RT->nodes.end())
    {
      int n = *it;
      GRAMM->info("\t {} \t {}", n, gNames[n]);

      if (it == RT->nodes.begin())
      {
        // The begining node in the resource tree will be always outPin for the FunCell
        //   - Finding the FuncCell based on this outPin ID.
        //     - finding ID for alu_x_y from this: "alu_x_y.outPinA"
        in_edge_iterator ei, ei_end;
        vertex_descriptor yD = vertex(n, *G);
        boost::tie(ei, ei_end) = in_edges(yD, *G);
        int FunCellLoc = source(*ei, *G);

        if ((*hConfig)[i].opcode == constant) // Fixing the constant names:
        {
          std::replace(hNames[i].begin(), hNames[i].end(), '|', '_');
          std::replace(hNames[i].begin(), hNames[i].end(), '=', '_');
          std::replace(hNames[i].begin(), hNames[i].end(), '.', '_');
        }

        funCellMapping[gNames[FunCellLoc]] = removeCurlyBrackets(hNames[i]);
        (*Users)[FunCellLoc].push_back(i);
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