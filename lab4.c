#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <wait.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define ProcessesCount (9)
#define StartingProcessId (1)
#define MaxChildsCount (2)
#define MaxUsrCount (101)

const unsigned char ChildsCount[ProcessesCount] =
{
    1, 1, 2, 1, 1, 0, 1, 1, 0
};

const unsigned char ChildsIds[ProcessesCount][MaxChildsCount] =
{
    {1}, {2}, {3, 4}, {6}, {5}, {0}, {7}, {8}, {0}
};

const unsigned char GroupTypes[ProcessesCount] =
{
    0, 1, 0, 1, 1, 1, 0, 1, 1
};

const char Receivers[ProcessesCount] =
{
    0, -2,  0,  0,  0, -6,  0,  0,  1
};

const unsigned int SendingSignals[ProcessesCount] =
{
    0, SIGUSR1, 0, 0, 0, SIGUSR1, 0, 0, SIGUSR1
};

const char ReceivedSignalsCount[2][ProcessesCount] =
{
    { 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

void SignalHandler(int signalNumber);
void SetSignalHandler(void (*handler)(int), int sig_no, int flags);;
void PrintErrorAndExit(const char *s_name, const char *message, const int proc_num);
void CreateProcessesTree(int currentNumber, int childsCount);
void KillAndWaitForChildren();
void WaitForChildren();

char *programName = NULL;
unsigned int processId = 0;
pid_t *pidsList = NULL;
char *tempFilePath = "/tmp/pids.log";

int main(int argc, char *argv[])
{
    programName = basename(argv[0]);
    pidsList = (pid_t*)mmap(pidsList, (2*ProcessesCount)*sizeof(pid_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int i = 0;
    for (i = 0; i < 2*ProcessesCount; ++i) {
        pidsList[i] = 0;
    }
    CreateProcessesTree(0, ChildsCount[0]);
    SetSignalHandler(&KillAndWaitForChildren, SIGTERM, 0);
    if (processId == 0) {
        WaitForChildren();
        munmap(pidsList, (2*ProcessesCount)*sizeof(pid_t));
        return 0;
    }
    on_exit(&WaitForChildren, NULL);
    pidsList[processId] = getpid();
    if (processId == StartingProcessId) {
        do {
            for (i = 1; (i <= ProcessesCount) && (pidsList[i] != 0); ++i) {
                if (pidsList[i] == -1) {
                    PrintErrorAndExit(programName, "Error: not all processes forked or initialized.", 0);
                    exit(1);
                }
            }
        } while (i < ProcessesCount);
        FILE *tempFile = fopen(tempFilePath, "wt");
        if (tempFile == NULL) {
            PrintErrorAndExit(programName, "Can't create temp file", 0);
        }
        for (i = 1; i < ProcessesCount; ++i) {
            fprintf(tempFile, "%d\n", pidsList[i]);
        }
        fclose(tempFile);
        pidsList[0] = 1;
        SetSignalHandler(&SignalHandler, 0, 0);
        do {
            for (i = 1+ProcessesCount; (i < 2*ProcessesCount)  && (pidsList[i] != 0); ++i) {
                if (pidsList[i] == -1) {
                    PrintErrorAndExit(programName, "Error: not all processes forked or initialized.", 0);
                    exit(1);
                }
            }
        } while (i < 2*ProcessesCount);
        for (i = ProcessesCount+1; i < 2*ProcessesCount; ++i) {
            pidsList[i] = 0;
        }
        SignalHandler(0); 
    } else {    
        do {} while (pidsList[0] == 0);
        SetSignalHandler(&SignalHandler, 0, 0);
    }
    while (1) {
        pause();
    }
    return 0;
}

void PrintErrorAndExit(const char *s_name, const char *message, const int proc_num) {
    fprintf(stderr, "%s: %s %d\n", s_name, message, proc_num);
    fflush(stderr);
    pidsList[proc_num] = -1;
    exit(1);
}

void WaitForChildren() {
    int i = ChildsCount[processId];
    while (i > 0) {
        wait(NULL);
        --i;
    }
}

long long GetCurrentTime() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_usec;
}

volatile int usrReceived[2] = {0, 0};
volatile int usrAmount[2][2] =
{
    {0, 0},
    {0, 0}
};

void KillAndWaitForChildren() {
    int i = 0;
    for (i = 0; i < ChildsCount[processId]; ++i) {
        kill(pidsList[ChildsIds[processId][i]], SIGTERM);
    }
    WaitForChildren();
    if (processId != 0)
    	printf("%d %d завершил работу после %d SIGUSR1 и %d SIGUSR2\n", getpid(), getppid(), usrAmount[0][1], usrAmount[1][1]);
    fflush(stdout);
    exit(0);
}

void SignalHandler(int signalNumber) {
    if (signalNumber == SIGUSR1) {
        signalNumber = 0;
    } else if (signalNumber == SIGUSR2) {
        signalNumber = 1;
    } else {
        signalNumber = -1;
    }
    if (signalNumber != -1) {
        ++usrAmount[signalNumber][0];
        ++usrReceived[signalNumber];
        printf("%d %d %d получил %s%d %lld\n", processId, getpid(), getppid(),
               "USR", signalNumber+1, GetCurrentTime() );
        fflush(stdout);
    if (processId == 1) {
        if (usrAmount[0][0] + usrAmount[1][0] == MaxUsrCount) {
            KillAndWaitForChildren();
        }
    }
    if (! ( (usrReceived[0] == ReceivedSignalsCount[0][processId]) && (usrReceived[1] == ReceivedSignalsCount[1][processId]) ) ) {
            return;
        }
        usrReceived[0] = usrReceived[1] = 0;
    }
    char receiveProcess = Receivers[processId];
    if (receiveProcess != 0) {
        signalNumber = ( (SendingSignals[processId] == SIGUSR1) ? 1 : 2);
        ++usrAmount[signalNumber-1][1];
    }
    if (receiveProcess != 0)
    	printf("%d %d %d послал %s%d %lld\n", processId, getpid(), getppid(), "USR", signalNumber, GetCurrentTime() );
    fflush(stdout);
    if (receiveProcess > 0) {
        kill(pidsList[receiveProcess], SendingSignals[processId]);
    } else if (receiveProcess < 0) {
        kill(-getpgid(pidsList[-receiveProcess]), SendingSignals[processId]);
    } else {
        return;
    }
}

void SetSignalHandler(void (*handler)(int), int sig_no, int flags) {
    struct sigaction signalAction, oldSignalAction;
    sigset_t blockMask;
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGUSR1);
    sigaddset(&blockMask, SIGUSR2);
    signalAction.sa_mask = blockMask;
    signalAction.sa_flags = flags;
    signalAction.sa_handler = handler;
    if (sig_no != 0) {
        sigaction(sig_no, &signalAction, &oldSignalAction);
        return;
    }
    int i = 0;
    for (i = 0; i < ProcessesCount; ++i) {
        char receiverProcess = Receivers[i];
        if ( ( (receiverProcess > 0) && (receiverProcess == processId) )  ||
             ( (receiverProcess < 0) && (getpgid(pidsList[-receiverProcess]) == getpgid(0)) ) ) {
            if (SendingSignals[i] != 0) {
                if (sigaction(SendingSignals[i], &signalAction, &oldSignalAction) == -1) {
                    PrintErrorAndExit(programName, "Can't set sighandler!", processId);
                }
            }
        }
    }
    pidsList[processId + ProcessesCount] = 1;
}

void CreateProcessesTree(int currentNumber, int childsCount) {
    pid_t pid = 0;
    int i = 0;
    for (i = 0; i < childsCount; ++i) {
        int childId = ChildsIds[currentNumber][i];
        if ( (pid = fork() ) == -1) {
            PrintErrorAndExit(programName, strerror(errno), childId);
        } else if (pid == 0) {
            processId = childId;
            if (ChildsCount[processId] != 0) {
                CreateProcessesTree(processId, ChildsCount[processId]);
            }
            break;
        } else {
            static int previousChildGroup = 0;
            int groupType = GroupTypes[childId];
            if (groupType == 0) {
                if (setpgid(pid, pid) == -1) {
                    PrintErrorAndExit(programName, strerror(errno), childId);
                } else {
                    previousChildGroup = pid;
                }
            } else if (groupType == 1) {
                if (setpgid(pid, getpgid(0)) == -1) {
                    PrintErrorAndExit(programName, strerror(errno), childId);
                }
            } else if (groupType == 2) {
                if (setpgid(pid, previousChildGroup) == -1) {
                    PrintErrorAndExit(programName, strerror(errno), childId);
                }
            }
        }
    }
}
