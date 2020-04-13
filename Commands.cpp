#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";


#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif
  #define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

string ReturnWord(const string& cmd_s,int num){
    unsigned long ind;
    string token=cmd_s;
    string requested_word;
    string space = " ";
    for (int i=0;i<num;i++){
        ind=token.find_first_not_of(space);
        if (ind==string::npos || token.empty()){
            requested_word="";
            return requested_word;
        }
        token = token.substr(ind, string::npos);
        if(i==num-1){
            break;
        }
        ind=token.find_first_of(space);
        token=token.substr(ind,string::npos);
    }
    requested_word = token.substr(0,token.find_first_of(space));
    return requested_word;

}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h
Command::Command():cmd_line(""){
    pid=getpid();

};
Command::Command(const char* cmd_line):cmd_line(cmd_line){
    pid=getpid();
}

BuiltInCommand::BuiltInCommand():Command(){}

BuiltInCommand::BuiltInCommand(const char* cmd_line):Command(cmd_line){}

ShowPidCommand::ShowPidCommand():BuiltInCommand(){}
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line){
    jobs_list=jobs;
    string word=ReturnWord(cmd_line,2);
    if(word=="kill"){
        isKillSpecified=true;
    }
    else{
        isKillSpecified=false;
    }
}
void QuitCommand::execute(){
    if(isKillSpecified){
        jobs_list->removeFinishedJobs();
        jobs_list->PrintForQuit();
        jobs_list->killAllJobs();
    }
    else{
        jobs_list->ClearJobsFromList();
    }

}
SmallShell::SmallShell():prompt_name("smash>") {
// TODO: add your implementation

}

SmallShell::~SmallShell() {
// TODO: add your implementation
}
void SmallShell::ChangePrompt(const string new_prompt){
    if(new_prompt==""){
        prompt_name="smash>";
    }
    else{
        prompt_name=new_prompt;
    }
}


void GetCurrDirCommand::execute(){
    char * buf= nullptr;
    size_t size=0;
    getcwd(buf,size);
    std::cout << buf << "\n";
    free(buf);
};



JobsList::JobsList():max_job_id(1) {
    vector<JobEntry*> lst=vector<JobEntry*>();
}
JobsList::~JobsList() {
    lst.clear();
}
void JobsList::addJob(Command* cmd, bool isStopped){
    removeFinishedJobs();
    max_job_id+=1;
    JobEntry* job= new JobEntry(max_job_id,isStopped,cmd);
    lst.push_back(job);

}
JobsList::JobEntry* JobsList::getJobById(int jobId){
    if(lst.empty()){
        return nullptr;
    }
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
        if((*iter)->GetJobID()==jobId){
            return *iter;
        }
    }
    return nullptr;
}

void JobsList::removeFinishedJobs(){
    int status;
    pid_t pid;
    vector<JobEntry*>::iterator iter;
    for (iter=lst.begin(); iter!=lst.end(); iter++){
        if((*iter)->isStopped()){
            continue;
        }
        pid=(*iter)->GetCommand()->GetPID();
        waitpid(pid,&status,WNOHANG);
        if(status==pid){
            lst.erase(iter);
        }
    }
    JobEntry* last_job=lst.back();
    max_job_id=last_job->GetJobID(); //the last job's ID is the maximum ID
}
void JobsList::printJobsList(){
    removeFinishedJobs();
    pid_t pid;
    time_t start_time,now;
    double duration;
    const char* cmd;
    int jobID;
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
        cmd=(*iter)->GetCommand()->GetCmdLine();
        pid=(*iter)->GetCommand()->GetPID();
        start_time=(*iter)->GetStartTime();
        jobID=(*iter)->GetJobID();
        now=time(nullptr);
        duration=difftime(now,start_time);
        std::cout << "[" << jobID << "] " << cmd << " : " << pid << " ";
        std::cout<<duration;
        if((*iter)->isStopped()){
            std::cout << " (stopped)";
        }
        std::cout<< "\n";
    }
}

void JobsList::ClearJobsFromList(){
    lst.clear();
}
void JobsList::killAllJobs(){
    int res;
    pid_t pid;//TODO: use to check return value of kill?
    //TODO: remove finished jobs?
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
        pid=(*iter)->GetCommand()->GetPID();
        kill(pid,SIGKILL);

    }
    lst.clear();
}
void JobsList::PrintForQuit(){
    pid_t pid;
    unsigned long size=lst.size();
    std::cout << "smash: sending SIGKILL signal to " << size <<" jobs:\n";
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
        pid=(*iter)->GetCommand()->GetPID();
        std::cout << pid << ": " << (*iter)->GetCommand() << "\n";
    }
    std::cout <<"Linux-shell:" << "\n";

}
JobsList::JobEntry* JobsList::getLastJob(int* lastJobId){//TODO: is that what they want with the ptr?
    //TODO: check for finished jobs?
    if(lst.empty()){
        return nullptr;
    }
    JobEntry* last_job=lst.back();
    *lastJobId=last_job->GetJobID();
    return last_job;
}
JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId){
    //TODO: check for finished jobs?
    if(lst.empty()){
        return nullptr;
    }
    for (vector<JobEntry*>::reverse_iterator iter=lst.rbegin(); iter!=lst.rend(); iter++){
        if((*iter)->isStopped()){
            *jobId=(*iter)->GetJobID();
            return (*iter);
        }
    }
    return nullptr;
}
void JobsList::removeJobById(int jobId){
    int removed_job_ID=0;
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
        if((*iter)->GetJobID()==jobId){
            removed_job_ID=(*iter)->GetJobID();
            lst.erase(iter);
            if(removed_job_ID==max_job_id){
                max_job_id=lst.back()->GetJobID();
            }
            break;
        }
    }
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    int num_of_args;
    char* arg_list[COMMAND_MAX_ARGS+1];
    num_of_args=_parseCommandLine(cmd_line,arg_list);
    string first_word=string(arg_list[0]);
    if(first_word=="chprompt"){
        ChangePromptCommand* chprompt=new ChangePromptCommand();
        string new_prompt=string(arg_list[1]);
        ChangePrompt(new_prompt);
        return chprompt;
    }
    else if(first_word=="showpid"){
        ShowPidCommand* show_cmd=new ShowPidCommand();
        return show_cmd;
    }
    else if(first_word=="pwd"){}
    else if(first_word=="cd"){}
    else if(first_word=="jobs"){}
    else if(first_word=="kill"){}
    else if(first_word=="fg"){}
    else if(first_word=="bg"){}
    else if(first_word=="quit"){}
    else{
        //TODO: create External command
    }


    // For example:
/*
  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  Command* cmd = CreateCommand(cmd_line);
  if(!cmd){
      return;
  }
  cmd->execute();
  delete cmd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}