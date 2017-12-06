// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// DFS.cpp
//
// Prints the results of traversing the graph defined by cities.txt
// using depth first search
//
#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <string>
#include <vector>
using namespace std;

struct Node
{
    string name;
    list<int> neighbors;
};

queue<int> doDepthFirstSearch(int iOriginNode, vector<Node>& database)
{
    queue<int>   searchOrder;
    vector<bool> visited (database.size(), false);
    vector<int>  toVisit;

    toVisit.push_back(iOriginNode);

    while (!toVisit.empty()) {
        int i = toVisit.back(); toVisit.pop_back();
        if (!visited[i]) {
            visited[i] = true;
            searchOrder.push(i);
            for (auto it = database[i].neighbors.rbegin(),
                end = database[i].neighbors.rend();
                it != end;
                ++it
            ) {
                if (!visited[*it]) {
                    toVisit.push_back(*it);
                }
            }
        }
    }
    return searchOrder;
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
        string fromCity, toCity;

        // read one edge
        getline(fin, fromCity);
        getline(fin, toCity);
        fin.ignore(1000, 10); // skip the line with distance
        fin.ignore(1000, 10); // skip the separator

        // add vertices for new cities included in the edge
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
        database[iFromNode].neighbors.push_back(iToNode);
        database[iToNode].neighbors.push_back(iFromNode);
    }
    fin.close();
    cout << "Input file processed\n\n";

    while (true)
    {
        // get the start city for the search
        string startCity;
        cout << "\nEnter the start city [blank to exit]: ";
        getline(cin, startCity);
        if (startCity.length() == 0) break;

        // find the start city
        int i;
        for (i = 0; i < database.size(); i++)
            if (database[i].name == startCity)
                break;

        // BFS result by copy-pop
        cout << "BFS";
        for (queue<int> q = doDepthFirstSearch(i, database); !q.empty(); q.pop())
            cout  << '-'<< database[q.front()].name;
        cout << endl;
    }
}