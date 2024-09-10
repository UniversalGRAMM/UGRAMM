//===================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                       //
// file : GRAMM.cpp                                                  //
// description: contains primary functions                           //
//===================================================================//

#include "../lib/GRAMM.h"
#include "../lib/utilities.h"
#include "../lib/routing.h"

// Declartion of variables:

// CGRA device model parameters:
int numR;    // Number of rows in the CGRA architecture
int numC;    // Number of columns in the CGRA architecture
int CGRAdim; // CGRA architecture dimension (3 means --> 3x3)

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

std::map<std::string, std::vector<std::string>> GrammConfig; // Gramm config parsed from Pragma

// Defining the JSON for the config file:
json jsonParsed;

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
        // GRAMM->debug("Picking up random node :: gNames - {} :: users - {} :: hNames - {} ", gNames[chooseRand], (*Users)[chooseRand].size(), hNames[y]);
        //  Finding the output Pin for the selected FunCell:
        selectedCellOutputPin = findOutputPinFromFuncell(chooseRand, G);
        vertex_found = true;
      }
    }

    GRAMM->debug("[RandomSelection] for application node :: {} :: Choosing starting vertex model as :: {} :: {}", hNames[y], gNames[chooseRand], gNames[selectedCellOutputPin]);

    // OB: In pin to pin mapping: adding outPin in the routing tree instead of the driver FunCell
    //(*Trees)[y].nodes.push_back(chooseRand);
    (*Trees)[y].nodes.push_back(selectedCellOutputPin);
    (*Users)[chooseRand].push_back(y); // Users and history cost is primarily tracked for FunCell nodes.
    (*Users)[selectedCellOutputPin].push_back(y);
    invUsers[y] = chooseRand;

    return 0;
  }

  // OK, at least one of y's fanins or fanouts have a vertex model
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

    ripUpRouting(y, G);
    // Checking if the current node 'y' has any Fanouts to be routed.
    (*Trees)[y].nodes.push_back(outputPin);
    (*Users)[i].push_back(y); // Users and history cost is primarily tracked for FunCell nodes.
    (*Users)[outputPin].push_back(y);
    invUsers[y] = i;

    // Cost and history costs are as well added for the FunCell instead of the PinCell.
    totalCosts[i] += (1 + (*HistoryCosts)[i]) * ((*Users)[i].size() * PFac);

    if (GRAMM->level() <= spdlog::level::trace)
    {
      printRouting(y);
    }

    totalCosts[i] += routeSignal(G, H, y, gConfig);
    GRAMM->debug("For application node {} :: routing for location [{}] has cost {}", hNames[y], gNames[outputPin], totalCosts[i]);

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

      int driverPinNode = findDriver(driver, gConfig);

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
  (*Trees)[y].nodes.push_back(bestIndexPincell);
  (*Users)[bestIndexFuncell].push_back(y); // Users and history cost is primarily tracked for FunCell nodes.
  (*Users)[bestIndexPincell].push_back(y);
  invUsers[y] = bestIndexFuncell;

  // Final-placement for node 'y':
  routeSignal(G, H, y, gConfig);

  // Final routing all of the inputs of the node 'y':
  boost::tie(ei, ei_end) = in_edges(yD, *H);
  for (; ei != ei_end; ei++)
  {
    int driver = source(*ei, *H);
    if ((*Trees)[driver].nodes.size() == 0)
      continue; // driver is not placed

    int driverPinNode = findDriver(driver, gConfig);

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

/*
  findMinorEmbedding() => determine if H is a minor of G
*/
int findMinorEmbedding(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig)
{

  int ordering[num_vertices(*H)]; // presently unused
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

    // randomizeList(ordering, num_vertices(*H));
    sortList(ordering, num_vertices(*H));

    for (int k = 0; k < num_vertices(*H); k++)
    {
      int y = ordering[k];

      GRAMM->trace(" Condition :: {} :: Type :: {} ", doPlacement((*hConfig)[y].Opcode, jsonParsed), (*hConfig)[y].Opcode);
      if (doPlacement((*hConfig)[y].Opcode, jsonParsed))
        continue;

      GRAMM->debug("\n");
      GRAMM->debug("--------------------- New Vertices Mapping Start ---------------------------");
      GRAMM->debug("Finding vertex model for: {} with Current vertex-size: {}", hNames[y], (*Trees)[y].nodes.size());
      if (computeTopoEnable)
        GRAMM->trace("TOPO ORDER: {}", (*TopoOrder)[y]);

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

int main(int argc, char **argv)
{

  //--------------------------------------------------------------------//
  //----------------- Setting up required variables --------------------//
  //--------------------------------------------------------------------//

  DirectedGraph H, G;
  boost::dynamic_properties dp(boost::ignore_other_properties);

  //----------------- For [H] --> Application Graph --------------------//
  dp.property("label", boost::get(&DotVertex::name, H));          //--> Contains name of the operation in Application graph (ex: Load_0)
  dp.property("opcode", boost::get(&DotVertex::opcode, H));       //--> Contains the Opcode of the operation (ex: op, const, input and output)
  dp.property("load", boost::get(&EdgeProperty::loadPin, H));     //-->Edge property describing the loadPin to use for the edge
  dp.property("driver", boost::get(&EdgeProperty::driverPin, H)); //-->Edge property describing the driverPin to use for the edge

  //---------------- For [G] --> Device Model Graph --------------------//
  dp.property("G_Name", boost::get(&DotVertex::G_Name, G));
  dp.property("G_NodeCell", boost::get(&DotVertex::G_NodeCell, G)); //--> Contains the Node CellType (FuncCell, RouteCell, PinCell)
  dp.property("G_NodeType", boost::get(&DotVertex::G_NodeType, G)); //--> Contains the NodeType (FuncCell CellType can have NodeTypes as ALU, MEMPORT)
  dp.property("G_VisualX", boost::get(&DotVertex::G_VisualX, G));   //--> X location for only visualization purpose.
  dp.property("G_VisualY", boost::get(&DotVertex::G_VisualY, G));   //--> Y location for only visualization purpose.

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
  desc.add_options()("help,h", "Print help messages")("num_rows,nr,NR", po::value<int>(&numR)->required(), "Number of rows")("num_cols,nc,NC", po::value<int>(&numC)->required(), "Number of Columns")("seed", po::value<int>(&seed_value)->required(), "Seed for the run")("verbose_level", po::value<int>(&verbose_level)->required(), "0: info, 1: debug, 2: trace")("dfile", po::value<std::string>(&deviceModelFile)->required(), "Device model file")("afile", po::value<std::string>(&applicationFile)->required(), "Application graph file")("config", po::value<std::string>(&configFile)->required(), "GRAMM config file");

  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 0;
  }

  po::notify(vm);

  //---------------------- setting up the input variables --------------//

  if (verbose_level == 0)
    GRAMM->set_level(spdlog::level::info); // Set global log level to debug
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
    jsonUppercase(jsonParsed);                              // Normalizing the JSON to uppercase letters.
    GRAMM->info("Parsed JSON file {} ", jsonParsed.dump()); // Printing JSON file for the info purpose.
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
  //--------- STEP 2: Initializing the mapping-datastructures ----------//
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

  /*
   ** compute a topological order
    -->  We also experimented with sorting the nodes of H according to their topological distancwe from a CGRA I/O or memory port; however, we found that this technique did not improve results beyond the ordering by the size of the vertex models.
  */
  // if (computeTopoEnable)
  //   computeTopo(&H, &hConfig); // not presently used

  //--------------------------------------------------------------------//
  //----------------- STEP 3: Finding minor embedding ------------------//
  //--------------------------------------------------------------------//

  //--------------- Starting timestamp -------------------------
  /* get start timestamp */
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t start = tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
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
  gettimeofday(&tv, NULL);
  uint64_t end = tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
  uint64_t elapsed = end - start;
  double seconds = static_cast<double>(elapsed) / 1000000.0;
  GRAMM->info("Total time taken :: {} Seconds", seconds);
  //------------------------------------------------------------

  return 0;
}
