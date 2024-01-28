#include<stdio.h>
#include<string>
#include<stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <tuple>
#include <set>
#include <algorithm>
using namespace std;

string readFile(string file_path){
    string content;
    ifstream file(file_path);
    char c;
    if (!file.is_open()) {
        cout << "Could not open file" << endl;
        return "";
    }
    while (file){
        c = file.get();
        content += c;
    }
    file.close();
    return content;
}

void writeFile(string file_path, string content){
    ofstream file(file_path);
    if (!file.is_open()) {
        cout << "Could not open file" << endl;
        return;
    }
    file << content;
    file.close();
}

vector<tuple<int, int, string>> parseMessage(string content){
    vector<tuple<int, int, string>> message;
    string token;
    stringstream ss(content), ss2;
    int from, to;
    while (getline(ss, token)){
        if (token.length() == 1) break;
        ss2 = stringstream(token);
        getline(ss2, token, ' ');
        from = stoi(token);
        getline(ss2, token, ' ');
        to = stoi(token);
        getline(ss2, token);
        message.push_back(make_tuple(from, to, token));
    }
    return message;
}

vector<tuple<int, int, int>> parseEdge(string content, bool undirected = false){
    vector<tuple<int, int, int>> edge;
    string token;
    stringstream ss(content), ss2;
    int from, to, cost;
    while (getline(ss, token)){
        if (token.length() == 1) break;
        ss2 = stringstream(token);
        getline(ss2, token, ' ');
        from = stoi(token);
        getline(ss2, token, ' ');
        to = stoi(token);
        getline(ss2, token, ' ');
        cost = stoi(token);
        edge.push_back(make_tuple(from, to, cost));
        if (undirected) edge.push_back(make_tuple(to, from, cost));
    }
    return edge;
}

int getNextHopNode(int start_node, int current_node, map<int, vector<int>> &prev_list, set<int> &known_next_hop_node) {
    if (current_node == start_node) return current_node;
    if (known_next_hop_node.find(current_node) != known_next_hop_node.end()) return 1000000000;
    int next_hop_node = -1, lowest_next_hop_node = -1;
    for (auto const &prev: prev_list[current_node]){
        next_hop_node = getNextHopNode(start_node, prev, prev_list, known_next_hop_node);
        if (next_hop_node == start_node) return current_node;
        if (lowest_next_hop_node == -1)
            lowest_next_hop_node = next_hop_node;
        else
            lowest_next_hop_node = min(lowest_next_hop_node, next_hop_node);
    }
    known_next_hop_node.insert(current_node);
    return lowest_next_hop_node;
}

map<int, pair<int, int>> getTopologyEntries(set<int> &nodes, int start_node, map<int, int> &dis, map<int, vector<int>> &prev_list) {
    map<int, pair<int, int>> topology_entries;
    for (auto const &node: nodes) {
        if (dis.find(node) == dis.end()) continue;
        set<int> known_next_hop_node;
        topology_entries[node] = make_pair(getNextHopNode(start_node, node, prev_list, known_next_hop_node), dis[node]);
    }
    return topology_entries;
}

pair<map<int, int>, map<int, vector<int>>> bellmanFordAlgorithm(set<int> &nodes, int start_node, vector<tuple<int, int, int>> &edge){
    map<int, int> dis;
    map<int, vector<int>> prev;
    bool relaxed;

    dis[start_node] = 0;
    prev[start_node].push_back(start_node);

    for (int i = 0 ; i < nodes.size() - 1 ; i++){
        relaxed = false;
        for (auto &[from, to, cost]: edge){
            if (cost == -999) continue;
            if (dis.find(from) != dis.end() && (dis.find(to) == dis.end() || dis[from] + cost <= dis[to])){
                if (dis[from] + cost < dis[to]) prev[to].clear();
                dis[to] = dis[from] + cost;
                prev[to].push_back(from);
                relaxed = true;
            }
        }
        if (!relaxed) break;
    }
    return make_pair(dis, prev);
}

string distanceVectorRoutingProtocol(vector<tuple<int, int, int>> &topology, vector<tuple<int, int, string>> &messages){
    map<int, map<int, pair<int, int>>> topology_entries_all;
    int next_hop_node;
    string content;
    set<int> nodes;
    for (auto const &[from, to, cost]: topology) {
        if (nodes.find(from) == nodes.end()) nodes.insert(from);
        if (nodes.find(to) == nodes.end()) nodes.insert(to);
    }
    for (auto const &current_node: nodes){
        auto [dis, prev] = bellmanFordAlgorithm(nodes, current_node, topology);
        topology_entries_all[current_node] = getTopologyEntries(nodes, current_node, dis, prev);
        for (auto const &[destination_node, hop_cost_pair]: topology_entries_all[current_node]){
            content += to_string(destination_node) + " " + to_string(hop_cost_pair.first) + " " + to_string(hop_cost_pair.second) + "\n";
        }
    }
    for (auto const &[start_node, destination_node, message]: messages) {
        if (topology_entries_all.find(start_node) == topology_entries_all.end() || topology_entries_all[start_node].find(destination_node) == topology_entries_all[start_node].end()) {
            content += "from " + to_string(start_node) + " to " + to_string(destination_node)+ " cost infinite hops unreachable message " + message + "\n";
            continue;
        }
        next_hop_node = start_node;
        content += "from " + to_string(start_node) + " to " + to_string(destination_node) + " cost " + to_string(topology_entries_all[start_node][destination_node].second) + " hops ";
        while (next_hop_node != topology_entries_all[next_hop_node][destination_node].first) {
            content += to_string(next_hop_node) + " ";
            next_hop_node = topology_entries_all[next_hop_node][destination_node].first;
        }
        content += "message " + message + "\n";
    }
    return content;
}

int main(int argc, char** argv) {
    vector<tuple<int, int, int>> topology, changes;
    vector<tuple<int, int, string>> messages;
    bool added;
    string content;
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    topology = parseEdge(readFile(argv[1]), true);
    messages = parseMessage(readFile(argv[2]));
    changes = parseEdge(readFile(argv[3]));

    content += distanceVectorRoutingProtocol(topology, messages);
    for (auto const &[from, to, cost]: changes) {
        added = false;
        for (int i=0;i<topology.size();i ++){
            if (get<0>(topology[i]) == from && get<1>(topology[i]) == to){
                added = true;
                topology[i] = make_tuple(from, to, cost);
            }
            if (get<0>(topology[i]) == to && get<1>(topology[i]) == from)
                topology[i] = make_tuple(to, from, cost);
        }
        if (!added) {
            topology.push_back(make_tuple(from, to, cost));
            topology.push_back(make_tuple(to, from, cost));
        }
        content += distanceVectorRoutingProtocol(topology, messages);
    }
    writeFile("output.txt", content);
    return 0;
}
