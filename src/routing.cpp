//===================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                       //
// file : routing.cpp                                                //
// description: contains routing-related functions                   //
//===================================================================//

#include "../lib/GRAMM.h"
#include "../lib/utilities.h"
#include "../lib/routing.h"

//-------------------------------------------------------------------//
//------------------- [Routing] Helper function ---------------------//
//-------------------------------------------------------------------//

/*
 * Checks whether the current opcode required by the application node is supported by the device model node.
 * 
 * This function determines if the opcode needed by the application node (represented by `hOpcode`)
 * is compatible with or supported by the device model node type (represented by `gType`).
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
  return false;
}

/*
 * For the given outputPin (signal), finds the associated FunCell node from the device model.
 * 
 * This function identifies the FunCell node in the device model graph G associated with the 
 * specified output pin (signal), tracing the source of this edge (walking uphill).
 * 
 * Ex: FunCell(signal) -> outPin (selectedCellOutputPin) [Walking uphill --- finding the source of this edge]
*/
int findFunCellFromOutputPin(int signal, DirectedGraph *G)
{
  vertex_descriptor signalVertex = vertex(signal, *G);
  in_edge_iterator ei, ei_end;
  boost::tie(ei, ei_end) = in_edges(signal, *G);
  int selectedCellFunCell = source(*ei, *G);

  return selectedCellFunCell;
}

/*
 * For the given FunCell (signal), finds the associated outputPin node from the device model.
 * 
 * This function identifies the outputPin node in the device model graph G that is associated 
 * with the given FunCell (signal), tracing the sink of the edge (walking downhill).
 * 
 * Ex: FunCell(signal) -> outPin (selectedCellOutputPin) [Walking downhill --- finding the sink of this edge]
*/
int findOutputPinFromFuncell(int signal, DirectedGraph *G)
{
  vertex_descriptor signalVertex = vertex(signal, *G);
  out_edge_iterator eo, eo_end;
  boost::tie(eo, eo_end) = out_edges(signalVertex, *G);
  int selectedCellOutputPin = target(*eo, *G);

  return selectedCellOutputPin;
}

/**
 * Finds the root of the vertex model for the given signal.
 * 
 * As GRAMM performs pin-to-pin mapping, the root of the vertex model, 
 * or starting point, will always be an output pin that serves as the 
 * driver for the routing fanout of the specified signal.
 */
int findRoot(int signal, std::map<int, NodeConfig> *gConfig)
{
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int>::iterator it = RT->nodes.begin();

  // find the node with no parent
  for (; it != RT->nodes.end(); it++)
  {
    if (!RT->parent.count(*it))
    {
      if ((*gConfig)[*it].Cell != "PINCELL")  //Driver has to be an output pin cell
      {
        GRAMM->error("FATAL ERROR -- For signal {}, driverNode is not a PinCell!!", *it);
        exit(-1);
      }
      return *it;
    }
  }
  return -1;
}

/**
 * Gets the current cost associated with the given signal.
 */
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

/**
 * Checks if there is an overlap for the given signal. 
 * Confirms by checking if there are more than one 
 * users for any element in the routing tree.
 */
bool hasOverlap(int signal)
{
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int>::iterator it = RT->nodes.begin();

  for (; it != RT->nodes.end(); it++)
    if ((*Users)[*it].size() > 1)
      return true;
  return false;
}

/**
 * Calculates the total amount of used resources in the device model graph.
 */
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

/**
 * Calculates the total amount of overused resources in the device model graph.
 */
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

/**
 * Adjusts the history costs for the device model graph.
*/
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

/**
 * Comparison function for sorting.
*/
int cmpfunc(const void *a, const void *b)
{

  int aI = *((int *)a);
  int bI = *((int *)b);

  int ret = ((*Trees)[bI].nodes.size() - (*Trees)[aI].nodes.size());
  if (ret == 0)
    ret = (rand() % 3 - 1);
  return ret;
}

/**
 * Sorting the nodes of H according to the size (number of vertices) of their vertex model
*/ 
void sortList(int *list, int n)
{
  qsort(list, n, sizeof(int), cmpfunc);
}

//-------------------------------------------------------------------//
//-------------------- [Routing] Main function ----------------------//
//-------------------------------------------------------------------//

/**
 * Removes the specified signal from the list of nodes.
*/
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

/**
 * Removes the routing associated with the given signal from the routing structure.
 * Make a list of used (device-model-nodes) for the given the given siganl (application-graph-node)
*/
void ripUpRouting(int signal, DirectedGraph *G)
{ 
  invUsers[signal] = -1;  //ripUp of invUsers as well!!
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int> toDel;
  toDel.clear();

  std::list<int>::iterator it = RT->nodes.begin();
  int driverFuncell = findFunCellFromOutputPin(*it, G);
  toDel.push_back(driverFuncell);
  for (; it != RT->nodes.end(); it++)
  {
    toDel.push_back(*it);
  }

  ripup(signal, &toDel);
}

/**
 * Deposits the route into the routing structure for the given signal.
*/
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

  it++;
  for (; it != (*nodes).rend(); it++)
  {
    int addNode = *it;

    (*Users)[addNode].push_back(signal);
    RT->children[prev].push_back(addNode);
    RT->parent[addNode] = prev;
    RT->nodes.push_back(addNode);
    prev = addNode;
  }
}

/**
 * Pathfinder approach for routing signal -> sink
*/
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

    GRAMM->trace("EXPANSION SOURCE : {}", gNames[rNode]);

    explored.set(rNode);
    expInt.push_back(rNode);
    PRQ.push(eNode);
  }

  GRAMM->trace("EXPANSION TARGET : {}", gNames[sink]);

  struct ExpNode popped;
  bool hit = false;
  while (!PRQ.empty() && !hit)
  {
    popped = PRQ.top();
    PRQ.pop();
    GRAMM->trace("Popped element: ", gNames[popped.i]);
    GRAMM->trace("PRQ POP COST: {}", popped.cost);

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

      eNodeNew.cost = popped.cost + (1 + (*HistoryCosts)[next]) * (1 + (*Users)[next].size() * PFac);

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

/**
 * Routes all fanout edges of the specified application-graph node.
*/
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
    std::string H_LoadPin = boost::get(&EdgeProperty::H_LoadPin, *H, *eo);
    std::string H_DriverPin = boost::get(&EdgeProperty::H_DriverPin, *H, *eo);

    if (load == y)
      continue; // JANDERS ignore feedback connections for the moment

    if(invUsers[load] == -1)
      continue; // load should be placed for the routing purpose!!

    // Since driver will always be the outPin of the FunCell, the type of loadOutPinCellLoc will be "pinCell"
    // int loadOutPinCellLoc = findRoot(load, gConfig);

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
      if ((*gConfig)[source_id].loadPin == H_LoadPin)
        loadInPinCellLoc = source_id;
    }

    GRAMM->trace("SOURCE : {} TARGET : {} TARGET-Pin : {}", hNames[y], hNames[load], H_LoadPin);
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
      GRAMM->trace("Skipped the routing for this application node {} due to high cost", hNames[y]);
    }
  }
  return totalCost;
}

