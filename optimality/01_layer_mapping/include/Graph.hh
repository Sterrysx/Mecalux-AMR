#ifndef GRAPH_H
#define GRAPH_H

class Graph {
public:
    Graph(int vertices);
    ~Graph();
    void addEdge(int src, int dest);
    void removeEdge(int src, int dest);
    void display() const;

private:
    int** adjMatrix;
    int numVertices;
};

#endif // GRAPH_H
