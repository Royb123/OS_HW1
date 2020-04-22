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
    jobs->addJob(smash.GetCurrCmd(),true);
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
    std::cout << "smash: got an alarm\n";




}

