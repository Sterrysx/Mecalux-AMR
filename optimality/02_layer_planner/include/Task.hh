#ifndef TASK_HH
#define TASK_HH

#include <iostream>

//Task class for warehouse robot system
class Task {

public:
    // Constructors and Destructor
    Task();
    Task(int taskId, int originNode, int destinationNode);
    ~Task();

    // Getters
    int getTaskId() const;
    int getOriginNode() const;
    int getDestinationNode() const;

    // Setters
    void setTaskId(int packetId);
    void setOriginNode(int originNode);
    void setDestinationNode(int destinationNode);

private:
    int taskId;
    int originNode;
    int destinationNode;
};


#endif // TASK_HH