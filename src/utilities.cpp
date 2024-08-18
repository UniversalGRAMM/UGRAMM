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
};

std::string input_pin_color = "#0000FF";
std::string output_pin_color = "#FF0000";
 std::map<int, std::string> funCellMapping;

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

void printRoutingResults(int y, std::ofstream &oFile, std::ofstream &sFile)
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

    if ((boost::algorithm::contains(gNames[RT->parent[m]], "inPin")) || (boost::algorithm::contains(gNames[m], "outPin")))
    {
      // OB: Right now, GRAMM does not include connection between inPin to the FunCell
      //     That means this if loop is probably hit due to outPin connections has been found (that means FuncCell --> outPin) connection.
      //     Adding a manual connection between inPin and FunCell.
      // TODO: Remove this manual connection; for this routing needs to be modified to incorporate last level connections.

      // parent[m] --> FunCell || m --> outPin
      oFile << gNames_deliemter_changes(gNames[RT->parent[m]]) << " -> " << gNames_deliemter_changes(gNames[m]) << "\n";
      sFile << funCellMapping[RT->parent[m]] << " -> " << gNames_deliemter_changes(gNames[m]) << "\n";

      // Adding the manual connection.
      if (boost::algorithm::contains(gNames[RT->parent[m]], "alu"))
      {
        // ALU cell --> Adding two inPin connections
        oFile << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_inPinA" << " -> " << gNames_deliemter_changes(gNames[RT->parent[m]]) << "\n";
        oFile << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_inPinB" << " -> " << gNames_deliemter_changes(gNames[RT->parent[m]]) << "\n";

        sFile << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_inPinA" << " -> " << funCellMapping[RT->parent[m]] << "\n";
        sFile << gNames_deliemter_changes(gNames[RT->parent[m]]) + "_inPinB" << " -> " << funCellMapping[RT->parent[m]] << "\n";
      }
    }
    else if (boost::algorithm::contains(gNames[RT->parent[m]], "outPin"))
    {

      bool hit = false;
      int current_sink = *it;

      while (hit == false)
      {
        it++;
        current_sink = *it;

        if (boost::algorithm::contains(gNames[current_sink], "inPin"))
        {
          hit = true;
          oFile << gNames_deliemter_changes(gNames[RT->parent[m]]) << " -> " << gNames_deliemter_changes(gNames[current_sink]) << "\n";
          sFile << gNames_deliemter_changes(gNames[RT->parent[m]]) << " -> " << gNames_deliemter_changes(gNames[current_sink]) << "\n";

          // Adding the manual connection.
          if (boost::algorithm::contains(gNames[current_sink], "LS"))
          { // LS Cell --> Adding one inPin connections (works only for Load as the output )
            oFile << gNames_deliemter_changes(gNames[current_sink]) << " -> " << string_remover(gNames_deliemter_changes(gNames[current_sink]), "_inPinA") << "\n";
            sFile <<  funCellMapping[current_sink] << " -> " << string_remover(gNames_deliemter_changes(gNames[current_sink]), "_inPinA") << "\n";
          }
        }
      }
    }
  }
}

void printPlacementResults(int gNumber, std::string gName, std::ofstream &oFile, std::ofstream &sFile, std::map<int, NodeConfig> *gConfig)
{
  int scale = 3;

  std::vector<std::string> parts; // Contains the sub-parts of the current string
  std::string part;               // Used while parsing the sub-string.
  std::stringstream ss(gName);    // Creating string stream input

  while (std::getline(ss, part, '.'))
  {
    parts.push_back(part);
  }

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

  if ((*gConfig)[gNumber].type == FuncCell)
  {
    int opcode_gNumber = (*gConfig)[gNumber].opcode;

    oFile << gNames_deliemter_changes(gName) << " [shape=\"circle\" width=0.5 fontsize=4 fillcolor=\"" << colors[opcode_gNumber] << "\" pos=\"" << scale * x << "," << scale * y << "!\"]\n";

    if ((*Users)[gNumber].size() >= 1)
    {
      sFile << funCellMapping[gNumber]  << " [shape=\"circle\" width=0.5 fontsize=12 fillcolor=\"" << colors[opcode_gNumber] << "\"]\n";

      /*
      //OB: hardcoded
      if (parts[4] == "alu")
      {
        sFile << gNames_deliemter_changes(gName) + "_inPinA" << " [shape=\"circle\" width=0.1 fontsize=1 fillcolor=\"" << input_pin_color << "\"]\n";
        sFile << gNames_deliemter_changes(gName) + "_inPinB" << " [shape=\"circle\" width=0.1 fontsize=1 fillcolor=\"" << input_pin_color << "\"]\n";
        sFile << gNames_deliemter_changes(gName) + "_outPinA" << " [shape=\"circle\" width=0.1 fontsize=1 fillcolor=\"" << output_pin_color << "\"]\n";
      }
      else{
        sFile << gNames_deliemter_changes(gName) + "_inPinA" << " [shape=\"circle\" width=0.1 fontsize=1 fillcolor=\"" << input_pin_color << "\"]\n";
        sFile << gNames_deliemter_changes(gName) + "_outPinA" << " [shape=\"circle\" width=0.1 fontsize=1 fillcolor=\"" << output_pin_color << "\"]\n";
      }
      */
    }
  }

  if (boost::algorithm::contains(gName, "Pin"))
  {

    float input_displacement = 0.80;
    float out_displacement = 1.0;

    // Modified combined string:
    std::string modified_name = gNames_deliemter_changes(gName);

    if ((*Users)[gNumber].size() >= 1)
    {
      if ((parts[5] == "inPinA") || (parts[5] == "inPinB"))
      {
        sFile << modified_name << " [shape=\"circle\" width=0.1 fontsize=1 fillcolor=\"" << input_pin_color << "\"]\n";
      }
      else
      {
        sFile << modified_name << " [shape=\"circle\" width=0.1 fontsize=1 fillcolor=\"" << output_pin_color << "\"]\n";
      }
    }

    if (parts[5] == "inPinA")
    {
      // Input-pin:
      oFile << modified_name << " [shape=\"circle\" width=0.1 fontsize=1 fillcolor=\"" << input_pin_color << "\" pos=\"" << scale * x - input_displacement << "," << scale * y + input_displacement << "!\"]\n";
    }
    else if (parts[5] == "inPinB")
    {
      oFile << modified_name << " [shape=\"circle\" width=0.1 fontsize=1 fillcolor=\"" << input_pin_color << "\" pos=\"" << scale * x + input_displacement << "," << scale * y + input_displacement << "!\"]\n";
    }
    else if (parts[5] == "outPinA")
    {
      oFile << modified_name << " [shape=\"circle\" width=0.1 fontsize=1 fillcolor=\"" << output_pin_color << "\" pos=\"" << scale * x << "," << scale * y - out_displacement << "!\"]\n";
    }
  }
}

void printMappedResults(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig)
{

  // Output stream for storing successful mapping:
  std::ofstream oFile;
  oFile.open("mapping_output.dot");
  std::cout << "Writing the ordered mapping output in file 'mapping_output.dot' \n";

  // Output stream for storing successful mapping:
  std::ofstream sFile;
  sFile.open("simple_mapping_output.dot");
  std::cout << "Writing the unordered mapping output in file 'simple_mapping_output.dot' \n";

  // Printing the start of the dot file:
  oFile << "digraph {\ngraph [pad=\"0.212,0.055\" bgcolor=lightgray]\nnode [style=filled]\nsplines=true;\n";
  sFile << "digraph {\ngraph [pad=\"0.212,0.055\" bgcolor=lightgray]\nnode [style=filled]\nsplines=true;\n";

  //------------------------
  // Draw/Print placement:
  //------------------------
  for (auto hElement : gNames)
  {
    int gNumber = hElement.first;
    std::string gName = hElement.second;

    // OB: std::cout <<  " (*Users)[gNumber].size()  :: " << (*Users)[gNumber].size() <<  " :: gName :: " << gName << "\n";

    printPlacementResults(gNumber, gName, oFile, sFile, gConfig);
  }

  //------------------------
  // Draw/Print routing:
  //------------------------
  for (int i = 0; i < num_vertices(*H); i++)
  {

    if ((*hConfig)[i].opcode == constant)
      continue;

    printRoutingResults(i, oFile, sFile);
  }

  oFile << "}\n";
  sFile << "}\n";
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
        funCellMapping[n] = removeCurlyBrackets(hNames[i]);
      }
    }
  }
}
