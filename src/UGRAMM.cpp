//===================================================================//
// Universal GRAaph Minor Mapping (UGRAMM) method for CGRA           //
// file : UGRAMM.cpp                                                 //
// description: contains primary functions                           //
//===================================================================//

#include "../lib/UGRAMM.h"
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
std::map<std::string, int> hNamesInv;
std::map<int, std::string> gNames;
std::map<std::string, int> gNamesInv;
std::map<std::string, int> gNamesInv_FuncCell;
std::set<int> fullyLockedNodes;
std::bitset<100000> explored;

std::vector<std::string> inPin = {"inPinA", "inPinB", "anyPins"};
std::vector<std::string> outPin = {"outPinA"};

// Logger & CLI global variables:
std::shared_ptr<spdlog::logger> UGRAMM = spdlog::stdout_color_mt("UGRAMM");
namespace po = boost::program_options;

//ugrammConfig structure which is parsed from Pragma's of Device and application graph
std::map<std::string, std::vector<std::string>> ugrammConfig; 

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
    if (invUsers[source(*ei, *G)] != NOT_PLACED) //invUsers hold the information whether the current HID is placed or not
      allEmpty = false;
  }

  out_edge_iterator eo, eo_end;
  boost::tie(eo, eo_end) = out_edges(yD, *H);
  for (; eo != eo_end; eo++)
  {
    if (invUsers[target(*eo, *H)] != NOT_PLACED) //invUsers hold the information whether the current HID is placed or not
      allEmpty = false;
  }

  if (allEmpty)
  {
    // choose a random unused node of G to be the vertex model for y because NONE of its fanins or fanouts have vertex models
    int chooseRand = (rand() % num_vertices(*G));
    bool vertex_found = false;

    while (vertex_found == false)
    {
      chooseRand = (chooseRand + 1) % num_vertices(*G);
      if (compatibilityCheck((*gConfig)[chooseRand].Type, (*hConfig)[y].Opcode))
      {
        vertex_found = true;
      }
    }

    UGRAMM->debug("[RandomSelection] for application node :: {} :: Choosing starting vertex model as :: {}", hNames[y], gNames[chooseRand]);

    //--- InvUsers and Users registering ----//
    invUsers[y] = chooseRand;
    (*Users)[chooseRand].push_back(y);
    //---------------------------------------//

    return 0;
  }

  // At least one of y's fanins or fanouts have a vertex model [not allEmpty cases]
  int bestCost = MAX_DIST;
  int bestIndexFuncCell = -1;

  int totalCosts[num_vertices(*G)];
  for (int i = 0; i < num_vertices(*G); i++)
    totalCosts[i] = 0;

  bool compatibilityStatus = true;

  UGRAMM->trace("[Locking] LockGNode {} :: Not Empty {}", (*hConfig)[y].LockGNode, !(*hConfig)[y].LockGNode.empty());
  bool lockingNodeStatus = !(*hConfig)[y].LockGNode.empty();

  if (lockingNodeStatus){
    std::vector<int> suitableGIDs;

    int GID = findGNodeID_FuncCell((*hConfig)[y].LockGNode, suitableGIDs);
    
    for (int i = 0; i < suitableGIDs.size(); i++){
      UGRAMM->trace("[Locking] hNames[{}] {} :: Lock gNames {} :: GID{} :: verify gNames {}", y, hNames[y], (*hConfig)[y].LockGNode, suitableGIDs[i], gNames[suitableGIDs[i]]);
    }
    
    for (int i = 0; i < suitableGIDs.size(); i++)
    { // first route the signal on the output of y

      int GID = suitableGIDs[i];
      // Confriming the Vertex correct type:
      if (!compatibilityCheck((*gConfig)[GID].Type, (*hConfig)[y].Opcode))
      {
        compatibilityStatus = false;
        continue;
      }

      //------------------ Routing Setup ---------------------//
      ripUpRouting(y, G);         //Ripup the previous routing
      (*Users)[GID].push_back(y);   //Users update                 
      invUsers[y] = GID;            //InvUsers update
      //------------------------------------------------------//

      // Cost and history costs are calculated for the FuncCell:
      totalCosts[GID] += (1 + (*HistoryCosts)[GID]) * ((*Users)[GID].size() * PFac);

      //Placement of the signal Y:
      totalCosts[GID] += routeSignal(G, H, y, gConfig);

      //-------- Debugging statements ------------//
      if (UGRAMM->level() <= spdlog::level::trace)
        printRouting(y);
        UGRAMM->debug("For application node {} :: routing for location [{}] has cost {}", hNames[y], gNames[GID], totalCosts[GID]);
      //------------------------------------------//

      //Early exit if the cost is greater than bestCost:
      if (totalCosts[GID] > bestCost)
        continue;

      //----------------------------------------------
      // now route the signals on the input of y
      //----------------------------------------------

      boost::tie(ei, ei_end) = in_edges(yD, *H);
      for (; (ei != ei_end); ei++)
      {
        int driverHNode = source(*ei, *H);

        if (invUsers[driverHNode] == NOT_PLACED)
          continue; // driver is not placed
        
        int driverGNode = invUsers[driverHNode];

        //------------------ Routing Setup ---------------------//
        ripUpRouting(driverHNode, G);
        (*Users)[driverGNode].push_back(driverHNode); //Users update
        invUsers[driverHNode] = driverGNode;          //InvUsers update
        //------------------------------------------------------//

        // Newfeature: rip up from load: ripUpLoad(G, driver, outputPin);
        totalCosts[GID] += routeSignal(G, H, driverHNode, gConfig);

        UGRAMM->debug("Routing the signals on the input of {}", hNames[y]);
        UGRAMM->debug("For {} -> {} :: {} -> {} has cost {} :: best cost {}", hNames[driverHNode], hNames[y], gNames[driverGNode], gNames[GID], totalCosts[GID], bestCost);

        if (totalCosts[GID] > bestCost)
          break;
      }

      UGRAMM->debug("Total cost for {} is {} :: best cost {}", hNames[y], totalCosts[GID], bestCost);

      if (totalCosts[GID] >= MAX_DIST)
        continue;

      if (totalCosts[GID] < bestCost)
      {
        bestIndexFuncCell = GID;
        bestCost = totalCosts[GID];
        compatibilityStatus = true;
      }
      else if (totalCosts[GID] == bestCost)
      {
        if (!(rand() % 2))
        {
          bestIndexFuncCell = GID;
        }
        compatibilityStatus = true;
      }
    }
  }

  if (!lockingNodeStatus){
    for (int i = 0; i < num_vertices(*G); i++) // don't care for locking
    { // first route the signal on the output of y

      // Confriming the Vertex correct type:
      if (!compatibilityCheck((*gConfig)[i].Type, (*hConfig)[y].Opcode))
      {
        compatibilityStatus = false;
        continue;
      }

      // Skip Fully Locked Nodes
      if (skipFullyLockedNodes){
        if (fullyLockedNodes.find(i) != fullyLockedNodes.end()){
          UGRAMM->info("Skipping locked device model node {} for mapping application graph node {} ", gNames[i], hNames[y]);
          continue;
        }
      }

      //------------------ Routing Setup ---------------------//
      ripUpRouting(y, G);         //Ripup the previous routing
      (*Users)[i].push_back(y);   //Users update                 
      invUsers[y] = i;            //InvUsers update
      //------------------------------------------------------//

      // Cost and history costs are calculated for the FuncCell:
      totalCosts[i] += (1 + (*HistoryCosts)[i]) * ((*Users)[i].size() * PFac);

      //Placement of the signal Y:
      totalCosts[i] += routeSignal(G, H, y, gConfig);

      //-------- Debugging statements ------------//
      if (UGRAMM->level() <= spdlog::level::trace)
        printRouting(y);
      UGRAMM->debug("For application node {} :: routing for location [{}] has cost {}", hNames[y], gNames[i], totalCosts[i]);
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
        int driverHNode = source(*ei, *H);

        if (invUsers[driverHNode] == NOT_PLACED)
          continue; // driver is not placed
        
        int driverGNode = invUsers[driverHNode];

        //------------------ Routing Setup ---------------------//
        ripUpRouting(driverHNode, G);
        (*Users)[driverGNode].push_back(driverHNode); //Users update
        invUsers[driverHNode] = driverGNode;          //InvUsers update
        //------------------------------------------------------//

        // Newfeature: rip up from load: ripUpLoad(G, driver, outputPin);
        totalCosts[i] += routeSignal(G, H, driverHNode, gConfig);

        UGRAMM->debug("Routing the signals on the input of {}", hNames[y]);
        UGRAMM->debug("For {} -> {} :: {} -> {} has cost {}", hNames[driverHNode], hNames[y], gNames[driverGNode], gNames[i], totalCosts[i]);

        if (totalCosts[i] > bestCost)
          break;
      }

      UGRAMM->debug("Total cost for {} is {}\n", hNames[y], totalCosts[i]);

      if (totalCosts[i] >= MAX_DIST)
        continue;

      if (totalCosts[i] < bestCost)
      {
        bestIndexFuncCell = i;
        bestCost = totalCosts[i];
        compatibilityStatus = true;
      }
      else if (totalCosts[i] == bestCost)
      {
        if (!(rand() % 2))
        {
          bestIndexFuncCell = i;
        }
        compatibilityStatus = true;
      }
    }
  }

  if (bestIndexFuncCell == -1)
  {
    UGRAMM->error("FATAL ERROR -- COULD NOT FIND A VERTEX MODEL FOR VERTEX {} {}", y, hNames[y]);
    if (!compatibilityStatus)
      UGRAMM->error("Nodes in the device model does not supports {} Opcode", (*hConfig)[y].Opcode);
    exit(-1);
  }

  UGRAMM->debug("The bertIndex found for {} from the device-model is {} with cost of {}", hNames[y], gNames[bestIndexFuncCell], bestCost);

  // Final rig-up before doing final routing:

  //------------- Final Routing Setup ---------------------//
  ripUpRouting(y, G);         //Ripup the previous routing
  invUsers[y] = bestIndexFuncCell;           //InvUsers update
  (*Users)[bestIndexFuncCell].push_back(y);  //Users update                 
  //------------------------------------------------------//

  // Final-placement for node 'y':
  routeSignal(G, H, y, gConfig);

  // Final routing all of the inputs of the node 'y':
  boost::tie(ei, ei_end) = in_edges(yD, *H);
  for (; ei != ei_end; ei++)
  {
    int driverHNode = source(*ei, *H);

    if (invUsers[driverHNode] == NOT_PLACED)
      continue; // driver is not placed

    int driverGNode = invUsers[driverHNode];

    //------------------ Routing Setup ---------------------//
    ripUpRouting(driverHNode, G);
    (*Users)[driverGNode].push_back(driverHNode); //Users update
    invUsers[driverHNode] = driverGNode;          //InvUsers update
    //------------------------------------------------------//

    routeSignal(G, H, driverHNode, gConfig);
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
    ordering[i] = i;
    UGRAMM->trace("Ordering[{}]: {} ", i, ordering[i]);
  }

  bool done = false;
  bool success = false;
  int iterCount = 0;

  explored.reset();
  float frac;

  while (!done)
  {

    iterCount++;
    UGRAMM->debug("\n\n");
    UGRAMM->info("***** BEGINNING OUTER WHILE LOOP ***** ITER {}", iterCount);

    // Not using randomize sorting:
    //randomizeList(ordering, num_vertices(*H));
    
    // Sorting the nodes of H according to the size (number of vertices) of their vertex model
    sortList(ordering, num_vertices(*H), hConfig);
    for (int i = 0; i < num_vertices(*H); i++){
      UGRAMM->info("Afer sortlist (sort) Interation {} | Ordering[{}]: {} | hNames[{}]: {}", iterCount, i, ordering[i], ordering[i], hNames[ordering[i]]);
    }

    for (int k = 0; k < num_vertices(*H); k++)
    {
      int y = ordering[k];
      
      if(hNames[y] == "NULL")
        continue;
      
      UGRAMM->debug("\n");
      UGRAMM->debug("--------------------- New Vertices Mapping Start ---------------------------");
      UGRAMM->debug("Finding vertex model for: {} with Current vertex-size: {}", hNames[y], (*Trees)[y].nodes.size());

      findMinVertexModel(G, H, y, hConfig, gConfig);
    } // for k

    int TO = totalOveruse(G);

    if (iterCount >= 2)
    {
      frac = (float)TO / totalUsed(G);
      UGRAMM->info("FRACTION OVERLAP: {}", frac);
      if (TO == 0)
      {
        UGRAMM->info("SUCCESS! :: [iterCount] :: {} :: [frac] :: {} :: [num_vertices(H)] :: {}", iterCount, frac, num_vertices(*H));
        done = true;
        success = true;
      }
    }

    adjustHistoryCosts(G);

    if (iterCount > maxIterations) // limit the iteration count to ~40 iterations!
    {
      UGRAMM->error("FAILURE. OVERUSED: {} USED: {}", TO, totalUsed(G));
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
  dp.property("lockGNode", boost::get(&DotVertex::H_LockGNode, H));        //--> [Required] Contains the property describing the fixed X location for placing the node

  //---------------- For [G] --> Device Model Graph --------------------//
  dp.property("G_Name", boost::get(&DotVertex::G_Name, G));         //--> [Required] Contains the unique name of the cell in the device model graph.
  dp.property("G_CellType", boost::get(&DotVertex::G_CellType, G)); //--> [Required] Contains the Opcode of the CellType (FuncCell, RouteCell, PinCell)
  dp.property("G_NodeType", boost::get(&DotVertex::G_NodeType, G)); //--> [Required] Contains the NodeType of Device Model Graph (For example "ALU" for CellType "FuncCell") 
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

  po::options_description desc("[UGRAMM] allowed options =");
  desc.add_options()("help,h", "Print help messages")("seed", po::value<int>(&seed_value)->required(), "Seed for the run")("verbose_level", po::value<int>(&verbose_level)->required(), "0: info, 1: debug, 2: trace")("dfile", po::value<std::string>(&deviceModelFile)->required(), "Device model file")("afile", po::value<std::string>(&applicationFile)->required(), "Application graph file")("config", po::value<std::string>(&configFile)->required(), "UGRAMM config file");

  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 0;
  }

  po::notify(vm);

  //---------------------- setting up the input variables --------------//

  if (verbose_level == 0)
    UGRAMM->set_level(spdlog::level::info);  // Set global log level to debug
  else if (verbose_level == 1)
    UGRAMM->set_level(spdlog::level::debug); // Set global log level to debug
  else if (verbose_level == 2)
    UGRAMM->set_level(spdlog::level::trace); // Set global log level to debug

  // Seed setup:
  srand(seed_value);

  // Config file parsing:
  if (!configFile.empty())
  {
    std::ifstream f(configFile);
    jsonParsed = json::parse(f);                            // Parsing the Json file.
    UGRAMM->info("Parsed JSON file {} ", jsonParsed.dump()); // Printing JSON file for the info purpose.
    jsonUppercase(jsonParsed);                              // Normalizing the JSON to uppercase letters.
    UGRAMM->info("Normalized JSON {} ", jsonParsed.dump()); // Printing JSON file for the info purpose.
  }

  //--------------------------------------------------------------------//
  //----------------- STEP 0 : READING DEVICE MODEL --------------------//
  //--------------------------------------------------------------------//

  std::ifstream dFile;                       // Defining the input file stream for device model dot file
  dFile.open(deviceModelFile);               // Passing the device_Model_dot file as an argument!
  readDeviceModelPragma(dFile, ugrammConfig); // Reading the device model pragma from the device-model dot file.
  boost::read_graphviz(dFile, G, dp);        // Reading the dot file
  readDeviceModel(&G, &gConfig);             // Reading the device model file.

  //--------------------------------------------------------------------//
  //----------------- STEP 1: READING APPLICATION DOT FILE -------------//
  //--------------------------------------------------------------------//

  // read the DFG from a file
  std::ifstream iFile;                            // Defining the input file stream for application_dot file
  iFile.open(applicationFile);                    // Passing the application_dot file as an argument!
  readApplicationGraphPragma(iFile, ugrammConfig); // Reading the application-graph pragma from the device-model dot file.
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
    printVertexModels(&H, &G, &hConfig, ugrammConfig, invUsers);

    // Visualizing mapping result in neato:
    printMappedResults(&H, &G, &hConfig, &gConfig, ugrammConfig);
  }

  //--------------- get elapsed time -------------------------
  UGRAMM->info("Total time taken for [DRC] :: {} Seconds", secondsDRC);
  //------------------------------------------------------------

  //--------------- get elapsed time -------------------------
  gettimeofday(&grammTime, NULL);
  uint64_t endGramm = grammTime.tv_sec * (uint64_t)1000000 + grammTime.tv_usec;
  uint64_t elapsedGramm = endGramm - startGramm;
  double secondsGramm = static_cast<double>(elapsedGramm) / 1000000.0;
  UGRAMM->info("Total time taken for [mapping] :: {} Seconds", secondsGramm);
  //------------------------------------------------------------

  //--------------- get elapsed time -------------------------
  gettimeofday(&totalTime, NULL);
  uint64_t endTotal = totalTime.tv_sec * (uint64_t)1000000 + totalTime.tv_usec;
  uint64_t elapsedTotal = endTotal - startTotal;
  double secondsTotal = static_cast<double>(elapsedTotal) / 1000000.0;
  UGRAMM->info("Total time taken for [UGRAMM]:: {} Seconds", secondsTotal);
  //------------------------------------------------------------

  return 0;
}
