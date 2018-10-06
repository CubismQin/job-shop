#include "ortools/constraint_solver/constraint_solver.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <string>

using namespace operations_research;

  void run(){
    Solver solver("jobshop");
    const int MACHINE_COUNT = 3;
    const int JOB_COUNT = 3;
    std::vector<std::vector<int> > machines;
    std::vector<std::vector<int> > times;
    //compute horizon
    int horizon = 0;
    for (int i = 0; i < times.size(); i++) {
      for (int j = 0; j < times[i].size(); j++) {
        horizon += times[i][j];
      }
    }
    //create jobs
    std::unordered_map<std::pair<int, int>, IntervalVar*> allTasks;
    for(int i=0; i<JOB_COUNT; i++)
      for(int j=0; i<machines[i].size(); j++)
        allTasks[std::make_pair(i, j)] = solver.MakeFixedDurationIntervalVar(0,
           horizon,
           times[i][j],
           false,
           "Job_" + std::to_string(i) + "_" + std::to_string(j));
    //create sequence variables and add disjunctive constraints
    std::vector<SequenceVar*> allSequences;
    for(int i = 0; i<MACHINE_COUNT; i++)
    {
      std::vector<IntervalVar*> machinesJobs;
      for(int j = 0; j<JOB_COUNT; j++)
        for(int k = 0; k<machines[j].size(); k++)
          if(machines[j][k] == i)
            machinesJobs.push_back(allTasks[std::make_pair(j, k)]);
      DisjunctiveConstraint* disj = solver.MakeDisjunctiveConstraint(machinesJobs, "machine " + std::to_string(i));
      allSequences.push_back(disj->MakeSequenceVar());
      solver.AddConstraint(disj);
    }
    //add conjuctice constraints
    // for(int i = 0; i<JOB_COUNT; i++)
    // {
    //   for(int j=0; machines[i].size()-1)
    //     solver.AddConstraint(allTasks[std::make_pair(i, j+1)])
    // }
  }



int main()
{
  
}
