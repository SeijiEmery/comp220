// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// GraphCheapest.cpp
//
// Executes and prints the results of a graph search between two cities
// using a cheapest-route (weighted edges) algorithm.
//

#include <fstream>
#include <iostream>
#include <iomanip>
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

struct Terminus {
    int         index;
    int         prev;
    double      cost;

    Terminus (int index, int prev = -1, double cost = 0)
        : index(index), prev(prev), cost(cost) {}
    Terminus (const Terminus&) = default;
    Terminus& operator= (const Terminus& other) = default;
};
bool operator< (const Terminus& a, const Terminus& b) {
    return b.cost < a.cost;     // reverse logic for low-to-high
}

void printGraph (vector<Node>& database) {
    // Write out graph state:
    std::vector<bool> bitset;

    std::cout << '\n' << std::setw(16) << " ";
    for (size_t i = 0; i < database.size(); ++i) {
        std::cout << ' ' << (database[i].name.size() ? database[i].name[0] : '-');
    }

    for (size_t i = 0; i < database.size(); ++i) {
        std::cout << '\n' << std::setw(16) << database[i].name;

        bitset.clear(); bitset.resize(database.size(), false);
        for (const auto& edge : database[i].neighbors) {
            bitset[edge.first] = true;
        }

        for (size_t j = 0; j < database.size(); ++j) {
            std::cout << ' ' << (bitset[j] ? 'X' : '.');
        }
    }
    std::cout << std::endl;
}

pair<stack<int>, double> getCheapestRoute(int iStart, int iEnd, vector<Node>& database)
{
    // Reset internal graph state
    for (auto& node : database) {
        node.prev = -1;
        node.cost = 0;
        node.isVisited = false;
    }
    pair<stack<int>, double> result;    // used only at end to accumulate results
    priority_queue<Terminus> toVisit;
    toVisit.emplace(iStart);

    while (!toVisit.empty()) {
        auto t = toVisit.top(); toVisit.pop();
        if (database[t.index].isVisited) {
            continue;
        }
        database[t.index].isVisited = true;
        database[t.index].cost      = t.cost;
        database[t.index].prev      = t.prev;

        // std::cout << "Exploring '" 
        //     << database[t.index].name << "', " 
        //     << t.cost;
        // if (t.prev >= 0) {
        //     std::cout << " (from '" 
        //         << database[t.prev].name 
        //         << "')\n";
        // } else {
        //     std::cout << " (root)\n";
        // }

        // If found destination node, build results and return
        if (t.index == iEnd) {
            // std::cout << "Reached target\n";
            assert(result.first.empty());
            assert(result.second == 0);

            // std::cout << "Reverse path: ";
            for (int i = iEnd; i >= 0; i = database[i].prev) {
                // std::cout << '\'' << database[i].name << "', ";

                assert(database[i].isVisited);
                database[i].isVisited = false;
                result.first.push(i);
            }
            // std::cout << "\b\b\n";
            result.second = database[iEnd].cost;
            return result;
        }

        // Otherwise, explore / add neighbors
        for (const auto& edge : database[t.index].neighbors) {
            // std::cout << "Inserting edge: '"
            //     << database[edge.first].name << "', "
            //     << edge.second << " + " << t.cost << " = "
            //     << (edge.second + t.cost) << '\n';
            toVisit.emplace(edge.first, t.index,
                edge.second + t.cost);
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

    // printGraph(database);

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

        pair<stack<int>, double> result = getCheapestRoute(iFrom, iTo, database);
        cout << "Total miles: " << result.second;  
        for (; !result.first.empty(); result.first.pop())
            cout << '-' << database[result.first.top()].name;
        cout << endl;
    }
}