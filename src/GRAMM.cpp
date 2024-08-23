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
std::vector<int> *HistoryCosts;
std::vector<int> *TraceBack;
std::vector<int> *TopoOrder;
std::map<int, std::string> hNames;
std::map<int, std::string> gNames;
std::bitset<100000> explored;

std::vector<std::string> inPin = {"inPinA", "inPinB", "anyPins"};
std::vector<std::string> outPin = {"outPinA"};

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

int findDriver(int signal)
{
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int>::iterator it = RT->nodes.begin();

  // find the node with no parent
  for (; it != RT->nodes.end(); it++)
  {
    if (!RT->parent.count(*it))
      return *it;
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

void ripUpRouting(int signal)
{
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int> toDel;
  toDel.clear();

  std::list<int>::iterator it = RT->nodes.begin();
  for (; it != RT->nodes.end(); it++)
  {
    toDel.push_back(*it);
    //    std::cout << "RIPUP ";
    //    printName(*it);
  }

  ripup(signal, &toDel);
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

  route->clear(); //Clearing the route list.

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
    
    if(DEBUG)
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
      if ((next != sink) && (*gConfig)[next].type != RouteCell && (*gConfig)[next].type != PinCell)
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
      std::cout << "OVERUSE " << temp << " ON ";
      // printName(i);
    }
  }

  std::cout << "TOTAL OVERUSE: " << total << "\n";

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
  if (DEBUG)
    std::cout << "BEGINNING ROUTE OF NAME :" << hNames[y] << "\n";

  vertex_descriptor yD = vertex(y, *H);
  int totalCost = 0;

  // i is a candidate root for signal y
  out_edge_iterator eo, eo_end;
  boost::tie(eo, eo_end) = out_edges(yD, *H);
  for (; eo != eo_end; eo++)
  {
    int load = target(*eo, *H);
    std::string loadPin   = boost::get(&EdgeProperty::loadPin, *H, *eo);
    std::string driverPin = boost::get(&EdgeProperty::driverPin, *H, *eo);

    if (load == y)
      continue; // JANDERS ignore feedback connections for the moment

    if ((*Trees)[load].nodes.size() == 0)
      continue; // load should be placed for the routing purpose!!

    // Since driver will always be the outPin of the FunCell, the type of loadOutPinCellLoc will be "pinCell"
    int loadOutPinCellLoc = findDriver(load);

    //Converting the driver(pinCell) to the actual input pin of FuncCell:
    in_edge_iterator ei, ei_end;
    vertex_descriptor yD = vertex(loadOutPinCellLoc, *G);
    boost::tie(ei, ei_end) = in_edges(yD, *G);

    //Step 1: Fetching "FunCell" ID from the edge: FunCell --> OutPin
    int loadFunCellLoc = source(*ei, *G);   

    //Step 2: Fetching given "inPin" ID from the edge: inPin --> FunCell (Note: there could be multiple input pins for the given FunCell)
    yD = vertex(loadFunCellLoc, *G);
    boost::tie(ei, ei_end) = in_edges(yD, *G);
    int loadInPinCellLoc = 0;
    for (; ei != ei_end; ei++)
    {
      int source_id = source(*ei, *G);
      if ((*gConfig)[source_id].loadPin == loadPin)
        loadInPinCellLoc = source_id;
    }

    if(DEBUG){
      std::cout << "SOURCE : " << hNames[y] << " TARGET : " << hNames[load] << " TARGET-Pin : " << loadPin << "\n";
      std::cout << "ROUTING LOAD: " << gNames[loadFunCellLoc] << " :: (InputPin) :: " << gNames[loadInPinCellLoc] << "\n";
    }

    if (loadInPinCellLoc < 0)
    {
      std::cout << "SIGNAL WITHOUT A DRIVER.\n";
      exit(-1);
    }

    int cost;
    std::list<int> path;

    cost = route(G, y, loadInPinCellLoc, &path, gConfig); 

    totalCost += cost;

    if (cost < MAX_DIST)
    {
      //Earlier in Func to Func Mapping, we used to remove the driver Function:
      //path.remove(loadPinCellLoc);
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
  { // choose a random node of G to be the vertex model for y because NONE of its fanins or fanouts have vertex models
    int chooseRand = (rand() % num_vertices(*G));
    do
    {
      chooseRand = (chooseRand + 1) % num_vertices(*G);
    } while (((*gConfig)[chooseRand].opcode != (*hConfig)[y].opcode));

    // Finding the output Pin for the selected FunCell:
    boost::tie(eo, eo_end) = out_edges(chooseRand, *G);
    int selectedCellOutputPin = target(*eo, *G);

    if (DEBUG)
    {
      std::cout << "For application node :: " << hNames[y] << " ::Chose a random vertex model :: " << gNames[chooseRand] << " :: " << gNames[selectedCellOutputPin] << "\n";
    }
    // OB: In pin to pin mapping: adding outPin in the routing tree instead of the driver FunCell
    //(*Trees)[y].nodes.push_back(chooseRand);
    (*Trees)[y].nodes.push_back(selectedCellOutputPin);
    (*Users)[selectedCellOutputPin].push_back(y);
    return 0;
  }

  // OK, at least one of y's fanins or fanouts have a vertex model
  int bestCost = MAX_DIST;
  int bestIndex = -1;

  int totalCosts[num_vertices(*G)];
  for (int i = 0; i < num_vertices(*G); i++)
    totalCosts[i] = 0;

  for (int i = 0; i < num_vertices(*G); i++)
  { // first route the signal on the output of y

    // Confriming the Vertex correct type:
    if ((*gConfig)[i].opcode != (*hConfig)[y].opcode)
      continue;

    // Finding the output Pin for the selected FunCell:
    boost::tie(eo, eo_end) = out_edges(i, *G);
    int outputPin = target(*eo, *G);

    if((*gConfig)[outputPin].type != PinCell){
      std::cout << "FATAL ERROR -- driverNode is not a PinCell!! \n"; 
      exit(-1);
    }

    ripUpRouting(y);
    (*Trees)[y].nodes.push_back(outputPin);
    (*Users)[outputPin].push_back(y);

    totalCosts[outputPin] += (1 + (*HistoryCosts)[outputPin]) * ((*Users)[outputPin].size() * PFac);

    if (DEBUG)
    {
      std::cout << "Trying location :: (selectedCell) " << gNames[i] << " :: (outputPin) :: " << gNames[outputPin] << "\n";
      std::cout << "INVOKING ROUTE FANOUT SIGNAL\n";
      printRouting(y);
    }

    totalCosts[outputPin] += routeSignal(G, H, y, gConfig);

    if (DEBUG)
      std::cout << "TOTAL COST " << totalCosts[outputPin] << "\n";

    if (totalCosts[outputPin] > bestCost)
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

      int driverNode = findDriver(driver);
      
      if((*gConfig)[driverNode].type != PinCell){
        std::cout << "FATAL ERROR -- driverNode is not a PinCell!! \n"; 
        exit(-1);
      }

      ripUpRouting(driver);
      (*Trees)[driver].nodes.push_back(driverNode);
      (*Users)[driverNode].push_back(driver);

      // Newfeature: rip up from load: ripUpLoad(G, driver, outputPin);

      totalCosts[outputPin] += routeSignal(G, H, driver, gConfig);

      if(DEBUG){
        std::cout << "ROUTING FANIN: (application_node) :: " << hNames[driver] << " :: (driver_node) :: " << gNames[driverNode] << "\n";
        std::cout << "COSTS: " << totalCosts[outputPin] << "\n";
      }

      if (totalCosts[outputPin] > bestCost)
        break;
    }

    if(DEBUG)
      std::cout << "TOTAL COST: " << totalCosts[outputPin] << "\n";

    if (totalCosts[outputPin] >= MAX_DIST)
      continue;

    if (totalCosts[outputPin] < bestCost)
    {
      bestIndex = outputPin;
      bestCost = totalCosts[outputPin];
    }
    else if (totalCosts[outputPin] == bestCost)
    {
      if (!(rand() % 2))
        bestIndex = outputPin;
    }
  }

  if (bestIndex == -1)
  {
    std::cout << "FATAL ERROR -- COULD NOT FIND A VERTEX MODEL FOR VERTEX " << y << " " << hNames[y] << "\n";
    exit(-1);
  }

  if(DEBUG)
    std::cout << "For application_node :: " << hNames[y] << " , bertIndex found from device model is " << gNames[bestIndex] << " with cost of " << bestCost << "\n";
 
  ripUpRouting(y);
  (*Trees)[y].nodes.push_back(bestIndex);
  (*Users)[bestIndex].push_back(y);
  
  routeSignal(G, H, y, gConfig);

  boost::tie(ei, ei_end) = in_edges(yD, *H);
  for (; ei != ei_end; ei++)
  {
    int driver = source(*ei, *H);
    if ((*Trees)[driver].nodes.size() == 0)
      continue; // driver is not placed

    int driverNode = findDriver(driver);

    if((*gConfig)[driverNode].type != PinCell){
      std::cout << "FATAL ERROR -- driverNode is not a PinCell!! \n"; 
      exit(-1);
    }

    ripUpRouting(driver);
    (*Trees)[driver].nodes.push_back(driverNode);
    (*Users)[driverNode].push_back(driver);
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
    if (DEBUG)
      std::cout << ordering[i] << " \n";
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
    std::cout << "***** BEGINNING OUTER WHILE LOOP ***** ITER " << iterCount << " \n";

    // randomizeList(ordering, num_vertices(*H));
    sortList(ordering, num_vertices(*H));

    for (int k = 0; k < num_vertices(*H); k++)
    {
      int y = ordering[k];

      if (DEBUG)
      {
        std::cout << "--------------------- New Vertices Mapping Start ---------------------------\n";
        std::cout << "Finding vertex model for: " << hNames[y] << "\n";
        std::cout << "SIZE: " << (*Trees)[y].nodes.size() << "\n";
        if (computeTopoEnable)
          std::cout << "TOPO ORDER: " << (*TopoOrder)[y] << "\n";
      }

      findMinVertexModel(G, H, y, hConfig, gConfig);
    } // for k

    int TO = totalOveruse(G);

    if (iterCount == 2)
    {
      frac = (float)TO / totalUsed(G);
      std::cout << "FRACTION OVERLAP: " << frac << "\n";
    }

    if ((iterCount >= 2) && (TO == 0))
    {
      std::cout << "SUCCESS! :: [iterCount] :: " << iterCount << " :: [frac] :: " << frac << " :: [num_vertices(H)] :: " << num_vertices(*H) << "\n";
      done = true;
      success = true;
    }

    adjustHistoryCosts(G);

    if (iterCount > maxIterations) // limit the iteration count to ~40 iterations!
    {
      std::cout << "FAILURE. OVERUSED: " << TO << " USED: " << totalUsed(G) << "\n";
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

void computeTopo(DirectedGraph *H, std::map<int, NodeConfig> *hConfig)
{

  std::queue<int> fifo;

  TopoOrder = new std::vector<int>(num_vertices(*H));

  for (int i = 0; i < num_vertices(*H); i++)
  {
    (*TopoOrder)[i] = -1;
    if ((*hConfig)[i].opcode == io || (*hConfig)[i].opcode == memport)
    {
      fifo.push(i);
      (*TopoOrder)[i] = 0; // IOs and memport are at level 0 in the topological ordering
    }
  }

  while (!fifo.empty())
  {
    int pop = fifo.front();
    fifo.pop();
    vertex_descriptor nD = vertex(pop, *H);
    out_edge_iterator eo, eo_end;
    boost::tie(eo, eo_end) = out_edges(nD, *H);
    for (; eo != eo_end; eo++)
    {
      int next = target(*eo, *H);
      if ((*TopoOrder)[next] == -1)
      {
        (*TopoOrder)[next] = (*TopoOrder)[pop] + 1;
        fifo.push(next);
      }
    }
    in_edge_iterator ei, ei_end;
    boost::tie(ei, ei_end) = in_edges(nD, *H);
    for (; ei != ei_end; ei++)
    {
      int next = source(*ei, *H);
      if ((*TopoOrder)[next] == -1)
      {
        (*TopoOrder)[next] = (*TopoOrder)[pop] + 1;
        fifo.push(next);
      }
    }
  }
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

    std::string arch_NodeType = boost::get(&DotVertex::G_NodeType, *G, v); // Contains the Node type of Device Model Graph (FuncCell, RouteCell, PinCell)
    std::string arch_Opcode = boost::get(&DotVertex::G_Opcode, *G, v);     // Contains the Opcode of the NodeType (For example "ALU" for NodeType "FuncCell")

    // OB: DEBUG: std::string node_name = (*G)[i].G_Name;
    // OB: DEBUG: std::cout << " i :: " << i << " :: " << node_name << std::endl;
    std::string arch_NodeName = boost::get(&DotVertex::G_Name, *G, v);
    gNames[i] = arch_NodeName;

    // TODO: Remove G_ID from the device model!!
    std::string arch_ID = boost::get(&DotVertex::G_ID, *G, v); // Contains the sequence ID for the given node of Device Model Graph

    if (DEBUG)
    {
      std::cout << "[G] arch_ID " << arch_ID << " ::  arch_NodeType " << arch_NodeType << " :: arch_Opcode " << arch_Opcode << "\n";
    }

    // Deciding the configuration based on attributes defined in the script:
    // arch_Opcode described in the device model graph file: MemPort, Mux, Constant, and ALU.
    if (arch_Opcode == "MemPort")
    {
      (*gConfig)[i].type = FuncCell;  // memport is part  of FuncCell
      (*gConfig)[i].opcode = memport; // Saving the opcode in the config array.
    }
    else if (arch_Opcode == "Mux")
    {
      (*gConfig)[i].type = RouteCell; // mux is part of RouteCell
      (*gConfig)[i].opcode = mux;     // Saving the opcode in the config array.
    }
    else if (arch_Opcode == "in")
    {
      (*gConfig)[i].type = PinCell; // constant is part of FuncCell
      (*gConfig)[i].opcode = in;    // Saving the opcode in the config array.
    }
    else if (arch_Opcode == "out")
    {
      (*gConfig)[i].type = PinCell; // constant is part of FuncCell
      (*gConfig)[i].opcode = out;   // Saving the opcode in the config array.
    }
    else if (arch_Opcode == "Constant")
    {
      (*gConfig)[i].type = FuncCell;   // constant is part of FuncCell
      (*gConfig)[i].opcode = constant; // Saving the opcode in the config array.
    }
    else if (arch_Opcode == "ALU")
    {
      (*gConfig)[i].type = FuncCell; // alu is part of FuncCell
      (*gConfig)[i].opcode = alu;    // Saving the opcode in the config array.
    }

    if ((*gConfig)[i].type == PinCell)
    {
      size_t pos = gNames[i].find_last_of('.');
      if (pos != std::string::npos)
      {
        (*gConfig)[i].loadPin = gNames[i].substr(pos + 1);
      }
    }

    if ((*gConfig)[i].type == FuncCell)
    {
      size_t pos = gNames[i].find('.');
      if (pos != std::string::npos)
      {
        (*gConfig)[i].tile = gNames[i].substr(0, pos);
      }
    }

    if (DEBUG)
    {
      std::cout << "[G] arch_ID " << arch_ID << " ::  [G] name " << gNames[i] << " :: Type ";
      if ((*gConfig)[i].type == FuncCell)
        std::cout << "FuncCell";
      else if ((*gConfig)[i].type == PinCell)
        std::cout << "PinCell";
      else if ((*gConfig)[i].type == RouteCell)
        std::cout << "RouteCell";

      std::cout << " :: arch_Opcode " << arch_Opcode << " :: load pin " << (*gConfig)[i].loadPin << " ::  tile " << (*gConfig)[i].tile << "\n";
    }
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
    std::string opcode = boost::get(&DotVertex::opcode, *H, v);
    std::string name = boost::get(&DotVertex::name, *H, v);

    hNames[i] = name;

    if (DEBUG)
    {
      std::cout << "[H] name " << name << " opcode " << opcode << "\n";
    }

    /*
      Translating the Opcodes from Application grpah into common NodeTypes
      -- We have three common NodeTypes
        1. FuncCell  (ALU, Constant, MemPort, IO)
        2. RouteCell (MUX, Reg)
        3. PinCell   (In, Out)
      -- Also storing the other important information such as opcode..
    */
    if (!RIKEN)
    {
      //==========================//
      //---------- ADRES ---------//
      //==========================//
      if (opcode == "const")
      {
        (*hConfig)[i].type = FuncCell;   // constant is part of FuncCell
        (*hConfig)[i].opcode = constant; // Saving Opcode in the config map.
      }
      else if (opcode == "input")
      {
        (*hConfig)[i].type = FuncCell; // io is part of FuncCell
        (*hConfig)[i].opcode = io;     // Saving Opcode in the config map.
      }
      else if (opcode == "output")
      {
        (*hConfig)[i].type = FuncCell; // io is part of FuncCell
        (*hConfig)[i].opcode = io;     // Saving Opcode in the config map.
      }
      else if (opcode == "load")
      {
        (*hConfig)[i].type = FuncCell;  // memport is part of FuncCell
        (*hConfig)[i].opcode = memport; // Saving Opcode in the config map.
      }
      else if (opcode == "store")
      {
        (*hConfig)[i].type = FuncCell;  // memport is part of FuncCell
        (*hConfig)[i].opcode = memport; // Saving Opcode in the config map.
      }
      else
      {
        (*hConfig)[i].type = FuncCell; // ALU is part of FuncCell
        (*hConfig)[i].opcode = alu;    // Saving Opcode in the config map.
      }
    }
    else
    {
      //==========================//
      //---------- RIKEN ---------//
      //==========================//
      if (opcode == "const")
      {
        (*hConfig)[i].type = FuncCell;   // constant is part of FuncCell
        (*hConfig)[i].opcode = constant; // Saving Opcode in the config map.
      }
      else if (opcode == "input")
      {
        (*hConfig)[i].type = FuncCell;  // memport is part of FuncCell
        (*hConfig)[i].opcode = memport; // Saving Opcode in the config map.
      }
      else if (opcode == "output")
      {
        (*hConfig)[i].type = FuncCell;  // memport is part of FuncCell
        (*hConfig)[i].opcode = memport; // Saving Opcode in the config map.
      }
      else
      {
        (*hConfig)[i].type = FuncCell; // ALU is part of FuncCell
        (*hConfig)[i].opcode = alu;    // Saving Opcode in the config map.
      }
    }
  }

  /*
    Verifying the the driver and source pin names for all the output edges of the nodes.
    If the pin names are incorrect, display a warning and exit the program

  */
  bool invalidPinNameDetected = 0;

  for (int i = 0; i < num_vertices(*H); i++)
  {
    vertex_descriptor v = vertex(i, *H);

    out_edge_iterator eo, eo_end;
    boost::tie(eo, eo_end) = out_edges(v, *H);
    for (; eo != eo_end; eo++)
    {

      auto it_inPin = std::find(inPin.begin(), inPin.end(), boost::get(&EdgeProperty::loadPin, *H, *eo));
      if (it_inPin != inPin.end())
      {
        // OB: std::cout << "[H] Edge (" << boost::get(&DotVertex::name, *H, boost::source(*eo, *H)) << " -> " << boost::get(&DotVertex::name, *H, boost::target(*eo, *H))  << ") | load pin name verification:  " << boost::get(&EdgeProperty::loadPin, *H, *eo) << " pin name verified" << std::endl;
      }
      else
      {
        // OB: std::cout << "[H] Edge (" << boost::get(&DotVertex::name, *H, boost::source(*eo, *H)) << " -> " << boost::get(&DotVertex::name, *H, boost::target(*eo, *H))  << ") | load pin name verification: " << boost::get(&EdgeProperty::loadPin, *H, *eo) << " pin name not valid" << std::endl;
        invalidPinNameDetected = 1;
      }

      auto it_outPin = std::find(outPin.begin(), outPin.end(), boost::get(&EdgeProperty::driverPin, *H, *eo));
      if (it_outPin != outPin.end())
      {
        // OB: std::cout << "[H] Edge (" << boost::get(&DotVertex::name, *H, boost::source(*eo, *H)) << " -> " << boost::get(&DotVertex::name, *H, boost::target(*eo, *H))  << ") | driver pin name verification:  " << boost::get(&EdgeProperty::driverPin, *H, *eo) << " pin name verified" << std::endl;
      }
      else
      {
        // OB: std::cout << "[H] Edge (" << boost::get(&DotVertex::name, *H, boost::source(*eo, *H)) << " -> " << boost::get(&DotVertex::name, *H, boost::target(*eo, *H))  << ") | driver pin name verification:  " << boost::get(&EdgeProperty::driverPin, *H, *eo) << " pin name not valid" << std::endl;
        invalidPinNameDetected = 1;
      }
    }
  }

  if (invalidPinNameDetected)
  {
    std::cout << "Invalid pin names detected, exiting the program\n";
    // exit(1);
  }
}

int main(int argc, char *argv[])
{

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
    dp.property("G_ID", boost::get(&DotVertex::G_ID, G));
    dp.property("G_NodeType", boost::get(&DotVertex::G_NodeType, G));
    dp.property("G_opcode", boost::get(&DotVertex::G_Opcode, G));
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
    dp.property("G_ID", boost::get(&DotVertex::G_ID, G));
    dp.property("G_NodeType", boost::get(&DotVertex::G_NodeType, G));
    dp.property("G_opcode", boost::get(&DotVertex::G_Opcode, G));
    dp.property("G_VisualX", boost::get(&DotVertex::G_VisualX, G));
    dp.property("G_VisualY", boost::get(&DotVertex::G_VisualY, G));
  }

  if (argc < 5)
  {
    std::cout << "Arguments are <application.dot> <device_model.dot> <numRows NR> <numCols NC> <seed>\n";
    exit(-1);
  }

  // Fetching NR and NC:
  int dim = CGRAdim = atoi(argv[3]);
  int NR = dim;
  int NC = atoi(argv[4]);
  numR = NR;
  numC = NC;

  std::ifstream iFile;
  iFile.open(argv[1]);

  srand(atoi(argv[5])); // Seed

  // removed:: std::map<int, nodeType> gTypes, hTypes;
  // Added type along with other config

  // gConfig and hConfig contains the configuration information about that particular node.
  std::map<int, NodeConfig> gConfig, hConfig;

  //--------------------------------------------------------------------
  //----------------- STEP 0 : READING DEVICE MODEL --------------------
  //--------------------------------------------------------------------

  std::ifstream dFile;                // Defining the input file stream for device model dot file
  dFile.open(argv[2]);                // Passing the device_Model_dot file as an argument!
  boost::read_graphviz(dFile, G, dp); // Reading the dot file
  readDeviceModel(&G, &gConfig);

  //--------------------------------------------------------------------
  //----------------- STEP 1: READING APPLICATION DOT FILE -------------
  //--------------------------------------------------------------------

  // read the DFG from a file
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
  if (computeTopoEnable)
    computeTopo(&H, &hConfig); // not presently used

  //--------------- Starting timestamp -------------------------
	/* get start timestamp */
 	struct timeval tv;
  gettimeofday(&tv,NULL);
  uint64_t start = tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
  //------------------------------------------------------------

  int success = findMinorEmbedding(&H, &G, &hConfig, &gConfig);

	//--------------- get elapsed time -------------------------
  gettimeofday(&tv,NULL);
  uint64_t end = tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
  uint64_t elapsed = end - start;
  double seconds = static_cast<double>(elapsed) / 1000000.0;
  std::cout << "Total time taken :: " << seconds << " Seconds"<< std::endl;
  //------------------------------------------------------------

  if(success){
    // Printing vertex model:
    printVertexModels(&H, &G, &hConfig);

    // Visualizing mapping result in neato:
    printMappedResults(&H, &G, &hConfig, &gConfig);
  }
  
  return 0;
}
