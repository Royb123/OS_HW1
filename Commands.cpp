#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <fstream>
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
/*========================================================================*/
/*========================General Help Functions==========================*/
/*========================================================================*/

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

void FreeCmdArray(char ** arg_list,int num_of_args){
    for (int i = 0; i < num_of_args;i++) {
        if (arg_list[i] == NULL) break;
        free(arg_list[i]);
    }
}


int FindIfIO(char** arg_list,int num_of_args){
    int res=REGULAR;
    for(int i=0;i<num_of_args;i++){
        if(string(arg_list[i])==">"){
            res=OVERRIDE;
            break;
        }
        else if(string(arg_list[i])==">>"){
            res=APPEND;
            break;
        }
        else if(string(arg_list[i])=="|"){
            res=PIPE;
            break;
        }
        else if(string(arg_list[i])=="|&"){
            res=PIPEERR;
            break;
        }

    }
    return res;
}
/*========================================================================*/
/*========================Command Class Functions=========================*/
/*========================================================================*/


// TODO: Add your implementation for classes in Commands.h
Command::Command():cmd_line(""){
    pid=getpid();

};
Command::Command(const char* cmd_line):cmd_line(cmd_line){
    pid=getpid();
}

/*========================================================================*/
/*===========================Built In Commands============================*/
/*========================================================================*/

/*---------------------BuiltInCommand Class Functions---------------------*/
BuiltInCommand::BuiltInCommand():Command(){}

BuiltInCommand::BuiltInCommand(const char* cmd_line):Command(cmd_line){}

/*-------------------------Command Redirection--------------------*/
RedirectionCommand::RedirectionCommand(const char* cmd_line,int type,Command* cmd):
Command(cmd_line),type(type),cmd(cmd){}

void RedirectionCommand::execute() {
    char* arg_list[COMMAND_MAX_ARGS+1];
    int num_of_args=_parseCommandLine(GetCmdLine(),arg_list);
    string redirection_sign;
    string file_name;
    if(type==APPEND){
        redirection_sign=">>";
    }
    else{
        redirection_sign=">";
    }
    for (int i = 1; i < num_of_args;i++) {
        if (arg_list[i-1] == redirection_sign){
            file_name=arg_list[i];
            break;
        }
    }
    ofstream out_file;
    //TODO: make sure the file exist!
    if(type==APPEND){
        out_file=ofstream(file_name,ofstream::out|ofstream::app);
    }
    else{
        out_file=ofstream(file_name,ofstream::out|ofstream::trunc);
    }
    streambuf* cout_buffer = cout.rdbuf();
    cout.rdbuf(out_file.rdbuf());
    cmd->execute();
    cout.rdbuf(cout_buffer);
    FreeCmdArray(arg_list,num_of_args);
    free(cmd);
}

/*-------------------------BuiltInCommand ChangePrompt--------------------*/
ChangePromptCommand::ChangePromptCommand(std::string* prompt,string new_prompt):
BuiltInCommand(),prompt_name(prompt),new_prompt(new_prompt){};

void ChangePromptCommand::execute() {
    if (new_prompt == "") {
        *prompt_name = "smash>";
    }
    else {
        *prompt_name = new_prompt;
    }

}


/*-------------------------BuiltInCommand ShowPid-------------------------*/
void ShowPidCommand::execute()  {
	std::cout << "smash pid is " << GetPID() << "\n";
};


/*-------------------------BuiltInCommand GetCurrDir-----------------------*/
void GetCurrDirCommand::execute() {
	char * buf = nullptr;
	size_t size = 0;
	getcwd(buf, size); //TODO: what about get_dir_by_name()
	perror("smash error: getcwd failed\n");
	std::cout << buf << "\n";
	free(buf);
}


/*-------------------------BuiltInCommand ChangeDirCommand-----------------*/
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) :BuiltInCommand(cmd_line) {
	this->plastPwd = plastPwd;
}

	//TODO: Check if "smash error" is actually "Prompt-Name error"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void ChangeDirCommand::execute() {
	int num_of_args;
	char* arg_list[COMMAND_MAX_ARGS + 1];
	num_of_args = _parseCommandLine(GetCmdLine(), arg_list);

	if (arg_list[2] != NULL) {
		cout << "smash error: cd: too many arguments\n";
	}

	if (string(arg_list[1]) == "-") {
		if (string(this->plastPwd[0]) == "") {
			cout << "smash error: cd: OLDPWD not set\n";
		}
		else {
			int i = 0;
			while (i <= COMMAND_MAX_ARGS && !(string(plastPwd[i]).empty())) {
				i++;
			}
			int state = chdir(this->plastPwd[i]);
			state == 0 ? plastPwd[i] = "" : cout << "smash error: > " << this->GetCmdLine() << endl;
		}
	}

	else {
		int state = chdir(arg_list[1]);
		if (state == 0) {
			int i = 0;
			while (i <= COMMAND_MAX_ARGS && plastPwd[i] != "") {
				i++;
			}
			plastPwd[i] = arg_list[1];
		}
		else {
			cout << "smash error: > " << this->GetCmdLine() << endl;
		}

	}


	
	for (int i = 0; i < COMMAND_MAX_ARGS; i++) {
		if (arg_list[i] == NULL) break;
		free(arg_list[i]);
	}
}


/*-------------------------BuiltInCommand JobsCommand-----------------------*/
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
	job_list = jobs;

}

void JobsCommand::execute() {
	job_list->printJobsList();
}


/*-------------------------BuiltInCommand KillCommand------------------------*/
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
	job_list = jobs;
}

void KillCommand::execute() {
	int num_of_args;
	int res;
	char* arg_list[COMMAND_MAX_ARGS + 1];
	num_of_args = _parseCommandLine(GetCmdLine(), arg_list);
	if (num_of_args != 3 || arg_list[1][0] != '-') {
		cout << "kill: invalid arguments\n";
		for (int i = 0; i < COMMAND_MAX_ARGS; i++) {
			if (arg_list[i] == NULL) break;
			free(arg_list[i]);
		}
		return;
	}
	JobsList::JobEntry* temp = this->job_list->getJobById();
	if (temp == nullptr) {
		cout << " kill: job-id" << arg_list[2] << "does not exist\n";
	}
	else{
		res=job_list->ExecuteSignal(arg_list[1]);
		if(res==OK){
            cout << "signal number " << arg_list[1] << "was sent to pid " << arg_list[2] << endl;

        }
	}



	for (int i = 0; i < COMMAND_MAX_ARGS; i++) {
		if (arg_list[i] == NULL) break;
		free(arg_list[i]);
	}
}


/*-------------------------BuiltInCommand ForegroundCommand------------------*/
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
	job_list = jobs;
}

void ForegroundCommand::execute() {
	int num_of_args;
	int jobID;
	pid_t jobPID;
	int res=0;
	char* args[COMMAND_MAX_ARGS + 1];
	num_of_args = _parseCommandLine(GetCmdLine(), args);
	switch (num_of_args) {
	case 1: {
		unsigned long list_size = job_list->ListSize();
		if (list_size == 0) {
			std::cout << "smash error: fg: jobs list is empty\n";
			break;
		}
		else {
			//TODO: add function to remove last job so no error occurs!
			JobsList::JobEntry* last_job = job_list->getLastJob(&jobID);
			job_list->removeJobById(jobID);
			jobPID = last_job->GetCommand()->GetPID();
			std::cout << last_job->GetCommand()->GetCmdLine() << " : " << jobPID << "\n";
			res=kill(jobPID, SIGCONT);
			if(res==-1) {
                perror("smash error: kill failed\n");
                break;
            }
			res=waitpid(jobPID, nullptr, 0);
            if(res==-1) {
                perror("smash error: waitpid failed\n");
            }
			break;
		}
	}
	case 2: {
		try {
			jobID = stoi(args[1]);
		}
		catch (out_of_range &e) {
			std::cout << "smash error: fg: invalid arguments\n";
			return;
		}
		JobsList::JobEntry* job = job_list->getJobById(jobID);
		if (!job) {
			std::cout << "smash error: fg: job-id " << jobID << " does not exist\n";
			return;
		}
		job_list->removeJobById(jobID);
		jobPID = job->GetCommand()->GetPID();
		std::cout << job->GetCommand()->GetCmdLine() << " : " << jobPID << "\n";
        res=kill(jobPID, SIGCONT);
        if(res==-1) {
            perror("smash error: kill failed\n");
            break;
        }
        res=waitpid(jobPID, nullptr, 0);
        if(res==-1) {
            perror("smash error: waitpid failed\n");
        }
		break;
	}
	default:
		std::cout << "smash error: fg: invalid arguments\n";
		break;

	}
}


/*-------------------------BuiltInCommand BackgroundCommand------------------*/
void BackgroundCommand::execute() {

}


/*-------------------------BuiltInCommand QuitCommand------------------------*/

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
	jobs_list = jobs;
	char * args[COMMAND_MAX_ARGS + 1];
	int num_of_args = _parseCommandLine(cmd_line, args);
	if (num_of_args == 1) {
		isKillSpecified = false;
	}
	else {
		string second_word = string(args[1]);
		if (second_word == "kill") {
			isKillSpecified = true;
		}
		else {
			isKillSpecified = false;
		}
	}
}

void QuitCommand::execute() {
	if (isKillSpecified) {
		jobs_list->removeFinishedJobs();
		jobs_list->PrintForQuit();
		jobs_list->killAllJobs();
	}
	else {
		jobs_list->ClearJobsFromList();
	}
	exit(0); //TODO:maybe we shouldn't put exit here
}







/*========================================================================*/
/*=============================Shell Functions============================*/
/*========================================================================*/

/*-------------------------Shell Class Functions--------------------------*/
SmallShell::SmallShell():prompt_name("smash>") {
    job_list=new JobsList();
	//old_pwd = "";

}
SmallShell::~SmallShell() {
    delete job_list;
}


/*--------------------------------Jobs Functions--------------------------*/
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

//this function need to make sure it's input are digits
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
    int res=0;
    vector<JobEntry*>::iterator iter;
    for (iter=lst.begin(); iter!=lst.end(); iter++){
        if((*iter)->isStopped()){
            continue;
        }
        pid=(*iter)->GetCommand()->GetPID();
        res=waitpid(pid,&status,WNOHANG);
        if(res==-1){
            perror("smash error: waitpid failed\n");
            continue;
        }
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
        if(now==-1) {
            perror("smash error: time failed\n");
        }
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
    int jobID;//TODO: use to check return value of kill?
    //TODO: remove finished jobs?
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
        jobID=(*iter)->GetJobID();
        ExecuteSignal(jobID,SIGKILL);
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
JobsList::JobEntry* JobsList::getLastJob(int* lastJobID){//TODO: is that what they want with the ptr?
    //TODO: check for finished jobs?
    if(lst.empty()){
        return nullptr;
    }
    JobEntry* last_job=lst.back();
    *lastJobID=last_job->GetJobID();
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
                if(lst.empty()){
                    max_job_id=1;
                }
                else {
                    max_job_id = lst.back()->GetJobID();
                }
            }
            break;
        }
    }
}
unsigned long JobsList::ListSize() {
    return lst.size();
}

int JobsList::ExecuteSignal(int jobID,int signal){
    //TODO: before using this func jobID should be checked
    JobEntry* job=getJobById(jobID);
    pid_t pid=job->GetCommand()->GetPID();
    int res=kill(pid,signal);
    if(res==-1){
        perror("smash error: kill failed\n");
        return ERROR;
    }
    switch(signal){
        case SIGCONT:
            removeJobById(jobID);
            break;

        case SIGKILL:
            removeJobById(jobID);
            break;

        case SIGSTOP:
            job->ChangeStoppedStatus();
            break;

        default:
            break;
    }
    return OK;

}
/*------------------------Commands In Shell Functions-----------------------*/
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    Command* cmd= nullptr;
    bool builtin=true;
    int num_of_args,res;
    char* arg_list[COMMAND_MAX_ARGS+1];
    num_of_args=_parseCommandLine(cmd_line,arg_list);
    res=FindIfIO(arg_list,num_of_args);
    string first_word=string(arg_list[0]);

    if(first_word=="chprompt"){
        string new_prompt;
        if(num_of_args==1){
            new_prompt="";
        }
        else{
            new_prompt=string(arg_list[1]);
        }
        cmd=new ChangePromptCommand(&prompt_name,new_prompt);
    }
    else if(first_word=="showpid"){
        cmd=new ShowPidCommand();
    }
    else if(first_word=="pwd"){
        cmd=new GetCurrDirCommand(cmd_line);
    }
    else if(first_word=="cd"){

    }
    else if(first_word=="jobs"){
        cmd=new JobsCommand(cmd_line,job_list);
    }
    else if(first_word=="kill"){}
    else if(first_word=="fg"){
        cmd=new ForegroundCommand(cmd_line,job_list);
    }
    else if(first_word=="bg"){}
    else if(first_word=="quit"){
        cmd=new QuitCommand(cmd_line,job_list);
    }
    else{
        builtin=false;
        //TODO: create External command
    }

    if(builtin && res!=REGULAR){
        FreeCmdArray(arg_list,num_of_args);
        if(res==OVERRIDE || res==APPEND) {
            RedirectionCommand* reder_cmd = new RedirectionCommand(cmd_line,res,cmd);
            return reder_cmd;
        }
        else{
            //TODO: pipeline command here
        }

    }

    FreeCmdArray(arg_list,num_of_args);
    return cmd;


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


// MAKE SURE THIS IS OK:
	for (i = 0; i < COMMAND_MAX_ARGS;i++) {
		if (arg_list[i] == NULL) break;
		free (arg_list[i]);
	}


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

//TO DO- COMPLETE
SmallShell::SmallShell() {
	this.old_pwd = new char*[HISTORY_MAX_RECORDS + 1];
	for (int i = 0; i <= HISTORY_MAX_RECORDS; i++) {
		this.old_pwd[i] = "";
	}
}
