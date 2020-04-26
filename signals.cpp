#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-Z\n";
    pid_t pid=smash.GetCurrCmd()->GetPID();
    if(pid==smash.GetPID()){
        return;
    }
    int res=kill(pid,SIGSTOP);
    if(res==-1){
        perror("smash error: kill failed\n");
        return;
    }
    JobsList* jobs=smash.GetJobList();
    Command* new_stopped_cmd=smash.GetCurrCmd();
    jobs->addJob(new_stopped_cmd,true);
    std::cout << "smash: process "<< pid << " was stopped\n";
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-C\n";
    pid_t pid=smash.GetCurrCmd()->GetPID();
    if(pid==smash.GetPID()){
        return;
    }
    int res=kill(pid,SIGKILL);
    if(res==-1){
        perror("smash error: kill failed\n");
        return;
    }
    std::cout << "smash: process "<< pid << " was killed\n";
}

void alarmHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    JobsList* jobs=smash.GetJobList();
    TimeoutList* times=smash.GetTimeoutList();
    TimeoutList::TimeoutEntry* first=times->GetFirstEntry();
    int jobid;
    int res1=0,res2=0;
    pid_t pid1,pid2;
    jobs->removeFinishedJobs();
    if(!first->GetCommand()){
        times->removeTimedJob();
        return;
    }
    pid1=first->GetCommand()->GetPID();
    pid2=first->GetCommand()->GetInternalCommand()->GetPID();
    std::cout << "smash: got an alarm\n";
    if(pid1==pid2){
        res1=kill(pid1,SIGKILL);
    }
    else{
        res1=kill(pid1,SIGKILL);
        res2=kill(pid2,SIGKILL);
    }

    if(res1==-1||res2==-1){
        perror("smash error: waitpid failed\n");
    }
    std::cout << first->GetCommand()->GetCmdLine() << " timed out!\n";
    if(first->GetBackGround()){
        jobs->removeJobById(first->GetJobID());
    }
    times->removeTimedJob();
}

