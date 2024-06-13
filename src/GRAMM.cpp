
// GRAMM prototype
// author: janders

#include <boost/config.hpp>
//#include <boost/graph/graphviz.hpp>
#include <iostream>
#include <fstream>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>
#include <queue>
#include <map>
#include <list>
#include <bitset>
#include <algorithm>

struct DotVertex {
    std::string name;
    std::string opcode;
    std::string arch_type;
    std::string arch_label;
};

// the routing tree for a signal
// explanation for the three fields:
// for each node in the tree, we have a map that returns a list of its child nodes (i.e. the nodes it drives)
// for each node in the tree, we have a map that returns its parent node (i.e., the unique node that drives it)
// we also keep track of the list of all nodes in the routing tree
struct RoutingTree {
  std::map<int, std::list<int>> children;
  std::map<int, int> parent;
  std::list<int> nodes;
};

std::vector<RoutingTree> *Trees;
std::vector<std::list<int>> *Users;
std::vector<int> *HistoryCosts;
std::vector<int> *TraceBack;

std::vector<int> *TopoOrder;
  
static float PFac = 1;
static float HFac = 1;

std::map<int, std::string> hNames; 
int CGRAdim;

std::bitset<100000> explored;

typedef boost::property<boost::edge_weight_t, int> EdgeWeightProperty;
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::bidirectionalS, DotVertex, EdgeWeightProperty > DirectedGraph;
typedef boost::graph_traits<DirectedGraph>::edge_iterator edge_iterator;
typedef boost::graph_traits<DirectedGraph>::in_edge_iterator in_edge_iterator;
typedef boost::graph_traits<DirectedGraph>::out_edge_iterator out_edge_iterator;
typedef boost::graph_traits<DirectedGraph>::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits<DirectedGraph>::out_edge_iterator OutEdgeIterator;
typedef DirectedGraph::edge_descriptor Edge;

typedef enum nodeT {io, alu, memport, reg, constant, wire, mux} nodeType;

#define MAX_DIST 10000000
#define RIKEN 1                 //Defining the architecture type
#define DEBUG 0                 //For enbaling the print-statements (mapped-DFG)
#define HARDCODE_DEVICE_MODEL 1 //Controls hardcoding of device model

int numR;
int numC;



std::string getString(int n) {

  int orign = n;
  
  if (n < numR)
    return hNames[(*Users)[n].front()];
  else if (n < 2*numR)
    return hNames[(*Users)[n].front()];
  else {
    n -= 2*numR;
    int row = n/(14*numC);
    int col = (n - row*numC*14)/14;
    int elem = n % 14;

    if (elem < 10) {
      std::string ret = std::string("SB_") + std::to_string(row) + "_" + std::to_string(col);
      return ret;
    }
    else if (elem == 10) {
      return hNames[(*Users)[orign + 3].front()];
    }
    else if (elem == 11) {
      return hNames[(*Users)[orign + 2].front()];
    }
    else if (elem == 13) {
      return hNames[(*Users)[orign].front()];
    }
    else {
      std::cout << "FATAL ERROR\n";
      exit(-1);
    }
  }  
}

void drawEdge(int n, int m, std::ofstream& oFile) {
  std::string nodeName = getString(n);
  oFile << nodeName;
  oFile << " -> ";
  nodeName = getString(m);
  oFile << nodeName << "\n";
}

void printPlaceAndRoute(int y, std::ofstream& oFile) {
  
  struct RoutingTree *RT = &((*Trees)[y]);

  if (RT->nodes.size() == 0) 
    return;

  int n = RT->nodes.front();
  int orign = n;

  oFile << hNames[y];
  
  if (n < numR)
    oFile << " [shape=\"circle\" fontsize=6 fillcolor=\"#2ca02c\" pos=\"" << "0," << 2*(-n) << "!\"]\n"; 
  else if (n < 2*numR)
    oFile << " [shape=\"circle\" fontsize=6 fillcolor=\"#2ca02c\" pos=\"" << 2*(numC+1) << "," << 2*(-(n-numR)) << "!\"]\n";
  else {
    n -= 2*numR;
    int row = n/(14*numC);
    int col = (n - row*numC*14)/14;
    oFile << " [shape=\"circle\" fontsize=6 fillcolor=\"#1f77b4\" pos=\"" << 2*(col + 1.25) << "," << 2*(-row-0.5) << "!\"]\n";
  }
  
  //  for (int i = 0; i < numR; i++)
  //    for (int j = 0; j < numC; j++) {
  //      std::cout << "SB_" << i << "_" << j << " [shape=\"circle\" fontsize=6 fillcolor=\"#1f77b4\" pos=\"" << j + 1.5 << "," << -i-0.5 << "!\"]\n";
  //    }
  
  std::list<int>::iterator it = RT->nodes.begin();

  for (; it != RT->nodes.end(); it++) {
    int m = *it;
    if (m == orign)
      continue;
    //    std::cout << "PARENT " << RT->parent[m] << "\n";
    drawEdge(RT->parent[m], m, oFile);
  }

  // dirty hack for a store
  if (RT->nodes.size() == 1) {
    if (orign < numR) {
      drawEdge(2*numR + 14*numC*orign + 2, orign, oFile);
    }
    else if (orign < 2*numR) {
      drawEdge(2*numR + 14*numC*(orign-numR) + (numC-1)*14+6, orign, oFile);
    }
  }
}

void printNameRIKEN(int n) {
  if (n < numR)
    std::cout << " memport LEFT row " << n;
  else if (n < 2*numR)
    std::cout << " memport RIGHT row " << n-numR;
  else {
    n -= 2*numR;
    int row = n/(14*numC);
    int col = (n - row*numC*14)/14;
    int elem = n % 14;
    switch(elem) {
    case 0: std::cout << " SB_S "; break;
    case 1: std::cout << " SB_SW "; break;
    case 2: std::cout << " SB_W "; break;
    case 3: std::cout << " SB_NW "; break;
    case 4: std::cout << " SB_N "; break;
    case 5: std::cout << " SB_NE "; break;
    case 6: std::cout << " SB_E "; break;
    case 7: std::cout << " SB_SE "; break;
    case 8: std::cout << " SB_INP_B "; break;
    case 9: std::cout << " SB_INP_A "; break;
    case 10: std::cout << " SB_MUX2_A "; break;
    case 11: std::cout << " SB_MUX2_B "; break;
    case 12: std::cout << " CONST "; break;
    case 13: std::cout << " ALU "; break;
    }
    std::cout << "row " << row << " col " << col;
  }
  std::cout << "\n";
}

void printName(int n) {
  if (RIKEN) {
    printNameRIKEN(n);
    return;
  }
  if (n < CGRAdim)
    std::cout << " io" << n;
  else if (n < CGRAdim*2)
    std::cout << " mp" << n-CGRAdim;
  else {
    n -= 2*CGRAdim;
    int row = n/(5*CGRAdim);
    int col = (n - row*CGRAdim*5)/(5);
    int elem = n%5;
    switch(elem) {
    case 0: std::cout << " muxA "; break;
    case 1: std::cout << " muxB "; break;
    case 2: std::cout << " const "; break;
    case 3: std::cout << " ALU "; break;
    case 4: std::cout << " muxOUT "; break;
    }
    std::cout << "row " << row << " col " << col;
  }
  std::cout << "\n";
}


void depositRoute(int signal, std::list<int> *nodes) {
  if (!nodes->size()) {
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
  for (; it != (*nodes).rend(); it++) {
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

int findDriver(int signal) {
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int>::iterator it = RT->nodes.begin();

  // find the node with no parent 
  for (; it != RT->nodes.end(); it++) {
    if (!RT->parent.count(*it))
      return *it;
  }
  return -1;
}

int getSignalCost(int signal) {
  int cost = 0;
  struct RoutingTree *RT = &((*Trees)[signal]);

  std::list<int>::iterator it = RT->nodes.begin();
  for (; it != RT->nodes.end(); it++) {
    cost += (1 << (*Users)[*it].size());
  }
  return cost;
}

bool hasOverlap(int signal) {
  
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int>::iterator it = RT->nodes.begin();

  for (; it != RT->nodes.end(); it++)
    if ((*Users)[*it].size() > 1)
      return true;
  return false;
}

void ripup(int signal, std::list<int> *nodes) {
  std::list<int>::iterator it = (*nodes).begin();
  struct RoutingTree *RT = &((*Trees)[signal]);
  
  for (; it != (*nodes).end(); it++) {
    int delNode = *it;
    RT->nodes.remove(delNode);

    if (RT->parent.count(delNode)) {
      RT->children[RT->parent[delNode]].remove(delNode);
      RT->parent.erase(delNode);
    }
    (*Users)[delNode].remove(signal);
  }
}

void ripUpRouting(int signal) {
  struct RoutingTree *RT = &((*Trees)[signal]);
  std::list<int> toDel;
  toDel.clear();

  std::list<int>::iterator it = RT->nodes.begin();
  for (; it != RT->nodes.end(); it++) {
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

int route(DirectedGraph* G, int signal, int sink, std::list<int> *route, std::map<int, nodeType> *GTypes) {

  route->clear();

  std::vector<int> expInt;
  expInt.clear();
    
  struct RoutingTree *RT = &((*Trees)[signal]);

  struct ExpNode {int i;
    int cost;};
  
  struct customLess {
    bool operator() (const struct ExpNode &l, const struct ExpNode &r) const { return l.cost > r.cost; }
  };
  
  std::priority_queue<struct ExpNode, std::vector<struct ExpNode>, customLess> PRQ;

  std::list<int>::iterator it = RT->nodes.begin();
  for (; it != RT->nodes.end(); it++) {
    int rNode = *it;
    struct ExpNode eNode;
    eNode.cost = 0;
    eNode.i = rNode;
    //    std::cout << "EXPANSION SOURCE : ";
    //    printName(rNode);
    explored.set(rNode);
    expInt.push_back(rNode);
    PRQ.push(eNode);
  }

  //  std::cout << "EXPANSION TARGET: \n";
  //  printName(sink);
  //  std::cout << "SINK USERS: \n";
  //  std::list<int>::iterator sit = (*Users)[sink].begin();
  //  for (; sit != (*Users)[sink].end(); sit++)
  //    std::cout << "\t" << hNames[*sit] << "\n";
  
  struct ExpNode popped;
  bool hit = false;
  while (!PRQ.empty() && !hit) {
    popped = PRQ.top();
    PRQ.pop();
    //    printName(popped.i);
    //    std::cout << "PRQ POP COST: " << popped.cost << "\n";
    if ((popped.cost > 0) &&
	(popped.i == sink)) { // cost must be higher than 0 (otherwise sink was part of initial expansion list)
      hit = true;
      break;
    }

    vertex_descriptor vPopped = vertex(popped.i, *G);
    out_edge_iterator eo, eo_end;
    boost::tie(eo, eo_end) = out_edges(vPopped, *G);
    for (; eo != eo_end; eo++) {
      vertex_descriptor next = target(*eo, *G);
      if (explored.test(next))
	continue;

      explored.set(next);
      expInt.push_back(next);

      // janders CHECK if a MUX, as ONLY MUXes can be used along the way
      if ((next != sink) && ((*GTypes)[next] != mux))
	continue;
      
      struct ExpNode eNodeNew;
      eNodeNew.i = next;
      //      eNodeNew.cost = popped.cost + (4 << (*Users)[next].size());
      //      eNodeNew.cost = popped.cost + (4 << (*Users)[next].size()) + (1 << (*HistoryCosts)[next]);
      eNodeNew.cost = popped.cost + (1 + (*HistoryCosts)[next])*(1 + (*Users)[next].size()*PFac);
      //      if ((*HistoryCosts)[next] >= 2) {
      //	eNodeNew.cost = MAX_DIST;
      //	(*HistoryCosts)[next] = 0;
      //      }
      (*TraceBack)[next] = popped.i;
      if ((eNodeNew.cost > 0) && (next == sink)) { // JANDERS fast targeting
      	popped = eNodeNew;
      	hit = true;
      	break;
      }
      PRQ.push(eNodeNew);
    }
  }

  int retVal;
  
  if (hit) {
    int current = popped.i;
    //  std::cout << "CURRENT: " << current << "\n";                                                                                
    while (1) {
      route->push_back(current);
      if ((*TraceBack)[current] == -1)
	break;
      current = (*TraceBack)[current];
    }
    
    retVal = popped.cost;
  }
  else // if (!hit)
    retVal = MAX_DIST;

  for (int i = 0; i < expInt.size(); i++) {
    explored.reset(expInt[i]);
    (*TraceBack)[expInt[i]] = -1;
  }

  return retVal;
}

void printRouting(int signal) {
  
  struct RoutingTree *RT = &((*Trees)[signal]);
  
  std::cout << "** routing for signal: " << signal << " " << hNames[signal] << "\n";
  
  std::list<int>::iterator it = RT->nodes.begin();
  for (; it != RT->nodes.end(); it++) {
    std::cout << "\t " << *it;
    
    // for ADRES                                                                                                                
    int n = *it;
    printName(n);
  }
}

void printVertexModels(DirectedGraph *H) {

  for (int i = 0; i < num_vertices(*H); i++) {
    printRouting(i);
  }
}

int totalUsed(DirectedGraph *G) {

  int total = 0;
  for (int i = 0; i < num_vertices(*G); i++) {
    int temp = (*Users)[i].size();
    if (temp) total++;
  }
  return total;
}


int totalOveruse(DirectedGraph *G) {

  int total = 0;
  for (int i = 0; i < num_vertices(*G); i++) {
    int temp = (*Users)[i].size() - 1;
    //    total += (temp >= 0) ? temp : 0;
    total += (temp > 0) ? 1 : 0;
    
    if (temp > 0) {
      std::cout << "OVERUSE " << temp << " ON ";
      printName(i);
    }
    
  }

  std::cout << "TOTAL OVERUSE: " << total << "\n";
  
  return total;
}

void adjustHistoryCosts(DirectedGraph *G) {
  for (int i = 0; i < num_vertices(*G); i++) {
    int temp = (*Users)[i].size() - 1;
    temp = (temp >= 0) ? temp : 0;
    if (temp > 0) {
      (*HistoryCosts)[i] = (*HistoryCosts)[i] + 1;
    }
  }
}

int cmpfunc (const void *a, const void *b) {

  int aI = *((int *)a);
  int bI = *((int *)b);

  //int ret = (*TopoOrder)[aI] - (*TopoOrder)[bI];
  
  int ret = ((*Trees)[bI].nodes.size() - (*Trees)[aI].nodes.size());
  if (ret == 0)
    ret = (rand() % 3 - 1);
  return ret;
    
}

void sortList(int *list, int n) {
  qsort(list, n, sizeof(int), cmpfunc);
}

void randomizeList(int *list, int n) {

  // do 3n random swaps
  for (int i = 0; i < 3*n; i++) {
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

int routeSignal(DirectedGraph *G, DirectedGraph *H, int y, std::map<int, nodeType> *GTypes) {

  //  std::cout << "BEGINNING ROUTE OF NAME :" << hNames[y] << "\n";
  
  vertex_descriptor yD = vertex(y, *H);
  int  totalCost = 0;
  
  // i is a candidate root for signal y
  out_edge_iterator eo, eo_end;
  boost::tie(eo, eo_end) = out_edges(yD, *H);
  for (; eo != eo_end; eo++) {
    int load = target(*eo, *H);

    //    std::cout << "SOURCE : " << hNames[y] << " TARGET : " << hNames[load] << "\n";
    
    if (load == y)
      continue; // JANDERS ignore feedback connections for the moment
                // Hamas: Why ignore feedback connections
                // Comments: route feedback to the pins
    
    if ((*Trees)[load].nodes.size() == 0)
      continue; // load is not placed                                                                                           
    
    int loadLoc = findDriver(load);
    //    std::cout << "ROUTING LOAD: ";
    //    printName(loadLoc);
    
    if (loadLoc < 0) {
      std::cout << "SIGNAL WITHOUT A DRIVER.\n";
      exit(-1);
    }
    int cost;
    std::list<int> path;
    cost = route(G, y, loadLoc, &path, GTypes);
    totalCost += cost;
    if (cost < MAX_DIST) {
      path.remove(loadLoc);
      depositRoute(y, &path);
    }
    else {
      //      std::cout << "NAME :" << hNames[y] << " LOAD: \n";
      //      printName(loadLoc);
    }
  }
  return totalCost;
}
	    

int findMinVertexModel(DirectedGraph *G, DirectedGraph *H, int y, 
		       std::map<int, nodeType> *HTypes, std::map<int, nodeType> *GTypes) { //TODO:

  bool allEmpty = true;

  // Hamas: Basically finding the in and out edges of a perticular node in the application DFG to iterate from
  // Hamas: As stated after, H is the application DFG and G is the Routing Resource Graph
  in_edge_iterator ei, ei_end; // Hamas: what is the difference between in_edge_iterator vs out_edge_iterator
  vertex_descriptor yD = vertex(y, *H);
  boost::tie(ei, ei_end) = in_edges(yD, *H); //comment: look for the fan ins and check if any of its node is mapped
  for (; ei != ei_end; ei++) {
    if ((*Trees)[source(*ei, *G)].nodes.size())
      allEmpty=false;
  }
  out_edge_iterator eo, eo_end;
  boost::tie(eo, eo_end) = out_edges(yD, *H); //comment: look for the fin out and check if any of its node is mapped
  for (; eo != eo_end; eo++) {
    if ((*Trees)[target(*eo, *H)].nodes.size())
      allEmpty=false;
  }
  
  if (allEmpty) { // choose a random node of G to be the vertex model for y because NONE of its fanins or fanouts have vertex models
                  // Hamas: Would it be better to map these last as they do not have any dependencies? --> only in the first iterations first node will not have any neighbors
    int chooseRand = (rand() % num_vertices(*G));
    do {
      chooseRand = (chooseRand + 1) % num_vertices(*G);
    } while (((*GTypes)[chooseRand] != (*HTypes)[y]) ); //Hamas: Ensure that the random node the G is the correct type // originally
    //    std::cout << "Chose a random vertex model " << chooseRand << ".\n";      
    (*Trees)[y].nodes.push_back(chooseRand); // Hamas: Review what the Trees and Users vectors are again //TODO:
    (*Users)[chooseRand].push_back(y); //push on the users nodes that uses it
    return 0;
  }   

  // OK, at least one of y's fanins or fanouts have a vertex model //comment: one of the vertex is mapped
  int bestCost = MAX_DIST;
  int bestIndex = -1;
  
  int totalCosts[num_vertices(*G)]; //Hamas: Vector of the total cost, guessing for each node. Cost at each node may differe
  for (int i = 0; i < num_vertices(*G); i++) 
    totalCosts[i] = 0;  
  
  for (int i = 0; i < num_vertices(*G); i++) { //Comment: walk all ndoes through the candidate graph and consider a candidate for the nodes

    if ((*GTypes)[i] != (*HTypes)[y]) //Hamas: Ensure that the random node the G is the correct type
      continue;

    // first route the signal on the output of y

       std::cout << "Trying location:\n"; //Hamas: Uncommented this for experimentation purpose
       printName(i); //Hamas: Uncommented this for experimentation purpose

    ripUpRouting(y); //comment rip up previous routing
    (*Trees)[y].nodes.push_back(i);
    (*Users)[i].push_back(y); //comment: y is the use of node i

    //    totalCosts[i] += (4 << ((*Users)[i].size()-1)); 
    //    totalCosts[i] += (4 << (*Users)[i].size()) + (1 << (*HistoryCosts)[i]);
    totalCosts[i] +=  (1 + (*HistoryCosts)[i])*((*Users)[i].size()*PFac); 
    
    //    std::cout << "INVOKING ROUTE FANOUT SIGNAL\n";
    //    printRouting(y);
    
    totalCosts[i] += routeSignal(G, H, y, GTypes); //Comment: assumes one output pin but may need to support multoiple output pins
                                                  // comment: only routes the output of node y

    //    std::cout << "TOTAL COST " << totalCosts[i] << "\n";
    
    if (totalCosts[i] > bestCost) //comment if the output cost is high, then just skip and do not bother routing the input of node y
      continue;
    
    // now route the signals on the input of y

    boost::tie(ei, ei_end) = in_edges(yD, *H);
    for (; (ei != ei_end); ei++) {
      int driver = source(*ei, *H);
      if ((*Trees)[driver].nodes.size() == 0)
	continue; // driver is not placed

      int driverNode = findDriver(driver);
      ripUpRouting(driver); //comment, find the rip up and reroute the routing //rip up all the routing of driver even if it has multiple fan outs //need to modify the ruip up routing code
      (*Trees)[driver].nodes.push_back(driverNode);
      (*Users)[driverNode].push_back(driver);
      //      ripUpLoad(G, driver, i);
      //      std::cout << "ROUTING FANIN: " << hNames[driver] << "\n";
      
      totalCosts[i] += routeSignal(G, H, driver, GTypes);

      //      std::cout << "COSTS: " << totalCosts[i] << "\n";
      
      if (totalCosts[i] > bestCost) 
      	break;
      
    }
    //    std::cout << "TOTAL COST: " << totalCosts[i] << "\n";

    if (totalCosts[i] >= MAX_DIST)
      continue;
    
    if (totalCosts[i] < bestCost) {
      bestIndex = i;
      bestCost = totalCosts[i];
    }
    else if (totalCosts[i] == bestCost) { //flip if the did not have the best cost
      if (!(rand() % 2))
	bestIndex = i;
    }
  
  }

  if (bestIndex == -1) {
    std::cout << "FATAL ERROR -- COULD NOT FIND A VERTEX MODEL FOR VERTEX " << y << " " << hNames[y] << "\n";
    exit(-1);
  }

  //Comment: rip up the last placement and redo the placement of the best placement which may occur early on.
  ripUpRouting(y);
  (*Trees)[y].nodes.push_back(bestIndex);
  (*Users)[bestIndex].push_back(y);
  
  routeSignal(G, H, y, GTypes);
  
  boost::tie(ei, ei_end) = in_edges(yD, *H);
  for (; ei != ei_end; ei++) {
    int driver = source(*ei, *H);
    if ((*Trees)[driver].nodes.size() == 0)
      continue; // driver is not placed                                                                                         
    
    int driverNode = findDriver(driver);
    ripUpRouting(driver);
    (*Trees)[driver].nodes.push_back(driverNode);
    (*Users)[driverNode].push_back(driver);
    //    ripUpLoad(G, driver, bestIndex);
    routeSignal(G, H, driver, GTypes);
  }
  
  
  return 0;
}


int findMinorEmbedding(DirectedGraph *H, DirectedGraph *G, std::map<int, nodeType> *hTypes, std::map<int, nodeType> *gTypes) { //TODO:
  // determine if H is a minor of G

  int ordering[num_vertices(*H)]; // presently unused --> Hamas: why is it not used?
  for (int i = 0; i < num_vertices(*H); i++) {
    
    ordering[i] = i;
    //Hamas: Added output terminal stream code for experimentation purpose
    std::cout << "List all the ordering start \n";
    std::cout << ordering[i] << " \n"; //Note: Uncommented this out for experimation purposes
    std::cout << "List all the ordering end \n";
  }
  
  bool done = false; bool success=false;
  int iterCount = 0; // Hamas: iteration count

  explored.reset(); // Hamas: bitset -- maybe used to bits explored --> reset the bitset
  float frac;       // Hamas: cost function fraction 
  
  while (!done) {

    iterCount++;
    std::cout << "***** BEGINNING OUTER WHILE LOOP ***** ITER " << iterCount << " \n";
    
    std::cout << "Num vertices: " << num_vertices(*H) << " \n"; //Note: Added commented this out for experimation purposes
    
    //randomizeList(ordering, num_vertices(*H));
    sortList(ordering, num_vertices(*H)); //Hamas: how is this sorted -- would it effect the pins initializing?
                                          //Hamas: Is this used as earleir, it mentions that ordering is not used in GRAMM?
    
    for (int k = 0; k < num_vertices(*H); k++) {

      int y = ordering[k];
      if (RIKEN && ((*hTypes)[y] == constant)) continue;
      //      std::cout << "Finding vertex model for: " << y << " " << hNames[y] << "\n";
      //      std::cout << "SIZE: " << (*Trees)[y].nodes.size() << "\n";
      //      std::cout << "TOPO ORDER: " << (*TopoOrder)[y] << "\n";

      //      if ((iterCount >= 3) && !hasOverlap(y)) // everything should be routed by 3rd iter
      //	continue; // skip signals with no overlap
      
      findMinVertexModel(G, H, y, hTypes, gTypes);
      //      printVertexModels(H);
      
    } // for k

    int TO = totalOveruse(G);

    if (iterCount == 2) {
      frac = (float)TO/totalUsed(G);
      std::cout << "FRACTION OVERLAP: " << frac << "\n";
    }
    
    if ((iterCount >=2) && (TO == 0)) {
      std::cout << "SUCCESS! " << iterCount << " " << frac << " " << num_vertices(*H) << "\n";
      done = true;
      success = true;
    }

    adjustHistoryCosts(G);

    if (iterCount > 39) { // limit the iteration count to ~40 iterations!
      std::cout << "FAILURE. OVERUSED: " << TO << " USED: " << totalUsed(G) << "\n";
      done = true;
    }    
    if (success) {
      if(DEBUG){
        printVertexModels(H);
      }

      //Output stream for storing successful mapping:
      std::ofstream oFile;
      oFile.open("mapping_output.dot");
      std::cout << "Writing the mapping output in file 'mapping_output.dot' \n";
      if (RIKEN) { // print out the solution using NEATO format
        oFile << "digraph {\ngraph [pad=\"0.212,0.055\" bgcolor=lightgray]\nnode [style=filled]\n";
        
        for (int i = 0; i < numR; i++)
          for (int j = 0; j < numC; j++) {
            oFile << "SB_" << i << "_" << j << " [shape=\"circle\" fontsize=6 fillcolor=\"#ff7f0e\" pos=\"" << 2*(j + 1) << "," << 2*(-i) << "!\"]\n";
          }
        
        // rename some signals to be compatible with graphviz
        for (int i = 0; i < num_vertices(*H); i++) {
          std::replace( hNames[i].begin(), hNames[i].end(), '{', '_'); // replace all 'x' to 'y'
          std::replace( hNames[i].begin(), hNames[i].end(), '}', '_'); // replace all 'x' to 'y'  
          
          //Omkar:
          //      Changing current format of _Const_27|float32=5.00_ to Const_27_float32_5_00_
          //      To make compatible with graphviz
          //std::replace( hNames[i].begin(), hNames[i].end(), '_C', 'C');
          std::replace( hNames[i].begin(), hNames[i].end(), '|', '_');
          std::replace( hNames[i].begin(), hNames[i].end(), '=', '_');
          std::replace( hNames[i].begin(), hNames[i].end(), '.', '_');
        }
        
        for (int i = 0; i < num_vertices(*H); i++) {
          
          if ((*hTypes)[i] == constant)
            continue;
          
          printPlaceAndRoute(i, oFile);
        }
        oFile << "}\n";
      }
    }

    PFac *= 1.1; // adjust present congestion penalty
    HFac *= 2; // adjust history of congestion penalty
    
  } // while !done
  
  return 0;
}

void createRIKEN(int NR, int NC, DirectedGraph *G, std::map<int, nodeType> *nodeTypes) {

  nodeTypes->clear();
  //Add total vertices into the graph:
  for (int i = 0; i < (NR + NR + 14*NR*NC); i++)
    boost::add_vertex(*G);

  int PEstep = 14;
  int PEstart = NR + NR; // first NR + NR nodes are I/O nodes; next are the PE nodes

  // add the connections to the LS units on the left and right side#
  for (int i = 0; i < NR; i++) {
    (*nodeTypes)[i] = memport; // left column
    (*nodeTypes)[i+NR] = memport; // right column

    // connectivity to the left-most column
    for (int j = 0; j < 10; j++) {
      if (j == 2) continue;
      boost::add_edge(i, PEstart + i*NC*PEstep + j, *G); // edges from the LS
    } 
    boost::add_edge(PEstart + i*NC*PEstep + 2, i, *G); // edge to the LS

    // connectivity to the right-most column
    for (int j = 0; j < 10; j++) {
      if (j == 6) continue;
      boost::add_edge(NR + i, PEstart + i*NC*PEstep + (NC-1)*PEstep + j, *G);
    }
    boost::add_edge(PEstart + i*NC*PEstep + (NC-1)*PEstep + 6, NR + i, *G);
   
  }

  // the connections between the SBs
  for (int i = 0; i < NR; i++) { // rows
    for (int j = 0; j < NC; j++) { // cols

      int dest_row, dest_col;

      // south
      dest_row = i+1;
      dest_col = j;
      if (i < (NR - 1)) {
        for (int k = 0; k < 10; k++) {
          if (k == 4) continue;
          boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 0,
              PEstart + dest_row*NC*PEstep + dest_col*PEstep + k, *G);
        }
      }
      // south west
      dest_row = i+1;
      dest_col = j-1;
      if ((i < (NR - 1)) && (j > 0)) {
        for (int k = 0; k < 10; k++) {
          if (k == 5) continue;
          boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 1,
              PEstart + dest_row*NC*PEstep + dest_col*PEstep + k, *G);
        }
      }
      // west                                                                                                                                         
      dest_row = i;
      dest_col = j-1;
      if ((j > 0)) {
        for (int k = 0; k < 10; k++) {
          if (k == 6) continue;
          boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 2,
                          PEstart + dest_row*NC*PEstep + dest_col*PEstep + k, *G);
        }
      }
      // north west                                                                                                                                         
      dest_row = i-1;
      dest_col = j-1;
      if ((j > 0) && (i > 0)) {
        for (int k = 0; k < 10; k++) {
          if (k == 7) continue;
          boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 3,
                          PEstart + dest_row*NC*PEstep + dest_col*PEstep + k, *G);
        }
      } 
      // north                                                                                                                                      
      dest_row = i-1;
      dest_col = j;
      if ((i > 0)) {
        for (int k = 0; k < 10; k++) {
          if (k == 0) continue;
          boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 4,
                          PEstart + dest_row*NC*PEstep + dest_col*PEstep + k, *G);
        }
      }
      // north east                                                                                                                                         
      dest_row = i-1;
      dest_col = j+1;
      if ((i > 0) && (j < (NC-1))) {
        for (int k = 0; k < 10; k++) {
          if (k == 1) continue;
          boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 5,
                          PEstart + dest_row*NC*PEstep + dest_col*PEstep + k, *G);
        }
      }
      // east                                                                                                                                         
      dest_row = i;
      dest_col = j+1;
      if ((j < (NC-1))) {
        for (int k = 0; k < 10; k++) {
          if (k == 2) continue;
          boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 6,
                          PEstart + dest_row*NC*PEstep + dest_col*PEstep + k, *G);
        }
      }
      // south east                                                                                                                                         
      dest_row = i+1;
      dest_col = j+1;
      if ((i < (NR - 1)) && (j < (NC-1))) {
        for (int k = 0; k < 10; k++) {
          if (k == 3) continue;
          boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 7,
                          PEstart + dest_row*NC*PEstep + dest_col*PEstep + k, *G);
        }
      }
    }
  }

  // intra-PE connectivity
  for (int i = 0; i < NR; i++) { // rows                                                                                                                    
    for (int j = 0; j < NC; j++) { // cols            

      boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 9, PEstart + i*NC*PEstep + j*PEstep + 10, *G);
      boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 8, PEstart + i*NC*PEstep + j*PEstep + 11, *G);

      // the const
      boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 12, PEstart + i*NC*PEstep + j*PEstep + 10, *G);
      boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 12, PEstart + i*NC*PEstep + j*PEstep + 11, *G);

      // ALU inputs
      boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 10, PEstart + i*NC*PEstep + j*PEstep + 13, *G);
      boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 11, PEstart + i*NC*PEstep + j*PEstep + 13, *G);

      // ALU output to SB outputs
      for (int k = 0; k < 10; k++) // janders -- the ALU can feed back to itself
        boost::add_edge(PEstart + i*NC*PEstep + j*PEstep + 13, PEstart + i*NC*PEstep + j*PEstep + k, *G);    
      }
  }

  // set the types of things
  for (int i = 0; i < NR; i++) { // rows                                                                                                                    
    for (int j = 0; j < NC; j++) { // cols

      int PEindex = PEstart + i*NC*PEstep + j*PEstep;

      for (int k = 0; k < 12; k++)
        (*nodeTypes)[PEindex + k] = mux;

      (*nodeTypes)[PEindex + 12] = constant;
      (*nodeTypes)[PEindex + 13] = alu;

    }
  }

}


void createAdres(int dim, DirectedGraph *G, std::map<int, nodeType> *nodeTypes) {

  nodeTypes->clear();
  
  for (int i = 0; i < (dim + dim + 5*dim*dim); i++)
    boost::add_vertex(*G);

  int IOstart = 0;
  int MPstart = dim;
  int PEstart = dim+dim;
  int PEstep = 5;

  // add the IO connections to the top row
  for (int i = 0; i < dim; i++) {
    (*nodeTypes)[IOstart + i] = io;
    boost::add_edge(IOstart + i, PEstart + i*PEstep, *G);
    boost::add_edge(IOstart + i, PEstart + i*PEstep + 1, *G);
    boost::add_edge(PEstart + i*PEstep + 4, IOstart + i, *G);
  }

  // add the memory port connections to the rows
  for (int i = 0; i < dim; i++) { // rows
    (*nodeTypes)[MPstart + i] = memport;
    for (int j = 0; j < dim; j++) { // cols
      boost::add_edge(MPstart + i, PEstart + i*dim*PEstep + j*PEstep, *G);
      boost::add_edge(MPstart + i, PEstart + i*dim*PEstep + j*PEstep + 1, *G);
      boost::add_edge(PEstart + i*dim*PEstep + j*PEstep + 4, MPstart + i, *G);
    }
  }

  // add the inter-PE connections
  for (int i = 0; i < dim; i++) { // rows                                                                                                   
    for	(int j = 0; j < dim; j++) { // cols     

      int dest_row, dest_col;

      // east
      dest_row = i;
      dest_col = (j + 1)%dim;

      boost::add_edge(PEstart + i*dim*PEstep + j*PEstep + 4, PEstart +	dest_row*dim*PEstep + dest_col*PEstep, *G);
      boost::add_edge(PEstart + i*dim*PEstep + j*PEstep + 4, PEstart +  dest_row*dim*PEstep + dest_col*PEstep + 1, *G);
      
      // west
      dest_row = i;
      dest_col = ((j - 1) >= 0) ? j - 1 : dim - 1;

      boost::add_edge(PEstart + i*dim*PEstep + j*PEstep + 4, PEstart +  dest_row*dim*PEstep + dest_col*PEstep, *G);
      boost::add_edge(PEstart + i*dim*PEstep + j*PEstep + 4, PEstart +  dest_row*dim*PEstep + dest_col*PEstep + 1, *G);
      
      // north
      dest_row = ((i - 1) >= 0) ? i - 1 : dim - 1; 
      dest_col = j;

      boost::add_edge(PEstart + i*dim*PEstep + j*PEstep + 4, PEstart +  dest_row*dim*PEstep + dest_col*PEstep, *G);
      boost::add_edge(PEstart + i*dim*PEstep + j*PEstep + 4, PEstart +  dest_row*dim*PEstep + dest_col*PEstep + 1, *G);
      
      // south
      dest_row = (i + 1)%dim;
      dest_col = j;

      boost::add_edge(PEstart + i*dim*PEstep + j*PEstep + 4, PEstart +  dest_row*dim*PEstep + dest_col*PEstep, *G);
      boost::add_edge(PEstart + i*dim*PEstep + j*PEstep + 4, PEstart +  dest_row*dim*PEstep + dest_col*PEstep + 1, *G);

      
    }
  }

  // add the intra-PE connections
  for (int i = 0; i < dim; i++) { // rows                                                                                                   
    for (int j = 0; j < dim; j++) { // cols                    

      int PEindex = PEstart + i*dim*PEstep + j*PEstep;

      (*nodeTypes)[PEindex] = mux;
      (*nodeTypes)[PEindex+1] = mux;
      (*nodeTypes)[PEindex+2] = constant;
      (*nodeTypes)[PEindex+3] = alu;
      (*nodeTypes)[PEindex+4] = mux;
      
      boost::add_edge(PEindex, PEindex + 3, *G); // mux A to ALU
      boost::add_edge(PEindex + 1, PEindex + 3, *G); // mux B to ALU
      boost::add_edge(PEindex + 2, PEindex, *G); // const to mux A
      boost::add_edge(PEindex + 2, PEindex + 1, *G); // const to mux B
      boost::add_edge(PEindex + 3, PEindex + 4, *G); // ALU to out MUX
      boost::add_edge(PEindex + 0, PEindex + 4, *G); // bypass to out MUX
      boost::add_edge(PEindex + 1, PEindex + 4, *G); // bypass to out MUX
      boost::add_edge(PEindex + 4, PEindex + 0, *G); // feedback path                                      
      boost::add_edge(PEindex + 4, PEindex + 1, *G); // feedback path      
    }
  }
      

  //  std::cout << "NUM VERTS: " << num_vertices(*G) << "\n";

  
  for (int y = 0; y < num_vertices(*G); y++) {
    
    in_edge_iterator ei, ei_end;
    vertex_descriptor yD = vertex(y, *G);
    boost::tie(ei, ei_end) = in_edges(yD, *G);
    for (; ei != ei_end; ei++) {
      //  std::cout << "Vertex " << source(*ei, *G) << " is a FANIN of " << y << "\n";
    }
    out_edge_iterator eo, eo_end;
    boost::tie(eo, eo_end) = out_edges(yD, *G);
    for (; eo != eo_end; eo++) {
      //      std::cout << "Vertex " << target(*eo, *G) << " is a FANOUT of " << y << "\n";
    }
  }
  
}

void computeTopo(DirectedGraph *H, std::map<int, nodeType> *HTypes) {
  
  std::queue<int> fifo;

  TopoOrder = new std::vector<int>(num_vertices(*H));
  
  for (int i = 0; i < num_vertices(*H); i++) {
    (*TopoOrder)[i] = -1;
    if ((*HTypes)[i] == io || (*HTypes)[i] == memport) {
      fifo.push(i);
      (*TopoOrder)[i] = 0; // IOs and memport are at level 0 in the topological ordering
    }
  }

  while (!fifo.empty()) {
    int pop = fifo.front();
    fifo.pop();
    vertex_descriptor nD = vertex(pop, *H);
    out_edge_iterator eo, eo_end;
    boost::tie(eo, eo_end) = out_edges(nD, *H);
    for (; eo != eo_end; eo++) {
      int next = target(*eo, *H);
      if ((*TopoOrder)[next] == -1) {
	(*TopoOrder)[next] = (*TopoOrder)[pop]+1;
	fifo.push(next);
      }
    }
    in_edge_iterator ei, ei_end;
    boost::tie(ei, ei_end) = in_edges(nD, *H);
    for (; ei != ei_end; ei++) {
      int next = source(*ei, *H);
      if ((*TopoOrder)[next] == -1) {
	(*TopoOrder)[next] = (*TopoOrder)[pop]+1;
        fifo.push(next);
      }
    }
  }
}

/*
  readDeviceModle() :: Reading device model dot file
  - DirectedGraph *G                  :: original device model read dot file
  - DirectedGraph *G_Modified         :: Sorted device model dot file based on original intended sequence.
  - std::map<int, nodeTsype> *gTypes  :: Store the type of each node (memport/mux/constant/alu)
*/
void readDeviceModle(DirectedGraph *G, DirectedGraph *G_Modified, std::map<int, nodeType> *gTypes){
  //--------------------------------------------------------------------
  //Omkar:  Reading device model dot file instead.
  //--------------------------------------------------------------------

  /*
    boost::read_graphviz does not preserve the node ordering sequence from the original dot file while reading.
    To enforce the sequence (which has an impact on run-time), the following map is used to compare the read index with the original index.
    (Original index is in the dot file itself supplied as "Arch_label" which is nothing but the node id)

    - std::map<int, int> *node_mapping         :: Key is the original_id and value is unsorted_boost_id  
    - std::map<int, int> *reverse_node_mapping :: Key is the unsorted_boost_id and value is original_id  
  */
  std::map<int, int> node_mapping;
  std::map<int, int> reverse_node_mapping;

  //Iterate over all the nodes of the device model graph and assign the type of nodes:
  for (int i = 0; i < num_vertices(*G); i++) {
    vertex_descriptor v = vertex(i, *G);
    std::string arch_type = boost::get(&DotVertex::arch_type, *G, v);
    std::string arch_label = boost::get(&DotVertex::arch_label, *G, v);
    int gTypes_index = std::stoi(arch_label);   //Arch_label is a node_id basically
                                                //Boost read_graphviz() doesn't preserve the node ordering from the read input dot file.
    
    //Storing the mapping which is later used for sorting:
    node_mapping[gTypes_index] = i;           
    reverse_node_mapping[i] = gTypes_index;

    //Deciding the gTypes based on arch_type attribute which is set in the script itself:
    if (arch_type == "memport") (*gTypes)[gTypes_index] = memport;
    else if (arch_type == "mux") (*gTypes)[gTypes_index] = mux;
    else if (arch_type == "constant") (*gTypes)[gTypes_index] = constant;
    else if (arch_type == "alu") (*gTypes)[gTypes_index] = alu;
  }

  //Adding vertices in new sorted 'G_Modified' graph equal to original 'G' graph.
  //Now the sequence of this will be equal to original intended one:
  // I.E. :
  //        Left-Right IOs first followed by all PEs (Mux (SB) + Mux (PE) + Constant(PE) + ALU(PE))
  for (int i = 0; i < num_vertices(*G); i++) {
    boost::add_vertex(*G_Modified);
  }
  
  //For the intended sequence changing the mapping of edges from unsorted_boost_read of dot file to original_sorted_IDs
  for (const auto& pair : node_mapping) {
      
      //Current source is unsorted_boost_id 
      auto currentSource = pair.second;
      //Finding all outgoing edges of this above source
      std::pair<OutEdgeIterator, OutEdgeIterator> ei = out_edges(currentSource, *G);

      //Iterating over all outgoing edges for one source:
      for (OutEdgeIterator it = ei.first; it != ei.second; ++it) {
          auto target_node = target(*it, *G);  //Finding the current target node for the current source.
          
          //Now while adding edge in new 'G_Modified' graph we want to change the mapping:
          //i.e Source will be "original_id" which is pair.first of node_mapping
          //    Target will also be "original_id" which is value (original_id) of reverse_node_mapping[target_node (in unsorted format)]
          boost::add_edge(pair.first, reverse_node_mapping[target_node], *G_Modified);
      }

  }

}

int main(int argc, char *argv[])
{

  DirectedGraph H, G, G_Modified;

  boost::dynamic_properties dp(boost::ignore_other_properties);
  if (!RIKEN) {
    dp.property("node_id",     boost::get(&DotVertex::name, H));
    dp.property("opcode",       boost::get(&DotVertex::opcode, H));
    /*dp.property("pin",       boost::get(&DotVertex::pin, H)); //Get the pin */
    dp.property("arch_type",       boost::get(&DotVertex::arch_type, G));
  }
  else {
    dp.property("label",     boost::get(&DotVertex::name, H));
    dp.property("type",       boost::get(&DotVertex::opcode, H));
    /*dp.property("pin",       boost::get(&DotVertex::pin, H)); //Get the pin */
    dp.property("arch_type",       boost::get(&DotVertex::arch_type, G));
    dp.property("arch_label",       boost::get(&DotVertex::arch_label, G));
  }
  
  if (argc < 5) {
    std::cout << "Arguments are <application.dot> <device_model.dot> <numRows NR> <numCols NC> <seed>\n";
    exit(-1);
  }

  //Fetching NR and NC:
  int dim = CGRAdim = atoi(argv[3]);
  int NR = dim;
  int NC = atoi(argv[4]);
  numR = NR;
  numC = NC;
  
  std::ifstream iFile;
  iFile.open(argv[1]);

  srand(atoi(argv[5])); //Seed
  
  std::map<int, nodeType> gTypes, hTypes;

  //--------------------------------------------------------------------
  //----------------- STEP 0 : READING DEVICE MODEL --------------------
  //--------------------------------------------------------------------

  if (HARDCODE_DEVICE_MODEL == 0){

    //---------------------------------------------------------------------
    // Old hardcoded device model generation:  
    //---------------------------------------------------------------------  

    if (!RIKEN) {   
      // create ADRES device model
      std::cout << "Creating ADRES Device Model" << std::endl;
      createAdres(dim, &G_Modified, &gTypes);
    }
    else {
      std::cout << "Creating RIKEN Device Model" << std::endl;
      createRIKEN(NR, NC, &G_Modified, &gTypes); // create RIKEN architecture modle
    }	

    if (DEBUG){
      //Omkar: checking the output of manual model for confirmation:
      std::ofstream device_model;
      device_model.open("device_model_hardcode.dot");
      boost::write_graphviz(device_model, G_Modified);
    }
  }
  else {

    //--------------------------------------------------------------------
    //Omkar:  Reading device model dot file instead.
    //--------------------------------------------------------------------

    std::ifstream dFile;                //Defining the input file stream for device model dot file
    dFile.open(argv[2]);                //Passing the device_Model_dot file as an argument!
    boost::read_graphviz(dFile, G, dp); //Reading the dot file


    readDeviceModle(&G, &G_Modified, &gTypes);
  }

  //--------------------------------------------------------------------
  //----------------- STEP 1: READING APPLICATION DOT FILE -------------
  //--------------------------------------------------------------------

  // read the DFG from a file
  boost::read_graphviz(iFile, H, dp);

  for (int i = 0; i < num_vertices(H); i++) {
    vertex_descriptor v = vertex(i, H);
    std::string opcode = boost::get(&DotVertex::opcode, H, v);
    std::string name = boost::get(&DotVertex::name, H, v);
    hNames[i] = name;
    if (DEBUG){
      std::cout << "name " << name <<  " opcode " << opcode << "\n";
    }
    if (!RIKEN) {
      if (opcode == "const") hTypes[i] = constant;
      else if (opcode == "input") hTypes[i] = io;
      else if (opcode == "output") hTypes[i] = io;
      else if (opcode == "load") hTypes[i] = memport;
      else if (opcode == "store") hTypes[i] = memport;
      else {
        hTypes[i] = alu;
      }
    }
    else { // RIKEN CGRA
      if (opcode == "const") hTypes[i] = constant;
      else if (opcode == "input") hTypes[i] = memport;
      else if (opcode == "output") hTypes[i] = memport;
      else {
        hTypes[i] = alu;
      }
    }
  }
  

  Trees = new std::vector<RoutingTree>(num_vertices(H)); // routing trees for every node in H
  Users = new std::vector<std::list<int>>(num_vertices(G_Modified)); // for every node in device model G track its users 
  HistoryCosts = new std::vector<int>(num_vertices(G_Modified)); // for history of congestion in PathFinder
  TraceBack = new std::vector<int>(num_vertices(G_Modified));
  
  for (int i = 0; i < num_vertices(G_Modified); i++) {
    (*Users)[i].clear();
    (*HistoryCosts)[i] = 0;
    (*TraceBack)[i] = -1;
  }

  for (int i = 0; i < num_vertices(H); i++) {
    (*Trees)[i].parent.clear();
    (*Trees)[i].children.clear();
    (*Trees)[i].nodes.clear();
  }

  // compute a topological order
  computeTopo(&H, &hTypes); // not presently used
  
  findMinorEmbedding(&H, &G_Modified, &hTypes, &gTypes);
    
  return 0;
 }
