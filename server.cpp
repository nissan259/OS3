#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
using namespace std;

// Define global variables for graph representation
int n = 0, m = 0; // Number of vertices and edges
vector<list<int>> adj; // Adjacency list for the graph
vector<list<int>> adjT; // Transpose adjacency list for Kosaraju's algorithm

// Helper function to trim leading/trailing whitespace
string trim(const string& str) {
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first) {
        return string();
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

// Function to initialize a new graph
void Newgraph(int clientSocket, int numVertices, int numEdges) {
    n = numVertices;
    m = numEdges;
    adj.clear();
    adj.resize(n);
    adjT.clear();
    adjT.resize(n);
    char buffer[1024];
    for (int i = 0; i < m; ++i) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(clientSocket, buffer, sizeof(buffer));
        if (bytesRead <= 0) {
            break;
        }
        int u = -1, v = -1;
        sscanf(buffer, "%d %d", &u, &v);
        if (u >= 0 && u < n && v >= 0 && v < n) {
            adj[u].push_back(v);
            adjT[v].push_back(u);
        } else {
            string invalidEdge = "Invalid edge: " + string(buffer) + "\n";
            write(clientSocket, invalidEdge.c_str(), invalidEdge.length());
        }
    }
}

// Kosaraju's algorithm function
void Kosaraju(int clientSocket) {
    if (n <= 0 || m <= 0 || m > 2 * n) {
        string result = "Invalid input\n";
        write(clientSocket, result.c_str(), result.length());
        return;
    }
    vector<bool> visited(n, false);
    list<int> order;
    function<void(int)> dfs1 = [&](int u) {
        visited[u] = true;
        for (int v : adj[u]) {
            if (!visited[v]) {
                dfs1(v);
            }
        }
        order.push_back(u);
    };
    for (int i = 0; i < n; ++i) {
        if (!visited[i]) {
            dfs1(i);
        }
    }
    reverse(order.begin(), order.end());
    vector<int> component(n, -1);
    vector<list<int>> components; // To store the nodes of each component
    function<void(int, int)> dfs2 = [&](int u, int comp) {
        component[u] = comp;
        components[comp].push_back(u);
        for (int v : adjT[u]) {
            if (component[v] == -1) {
                dfs2(v, comp);
            }
        }
    };
    int comp = 0;
    for (int u : order) {
        if (component[u] == -1) {
            components.push_back(list<int>()); // Add a new component
            dfs2(u, comp++);
        }
    }
    string result = "Number of strongly connected components: " + to_string(comp) + "\n";
    for (int i = 0; i < comp; ++i) {
        result += "Component " + to_string(i + 1) + ": ";
        for (int node : components[i]) {
            result += to_string(node) + " ";
        }
        result += "\n";
    }
    write(clientSocket, result.c_str(), result.length());
}

// Function to add a new edge
void Newedge(int u, int v) {
    if (u >= 0 && u < n && v >= 0 && v < n) { // Check for valid vertex indices
        adj[u].push_back(v);
        adjT[v].push_back(u);
    } else {
        cerr << "Invalid vertex index: " << u << " " << v << endl;
    }
}

// Function to remove an edge
void Removeedge(int u, int v) {
    if (u >= 0 && u < n && v >= 0 && v < n) { // Check for valid vertex indices
        auto it = find(adj[u].begin(), adj[u].end(), v);
        if (it != adj[u].end()) {
            adj[u].erase(it);
        }
        it = find(adjT[v].begin(), adjT[v].end(), u);
        if (it != adjT[v].end()) {
            adjT[v].erase(it);
        }
    } else {
        cerr << "Invalid vertex index: " << u << " " << v << endl;
    }
}

// Function to handle client commands
void handleClient(int clientSocket) {
    string command;
    char buffer[1024];
    while (true) {
        // Receive command/data from client
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(clientSocket, buffer, sizeof(buffer));
        if (bytesRead <= 0) {
            break;
        }

        string data(buffer, bytesRead);
        istringstream iss(data);
        string line;

        while (getline(iss, line)) {
            line = trim(line);  // Trim leading/trailing whitespace

            if (line.empty()) {
                continue;
            }

            if (line.find("Newgraph") != string::npos) {
                int numVertices, numEdges;
                sscanf(line.c_str(), "Newgraph %d %d", &numVertices, &numEdges);
                write(clientSocket, "Enter edges:\n", 13);
                Newgraph(clientSocket, numVertices, numEdges);
                write(clientSocket, "Graph created\n", 14);
            } else if (line.find("Kosaraju") != string::npos) {
                Kosaraju(clientSocket);
            } else if (line.find("Newedge") != string::npos) {
                int u, v;
                sscanf(line.c_str(), "Newedge %d %d", &u, &v);
                Newedge(u, v);
                write(clientSocket, "Edge added\n", 11);
            } else if (line.find("Removeedge") != string::npos) {
                int u, v;
                sscanf(line.c_str(), "Removeedge %d %d", &u, &v);
                Removeedge(u, v);
                write(clientSocket, "Edge removed\n", 13);
            } 
            else if(line.find("Exit") != string::npos){
                write(clientSocket, "Exiting...\n", 11);
                close(clientSocket);
                break;
            }
            else {
                write(clientSocket, "Invalid command\n", 16);
            }
        }
    }
    close(clientSocket);
}

// Function to create server socket
int createServerSocket(int port) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Failed to create socket" << endl;
        return -1;
    }

    int optval = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Failed to bind socket" << endl;
        close(serverSocket);
        return -1;
    }

    if (listen(serverSocket, 5) == -1) {
        cerr << "Failed to listen on socket" << endl;
        close(serverSocket);
        return -1;
    }

    return serverSocket;
}

int main() {
    int serverSocket = createServerSocket(9034);
    if (serverSocket == -1) {
        return 1;
    }

    cout << "Server started. Listening on port 9034..." << endl;

    while (true) {
        sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressLength);
        if (clientSocket == -1) {
            cerr << "Failed to accept client connection" << endl;
            continue;
        }

        handleClient(clientSocket);
    }

    close(serverSocket);

    return 0;
}
//make
//./server
//telnet localhost 9034
//Newgraph 3 3
//0 1
//0 2
//1 2
//Kosaraju
//Newedge 2 0
//Removeedge 0 1
//Kosaraju
