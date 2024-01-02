// --------------------------------------------------------------------------
#include <string>
#include <vector>
#include <getopt.h>
#include "Debug.h"
#include "UniSetTypes.h"
#include "PassiveTimer.h"
#include "Exceptions.h"
#include "LogReader.h"
#include "LogAgregator.h"
#include "LogServer.pb.h"
// --------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] =
{
    { "help", no_argument, 0, 'h' },
    { "verbose", no_argument, 0, 'v' },
    { "filter", required_argument, 0, 'f' },
    { "iaddr", required_argument, 0, 'i' },
    { "port", required_argument, 0, 'p' },
    { "add", required_argument, 0, 'a' },
    { "del", required_argument, 0, 'd' },
    { "set", required_argument, 0, 's' },
    { "off", required_argument, 0, 'o' },
    { "on", required_argument, 0, 'e' },
    { "save-loglevels", required_argument, 0, 'u' },
    { "restore-loglevels", required_argument, 0, 'y' },
    { "view-default-loglevels", required_argument, 0, 'b' },
    { "list", optional_argument, 0, 'l' },
    { "rotate", optional_argument, 0, 'r' },
    { "logfilter", required_argument, 0, 'n' },
    { "command-only", no_argument, 0, 'c' },
    { "timeout", required_argument, 0, 't' },
    { "reconnect-delay", required_argument, 0, 'x' },
    { "logfile", required_argument, 0, 'w' },
    { "logfile-truncate", required_argument, 0, 'z' },
    { "grep", required_argument, 0, 'g' },
    { "timezone", required_argument, 0, 'm' },
    { "set-verbosity", required_argument, 0, 'q' },
    { NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
    printf("Configs:\n");
    printf("-h, --help                  - this message\n");
    printf("-v, --verbose               - Print all umessage to stdout\n");
    printf("[-i|--iaddr] addr           - LogServer ip or hostname.\n");
    printf("[-p|--port] port            - LogServer port.\n");
    printf("[-c|--command-only]         - Send command and break. (No read logs).\n");
    printf("[-t|--timeout] msec         - Timeout for wait data. Default: WaitUpTime - endless waiting\n");
    printf("[-x|--reconnect-delay] msec - Pause for repeat connect to LogServer. Default: 5000 msec.\n");
    printf("[-w|--logfile] logfile      - Save log to 'logfile'.\n");
    printf("[-z|--logfile-truncate]     - Truncate log file before write. Use with -w|--logfile \n");

    printf("\n");
    printf("Commands:\n");

    printf("[-l | --list] [objName]                   - Show logs hierarchy from logname. Default: ALL\n");
    printf("[-b | --view-default-loglevels] [objName] - Show current log levels for logname.\n");

    printf("[-a | --add] info,warn,crit,... [objName] - Add log levels.\n");
    printf("[-d | --del] info,warn,crit,... [objName] - Delete log levels.\n");
    printf("[-s | --set] info,warn,crit,... [objName] - Set log levels.\n");
    printf("[-q | --set-verbosity] level [objName]    - Set verbose level.\n");

    printf("[-f | --filter] logname                   - ('filter mode'). View log only from 'logname'(regexp)\n");
    printf("[-g | --grep pattern                      - Print lines matching a pattern (c++ regexp)\n");

    printf("\n");
    printf("Note: 'objName' - regexp for name of log. Default: ALL logs.\n");
    printf("\n");
    printf("Special commands:\n");
    printf("[-o | --off] [objName]                    - Off the write log file (if enabled).\n");
    printf("[-e | --on] [objName]                     - On(enable) the write log file (if before disabled).\n");
    printf("[-r | --rotate] [objName]                 - rotate log file.\n");
    printf("[-u | --save-loglevels] [objName]         - save log levels (disable restore after disconnected).\n");
    printf("[-y | --restore-loglevels] [objName]      - restore default log levels.\n");
    printf("[-m | --timezone] [local|utc]             - set time zone for log, local or UTC.\n");

    printf("\n");
    printf("Examples:\n");
    printf("=========\n");
    printf("log hierarchy:\n");
    printf("SESControl1/TV1\n");
    printf("SESControl1/TV1/HeatExchanger\n");
    printf("SESControl1/TV1/MyCustomLog\n");
    printf("\n");
    printf("* Show all logs for SESControl1 (only for SESControl1 and it's childrens)\n");
    printf("uniset3-log -i host -p 30202 --del any --set any SESControl1\n");
    printf("* Show all logs for TV1\n");
    printf("uniset3-log -i host -p 30201 --del any --set any TV1.*\n");
    printf("* Show all logs for MyCustomLog\n");
    printf("uniset3-log -i host -p 30201 --del any --set any MyCustomLog\n");
    printf("* Show info logs with special text for TV1\n");
    printf("uniset3-log -i host -p 30202 --del any --set info TV1 --grep [Tt]ransient\n");
}
// --------------------------------------------------------------------------
static char* checkArg( int i, int argc, char* argv[] );
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
int main( int argc, char** argv )
{
    //  std::ios::sync_with_stdio(false); // нельзя отключать.. тогда "обмен с сервером" рассинхронизируется

    int optindex = 0;
    int opt = 0;
    int verb = 0;
    string addr = "localhost";
    int port = 3333;
    DebugStream dlog;
    logserver::LogCommandList cmdlist;
    string logfilter = "";
    bool cmdGetList = false;
    bool cmdDefaultLogLevel = false;
    bool cmdOnly = false;
    std::string textfilter = "";
    timeout_t tout = UniSetTimer::WaitUpTime;
    timeout_t rdelay = 8000;
    string logfile = "";
    bool logtruncate = false;

    try
    {
        while(1)
        {
            opt = getopt_long(argc, argv, "chvlf:a:p:i:d:s:n:eorbx:w:zt:g:uby:m:q:", longopts, &optindex);

            if( opt == -1 )
                break;

            switch (opt)
            {
                case 'h':
                    print_help();
                    return 0;

                case 'a':
                {
                    logserver::LogCommand cmd;
                    cmd.set_cmd(logserver::LOG_CMD_ADD);
                    cmd.set_data((int)Debug::value(string(optarg)));
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        cmd.set_logname(arg2);

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 'd':
                {
                    logserver::LogCommand cmd;
                    cmd.set_cmd(logserver::LOG_CMD_DEL);
                    cmd.set_data((int)Debug::value(string(optarg)));
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        cmd.set_logname(arg2);

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 's':
                {
                    logserver::LogCommand cmd;
                    cmd.set_cmd(uniset3::logserver::LOG_CMD_SET);
                    cmd.set_data((int)Debug::value(string(optarg)));
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        cmd.set_logname(arg2);

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 'q':
                {
                    logserver::LogCommand cmd;
                    cmd.set_cmd(uniset3::logserver::LOG_CMD_VERBOSITY);
                    cmd.set_data((int)Debug::value(string(optarg)));
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        cmd.set_logname(arg2);

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 'l':
                {
                    cmdGetList = true;
                    std::string filter("");
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        logfilter = string(arg2);
                }
                break;

                case 'o':
                {
                    logserver::LogCommand cmd;
                    cmd.set_cmd(uniset3::logserver::LOG_CMD_LOGFILE_DISABLE);
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        cmd.set_logname(arg2);

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 'u':  // --save-loglevels
                {
                    logserver::LogCommand cmd;
                    cmd.set_cmd(uniset3::logserver::LOG_CMD_SAVE_LOGLEVEL);
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        cmd.set_logname(arg2);

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 'y':  // --restore-loglevels
                {
                    logserver::LogCommand cmd;
                    cmd.set_cmd(uniset3::logserver::LOG_CMD_RESTORE_LOGLEVEL);
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        cmd.set_logname(arg2);

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 'b':  // --view-default-loglevels
                {
                    cmdDefaultLogLevel = true;
                    std::string filter("");
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        logfilter = string(arg2);
                }
                break;

                case 'e':
                {
                    logserver::LogCommand cmd;
                    cmd.set_cmd(uniset3::logserver::LOG_CMD_LOGFILE_ENABLE);
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        cmd.set_logname(arg2);

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 'f':
                {
                    logserver::LogCommand cmd;
                    cmd.set_cmd(uniset3::logserver::LOG_CMD_FILTER);
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        cmd.set_logname(arg2);

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 'r':
                {
                    logserver::LogCommand cmd;
                    cmd.set_cmd(uniset3::logserver::LOG_CMD_ROTATE);
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        cmd.set_logname(arg2);

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 'm':
                {
                    logserver::LogCommand cmd;
                    std::string tz(optarg);

                    if( tz == "local" )
                        cmd.set_cmd(uniset3::logserver::LOG_CMD_SHOW_LOCALTIME);
                    else if( tz == "utc" )
                        cmd.set_cmd(uniset3::logserver::LOG_CMD_SHOW_UTCTIME);
                    else
                    {
                        cerr << "Error: Unknown timezone '" << tz << "'. Must be 'local' or 'utc'" << endl;
                        return 1;
                    }

                    cmdlist.add_cmd()->PackFrom(cmd);
                }
                break;

                case 'c':
                    cmdOnly = true;
                    break;

                case 'i':
                    addr = string(optarg);
                    break;

                case 'p':
                    port = uni_atoi(optarg);
                    break;

                case 'x':
                    rdelay = uni_atoi(optarg);
                    break;

                case 't':
                    tout = uni_atoi(optarg);
                    break;

                case 'w':
                    logfile = string(optarg);
                    break;

                case 'g':
                    textfilter = string(optarg);
                    break;

                case 'z':
                    logtruncate = true;
                    break;

                case 'v':
                    verb = 1;
                    break;

                case '?':
                default:
                    printf("Unknown argumnet\n");
                    return 0;
            }
        }

        if( verb )
        {
            cout << "(init): read from " << addr << ":" << port << endl;
            dlog.addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );
        }

        LogReader lr;
        lr.setTimeout(tout);
        lr.setReconnectDelay(rdelay);
        lr.setTextFilter(textfilter);

        if( !logfile.empty() )
            lr.log()->logFile(logfile, logtruncate);

        if( cmdGetList )
            return lr.list(addr, port, logfilter, verb) ? 0 : 1;

        if( cmdDefaultLogLevel )
            return lr.loglevel(addr, port, logfilter, verb) ? 0 : 1;

        if( !cmdlist.cmd().empty() && cmdOnly )
            return lr.command( addr, port, cmdlist, verb ) ? 0 : 1;

        lr.readLoop( addr, port, cmdlist, verb );
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(log): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(log): catch(...)" << endl;
    }

    return 1;
}
// --------------------------------------------------------------------------
char* checkArg( int i, int argc, char* argv[] )
{
    if( i < argc && (argv[i])[0] != '-' )
        return argv[i];

    return 0;
}
// --------------------------------------------------------------------------
