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
        pid = smash.GetCurrCmd()->GetPID();
        int res1 = killpg(pid, SIGSTOP);
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
        int res = kill(pid, SIGSTOP);
        if (res == -1) {
            perror("smash error: kill failed");
            return;
        }
    }
    JobsList* jobs=smash.GetJobList();
    Command* new_stopped_cmd=smash.GetCurrCmd();
    if(new_stopped_cmd->GetCounter()==0 &&!new_stopped_cmd->GetBackground()){
        jobs->removeFinishedJobs();
        jobs->addJob(new_stopped_cmd,true);
        new_stopped_cmd->IncCounter();

    }
    else{
        int jobid=new_stopped_cmd->GetJobID();
        JobsList::JobEntry* entry = jobs->getJobById(jobid);
        entry->SetNewStartTime();
        if(!entry->isStopped()){
            entry->ChangeStoppedStatus();
        }

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
        pid = smash.GetCurrCmd()->GetPID();
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
    std::cout << "smash: got an alarm\n";
    int jobid;
    int res1=0;
    pid_t pid1;
    jobs->removeFinishedJobs();
    jobid=first->GetCommand()->GetJobID();
    JobsList::JobEntry* entry=jobs->getJobById(jobid);
    if((!entry && first->GetCommand()->GetBackground())||!first->GetCommand()->GetIsExternal()){
        times->removeTimedJob();
        return;
    }
    pid1=first->GetCommand()->GetPID();
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

