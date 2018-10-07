#include "ortools/constraint_solver/constraint_solver.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <string>
#include <fstream>

#define ORLIB 0
#define TAILLARD 1


class Data{
  public:
    bool load(std::string filename, int fileType)
    {
      std::ifstream file(filename);
      if(file.fail()) return false;
      if(fileType == ORLIB){
        file>>jobCount>>machineCount;
        while(!file.eof())
        {
            std::vector<int> jobTimes, jobMachines;
            for(int i=0; i<machineCount && !file.eof(); i++)
            {
              int time, machine;
              file>>machine>>time;
              jobTimes.push_back(time);
              jobMachines.push_back(machine);
            }
            machines.push_back(jobMachines);
            times.push_back(jobTimes);
        }
      }
      else if(fileType == TAILLARD){
        //trzeba zaimplementowac parsowanie plikow w tym drugim formacie
        return false;
      }
    }
    std::vector<std::vector<int> > getMachines() {return machines;}
    std::vector<std::vector<int> > getTimes() {return times;}
    int getMachineCount() {return machineCount;}
    int getJobCount() {return jobCount;}
  private:
    std::vector<std::vector<int> > machines;
    std::vector<std::vector<int> > times;
    int machineCount;
    int jobCount;
};

  void run(Data &data){
    using namespace operations_research;
    Solver solver("jobshop");
    int MACHINE_COUNT = data.getMachineCount();
    int JOB_COUNT = data.getJobCount();
    std::vector<std::vector<int> > machines = data.getMachines();
    std::vector<std::vector<int> > times = data.getTimes();
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
      for(int j=0; j<machines[i].size(); j++)
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
    for(int i = 0; i<JOB_COUNT; i++)
    {
      for(int j=0; j<machines[i].size()-1; j++)
      {
          IntervalVar* task1 = allTasks[std::make_pair(i, j)];
          IntervalVar* task2 = allTasks[std::make_pair(i, j+1)];
          Constraint* cons = solver.MakeIntervalVarRelation(task2, Solver::STARTS_AFTER_END, task1);
          solver.AddConstraint(cons);
      }
    }
    //set the objective
    //create end times vector
    std::vector<IntVar*> allEnds;
    for(int i=0; i<JOB_COUNT; i++)
    {
      int size = machines[i].size()-1;
      allEnds.push_back(allTasks[std::make_pair(i, size)] -> EndExpr() -> Var());
    }
    //objective
    IntVar*  objectiveVar = solver.MakeMax(allEnds)->Var();
    OptimizeVar* objectiveMonitor = solver.MakeMinimize(objectiveVar, 1);
    //create search phases
    DecisionBuilder* sequencePhase = solver.MakePhase(allSequences, Solver::SEQUENCE_DEFAULT);
    DecisionBuilder* varsPhase = solver.MakePhase(objectiveVar, Solver::CHOOSE_FIRST_UNBOUND, Solver::ASSIGN_MIN_VALUE);
    DecisionBuilder* mainPhase = solver.Compose(sequencePhase, varsPhase);

    //collector
    SolutionCollector* collector = solver.MakeLastSolutionCollector();

    collector -> Add(allSequences);
    collector -> AddObjective(objectiveVar);
    for(int i=0; i<MACHINE_COUNT; i++)
    {
      SequenceVar* sequence = allSequences[i];
      for(int j=0; j<sequence->size(); j++)
      {
        IntervalVar* t = sequence -> Interval(j);
        collector->Add(t->StartExpr()->Var());
        collector->Add(t->EndExpr()->Var());
      }
    }

    if(solver.Solve(mainPhase, objectiveMonitor, collector))
    std::cout << std::to_string(collector->objective_value(0))<< std::endl;

    /* #cringe, napewno mozna napisac to wyswietlanie lepiej ale narazie
      sam nie wiem o co do konca chodzi w tym kodzie xD;
      ale działa xD*/

    std::unordered_map<std::string, int> taskTimes;
    for(int i = 0; i<MACHINE_COUNT; i++)
    {
      auto seq = allSequences[i];
      auto sequence = collector->ForwardSequence(0, seq);
      for(int j=0; j<sequence.size(); j++)
      {
        auto t = seq->Interval(sequence[j]);
        taskTimes[t->name()] = collector->Value(0, t->StartExpr()->Var());
      }

    }

    for(int i=0; i<JOB_COUNT; i++)
    {
      for(int j=0; j<times[i].size(); j++)
      {
        std::string id = "Job_" + std::to_string(i)+"_"+std::to_string(j);
        std::cout<<taskTimes[id];
         if(j < times[i].size()-1)
           std::cout<<" ";
      }
      std::cout<<std::endl;
    }
  }


int main(int argc, char** argv)
{
  if(argc == 1)
  {
    std::cout<<"No input file specified, terminating...\n";
    return 1;
  }
  Data data;
  std::string filename(argv[1]);
  data.load(filename, ORLIB);
  run(data);
  return 0;
}
