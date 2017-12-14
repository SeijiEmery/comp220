// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// GraphCheapest.cpp
//
// Executes and prints the results of a graph search between two cities
// using a shortest-route (unweighted edges) algorithm.
//

#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <stack>
#include <string>
#include <vector>
#include <utility>
using namespace std;

#include <cstdlib>
#include <cassert>

struct Node
{
    typedef pair<int,double> Edge;

    string      name;
    list<Edge>  neighbors;
    double      cost;
    int         prev;
    bool        isVisited;
};

pair<stack<int>, double> getShortestRoute(int iStart, int iEnd, vector<Node>& database)
{
    // Reset internal graph state
    for (auto& node : database) {
        node.prev = -1;
        node.cost = 0;
    }
    pair<stack<int>, double> result;    // used only at end to accumulate results
    queue<int> toVisit;
    toVisit.push(iStart);
    database[iStart].isVisited = true;

    while (!toVisit.empty()) {
        int i = toVisit.back(); toVisit.pop();
        for (auto& edge : database[i].neighbors) {
            if (database[edge.first].isVisited) {
                continue;
            }
            database[edge.first].isVisited = true;
            database[edge.first].cost = database[i].cost + 1;
            database[edge.first].prev = i;
            toVisit.push(edge.first);

            // Found destination node -- build results and return
            if (edge.first == iEnd) {
                assert(result.first.empty());
                assert(result.second == 0);

                for (i = iEnd; i >= 0; i = database[i].prev) {
                    assert(database[i].isVisited);
                    database[i].isVisited = false;
                    result.first.push(i);
                    result.second += 1;
                }
                --result.second;    // don't count 1st / starting node in # of connections
                return result;
            }
        }
    }
    // Destination node not found; return "empty" results (should default to empty stack, cost zero)
    assert(result.first.empty());
    assert(result.second == 0); 
    return result;
}

int main()
{
    std::cout << "Programmer: Seiji Emery\n"
              << "Programmer's id: M00202623\n"
              << "File: " __FILE__ "\n\n";

    ifstream fin;
    fin.open("cities.txt");
    if (!fin.good()) throw "I/O error";  

    // process the input file
    vector<Node> database;
    while (fin.good()) // EOF loop
    {
        string fromCity, toCity, cost;

        // read one edge
        getline(fin, fromCity);
        getline(fin, toCity);
        getline(fin, cost);
        fin.ignore(1000, 10); // skip the separator

        // add nodes for new cities included in the edge
        int iToNode = -1, iFromNode = -1, i;
        for (i = 0; i < database.size(); i++) // seek "to" city
            if (database[i].name == fromCity)
                break;
        if (i == database.size()) // not in database yet
        {
            // store the node if it is new
            Node fromNode = {fromCity};
            database.push_back(fromNode);
        }
        iFromNode = i; 

        for (i = 0; i < database.size(); i++) // seek "from" city
            if (database[i].name == toCity)
                break;
        if (i == database.size()) // not in vector yet
        {
            // store the node if it is new
            Node toNode = {toCity};
            database.push_back(toNode);
        }
        iToNode = i; 

        // store bi-directional edges
        double edgeCost = atof(cost.c_str());
        database[iFromNode].neighbors.push_back(pair<int, double>(iToNode, edgeCost));
        database[iToNode].neighbors.push_back(pair<int, double>(iFromNode, edgeCost));
    }
    fin.close();
    cout << "Input file processed\n\n";

    while (true)
    {
        string fromCity, toCity;
        cout << "\nEnter the source city [blank to exit]: ";
        getline(cin, fromCity);
        if (fromCity.length() == 0) break;

        // find the from city
        int iFrom;
        for (iFrom = 0; iFrom < database.size(); iFrom++)
            if (database[iFrom].name == fromCity)
                break;

        cout << "Enter the destination city [blank to exit]: ";
        getline(cin, toCity);
        if (toCity.length() == 0) break;

        // find the destination city
        int iTo;
        for (iTo = 0; iTo < database.size(); iTo++)
            if (database[iTo].name == toCity)
                break;

        pair<stack<int>, double> result = getShortestRoute(iFrom, iTo, database);
        cout << "Total edges: " << result.second;  
        for (; !result.first.empty(); result.first.pop())
            cout << '-' << database[result.first.top()].name;
        cout << endl;
    }
}