//=======================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                           //
// file : utilities.cp                                                   //
// description: contains functions needed for printing and visualization //
//=======================================================================//

#include "../lib/GRAMM.h"
#include "../lib/utilities.h"

std::vector<std::string> colors = {
    "#FFFF00", // Yellow
    "#40E0D0", // Turquoise
    "#A52A2A", // Brown
    "#00FFFF", // Cyan
    "#FF00FF", // Magenta
    "#FFA500", // Orange
    "#800080", // Purple
    "#FFC0CB", // Pink
    "#40E0D0", // Turquoise
    "#4B0082"  // Indigo
}; //Color code for different FunCell no

std::string input_pin_color   = "#0000FF";  //Pre-defined color-code for the input-pin cell
std::string output_pin_color  = "#FF0000";  //Pre-defined color-code for the output-pin cell
std::string unused_cell_color = "#A9A9A9";  //Pre-defined color-code for the unused cell
std::map<std::string, std::string> funCellMapping; //Key-> Device-model node-name :: Value-> Mapped application name.

std::string removeCurlyBrackets(const std::string& input) {
    std::string result = input;
    
    // Find and remove the opening curly bracket
    size_t pos = result.find('{');
    if (pos != std::string::npos) {
        result.erase(pos, 1);
    }

    // Find and remove the closing curly bracket
    pos = result.find('}');
    if (pos != std::string::npos) {
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

void printRoutingResults(int y, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile)
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

    //Checking if either source is "inPin" or sink is "outPin"
    if ((boost::algorithm::contains(gNames[RT->parent[m]], "inPin")) || (boost::algorithm::contains(gNames[m], "outPin")))
    {
      // parent[m] --> FunCell || m --> outPin
      positionedOutputFile << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_" + funCellMapping[gNames[RT->parent[m]]] << " -> " << gNames_deliemter_changes(gNames[m]) << "\n";
      unpositionedOutputFile << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_" + funCellMapping[gNames[RT->parent[m]]] << " -> " << gNames_deliemter_changes(gNames[m]) << "\n";

      // OB: Right now, GRAMM does not include connection between inPin to the FunCell
      //     That means this, if loop is probably will be hit due to outPin connection (FuncCell --> outPin) connection.
      //     For the FunCell which is being used, adding a manual connection between inPin and FunCell.
      // TODO: Remove this manual connection in future; for this routing needs to be modified to incorporate last level connections.

      // Adding the manual connection.
      if (boost::algorithm::contains(gNames[RT->parent[m]], "alu"))
      {
        // ALU cell --> Adding two inPin connections
        positionedOutputFile << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_inPinA" << " -> " << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_" + funCellMapping[gNames[RT->parent[m]]]  << "\n";
        positionedOutputFile << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_inPinB" << " -> " << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_" + funCellMapping[gNames[RT->parent[m]]]  << "\n";

        unpositionedOutputFile << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_inPinA" << " -> " << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_" + funCellMapping[gNames[RT->parent[m]]] << "\n";
        unpositionedOutputFile << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_inPinB" << " -> " << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_" + funCellMapping[gNames[RT->parent[m]]] << "\n";
      }
    }
    else if (boost::algorithm::contains(gNames[RT->parent[m]], "outPin"))
    {
      /*
        This else if loop is hit when the source is outPin and the while loop below traces the connection from the outPin and exits when found valid inPin.
        ex: outPin -> switchblock -> switchblock_pe_input -> pe_inPin
            This loop will trace the above connection and show a connection between outPin -> pe_inPin
      */
      bool hit = false;
      int current_sink = *it;

      while (hit == false)
      {
        it++;
        current_sink = *it;

        if (boost::algorithm::contains(gNames[current_sink], "inPin"))
        {
          hit = true; //Exits when a sink inPin is detected!!
          positionedOutputFile << gNames_deliemter_changes(gNames[RT->parent[m]]) << " -> " << gNames_deliemter_changes(gNames[current_sink]) << "\n";
          unpositionedOutputFile << gNames_deliemter_changes(gNames[RT->parent[m]]) << " -> " << gNames_deliemter_changes(gNames[current_sink]) << "\n";

          // Adding the manual connection for the store operation.
          // TODO: Remove this in future.
          if (boost::algorithm::contains(gNames[current_sink], "LS"))
          { // LS Cell --> Adding a inPin connections (required for Store, to connect input_pin to the Memport)
            std::string funcCell_Index = string_remover(gNames[current_sink], ".inPinA");
            positionedOutputFile << gNames_deliemter_changes(gNames[current_sink]) << " -> " <<  gNames_deliemter_changes(funcCell_Index) + "_" + funCellMapping[funcCell_Index]  << "\n";
            unpositionedOutputFile <<  gNames_deliemter_changes(gNames[current_sink]) << " -> " << gNames_deliemter_changes(funcCell_Index) + "_" + funCellMapping[funcCell_Index]   << "\n";
          }
        }
      }
    }
  }
}

void printPlacementResults(int gNumber, std::string gName, std::ofstream &positionedOutputFile, std::ofstream &unpositionedOutputFile, std::map<int, NodeConfig> *gConfig)
{
  //Pre-defined scale and displacement values for the positioned-dot file:
  int scale = 6;
  float input_displacement = 0.65 * (scale/3);
  float out_displacement = 1.0 * (scale/3);

  //Parsing the sub_strings from the device model node's name:
  std::vector<std::string> parts; // Contains the sub-parts of the current string
  std::string part;               // Used while parsing the sub-string.
  std::stringstream ss(gName);    // Creating string stream input

  while (std::getline(ss, part, '.'))
  {
    parts.push_back(part);
  }

  //Parsing the X & Y co-ordinates from the sub_strings
  // '0' subtraction is used to convert Char into Integer!!
  int x = parts[2][1] - '0'; // parts[2] --> 'cX' :: X will contain the column location of the node
  int y = parts[3][1] - '0'; // parts[3] --> 'rX' :: Y will contain the row location of the node

  if (DEBUG)
  {
    for (const std::string &p : parts)
    { // Print the parts
      std::cout << p << " :: ";
    }
    std::cout << std::endl;
  }

  // OB: skipping the constant as of now:
  if (parts[4] == "const")
  {
    return;
  }

  //---------------------------------------------------
  // Case0: if the gName is FuncCell (mem/constant/alu)
  //---------------------------------------------------
  if ((*gConfig)[gNumber].type == FuncCell)
  {
    int opcode_gNumber = (*gConfig)[gNumber].opcode;

    if ((*Users)[gNumber].size() >= 1)  //Users list is used for determining whether the current device-model cell is used or not.
    { 
      unpositionedOutputFile << gNames_deliemter_changes(gName) + "_" + funCellMapping[gName]  << " [shape=\"rectangle\" width=0.5 fontsize=12 fillcolor=\"" << colors[opcode_gNumber] << "\"]\n";
      positionedOutputFile << gNames_deliemter_changes(gName) + "_" + funCellMapping[gName] << " [shape=\"rectangle\" width=0.5 fontsize=12 fillcolor=\"" << colors[opcode_gNumber] << "\" pos=\"" << scale * x << "," << scale * y << "!\"]\n";
    }
    else
    {
      positionedOutputFile << gNames_deliemter_changes(gName) << " [shape=\"rectangle\" width=0.5 fontsize=12 fillcolor=\"" << unused_cell_color << "\" pos=\"" << scale * x << "," << scale * y << "!\"]\n";
    }
  }

  //-------------------------------------------------------
  // Case1: if the gName is pinCell (inPinA/inPinB/outPin)
  //-------------------------------------------------------
  if (boost::algorithm::contains(gName, "Pin"))
  {

    // Modified combined string:
    std::string modified_name = gNames_deliemter_changes(gName);

    if ((*Users)[gNumber].size() >= 1) //Users list is used for determining whether the current device-model cell is used or not.
    {
      if (parts[5] == "inPinA")
      {
        // Input-pin:
        positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << input_pin_color << "\" pos=\"" << scale * x - input_displacement << "," << scale * y + input_displacement << "!\"]\n";
        unpositionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=12 fillcolor=\"" << input_pin_color << "\"]\n";
      }
      else if (parts[5] == "inPinB")
      {
        positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << input_pin_color << "\" pos=\"" << scale * x + input_displacement << "," << scale * y + input_displacement << "!\"]\n";
        unpositionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=12 fillcolor=\"" << input_pin_color << "\"]\n";
      }
      else if (parts[5] == "outPinA")
      {
        positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << output_pin_color << "\" pos=\"" << scale * x << "," << scale * y - out_displacement << "!\"]\n";
        unpositionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=12 fillcolor=\"" << output_pin_color << "\"]\n";
      }
    }
    else{
      if (parts[5] == "inPinA")
      {
        // Input-pin:
        positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << unused_cell_color << "\" pos=\"" << scale * x - input_displacement << "," << scale * y + input_displacement << "!\"]\n";
      }
      else if (parts[5] == "inPinB")
      {
        positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << unused_cell_color << "\" pos=\"" << scale * x + input_displacement << "," << scale * y + input_displacement << "!\"]\n";
      }
      else if (parts[5] == "outPinA")
      {
        positionedOutputFile << modified_name << " [shape=\"oval\" width=0.1 fontsize=10 fillcolor=\"" << unused_cell_color << "\" pos=\"" << scale * x << "," << scale * y - out_displacement << "!\"]\n";
      }
    }

  }
}

void printMappedResults(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig)
{

  // Output stream for storing successful mapping: The positioned-output dot file stream (this contains actual co-ordinates of the node cells).
  std::ofstream positionedOutputFile;
  positionedOutputFile.open("positioned_dot_output.dot");
  std::cout << "Writing the positioned mapping output in file 'positioned_dot_output.dot' \n";

  // Output stream for storing successful mapping:
  std::ofstream unpositionedOutputFile;
  unpositionedOutputFile.open("unpositioned_dot_output.dot");
  std::cout << "Writing the unpositioned mapping output in file 'unpositioned_dot_output.dot' \n";

  // Printing the start of the dot file:
  positionedOutputFile << "digraph {\ngraph [bgcolor=lightgray]\nnode [style=filled]\nsplines=ortho;\n";
  unpositionedOutputFile << "digraph {\ngraph [bgcolor=lightgray]\nnode [style=filled]\nsplines=true; rankdir=LR;\n";

  //---------------------------------------------------
  //Adding nodes of kernel into simplified_file_output:
  //---------------------------------------------------
  unpositionedOutputFile << "subgraph cluster_1 {\n label = \"Input Kernel\"; fontsize = 40; style=dashed;\n";
  for (int i = 0; i < num_vertices(*H); i++)
  {
    if ((*hConfig)[i].opcode == constant)
      std::replace( hNames[i].begin(), hNames[i].end(), '|', '_');
      std::replace( hNames[i].begin(), hNames[i].end(), '=', '_');
      std::replace( hNames[i].begin(), hNames[i].end(), '.', '_');

    unpositionedOutputFile << removeCurlyBrackets(hNames[i]) << ";\n";
  }

  std::pair<edge_iterator, edge_iterator> edge_pair = edges(*H);
  for (edge_iterator e_it = edge_pair.first; e_it != edge_pair.second; ++e_it) {
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
    std::string gName = hElement.second;
    printPlacementResults(gNumber, gName, positionedOutputFile, unpositionedOutputFile, gConfig);
  }

  //------------------------
  // Draw/Print routing:
  //------------------------
  for (int i = 0; i < num_vertices(*H); i++)
  {
    //OB: Skipping the routing of constant as of now.
    if ((*hConfig)[i].opcode == constant)
      continue;

    printRoutingResults(i, positionedOutputFile, unpositionedOutputFile);
  }

  positionedOutputFile << "}\n"; //End of digraph
  unpositionedOutputFile << "}\n";  //End of second cluster
  unpositionedOutputFile << "}\n"; //End of digraph
}

void printName(int n)
{
  std::cout << gNames[n] << "\n";
}

void printVertexModels(DirectedGraph *H, DirectedGraph *G)
{
  for (int i = 0; i < num_vertices(*H); i++)
  {
    struct RoutingTree *RT = &((*Trees)[i]);

    std::cout << "** routing for i: " << i << " " << hNames[i] << "\n";

    std::list<int>::iterator it = RT->nodes.begin();

    for (; it != RT->nodes.end(); it++)
    {
      std::cout << "\t " << *it << "\t "; // for ADRES
      int n = *it;
      std::cout << gNames[n] << std::endl;

      if(it == RT->nodes.begin()){
        funCellMapping[gNames[n]] = removeCurlyBrackets(hNames[i]);
      }
    }
  }
}
