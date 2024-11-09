//===================================================================//
// Universal GRAaph Minor Mapping (UGRAMM) method for CGRA           //
// file : routing.cpp                                                //
// description: contains routing-related functions                   //
//===================================================================//

#include "../lib/UGRAMM.h"
#include "../lib/utilities.h"
#include "../lib/routing.h"

//-------------------------------------------------------------------//
//------------------- [Routing] Helper function ---------------------//
//-------------------------------------------------------------------//

/*
 * This function enables wildcard naming for the locked. 
 *
 * It breaks the provided lock Name string into substrings based on a key. Once a list of 
 * substrings have been created, it checks in the gNamesInv map to see if all substring is 
 * present within a gName.
*/
bool matchesPattern(const std::string& key, const std::string& gName, const std::string& lockName) {
    size_t gNamePos = 0;  // Current position in the gName
    size_t lockNamePos = 0;  // Current position in the lockName

    UGRAMM->trace("\t[Matches Pattern - Locking] key {} :: lockNode {} :: gNode {}", key, lockName, gName);

    while (lockNamePos < lockName.size()) {
        size_t nextKey = lockName.find(key, lockNamePos);
        std::string part = lockName.substr(lockNamePos, nextKey - lockNamePos);  // Extract the part between keys
        
        if (!part.empty()) {
            gNamePos = gName.find(part, gNamePos);  // Find the part in the gName starting from 'pos'
            if (gNamePos == std::string::npos) return false;  // If part not found, return false
            gNamePos += part.size();  // Move position past the matched part
        }

        if (nextKey == std::string::npos) break;  // If no more keys in the lockName, we're done
        lockNamePos = nextKey + 1;  // Move lockName position past keys
    }

    return true;
}

/*
 * This function gets the GID for the the funcCell node in the device model graph that
 * meets the locking requirements. That is, the funcCell node must have a type that
 * can support the operation as well must be located in specified x and y locations
 * 
 */
void findGNodeID_FuncCell(const std::string &lockedNodeName, std::vector<int> &suitableGIDs){

  suitableGIDs.clear();

  // Get the GID on the lock when not using wildcards
  if (!allowWildcardInLocking){
    int GID = gNamesInv_FuncCell[lockedNodeName];
    suitableGIDs.push_back(GID);
  }

  if (allowWildcardInLocking){
    //wildcard key is "*""
    std::string key = "*";
    if (!boost::algorithm::contains(lockedNodeName, key)){
      int GID = gNamesInv_FuncCell[lockedNodeName];
      suitableGIDs.push_back(GID);
    } else {
      for (const auto& pair : gNamesInv_FuncCell) {
        //Get the current name for the gNode and the lock node
        std::string gNode = pair.first;
        std::string lockedNode = lockedNodeName;
        
        // String matching when locked nodes contains a wildcard
        if (allowWildcardInLocking){
          if (matchesPattern(key, gNode, lockedNode)) {
            suitableGIDs.push_back(pair.second);
          }
        }
        UGRAMM->trace("\t[Locking] Lock node {} :: gNode {} :: GID {}", lockedNode, gNode, pair.second);
      }
    }
  }

  return;
}


/*
 * For the given outputPin (signal), finds the associated FuncCell node from the device model.
 * 
 * This function identifies the FuncCell node in the device model graph G associated with the 
 * specified output pin (signal), tracing the source of this edge (walking uphill).
 * 
 * Ex: FuncCell(signal) -> outPin (selectedCellOutputPin) [Walking uphill --- finding the source of this edge]
*/
int findFuncCellFromOutputPin(int signal, DirectedGraph *G)
{
  vertex_descriptor signalVertex = vertex(signal, *G);
  in_edge_iterator ei, ei_end;
  boost::tie(ei, ei_end) = in_edges(signal, *G);
  int selectedCellFuncCell = source(*ei, *G);

  return selectedCellFuncCell;
}

/*
 * For the given FuncCell (signal), finds the associated outputPin node from the device model.
 * 
 * This function identifies the outputPin node in the device model graph G that is associated 
 * with the given FuncCell (signal), tracing the sink of the edge (walking downhill).
 * 
 * Ex: FuncCell(signal) -> outPin (selectedCellOutputPin) [Walking downhill --- finding the sink of this edge]
*/
int findOutputPinFromFuncCell(int signal, DirectedGraph *G)
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
 * As UGRAMM performs pin-to-pin mapping, the root of the vertex model, 
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
        UGRAMM->error("FATAL ERROR -- For signal {}, driverNode is not a PinCell!!", *it);
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
      UGRAMM->debug("{} OVERUSE ON {} with following users:", temp, gNames[i]);
      for (int value : (*Users)[i])
      {
        UGRAMM->debug("{}", hNames[value]);
      }
    }
  }

  UGRAMM->info("TOTAL OVERUSE: {}", total);

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


struct CustomComparator {
    std::map<int, NodeConfig>* hConfig;  // Pointer to the graph for comparison

    // Constructor to initialize the graph pointer
    CustomComparator(std::map<int, NodeConfig>* graphConfig) : hConfig(graphConfig) {}

    // Overload operator() to behave like a comparison function for std::sort
    bool operator()(int aI, int bI) const {
        // // Get the vertex name property using Boost's get method
        // auto vertexNameMap = get(vertex_name, *H);
        int ret;

        //------- Sorting the nodes with priority constraints ------------//
        //------- Locking constraints 
        bool has_aI_LockGNode = (!(*hConfig)[aI].LockGNode.empty()); //&& ((*hConfig)[aI].Location.second != -1);
        bool has_bI_LockGNode = (!(*hConfig)[bI].LockGNode.empty()); //&& ((*hConfig)[bI].Location.second != -1);

        if (has_aI_LockGNode || has_bI_LockGNode){
          if (has_aI_LockGNode)
            return true;
          else
            return false;
        }

        //------- Randomly sorting the remaining non-priority nodes ------------//
        ret = ((*Trees)[bI].nodes.size() - (*Trees)[aI].nodes.size());
        if (ret == 0)
          ret = (rand() % 3 - 1);



        if (ret == -1){
          return true;
        } else {
          return rand() % 2;
        }
    }
};

/**
 * Sorting the nodes of H according to the size (number of vertices) of their vertex model
*/ 
void sortList(int list[], int n, std::map<int, NodeConfig> *hConfig)
{
  if (!sortAlgorithm)
    qsort(list, n, sizeof(int), cmpfunc);
   else
    std::sort(list, list+n, CustomComparator(hConfig));
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
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int> toDel;
  toDel.clear();

  std::list<int>::iterator it = RT->nodes.begin();

  if(invUsers[signal] != NOT_PLACED)
  { 
    //Safety Check
    if( RT->nodes.size() != 0)
    {
      if (findFuncCellFromOutputPin(*it, G) != invUsers[signal])
        UGRAMM->error("For {} Fatal error, the invUsers [{}] not matching the routing tree [{}]", hNames[signal], gNames[invUsers[signal]], gNames[findFuncCellFromOutputPin(*it, G)]);
    }

    toDel.push_back(invUsers[signal]); //ripUp of invUsers as well!!
    invUsers[signal] = NOT_PLACED;    
  }
    
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

/*
  Calculate Pathfinder based cost
*/
float calculate_cost (vertex_descriptor next)
{
  /*
    wPathFinder(g) = (1 + |S(g)|) × P × (1 + H(g))
    where S(g) is as defined previously: the set of vertices of H that use g in their vertex model. P(PFac) is a scalar constant reflecting the penalty for overuse. We set P to one initially, and increase it geometrically by pfac_mult in each iteration (determined empirically). H(g) is the history term, initialized to zero for each vertex. After each iteration of the while loop in Algorithm 2, we increase H(g) by one for each overused vertex g ∈ G (i.e., where |S(g)| > 1).
  */
  return (1 + (*HistoryCosts)[next]) * (1 + (*Users)[next].size() * PFac);
}

/**
 * Pathfinder approach for routing signal -> sink
*/
int route(DirectedGraph *G, int signal, std::set<int> sink, std::list<int> *route, std::map<int, NodeConfig> *gConfig, std::map<int, NodeConfig> *hConfig)
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

    UGRAMM->trace("(route) EXPANSION SOURCE : {}", gNames[rNode]);

    explored.set(rNode);
    expInt.push_back(rNode);
    PRQ.push(eNode);
  }

  if (UGRAMM->level() <= spdlog::level::trace)
  {
      for (const auto& item : sink) 
      {
          UGRAMM->trace("(route) EXPANSION TARGET : {}", gNames[item]); 
      }
  }

  struct ExpNode popped;
  bool hit = false;
  while (!PRQ.empty() && !hit)
  {
    popped = PRQ.top();
    PRQ.pop();

    UGRAMM->trace("(route) Popped element: {}", gNames[popped.i]);
    UGRAMM->trace("(route) PRQ POP COST: {}", popped.cost);

    auto it = sink.find(popped.i);

    if ((popped.cost > 0) &&
       (it != sink.end()))
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

      //OB: Routing-level check for width-compatibility 
      if(!widthCheck((*hConfig)[signal].width, (*gConfig)[next].width))
        continue;

      explored.set(next);
      expInt.push_back(next);

      it = sink.find(next);

      // Verifying if the node is type RouteCell or PinCell as ONLY they can be used along the way for routing
      if ( (it == sink.end()) && (*gConfig)[next].Cell != "ROUTECELL" && (*gConfig)[next].Cell != "PINCELL")
        continue;

      struct ExpNode eNodeNew;
      eNodeNew.i = next;

      //Cost function:
      float nextNodeCost = calculate_cost(next);

	  //Cost is accumulated upto sink node:
      eNodeNew.cost = popped.cost + nextNodeCost;
      UGRAMM->trace("(route) Cost of popped node: {} [{}] :: Cost of next node: {} [{}] :: Total: [{}]", gNames[popped.i],popped.cost, gNames[next], nextNodeCost,eNodeNew.cost);
      
      (*TraceBack)[next] = popped.i;
      it = sink.find(next);
      if ((eNodeNew.cost > 0) && (it != sink.end()))
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

    UGRAMM->trace("(route) ----------- HIT for {} at {} -----------", hNames[signal], gNames[current]);

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
  { 
    UGRAMM->error("(route) ----------- FAILED routing for {} -----------", hNames[signal]);
    retVal = MAX_DIST;
  }
    

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
int routeSignal(DirectedGraph *G, DirectedGraph *H, int y, std::map<int, NodeConfig> *gConfig, std::map<int, NodeConfig> *hConfig)
{
  UGRAMM->trace("(routeSignal) ----------- BEGINNING ROUTE OF NAME : {} -----------", hNames[y]);

  vertex_descriptor yD = vertex(y, *H);
  int totalCost = 0;

  // Calculating cost for routing outgoing edges of signal y.
  out_edge_iterator eo, eo_end;
  boost::tie(eo, eo_end) = out_edges(yD, *H);
  for (; eo != eo_end; eo++)
  { 
    int load = target(*eo, *H);

    //---------------------------------------------------------------//
    //------------------------- safety checks -----------------------//
    //---------------------------------------------------------------//

    std::string H_LoadPin = boost::get(&EdgeProperty::H_LoadPin, *H, *eo);
    std::string H_DriverPin = boost::get(&EdgeProperty::H_DriverPin, *H, *eo);

    if (load == y)
      continue; // JANDERS ignore feedback connections for the moment

    //UserCheck for load:
    if(invUsers[load] == NOT_PLACED)
      continue; // load should be placed for the routing purpose!!

    //UserCheck for driver:
    if(invUsers[y] == NOT_PLACED)
    {
      UGRAMM->error("(routeSignal) While routing, invUsers for node {} found to be NULL", hNames[y]);
      exit(-1);
    }

    //Routing tree should be empty for the current application-node (signal y) 
    if((*Trees)[y].nodes.size() != 0)
    {
      UGRAMM->error("(routeSignal) For application node {}, the routing tree already has elements", hNames[y]);
      printRouting(y);
      exit(-1);
    }
    //-------------------------------------------------------------//

     UGRAMM->debug("(routeSignal) ----------- ROUTE: {} -> {} -----------", hNames[y], hNames[load]);

    //----------------------------------------------------------------//
    //--------- Finding appropriate outputPin for the driver ---------//
    //----------------------------------------------------------------//
    int driverFunCell = invUsers[y];
    int driverOutPin = -1;

    //Finding the outputPin based on attribute given in the benchmark:
    std::string driverPinName = boost::to_upper_copy(boost::get(&EdgeProperty::H_DriverPin, *H, *eo));
    vertex_descriptor driverD = vertex(driverFunCell, *G);
    out_edge_iterator eoY, eoY_end;
    boost::tie(eoY, eoY_end) = out_edges(driverD, *G);
    for (; eoY != eoY_end; eoY++)
    {
      int outPinID = target(*eoY, *G);
      if ((*gConfig)[outPinID].pinName == driverPinName)
      { 
        //Width-check:
        if(widthCheck((*hConfig)[y].width, (*gConfig)[outPinID].width))
        {
          driverOutPin = outPinID;
        }
        else 
        {
          UGRAMM->error("(routeSignal) The width of device-model node {} | {} is not compatibile with the requirement of {} | {}", gNames[outPinID], (*gConfig)[outPinID].width,hNames[y], (*hConfig)[y].width);
          exit(-1);
        }
      }
    }

    //Check:
    if (driverOutPin < 0)
    {
      UGRAMM->error("(routeSignal) Could not find outputPin for the driver\n");
      exit(-1);
    }

    //------------------------------------------------------//
    //    Adding outputPin in routing data structure:       //
    //(This is used as a starting point in route() function)//
    //------------------------------------------------------//
    (*Users)[driverOutPin].push_back(y);
    (*Trees)[y].nodes.push_back(driverOutPin);
    //------------------------------------------------------//

    //----------------------------------------------------------------//

    //----------------------------------------------------------------//
    //--------- Finding appropriate inputPin for the load ------------//
    //----------------------------------------------------------------//
    int loadFunCell = invUsers[load];
    std::set<int> loadIDList;

    //Finding the inputPin based on attribute given in the benchmark:
    std::string loadPinList = boost::get(&EdgeProperty::H_LoadPin, *H, *eo);
    std::string upperCaseloadPinList = boost::to_upper_copy(loadPinList);

    //Converting above string into set of strings:
    std::set<std::string> temp;
    boost::split(temp, upperCaseloadPinList, boost::is_any_of(","));

    // Convert the vector into a set to remove duplicates
    std::set<std::string> loadPinSet(temp.begin(), temp.end());

    vertex_descriptor loadD = vertex(loadFunCell, *G);
    in_edge_iterator eiL, eiL_end;
    boost::tie(eiL, eiL_end) = in_edges(loadD, *G);
    for (; eiL != eiL_end; eiL++)
    {
      int inPinId = source(*eiL, *G);
      auto it = loadPinSet.find((*gConfig)[inPinId].pinName);
      //OB: UGRAMM->debug(" loadPinList : {} :: pinName : {}", loadPinList, (*gConfig)[inPinId].pinName );
      if ( it != loadPinSet.end())
      { 
        //Width-check:
        if(widthCheck((*hConfig)[load].width, (*gConfig)[inPinId].width))
        {
          loadIDList.insert(inPinId);
          UGRAMM->debug("(routeSignal) Load [{}] currently mapped at [{}] has driverPinList as {}", hNames[load], gNames[inPinId], loadPinList);
        }
        else 
        {
          UGRAMM->error("(routeSignal) The width of device-model node {} | {} is not compatibile with the requirement of {} | {}", gNames[inPinId], (*gConfig)[inPinId].width,hNames[load], (*hConfig)[load].width);
          exit(-1);
        }
      }
    }


    //Check:
    if (loadIDList.size() == 0)
    {
      UGRAMM->error("(routeSignal) Could not find inputPin for the dloadriver\n");
      exit(-1);
    }

    //----------------------------------------------------------------//

    //Debugging statement:
    if (UGRAMM->level() <= spdlog::level::debug)
    {
      for (const auto& load : loadIDList) // Use a range-based for loop
      {
          UGRAMM->debug("(routeSignal) ----------- D-ROUTE: {} -> {} -----------", gNames[driverOutPin], gNames[load]);
      }
    }
    //----------------------------------------------------------------//

    int cost;
    std::list<int> path;

    cost = route(G, y, loadIDList, &path, gConfig, hConfig);

    totalCost += cost;

    if (cost < MAX_DIST)
    {
      // Earlier in Func to Func Mapping, we used to remove the driver Function:
      // path.remove(loadPinCellLoc);
      depositRoute(y, &path);
    }
    else
    {
      UGRAMM->trace("(routeSignal) Skipped the routing for this application node {} due to high cost", hNames[y]);
    }
  }
  
  return totalCost;
}

