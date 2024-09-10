//===================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                       //
// file : GRAMM.cpp                                                  //
// description: contains primary functions                           //
//===================================================================//

#include "../lib/GRAMM.h"
#include "../lib/utilities.h"

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

//Defining the JSON for the config file:
json jsonParsed;

/*
  Description: this function checks whether current opcode required by the
               application node is supported by device model node.
*/
bool compatibilityCheck(const std::string &gType, const std::string &hOpcode)
{
  // For nodes such as RouteCell, PinCell the GrammConfig array will be empty.
  // Pragma array's are only parsed for the FunCell types.
  if (GrammConfig[gType].size() == 0)
    return false;

  // Finding Opcode required by the application graph supported by the device model or not.
  for (const auto pair : GrammConfig[gType])
  {
    if (pair == hOpcode)
    {
      GRAMM->debug("{} node from device model supports {} Opcode", gType, hOpcode);
      return true;
    }
  }
  // GRAMM->error("{} node from device model does not supports {} Opcode", gType, hOpcode);
  return false;
}

/*
  Description: For the given FunCell (signal), finds the associated outputPin node from the deviceModel.
  Ex: FunCell(signal) -> outPin (selectedCellOutputPin) [Walking downhill --- finding the sink of this edge]
*/
int findOutputPinFromFuncell(int signal, DirectedGraph *G)
{
  vertex_descriptor signalVertex = vertex(signal, *G);
  out_edge_iterator eo, eo_end;
  boost::tie(eo, eo_end) = out_edges(signalVertex, *G);
  int selectedCellOutputPin = target(*eo, *G);

  return selectedCellOutputPin;
}

void depositRoute(int signal, std::list<int> *nodes)
{
  if (!nodes->size())
  {
    std::cout << "NOTHING TO DEPOSIT\n";
    return;
  }
  std::list<int>::reverse_iterator it = (*nodes).rbegin();
  struct RoutingTree *RT = &((*Trees)[signal]);
  int prev = *it; // first should should already be part of the route
  //  std::cout << "PATH:\n";
  //  std::cout << "USERS: " << (*Users)[prev].size() << " ";
  //  printName(prev);
  it++;
  for (; it != (*nodes).rend(); it++)
  {
    int addNode = *it;
    //    std::cout << "USERS: " << (*Users)[addNode].size() << " ";
    //    printName(addNode);
    (*Users)[addNode].push_back(signal);
    RT->children[prev].push_back(addNode);
    RT->parent[addNode] = prev;
    RT->nodes.push_back(addNode);
    prev = addNode;
  }
}

int findDriver(int signal, std::map<int, NodeConfig> *gConfig)
{
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int>::iterator it = RT->nodes.begin();

  // find the node with no parent
  for (; it != RT->nodes.end(); it++)
  {
    if (!RT->parent.count(*it))
    {
      if ((*gConfig)[*it].Cell != "PINCELL")
      {
        GRAMM->error("FATAL ERROR -- For signal {}, driverNode is not a PinCell!!", *it);
        exit(-1);
      }
      return *it;
    }
  }
  return -1;
}

int getSignalCost(int signal)
{
  int cost = 0;
  struct RoutingTree *RT = &((*Trees)[signal]);

  std::list<int>::iterator it = RT->nodes.begin();
  for (; it != RT->nodes.end(); it++)
  {
    cost += (1 << (*Users)[*it].size());
  }
  return cost;
}

bool hasOverlap(int signal)
{

  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int>::iterator it = RT->nodes.begin();

  for (; it != RT->nodes.end(); it++)
    if ((*Users)[*it].size() > 1)
      return true;
  return false;
}

#if 0 // JANDERS NEEDS WORK 
void ripUpToFork(int signal, int node) {
  struct RoutingTree *RT = &((*Trees)[signal]);
  int current = node;
  
  if (find(RT->nodes.begin(), RT->nodes.end(), node) == RT->nodes.end()) // node not found
    return;

  std::list<int> toDel;
  toDel.clear();
  
  while (RT->children.count(current) <= 1) {

    if (!RT->parent.count(current))
      break; // reached the driver -- don't remove it         
    
    toDel.push_back(current);
    current = RT->parent[current];
  }
  ripup(signal, &toDel);
}
#endif

#if 0
void ripUpLoad(DirectedGraph *G, int signal, int load) {
  struct RoutingTree *RT = &((*Trees)[signal]);

  bool found = true;

  while (found) {
    found = false;
    std::list<int>::iterator it = RT->nodes.begin();
    for (; it != RT->nodes.end(); it++) {
      int n = *it;
      vertex_descriptor nD = vertex(n, *G);
      out_edge_iterator eo, eo_end;
      boost::tie(eo, eo_end) = out_edges(nD, *G);
      for (; eo != eo_end; eo++) {
	int next = target(*eo, *G);
	if (find((*Users)[next].begin(), (*Users)[next].end(), load) != (*Users)[next].end()) {
	  ripUpToFork(signal, n);
	  found = true;
	}
      }
    }
  }
}
#endif

// For reference, got this from routeSignal function
// route(G, H, y, loadLoc, &path, gConfig);
int route(DirectedGraph *G, int signal, int sink, std::list<int> *route, std::map<int, NodeConfig> *gConfig)
{

  route->clear(); // Clearing the route list.

  std::vector<int> expInt;
  expInt.clear();

  struct RoutingTree *RT = &((*Trees)[signal]);

  struct ExpNode
  {
    int i;
    int cost;
  };

  struct customLess
  {
    bool operator()(const struct ExpNode &l, const struct ExpNode &r) const { return l.cost > r.cost; }
  };

  std::priority_queue<struct ExpNode, std::vector<struct ExpNode>, customLess> PRQ;

  std::list<int>::iterator it = RT->nodes.begin();
  for (; it != RT->nodes.end(); it++)
  {
    int rNode = *it;
    struct ExpNode eNode;
    eNode.cost = 0;
    eNode.i = rNode;

    if (DEBUG)
      std::cout << "EXPANSION SOURCE : " << gNames[rNode] << "\n";

    explored.set(rNode);
    expInt.push_back(rNode);
    PRQ.push(eNode);
  }

  //  std::cout << "EXPANSION TARGET: \n";
  //  printName(sink);

  struct ExpNode popped;
  bool hit = false;
  while (!PRQ.empty() && !hit)
  {
    popped = PRQ.top();
    PRQ.pop();
    //    printName(popped.i);
    //    std::cout << "PRQ POP COST: " << popped.cost << "\n";
    if ((popped.cost > 0) &&
        (popped.i == sink))
    { // cost must be higher than 0 (otherwise sink was part of initial expansion list)
      hit = true;
      break;
    }

    vertex_descriptor vPopped = vertex(popped.i, *G);
    out_edge_iterator eo_G, eo_end_G;
    boost::tie(eo_G, eo_end_G) = out_edges(vPopped, *G);
    for (; eo_G != eo_end_G; eo_G++)
    {
      vertex_descriptor next = target(*eo_G, *G);
      if (explored.test(next))
        continue;

      explored.set(next);
      expInt.push_back(next);

      // Verifying if the node is type RouteCell or PinCell as ONLY they can be used along the way for routing
      if ((next != sink) && (*gConfig)[next].Cell != "ROUTECELL" && (*gConfig)[next].Cell != "PINCELL")
        continue;

      struct ExpNode eNodeNew;
      eNodeNew.i = next;
      //      eNodeNew.cost = popped.cost + (4 << (*Users)[next].size());
      //      eNodeNew.cost = popped.cost + (4 << (*Users)[next].size()) + (1 << (*HistoryCosts)[next]);
      eNodeNew.cost = popped.cost + (1 + (*HistoryCosts)[next]) * (1 + (*Users)[next].size() * PFac);
      //      if ((*HistoryCosts)[next] >= 2) {
      //	eNodeNew.cost = MAX_DIST;
      //	(*HistoryCosts)[next] = 0;
      //      }
      (*TraceBack)[next] = popped.i;
      if ((eNodeNew.cost > 0) && (next == sink))
      { // JANDERS fast targeting
        popped = eNodeNew;
        hit = true;
        break;
      }
      PRQ.push(eNodeNew);
    }
  }

  int retVal;

  if (hit)
  {
    int current = popped.i;
    //  std::cout << "CURRENT: " << current << "\n";
    while (1)
    {
      route->push_back(current);
      if ((*TraceBack)[current] == -1)
        break;
      current = (*TraceBack)[current];
    }

    retVal = popped.cost;
  }
  else // if (!hit)
    retVal = MAX_DIST;

  for (int i = 0; i < expInt.size(); i++)
  {
    explored.reset(expInt[i]);
    (*TraceBack)[expInt[i]] = -1;
  }

  return retVal;
}

int totalUsed(DirectedGraph *G)
{

  int total = 0;
  for (int i = 0; i < num_vertices(*G); i++)
  {
    int temp = (*Users)[i].size();
    if (temp)
      total++;
  }
  return total;
}

int totalOveruse(DirectedGraph *G)
{

  int total = 0;
  for (int i = 0; i < num_vertices(*G); i++)
  {
    int temp = (*Users)[i].size() - 1;
    //    total += (temp >= 0) ? temp : 0;
    total += (temp > 0) ? 1 : 0;

    if (temp > 0)
    {
      GRAMM->debug("{} OVERUSE ON {} with following users:", temp, gNames[i]);
      for (int value : (*Users)[i])
      {
        GRAMM->debug("{}", hNames[value]);
      }
    }
  }

  GRAMM->info("TOTAL OVERUSE: {}", total);

  return total;
}

void adjustHistoryCosts(DirectedGraph *G)
{
  for (int i = 0; i < num_vertices(*G); i++)
  {
    int temp = (*Users)[i].size() - 1;
    temp = (temp >= 0) ? temp : 0;
    if (temp > 0)
    {
      (*HistoryCosts)[i] = (*HistoryCosts)[i] + 1;
    }
  }
}

int cmpfunc(const void *a, const void *b)
{

  int aI = *((int *)a);
  int bI = *((int *)b);

  // int ret = (*TopoOrder)[aI] - (*TopoOrder)[bI];

  int ret = ((*Trees)[bI].nodes.size() - (*Trees)[aI].nodes.size());
  if (ret == 0)
    ret = (rand() % 3 - 1);
  return ret;
}

void sortList(int *list, int n)
{
  qsort(list, n, sizeof(int), cmpfunc);
}

void randomizeList(int *list, int n)
{

  // do 3n random swaps
  for (int i = 0; i < 3 * n; i++)
  {
    int j = rand() % n;
    int k = rand() % n;
    if (j == k)
      j = (j + 1) % n;

    int temp;
    temp = list[j];
    list[j] = list[k];
    list[k] = temp;
  }
}

int routeSignal(DirectedGraph *G, DirectedGraph *H, int y, std::map<int, NodeConfig> *gConfig)
{
  GRAMM->trace("BEGINNING ROUTE OF NAME : {}", hNames[y]);

  vertex_descriptor yD = vertex(y, *H);
  int totalCost = 0;

  // i is a candidate root for signal y
  out_edge_iterator eo, eo_end;
  boost::tie(eo, eo_end) = out_edges(yD, *H);
  for (; eo != eo_end; eo++)
  {
    int load = target(*eo, *H);
    std::string loadPin = boost::get(&EdgeProperty::loadPin, *H, *eo);
    std::string driverPin = boost::get(&EdgeProperty::driverPin, *H, *eo);

    if (load == y)
      continue; // JANDERS ignore feedback connections for the moment

    if ((*Trees)[load].nodes.size() == 0)
      continue; // load should be placed for the routing purpose!!

    // Since driver will always be the outPin of the FunCell, the type of loadOutPinCellLoc will be "pinCell"
    // int loadOutPinCellLoc = findDriver(load, gConfig);

    // Converting the driver(pinCell) to the actual input pin of FuncCell:
    // Step 1: Fetching "FunCell" ID from the edge: FunCell --> OutPin
    // int loadFunCellLoc = findFunCellFromOutputPin(loadOutPinCellLoc, G);
    int loadFunCellLoc = invUsers[load];

    // Step 2: Fetching given "inPin" ID from the edge: inPin --> FunCell (Note: there could be multiple input pins for the given FunCell)
    vertex_descriptor yD = vertex(loadFunCellLoc, *G);
    in_edge_iterator ei, ei_end;
    boost::tie(ei, ei_end) = in_edges(yD, *G);
    int loadInPinCellLoc = 0;
    for (; ei != ei_end; ei++)
    {
      int source_id = source(*ei, *G);
      if ((*gConfig)[source_id].loadPin == loadPin)
        loadInPinCellLoc = source_id;
    }

    GRAMM->trace("SOURCE : {} TARGET : {} TARGET-Pin : {}", hNames[y], hNames[load], loadPin);
    GRAMM->trace("ROUTING LOAD: {} :: (InputPin) :: {}", gNames[loadFunCellLoc], gNames[loadInPinCellLoc]);

    if (loadInPinCellLoc < 0)
    {
      GRAMM->error("SIGNAL WITHOUT A DRIVER.\n");
      exit(-1);
    }

    int cost;
    std::list<int> path;

    cost = route(G, y, loadInPinCellLoc, &path, gConfig);

    totalCost += cost;

    if (cost < MAX_DIST)
    {
      // Earlier in Func to Func Mapping, we used to remove the driver Function:
      // path.remove(loadPinCellLoc);
      depositRoute(y, &path);
    }
    else
    {
      // uncommenting this
      //  OB:  std::cout << "NAME :" << hNames[y] << " LOAD: \n";
      //  OB:      printName(loadLoc);
    }
  }
  return totalCost;
}

int findMinVertexModel(DirectedGraph *G, DirectedGraph *H, int y,
                       std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig)
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
        if ((*Users)[chooseRand].size() == 0)
          vertex_found = true;
      }
    }

    GRAMM->debug("For application node :: {} :: Choosing starting vertex model as :: {} :: {}", hNames[y], gNames[chooseRand], gNames[selectedCellOutputPin]);

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

/*
  readDeviceModel() => Reading device model dot file
  - DirectedGraph *G                  :: original device model read dot file
  - std::map<int, NodeConfig> *gConfig  :: Store the configuration of each node (type,opcode, latenct etc.)
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

    invUsers[i] = -1; // Initialize the invUsers list.
    GRAMM->trace("[G] arch_NodeName {} :: arch_NodeCell {} :: arch_NodeType {}", arch_NodeName, upperCaseNodeCell, upperCaseType);
  }
}

/*
  readDeviceModel() => Reading device model dot file
  - DirectedGraph *H                    :: original device model read dot file
  - std::map<int, NodeConfig> *hConfig  :: Store the configuration of each node (type,opcode, latenct etc.)
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

    // Fetching type from the application-graph:
    // Contains the type of the operation (ex: ALU, MEMPORT, CONST etc.)
    // std::string applicationType = boost::get(&DotVertex::type, *H, v);
    // std::string upperCaseType = boost::to_upper_copy(applicationType);
    //(*hConfig)[i].Type = upperCaseType;

    GRAMM->trace("[H] name {} :: applicationOpcode {} ", name, upperCaseOpcode);
  }
}

int main(int argc, char **argv)
{

  //--------------------------------------------------------------------
  //----------------- Setting up required variables --------------------
  //--------------------------------------------------------------------

  DirectedGraph H, G;
  boost::dynamic_properties dp(boost::ignore_other_properties);
  if (!RIKEN)
  {
    // H --> Application Graph
    dp.property("node_id", boost::get(&DotVertex::name, H));
    dp.property("opcode", boost::get(&DotVertex::opcode, H));
    dp.property("load", boost::get(&EdgeProperty::loadPin, H));
    dp.property("driver", boost::get(&EdgeProperty::driverPin, H));

    // For [G] --> Device Model Graph
    // DotVertex::G_ID --> Contains the sequence ID for the given node of Device Model Graph
    // DotVertex::G_NodeType --> Contains the Node type of Device Model Graph (FuncCell, RouteCell, PinCell)
    // DotVertex::G_Opcode --> Contains the Opcode of the NodeType (For example "ALU" for NodeType "FuncCell")
    dp.property("G_Name", boost::get(&DotVertex::G_Name, G));
    dp.property("G_NodeCell", boost::get(&DotVertex::G_NodeCell, G));
    dp.property("G_NodeType", boost::get(&DotVertex::G_NodeType, G));
    dp.property("G_VisualX", boost::get(&DotVertex::G_VisualX, G));
    dp.property("G_VisualY", boost::get(&DotVertex::G_VisualY, G));
  }
  else
  {
    // For [H] --> Application Graph
    // DotVertex::name   --> Contains name of the operation in Application graph (ex: Load_0)
    // DotVertex::H_Opcode --> Contains the Opcode of the operation (ex: op, const, input and output)
    dp.property("label", boost::get(&DotVertex::name, H));
    dp.property("opcode", boost::get(&DotVertex::opcode, H));
    dp.property("load", boost::get(&EdgeProperty::loadPin, H));
    dp.property("driver", boost::get(&EdgeProperty::driverPin, H));

    // For [G] --> Device Model Graph
    // DotVertex::G_ID --> Contains the sequence ID for the given node of Device Model Graph
    // DotVertex::G_NodeType --> Contains the Node type of Device Model Graph (FuncCell, RouteCell, PinCell)
    // DotVertex::G_Opcode --> Contains the Opcode of the NodeType (For example "ALU" for NodeType "FuncCell")
    // DotVertex::G_VisualX --> X location for only visualization purpose.
    // DotVertex::G_VisualX --> Y location for only visualization purpose.
    dp.property("G_Name", boost::get(&DotVertex::G_Name, G));
    dp.property("G_NodeCell", boost::get(&DotVertex::G_NodeCell, G));
    dp.property("G_NodeType", boost::get(&DotVertex::G_NodeType, G));
    dp.property("G_VisualX", boost::get(&DotVertex::G_VisualX, G));
    dp.property("G_VisualY", boost::get(&DotVertex::G_VisualY, G));
  }

  // gConfig and hConfig contains the configuration information about that particular node.
  std::map<int, NodeConfig> gConfig, hConfig;

  //--------------------------------------------------------------------
  //---------------------- Command line arguments ----------------------
  //--------------------------------------------------------------------

  po::variables_map vm; // Value map

  // Input variables:
  std::string deviceModelFile;
  std::string applicationFile;
  std::string configFile;
  int seed_value;
  int verbose_level;

  po::options_description desc("[GRAMM] allowed options =");
  desc.add_options()("help,h", "Print help messages")("num_rows,nr,NR", po::value<int>(&numR)->required(), "Number of rows")("num_cols,nc,NC", po::value<int>(&numC)->required(), "Number of Columns")("seed", po::value<int>(&seed_value)->required(), "Seed for the run")("verbose_level", po::value<int>(&verbose_level)->required(), "0: info, 1: debug, 2: trace")("dfile", po::value<std::string>(&deviceModelFile)->required(), "Device model file")("afile", po::value<std::string>(&applicationFile)->required(), "Application graph file")("config", po::value<std::string>(&configFile)->required(), "GRAMM config file");

  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 0;
  }

  po::notify(vm);

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
    jsonParsed = json::parse(f);
    jsonUppercase(jsonParsed);
    GRAMM->info("Parsed JSON file {} ", jsonParsed.dump());
  }

  //--------------------------------------------------------------------
  //----------------- STEP 0 : READING DEVICE MODEL --------------------
  //--------------------------------------------------------------------

  std::ifstream dFile;                       // Defining the input file stream for device model dot file
  dFile.open(deviceModelFile);               // Passing the device_Model_dot file as an argument!
  readDeviceModelPragma(dFile, GrammConfig); // Reading the device model pragma from the device-model dot file.
  boost::read_graphviz(dFile, G, dp);        // Reading the dot file
  readDeviceModel(&G, &gConfig);

  //--------------------------------------------------------------------
  //----------------- STEP 1: READING APPLICATION DOT FILE -------------
  //--------------------------------------------------------------------

  // read the DFG from a file
  std::ifstream iFile;
  iFile.open(applicationFile);
  readApplicationGraphPragma(iFile, GrammConfig); // Reading the application-graph pragma from the device-model dot file.
  boost::read_graphviz(iFile, H, dp);
  readApplicationGraph(&H, &hConfig);

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
  }

  /*
   ** compute a topological order
    -->  We also experimented with sorting the nodes of H according to their topological distancwe from a CGRA I/O or memory port; however, we found that this technique did not improve results beyond the ordering by the size of the vertex models.
  */
  // if (computeTopoEnable)
  //   computeTopo(&H, &hConfig); // not presently used

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

  // GRAMM->info("Welcome to spdlog!");
  // GRAMM->debug("This message should be displayed..");
  // GRAMM->trace("This message should be displayed..");
  // GRAMM->error("Some error message with arg: {}", 1);

  return 0;
}
