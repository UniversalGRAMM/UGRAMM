#ifndef ROUTING_H
#define ROUTING_H

#include <boost/config.hpp>
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
#include <boost/algorithm/string.hpp>

bool compatibilityCheck(const std::string &gType, const std::string &hOpcode);

void ripup(int signal, std::list<int> *nodes);

void ripUpRouting(int signal, DirectedGraph *G);

int findFunCellFromOutputPin(int signal, DirectedGraph *G);

int findOutputPinFromFuncell(int signal, DirectedGraph *G);

void depositRoute(int signal, std::list<int> *nodes);

int findDriver(int signal, std::map<int, NodeConfig> *gConfig);

int getSignalCost(int signal);

bool hasOverlap(int signal);

int route(DirectedGraph *G, int signal, int sink, std::list<int> *route, std::map<int, NodeConfig> *gConfig);

int totalUsed(DirectedGraph *G);

int totalOveruse(DirectedGraph *G);

void adjustHistoryCosts(DirectedGraph *G);

int cmpfunc(const void *a, const void *b);

void sortList(int *list, int n);

void randomizeList(int *list, int n);

int routeSignal(DirectedGraph *G, DirectedGraph *H, int y, std::map<int, NodeConfig> *gConfig);

#endif