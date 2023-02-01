#include <dirent.h>

#include "theader.h"
#include "tglobal.h"

#include <sys/time.h>
#include <signal.h>
#include <sys/file.h>

#include "tmain.h"

#include "tini.h"
#include "tlog.h"

#include "tinterface.h"

#include "tchannel.h"
#include "ttime.h"

#include <execinfo.h>

// **************************************************
static void full_write(int fd, const char *buf, size_t len)
{
        while (len > 0) {
                ssize_t ret = write(fd, buf, len);

                if ((ret == -1) && (errno != EINTR))
                        break;

                buf += ret;
                len -= (size_t) ret;
        }
}

void print_backtrace()
{
    int fd = open("./err.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (!fd) {
        printf("err open file\n);");
    }

        static const char start[] = "BACKTRACE ------------\n";
        static const char end[] = "----------------------\n";

        void *bt[1024];
        int bt_size;
        char **bt_syms;
        int i;

        bt_size = backtrace(bt, 1024);
        bt_syms = backtrace_symbols(bt, bt_size);
        full_write(fd, start, strlen(start));
        for (i = 1; i < bt_size; i++) {
                size_t len = strlen(bt_syms[i]);
                full_write(fd, bt_syms[i], len);
                full_write(fd, "\n", 1);
        }


        full_write(fd, end, strlen(end));


    free(bt_syms);

    close(fd);

}


[[ noreturn ]] void posix_death_signal(int signum)
{
    print_backtrace();
    signal(signum, SIG_DFL);
    exit(3);
}
// **************************************************




// return -1 or founded pid , skipped current pid
int getProcIdByName(const char * task_name)
{
    int pid_current = getpid();

    const unsigned BUF_SIZE = 256;

    DIR *dir = opendir("/proc");
    if (!dir) return -1;

    struct dirent *ptr;

    char filepath[512];
    FILE *fp;

    char buf[BUF_SIZE];

    char cur_task_name[256];

    while (1)
    {
         ptr = readdir(dir);
         if (!ptr) break;

         if ((strcmp(ptr->d_name,".") == 0 ) || (strcmp(ptr->d_name,"..") ==0))
             continue;

         if (DT_DIR != ptr->d_type)
             continue;

         sprintf(filepath,"/proc/%s/status",ptr->d_name);
         fp=fopen(filepath,"r");

         if (fp) {

             if (fgets(buf,BUF_SIZE-1,fp)==nullptr) {
                 fclose(fp);
                 continue;
             }

            sscanf(buf,"%*s %s",cur_task_name);

             if (!strcmp(task_name,cur_task_name)) {
                 int pid = atoi(ptr->d_name);

                 if (pid>0) {

                     if (pid != pid_current) {
                           //printf("PID: %d    task=%s    cur_task=%s    cur_pid=%d\n",pid,task_name,cur_task_name,pid_current);
                           fclose(fp);
                           return pid;
                           }
                      }
             }
             fclose(fp);
           }

    } // while()



    closedir(dir);

    return -1;
}

bool IsRunning(const string& name)
{
    int pid = getProcIdByName(name.c_str());

    if (pid==-1)
        return false;

    return true;
}


//
[[ noreturn ]] void ExitWithError(const char*msg)
{
    Log.ScrFile("%s\n",msg);
    main_stop(0);
    exit(1); //exit with error
}


void SignalHandler(int signo)
{
    if ( (signo == SIGINT) ||
         (signo == SIGUSR1) ||
         (signo == SIGTERM) ) {

        if (main_stop(signo))
            exit(0);  // exit normal
        }

    if (signo == SIGHUP) {
        // reload config
        Log.ScrFile("signal Reload\n");
        }
}







bool WorkSetSignals()
{
    if (signal(SIGSEGV, posix_death_signal) == SIG_ERR) return false;

    if (signal(SIGUSR1, SignalHandler) == SIG_ERR) return false;
    if (signal(SIGINT, SignalHandler) == SIG_ERR) return false;
    if (signal(SIGTERM, SignalHandler) == SIG_ERR) return false;
    if (signal(SIGHUP, SignalHandler) == SIG_ERR) return false;   // reload config

    return true;
}

void OutHelp()
{
    printf("valid options:\n");
    printf("console\t- run console mode\n");
    printf("start\t- start service mode\n");
    printf("stop\t- stop service mode\n");
}


int main(int argc,char *argv[])
{
    setlocale(LC_CTYPE, "");
    umask(0); // разрешаем выставлять все биты прав на создаваемые файлы

    if ( argc != 2 ) {
        OutHelp();
        return 1;
        }

    string param = argv[1];
    // valid: start stop console
    if ( (param != "start") &&  (param != "stop") && (param != "console")  )  {
        OutHelp();
        return 1;
        }

    g.Initialize(argv[0]);           // argv[0] = this program name
    Time.ReadSystemDateTime();

    if (Ini.Read(g.getCurrentIni())==false) {
       printf("config file %s not found\n",g.getCurrentIni().c_str());
       Ini.GenerateConfig();
       string cfgname = g.getCurrentName() + ".cfg";
       Ini.Write(cfgname);
       return 1;
       }
    printf("read config file %s\n",g.getCurrentIni().c_str());
    // ++++++++++++++++++

    string log_folder = Ini.GetStr("Common","log_folder","");
    if (log_folder=="") {
        printf("not found log_folder=\n");
        return 1;
        }
    g.setLogFolder(log_folder);

    Log.Initialize();

    Log.Scr("use path=%s\n",g.getCurrentPath().c_str());
    Log.Scr("use bin=%s\n",g.getCurrentName().c_str());
    Log.Scr("use ini=%s\n",g.getCurrentIni().c_str());

    bool af_mode = true;
    g.set_AF_Mode(af_mode);

    g.th_running=1;      //enable all thread

    //***************************** segfault handler

    //struct sigaction sa {};
    //sigemptyset(&sa.sa_mask);
    //sa.sigaction = segf
    //if (signal(SIGSEGV, posix_death_signal) == SIG_ERR) return false;

    //***************************** check run permission

    if (g.IsRootPermission() == false) {
         Log.Scr("run witch root permission\n");
         return 1;
         }


    //***************************** run console mode

    if (param=="console") {

        if (IsRunning(g.getCurrentName())) {
            Log.Scr("found running -> exit..\n");
            return 1;
            }

        Log.ScrFile("console mode start ..\n");

        if (!WorkSetSignals()) {
            Log.ScrFile("catch error -> exit..\n");
            return 1;
            }

        main_loop(true);

        Log.ScrFile("exit console\n");
        return 0;
        }


    //***************************** stop process

    if (param=="stop")  {
        if (!IsRunning(g.getCurrentName())) {
            Log.Scr("%s not running\n",g.getCurrentName().c_str());
            return 1;
            }

        int pid_running = getProcIdByName(g.getCurrentName().c_str());
        kill(pid_running, SIGUSR1);

        int delay_second=0;
        while(1){
            std::this_thread::sleep_for(std::chrono::seconds(1));

            Log.Scr("wait %d\n",delay_second++);

            int pid = getProcIdByName(g.getCurrentName().c_str());
            if (pid != pid_running) break;
            }
        Log.Scr("stopped\n");
        return 0;
        }

    //***************************** run service mode

    if (param =="start") {

        if (IsRunning(g.getCurrentName())) {
            Log.Scr("%s already running -> exit\n",g.getCurrentName().c_str());
            return 0;
            }

        int pid = fork();

        if (pid>0)   // если это родитель
        {
            sleep(5); // wait, while initing child
            exit(0);  //exit if parent
        }

        if (pid == -1) {
            Log.Scr("start service error: %s\n", strerror(errno));
            exit(1);
            }

        // pid==0 - это потомок


        umask(0); // разрешаем выставлять все биты прав на создаваемые файлы,
        setsid(); // создаём новый сеанс, чтобы не зависеть от родителя

        //g.setServiceMode(true);

        if (!WorkSetSignals()) {
            Log.Scr("catch error -> exit..\n");
            return 1;
            }

        //initialize & loop analize
        main_loop(false);

        //Log.ScrFile("exit service\n");
        }

    return 0;
}
