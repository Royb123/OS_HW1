#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-Z\n";
    if(!smash.GetCurrCmd()){
        return;
    }
    pid_t pid;
    if(smash.GetCurrCmd()->GetIsPipe()) {
        PipeCommand *p_cmd = (PipeCommand *) smash.GetCurrCmd();
        pid = p_cmd->GetPID();
        int res1 = killpg(pid, SIGSTOP);
        if (res1 == -1) {
            perror("smash error: kill failed");
            return;
        }
    }
    else {
        pid = smash.GetCurrCmd()->GetPID();
        std::cout << "pid is: " << pid;
        if (pid == smash.GetPID()) {
            return;
        }
        int res = kill(pid, SIGSTOP);
        if (res == -1) {
            perror("smash error: kill failed");
            return;
        }
    }
    JobsList* jobs=smash.GetJobList();
    Command* new_stopped_cmd=smash.GetCurrCmd();
    if(new_stopped_cmd->GetCounter()==0){
        jobs->removeFinishedJobs();
        jobs->addJob(new_stopped_cmd,true);
        new_stopped_cmd->IncCounter();

    }
    std::cout << "smash: process "<< pid << " was stopped\n";
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    pid_t pid;
    std::cout << "smash: got ctrl-C\n";
    if(!smash.GetCurrCmd()){
        return;
    }
    if(smash.GetCurrCmd()->GetIsPipe()) {
        PipeCommand *p_cmd = (PipeCommand *) smash.GetCurrCmd();
        pid = p_cmd->GetPID();
        int res1 = killpg(pid, SIGKILL);
        if (res1 == -1) {
            perror("smash error: kill failed");
            return;
        }
    }
    else {
        pid = smash.GetCurrCmd()->GetPID();
        if (pid == smash.GetPID()) {
            return;
        }
        int res = kill(pid, SIGKILL);
        if (res == -1) {
            perror("smash error: kill failed");
            return;
        }
    }
    std::cout << "smash: process "<< pid << " was killed\n";
}

void alarmHandler(int sig_num){
    SmallShell& smash = SmallShell::getInstance();
    JobsList* jobs=smash.GetJobList();
    TimeoutList* times=smash.GetTimeoutList();
    TimeoutList::TimeoutEntry* first=times->GetFirstEntry();
    int jobid;
    int res1=0;
    pid_t pid1;
    jobs->removeFinishedJobs();
    jobid=first->GetCommand()->GetJobID();
    JobsList::JobEntry* entry=jobs->getJobById(jobid);

    if(!entry && first->GetCommand()->GetBackground()){
        times->removeTimedJob();
        return;
    }
    pid1=first->GetCommand()->GetPID();
    std::cout << "smash: got an alarm\n";
    int status;
    waitpid(pid1,&status,WNOHANG|WUNTRACED);
    if(!WIFSIGNALED(status)){
        res1=kill(pid1,SIGKILL);
        if(res1==-1){
            perror("smash error: kill failed");
            return;
        }
    }
    std::cout << first->GetCommand()->GetCmdLine() << " timed out!\n";
    if(first->GetBackGround()){
        jobs->removeJobById(first->GetJobID());
    }
    times->removeTimedJob();
}

