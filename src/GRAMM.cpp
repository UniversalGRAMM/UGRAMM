//===================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                       //
// file : GRAMM.cpp                                                  //
// description: contains primary functions                           //
//===================================================================//

#include "../lib/GRAMM.h"
#include "../lib/utilities.h"
#include "../lib/routing.h"
#include "../lib/drc.h"

//-------------------------------------------------------------------//
//------------------- Declartion of variables -----------------------//
//-------------------------------------------------------------------//

// Pathefinder cost parameters:
float PFac = 1; // Congestion cost factor
float HFac = 1; // History cost factor

// Mapping related variables:
std::vector<RoutingTree> *Trees;
std::vector<std::list<int>> *Users;
std::map<int, int> invUsers;
std::vector<int> *HistoryCosts;
std::vector<int> *TraceBack;
std::vector<int> *TopoOrder;
std::map<int, std::string> hNames;
std::map<int, std::string> gNames;
std::bitset<100000> explored;

std::vector<std::string> inPin = {"inPinA", "inPinB", "anyPins"};
std::vector<std::string> outPin = {"outPinA"};

// Logger & CLI global variables:
std::shared_ptr<spdlog::logger> GRAMM = spdlog::stdout_color_mt("GRAMM");
namespace po = boost::program_options;

//GrammConfig structure which is parsed from Pragma's of Device and application graph
std::map<std::string, std::vector<std::string>> GrammConfig; 

// Defining the JSON for the config file:
json jsonParsed;

//---------------------------------------------------------------------//

/*
  Description: For every application graph node, find a minimal vertex model.
*/
int findMinVertexModel(DirectedGraph *G, DirectedGraph *H, int y, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig)
{
  //--------------------------------------
  // Step 1: Checking all ϕ(xj) are empty
  //--------------------------------------
  bool allEmpty = true;

  in_edge_iterator ei, ei_end;
  vertex_descriptor yD = vertex(y, *H);
  boost::tie(ei, ei_end) = in_edges(yD, *H);
  for (; ei != ei_end; ei++)
  {
    if ((*Trees)[source(*ei, *G)].nodes.size())
      allEmpty = false;
  }

  out_edge_iterator eo, eo_end;
  boost::tie(eo, eo_end) = out_edges(yD, *H);
  for (; eo != eo_end; eo++)
  {
    if ((*Trees)[target(*eo, *H)].nodes.size())
      allEmpty = false;
  }

  if (allEmpty)
  {
    // choose a random unused node of G to be the vertex model for y because NONE of its fanins or fanouts have vertex models
    int selectedCellOutputPin = 0;
    int chooseRand = (rand() % num_vertices(*G));
    bool vertex_found = false;

    while (vertex_found == false)
    {
      chooseRand = (chooseRand + 1) % num_vertices(*G);
      if (compatibilityCheck((*gConfig)[chooseRand].Type, (*hConfig)[y].Opcode))
      {
        //  Finding the output Pin for the selected FunCell:
        selectedCellOutputPin = findOutputPinFromFuncell(chooseRand, G);
        vertex_found = true;
      }
    }

    GRAMM->debug("[RandomSelection] for application node :: {} :: Choosing starting vertex model as :: {} :: {}", hNames[y], gNames[chooseRand], gNames[selectedCellOutputPin]);

    // OB: In pin to pin mapping: adding outPin in the routing tree instead of the driver FunCell node.
    //     But note that costing is still done with respect to the FunCell node as costing based on output pin may not show overlap correctly

    (*Trees)[y].nodes.push_back(selectedCellOutputPin); // Routing data structure starts with outpin itself
    (*Users)[chooseRand].push_back(y);                  // Users and history cost is primarily tracked for FunCell nodes.
    (*Users)[selectedCellOutputPin].push_back(y);       
    invUsers[y] = chooseRand;

    return 0;
  }

  // At least one of y's fanins or fanouts have a vertex model [not allEmpty cases]
  int bestCost = MAX_DIST;
  int bestIndexPincell = -1;
  int bestIndexFuncell = -1;

  int totalCosts[num_vertices(*G)];
  for (int i = 0; i < num_vertices(*G); i++)
    totalCosts[i] = 0;

  bool compatibilityStatus = true;

  for (int i = 0; i < num_vertices(*G); i++)
  { // first route the signal on the output of y

    // Confriming the Vertex correct type:
    if (!compatibilityCheck((*gConfig)[i].Type, (*hConfig)[y].Opcode))
    {
      compatibilityStatus = false;
      continue;
    }

    // Finding the output Pin for the selected FunCell:
    int outputPin = findOutputPinFromFuncell(i, G);
    
    // OB: In pin to pin mapping: adding outPin in the routing tree instead of the driver FunCell node.
    //     But note that costing is still done with respect to the FunCell node as costing based on output pin may not show overlap correctly
    ripUpRouting(y, G);
    (*Users)[i].push_back(y);                   
    (*Users)[outputPin].push_back(y);
    (*Trees)[y].nodes.push_back(outputPin);
    invUsers[y] = i;

    // Cost and history costs are calculated for the FunCell:
    totalCosts[i] += (1 + (*HistoryCosts)[i]) * ((*Users)[i].size() * PFac);

    totalCosts[i] += routeSignal(G, H, y, gConfig);

    //-------- Debugging statements ------------//
    if (GRAMM->level() <= spdlog::level::trace)
      printRouting(y);
    GRAMM->debug("For application node {} :: routing for location [{}] has cost {}", hNames[y], gNames[outputPin], totalCosts[i]);
    //------------------------------------------//

    //Early exit if the cost is greater than bestCost:
    if (totalCosts[i] > bestCost)
      continue;

    //----------------------------------------------
    // now route the signals on the input of y
    //----------------------------------------------

    boost::tie(ei, ei_end) = in_edges(yD, *H);
    for (; (ei != ei_end); ei++)
    {
      int driver = source(*ei, *H);

      if ((*Trees)[driver].nodes.size() == 0)
        continue; // driver is not placed

      int driverPinNode = findRoot(driver, gConfig);

      ripUpRouting(driver, G);
      (*Trees)[driver].nodes.push_back(driverPinNode);

      // Need to find the FunCell associated with driverPinNode (which is PinCell)
      int driverFunCellNode = findFunCellFromOutputPin(driverPinNode, G);
      (*Users)[driverFunCellNode].push_back(driver); // Users and history cost is primarily tracked for FunCell nodes.
      (*Users)[driverPinNode].push_back(driver);
      invUsers[driver] = driverFunCellNode;

      // Newfeature: rip up from load: ripUpLoad(G, driver, outputPin);
      totalCosts[i] += routeSignal(G, H, driver, gConfig);

      GRAMM->debug("Routing the signals on the input of {}", hNames[y]);
      GRAMM->debug("For {} -> {} :: {} -> {}'s input-pin has cost {}", hNames[driver], hNames[y], gNames[driverPinNode], gNames[i], totalCosts[i]);

      if (totalCosts[i] > bestCost)
        break;
    }

    GRAMM->debug("Total cost for {} is {}\n", hNames[y], totalCosts[i]);

    if (totalCosts[i] >= MAX_DIST)
      continue;

    if (totalCosts[i] < bestCost)
    {
      bestIndexFuncell = i;
      bestIndexPincell = outputPin;
      bestCost = totalCosts[i];
      compatibilityStatus = true;
    }
    else if (totalCosts[i] == bestCost)
    {
      if (!(rand() % 2))
      {
        bestIndexFuncell = i;
        bestIndexPincell = outputPin;
      }
      compatibilityStatus = true;
    }
  }

  if ((bestIndexPincell == -1) || (bestIndexFuncell == -1))
  {
    GRAMM->error("FATAL ERROR -- COULD NOT FIND A VERTEX MODEL FOR VERTEX {} {}", y, hNames[y]);
    if (!compatibilityStatus)
      GRAMM->error("Nodes in the device model does not supports {} Opcode", (*hConfig)[y].Opcode);
    exit(-1);
  }

  GRAMM->debug("The bertIndex found for {} from the device-model is {} with cost of {}", hNames[y], gNames[bestIndexPincell], bestCost);

  // Final rig-up before doing final routing:
  ripUpRouting(y, G);
  invUsers[y] = bestIndexFuncell;
  (*Users)[bestIndexFuncell].push_back(y); // Users and history cost is primarily tracked for FunCell nodes.
  
  //------------------------- Corner case: -------------------------------//
  // Checking whether current application node has any Fan-outs or not:
  // If fan-outs available for the current application-node, then only adding placement information into 
  // the vertex model.
  out_edge_iterator eout, eout_end;
  boost::tie(eout, eout_end) = out_edges(yD, *H);
  if (eout != eout_end) //Fanout available
  { 
    (*Trees)[y].nodes.push_back(bestIndexPincell);
    (*Users)[bestIndexPincell].push_back(y);
  }
  //-----------------------------------------------------------------------//

  // Final-placement for node 'y':
  routeSignal(G, H, y, gConfig);

  // Final routing all of the inputs of the node 'y':
  boost::tie(ei, ei_end) = in_edges(yD, *H);
  for (; ei != ei_end; ei++)
  {
    int driver = source(*ei, *H);
    if ((*Trees)[driver].nodes.size() == 0)
      continue; // driver is not placed

    int driverPinNode = findRoot(driver, gConfig);

    ripUpRouting(driver, G);
    (*Trees)[driver].nodes.push_back(driverPinNode);

    // Need to find the FunCell associated with driverPinNode (which is PinCell)
    int driverFunCellNode = findFunCellFromOutputPin(driverPinNode, G);
    (*Users)[driverFunCellNode].push_back(driver); // Users and history cost is primarily tracked for FunCell nodes.
    (*Users)[driverPinNode].push_back(driver);
    invUsers[driver] = driverFunCellNode;

    routeSignal(G, H, driver, gConfig);
  }

  return 0;
}

//---------------------------------------------------------------------//

/*
  Description: Find minor embedding for all of the nodes of the application graph.
*/
int findMinorEmbedding(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig)
{

  int ordering[num_vertices(*H)];
  for (int i = 0; i < num_vertices(*H); i++)
  {
    GRAMM->trace("{} ", ordering[i]);
    ordering[i] = i;
  }

  bool done = false;
  bool success = false;
  int iterCount = 0;

  explored.reset();
  float frac;

  while (!done)
  {

    iterCount++;
    GRAMM->debug("\n\n");
    GRAMM->info("***** BEGINNING OUTER WHILE LOOP ***** ITER {}", iterCount);

    // Not using randomize sorting:
    //randomizeList(ordering, num_vertices(*H));
    
    // Sorting the nodes of H according to the size (number of vertices) of their vertex model
    sortList(ordering, num_vertices(*H));

    for (int k = 0; k < num_vertices(*H); k++)
    {
      int y = ordering[k];
      
      if(hNames[y] == "NULL")
        continue;
      
      GRAMM->debug("\n");
      GRAMM->debug("--------------------- New Vertices Mapping Start ---------------------------");
      GRAMM->debug("Finding vertex model for: {} with Current vertex-size: {}", hNames[y], (*Trees)[y].nodes.size());

      findMinVertexModel(G, H, y, hConfig, gConfig);
    } // for k

    int TO = totalOveruse(G);

    if (iterCount >= 2)
    {
      frac = (float)TO / totalUsed(G);
      GRAMM->info("FRACTION OVERLAP: {}", frac);
      if (TO == 0)
      {
        GRAMM->info("SUCCESS! :: [iterCount] :: {} :: [frac] :: {} :: [num_vertices(H)] :: {}", iterCount, frac, num_vertices(*H));
        done = true;
        success = true;
      }
    }

    adjustHistoryCosts(G);

    if (iterCount > maxIterations) // limit the iteration count to ~40 iterations!
    {
      GRAMM->error("FAILURE. OVERUSED: {} USED: {}", TO, totalUsed(G));
      done = true;
    }

    if (success)
    {
      return 1;
    }

    PFac *= 1.1; // adjust present congestion penalty
    HFac *= 2;   // adjust history of congestion penalty

  } // while !done

  return 0;
}

//---------------------------------------------------------------------//

/*
  Description: main function
*/
int main(int argc, char **argv)
{

  //--------------- Starting timestamp -------------------------
  /* get start timestamp */
  struct timeval totalTime;
  gettimeofday(&totalTime, NULL);
  uint64_t startTotal = totalTime.tv_sec * (uint64_t)1000000 + totalTime.tv_usec;
  //------------------------------------------------------------

  //--------------------------------------------------------------------//
  //----------------- Setting up required variables --------------------//
  //--------------------------------------------------------------------//

  DirectedGraph H, G;
  boost::dynamic_properties dp(boost::ignore_other_properties);

  //----------------- For [H] --> Application Graph --------------------//
  dp.property("label", boost::get(&DotVertex::H_Name, H));                 //--> [Required] Contains name of the operation in Application graph (ex: Load_0)
  dp.property("opcode", boost::get(&DotVertex::H_Opcode, H));              //--> [Required] Contains the Opcode of the operation (ex: op, const, input and output)
  dp.property("load", boost::get(&EdgeProperty::H_LoadPin, H));            //--> [Required] Edge property describing the loadPin to use for the edge
  dp.property("driver", boost::get(&EdgeProperty::H_DriverPin, H));        //--> [Required] Edge property describing the driverPin to use for the edge
  dp.property("latency", boost::get(&DotVertex::H_Latency, H));            //--> [Required] Contains for latency of the node --> check this as it needs to be between the edges
  dp.property("placementX", boost::get(&DotVertex::H_PlacementX, H));      //--> [Required] Contains the property describing the fixed X location for placing the node
  dp.property("placementY", boost::get(&DotVertex::H_PlacementY, H));      //--> [Required] Contains the property describing the fixed Y location for placing the node

  //---------------- For [G] --> Device Model Graph --------------------//
  dp.property("G_Name", boost::get(&DotVertex::G_Name, G));         //--> [Required] Contains the unique name of the cell in the device model graph.
  dp.property("G_NodeCell", boost::get(&DotVertex::G_NodeCell, G)); //--> [Required] Contains the Node CellType (FuncCell, RouteCell, PinCell)
  dp.property("G_NodeType", boost::get(&DotVertex::G_NodeType, G)); //--> [Required] Contains the NodeType (FuncCell CellType can have NodeTypes as ALU, MEMPORT)
  dp.property("G_VisualX", boost::get(&DotVertex::G_VisualX, G));   //--> [Optional] X location for only visualization purpose.
  dp.property("G_VisualY", boost::get(&DotVertex::G_VisualY, G));   //--> [Optional] Y location for only visualization purpose.

  // gConfig and hConfig contains the configuration information about the particular node.
  std::map<int, NodeConfig> gConfig, hConfig;

  //--------------------------------------------------------------------//
  //---------------------- Command line arguments ----------------------//
  //--------------------------------------------------------------------//

  po::variables_map vm; // Value map

  // Input variables:
  std::string deviceModelFile; // Device Model graph file
  std::string applicationFile; // Application Model graph file
  std::string configFile;      // Config file
  int seed_value;              // Seed number
  int verbose_level;           // Verbosity level => [0: info], [1: debug], [2: trace]

  po::options_description desc("[GRAMM] allowed options =");
  desc.add_options()("help,h", "Print help messages")("seed", po::value<int>(&seed_value)->required(), "Seed for the run")("verbose_level", po::value<int>(&verbose_level)->required(), "0: info, 1: debug, 2: trace")("dfile", po::value<std::string>(&deviceModelFile)->required(), "Device model file")("afile", po::value<std::string>(&applicationFile)->required(), "Application graph file")("config", po::value<std::string>(&configFile)->required(), "GRAMM config file");

  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 0;
  }

  po::notify(vm);

  //---------------------- setting up the input variables --------------//

  if (verbose_level == 0)
    GRAMM->set_level(spdlog::level::info);  // Set global log level to debug
  else if (verbose_level == 1)
    GRAMM->set_level(spdlog::level::debug); // Set global log level to debug
  else if (verbose_level == 2)
    GRAMM->set_level(spdlog::level::trace); // Set global log level to debug

  // Seed setup:
  srand(seed_value);

  // Config file parsing:
  if (!configFile.empty())
  {
    std::ifstream f(configFile);
    jsonParsed = json::parse(f);                            // Parsing the Json file.
    GRAMM->info("Parsed JSON file {} ", jsonParsed.dump()); // Printing JSON file for the info purpose.
    jsonUppercase(jsonParsed);                              // Normalizing the JSON to uppercase letters.
    GRAMM->info("Normalized JSON {} ", jsonParsed.dump()); // Printing JSON file for the info purpose.
  }

  //--------------------------------------------------------------------//
  //----------------- STEP 0 : READING DEVICE MODEL --------------------//
  //--------------------------------------------------------------------//

  std::ifstream dFile;                       // Defining the input file stream for device model dot file
  dFile.open(deviceModelFile);               // Passing the device_Model_dot file as an argument!
  readDeviceModelPragma(dFile, GrammConfig); // Reading the device model pragma from the device-model dot file.
  boost::read_graphviz(dFile, G, dp);        // Reading the dot file
  readDeviceModel(&G, &gConfig);             // Reading the device model file.

  //--------------------------------------------------------------------//
  //----------------- STEP 1: READING APPLICATION DOT FILE -------------//
  //--------------------------------------------------------------------//

  // read the DFG from a file
  std::ifstream iFile;                            // Defining the input file stream for application_dot file
  iFile.open(applicationFile);                    // Passing the application_dot file as an argument!
  readApplicationGraphPragma(iFile, GrammConfig); // Reading the application-graph pragma from the device-model dot file.
  boost::read_graphviz(iFile, H, dp);             // Reading the dot file
  readApplicationGraph(&H, &hConfig);             // Reading the Application graph file.

  //--------------------------------------------------------------------//
  //----------------- STEP 2: DRC Verification -------------------------//
  //--------------------------------------------------------------------//
  double secondsDRC;
  secondsDRC = runDRC(&H, &G, &hConfig, &gConfig);

  //--------------------------------------------------------------------//
  //--------- STEP 3: Initializing the mapping-datastructures ----------//
  //--------------------------------------------------------------------//

  Trees = new std::vector<RoutingTree>(num_vertices(H));    // routing trees for every node in H
  Users = new std::vector<std::list<int>>(num_vertices(G)); // for every node in device model G track its users
  HistoryCosts = new std::vector<int>(num_vertices(G));     // for history of congestion in PathFinder
  TraceBack = new std::vector<int>(num_vertices(G));

  for (int i = 0; i < num_vertices(G); i++)
  {
    (*Users)[i].clear();
    (*HistoryCosts)[i] = 0;
    (*TraceBack)[i] = -1;
  }

  for (int i = 0; i < num_vertices(H); i++)
  {
    (*Trees)[i].parent.clear();
    (*Trees)[i].children.clear();
    (*Trees)[i].nodes.clear();
    invUsers[i] = -1;
  }

  //--------------------------------------------------------------------//
  //----------------- STEP 4: Finding minor embedding ------------------//
  //--------------------------------------------------------------------//

  //--------------- Starting timestamp -------------------------
  /* get start timestamp */
  struct timeval grammTime;
  gettimeofday(&grammTime, NULL);
  uint64_t startGramm = grammTime.tv_sec * (uint64_t)1000000 + grammTime.tv_usec;
  //------------------------------------------------------------

  int success = findMinorEmbedding(&H, &G, &hConfig, &gConfig);

  if (success)
  {
    // Printing vertex model:
    printVertexModels(&H, &G, &hConfig, GrammConfig, invUsers);

    // Visualizing mapping result in neato:
    printMappedResults(&H, &G, &hConfig, &gConfig, GrammConfig);
  }

  //--------------- get elapsed time -------------------------
  GRAMM->info("Total time taken for [DRC] :: {} Seconds", secondsDRC);
  //------------------------------------------------------------

  //--------------- get elapsed time -------------------------
  gettimeofday(&grammTime, NULL);
  uint64_t endGramm = grammTime.tv_sec * (uint64_t)1000000 + grammTime.tv_usec;
  uint64_t elapsedGramm = endGramm - startGramm;
  double secondsGramm = static_cast<double>(elapsedGramm) / 1000000.0;
  GRAMM->info("Total time taken for [mapping] :: {} Seconds", secondsGramm);
  //------------------------------------------------------------

  //--------------- get elapsed time -------------------------
  gettimeofday(&totalTime, NULL);
  uint64_t endTotal = totalTime.tv_sec * (uint64_t)1000000 + totalTime.tv_usec;
  uint64_t elapsedTotal = endTotal - startTotal;
  double secondsTotal = static_cast<double>(elapsedTotal) / 1000000.0;
  GRAMM->info("Total time taken for [GRAMM]:: {} Seconds", secondsTotal);
  //------------------------------------------------------------

  return 0;
}
