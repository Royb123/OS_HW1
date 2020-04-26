#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <unistd.h>
#include <vector>
#include <string>
#include <fcntl.h>
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)
#define OK (0)
#define ERROR (-1)
#define OVERRIDE (0)
#define APPEND (1)
#define PIPE (2)
#define PIPEERR (3)
#define REGULAR (-1)

class Command {
// TODO: Add your data members
    const char* cmd_line;
    pid_t pid;
    bool background;
    std::string* cmd_str;
public:
    Command();
    explicit Command(const char* cmd_line);
    virtual ~Command();
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    const char* GetCmdLine(){return cmd_line;};
    const pid_t GetPID(){return pid;}
    bool GetBackground(){return background;};
    void ChangePID(pid_t new_pid){pid=new_pid;};
    void ChangeBackground(){
    if(!background){
        background=true;
        }
    else{
        background=false;
        }
    }

};

class JobsList {
public:
    class JobEntry {
        const int jobid;
        bool stopped;
        Command* command;
        time_t start_time;
    public:
        JobEntry(int ID, bool stopped, Command* cmd) : jobid(ID), stopped(stopped) {
            command = cmd;
            start_time = time(nullptr); //TODO: handle errors
        }
        ~JobEntry() = default;
        int GetJobID() { return jobid; }
        time_t GetStartTime() { return start_time; }
        Command* GetCommand() { return command; }
        bool isStopped() { return stopped; }
        void ChangeStoppedStatus(){
            if(stopped){
                stopped=false;
            }
            else{
                stopped=true;
            }
        }

        // TODO: Add your data members
    };
private:
    int max_job_id;
    std::vector<JobEntry*> lst;

public:
    JobsList();
    ~JobsList();
    JobEntry* addJob(Command* cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    void ClearJobsFromList();
    void PrintForQuit();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobID);
    JobEntry *getLastStoppedJob(int *jobId);
    unsigned long ListSize();
    int ExecuteSignal(int jobID,int signal);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand();
    explicit BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand()=default;
    void execute() override=0;
};

class ExternalCommand : public Command {
    JobsList* jobs;
    char * cmd_without_bck;
public:
    ExternalCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ExternalCommand();
    void execute() override;

};

class PipeCommand : public Command {
    int type;
    Command* cmd1;
    Command* cmd2;
public:
    PipeCommand(const char* cmd_line,int type,Command* cmd1,Command* cmd2);
    virtual ~PipeCommand();
    void execute() override;
};

class RedirectionCommand : public Command {
    int type;
    Command* cmd;
public:
    explicit RedirectionCommand(const char* cmd_line,int type,Command* cmd);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class CopyCommand : public Command {
    JobsList* jobs;
public:
    CopyCommand(const char* cmd_line, JobsList* jobs);
    virtual ~CopyCommand() {}
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
    std::string* prompt_name;
    std::string new_prompt;
public:
    ChangePromptCommand(std::string* prompt,std::string new_prompt);
    virtual ~ChangePromptCommand()=default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand()=default;
    virtual ~ShowPidCommand() = default;
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const char* cmd_line) :BuiltInCommand(cmd_line) {};
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    char* old_pwd;
    char* curr_pwd;
public:
    ChangeDirCommand(const char* cmd_line, char* old_pwd, char* curr_pwd);
    virtual ~ChangeDirCommand() {};
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
    JobsList* job_list;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() = default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* job_list;
    // TODO: Add your data members
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* job_list;
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* job_list;
    // TODO: Add your data members
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
    JobsList* jobs_list;
    bool isKillSpecified;
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand()= default;
    void execute() override;
};

class TimeoutCommand;

class TimeoutList{
public:
    class TimeoutEntry {
        int jobid;
        TimeoutCommand* command;
        time_t start_time;
        time_t finish_time;
        int duration;
        bool background;
    public:
        TimeoutEntry(TimeoutCommand* cmd,int duration,bool back):
        duration(duration){
            jobid=-1;
            background=back;
            command = cmd;
            start_time = time(nullptr); //TODO: handle errors
            finish_time= start_time+duration;
        }
        ~TimeoutEntry() = default;
        int GetJobID(){return jobid;}
        time_t GetStartTime(){ return start_time; }
        bool GetBackGround(){return background;}
        time_t GetDuration(){ return duration; }
        time_t GetFinishTime(){ return finish_time;}
        TimeoutCommand* GetCommand(){ return command; }
        void ChangeJobID(int new_id){jobid=new_id;}
        void ChangeBackground(){
            if(background){
                background=false;
            }
            else{
                background=true;
            }
        }
    };
private:
    std::vector<TimeoutEntry*> timeout_list;

public:
    TimeoutList();
    ~TimeoutList();
    TimeoutEntry* addTimedJob(TimeoutCommand* cmd, int duration);
    void removeTimedJob();
    TimeoutEntry* GetFirstEntry();
};

class TimeoutCommand : public Command {
    JobsList* jobs;
    Command* cmd;
    TimeoutList* times;
public:
    TimeoutCommand(const char* cmd_line, Command* cmd, JobsList* jobslst,TimeoutList* t_list);
    virtual ~TimeoutCommand();
    void execute() override;
    Command* GetInternalCommand();
};

class SmallShell {
private:
    JobsList* job_list;
    TimeoutList* timeout_commands;
    std::string prompt_name;
    char* old_pwd;
    char* curr_pwd;
    pid_t pid;
    Command* current_cmd;
    SmallShell();
public:

    ~SmallShell();
    Command* CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    void executeCommand(const char* cmd_line);
    std::string GetPromptName(){return prompt_name;};
    pid_t GetPID(){return pid;};
    JobsList* GetJobList(){return job_list;}; //TODO: maybe create interface
    Command* GetCurrCmd(){return current_cmd;};
    TimeoutList* GetTimeoutList(){return timeout_commands;};
};

#endif //SMASH_COMMAND_H_
