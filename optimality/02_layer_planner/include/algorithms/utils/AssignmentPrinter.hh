#ifndef ASSIGNMENT_PRINTER_HH
#define ASSIGNMENT_PRINTER_HH

#include <string>
#include <vector>
#include "Graph.hh"
#include "Robot.hh"
#include "Task.hh"

class AssignmentPrinter {
public:
    static void printBeautifiedAssignment(
        const std::string& algorithmName,
        const std::vector<Robot>& robots,
        const std::vector<std::vector<Task>>& assignment,
        const Graph& graph
    );
};

#endif // ASSIGNMENT_PRINTER_HH
