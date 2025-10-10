#include "../include/Task.hh"
using namespace std;

Task::Task() : taskId(0), originNode(0), destinationNode(0) {}

Task::Task(int taskId, int originNode, int destinationNode)
    : taskId(taskId), originNode(originNode), destinationNode(destinationNode) {}

Task::~Task() {}

int Task::getTaskId() const {
    return taskId;
}

int Task::getOriginNode() const {
    return originNode;
}

int Task::getDestinationNode() const {
    return destinationNode;
}

void Task::setTaskId(int taskId) {
    this->taskId = taskId;
}

void Task::setOriginNode(int originNode) {
    this->originNode = originNode;
}

void Task::setDestinationNode(int destinationNode) {
    this->destinationNode = destinationNode;
}

void Task::readFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return;
    }

    int id, origin, destination;
    while (file >> id >> origin >> destination) {
        Task task(id, origin, destination);
        // Add task to the queue
    }

    file.close();
}