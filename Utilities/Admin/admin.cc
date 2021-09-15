// --------------------------------------------------------------------------
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <getopt.h>
// --------------------------------------------------------------------------
#include "Exceptions.h"
#include "UniSetObject.h"
#include "UniSetTypes.h"
#include "UniSetManager.h"
#include "UInterface.h"
#include "Configuration.h"
#include "MessageTypes.pb.h"
#include "Debug.h"
#include "UHelpers.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
enum Command
{
    StartUp,
    FoldUp,
    Finish,
    Exist,
    Configure,
    LogRotate
};

static struct option longopts[] =
{
    { "help", no_argument, 0, 'h' },
    { "confile", required_argument, 0, 'c' },
    { "exist", no_argument, 0, 'e' },
    { "omap", no_argument, 0, 'o' },
    { "start", no_argument, 0, 's' },
    { "finish", no_argument, 0, 'f' },
    { "foldUp", no_argument, 0, 'u' },
    { "configure", optional_argument, 0, 'r' },
    { "logrotate", optional_argument, 0, 'l' },
    { "info", required_argument, 0, 'i' },
    { "setValue", required_argument, 0, 'x' },
    { "getValue", required_argument, 0, 'g' },
    { "getRawValue", required_argument, 0, 'w' },
    { "getCalibrate", required_argument, 0, 'y' },
    { "getTimeChange", required_argument, 0, 't' },
    { "oinfo", required_argument, 0, 'p' },
    { "sinfo", required_argument, 0, 'j' },
    { "verbose", no_argument, 0, 'v' },
    { "quiet", no_argument, 0, 'q' },
    { "csv", required_argument, 0, 'k' },
    { "sendText", required_argument, 0, 'm' },
    { "freeze", required_argument, 0, 'z' },
    { "unfreeze", required_argument, 0, 'n' },
    { NULL, 0, 0, 0 }
};

string conffile("configure.xml");

// --------------------------------------------------------------------------
static bool commandToAll( UInterface& ui, const string& section, std::shared_ptr<grpc::Channel>& rep, Command cmd );
static void errDoNotResolve( const std::string& oname );
static char* checkArg( int ind, int argc, char* argv[] );
// --------------------------------------------------------------------------
int omap();
int configure( const string& args, UInterface& ui );
int logRotate( const string& args, UInterface& ui );
int setValue( const string& args, UInterface& ui );
int getValue( const string& args, UInterface& ui );
int getRawValue( const string& args, UInterface& ui );
int getTimeChange( const string& args, UInterface& ui );
int getState( const string& args, UInterface& ui );
int getCalibrate( const string& args, UInterface& ui );
int oinfo(const string& args, UInterface& ui, const string&  userparam );
int sinfo(const string& args, UInterface& ui);
void sendText( const string& args, UInterface& ui, const string& txt, int mtype );
int freezeValue( const string& args, bool set, UInterface& ui );
// --------------------------------------------------------------------------
static void print_help(int width, const string& cmd, const string& help, const string& tab = " ", const string& sep = " - " )
{
    uniset3::ios_fmt_restorer ifs(cout);
    cout.setf(ios::left, ios::adjustfield);
    cout << tab << setw(width) << cmd << sep << help;
}
// --------------------------------------------------------------------------
static void short_usage()
{
    cout << "Usage: uniset-admin [--confile configure.xml] --command [arg]    \n for detailed information arg --help" << endl;
}

// --------------------------------------------------------------------------
static void usage()
{
    cout << "\nUsage: \n\tuniset-admin [--confile configure.xml] --command [arg]\n";
    cout << endl;
    cout << "commands list:\n";
    cout << "-----------------------------------------\n";
    print_help(24, "-с|--confile file.xml ", "Используемый конфигурационный файл\n");
    cout << endl;
    print_help(24, "-e|--exist ", "Вызов функции exist() показывающей какие объекты зарегистрированы и доступны.\n");
    print_help(24, "-o|--omap ", "Вывод на экран списка объектов с идентификаторами.\n");
    print_help(24, "-s|--start ", "Посылка SystemMessage::StartUp всем объектам (процессам)\n");
    print_help(24, "-u|--foldUp ", "Посылка SystemMessage::FoldUp всем объектам (процессам)\n");
    print_help(24, "-f|--finish ", "Посылка SystemMessage::Finish всем объектам (процессам)\n");
    print_help(24, "-h|--help  ", "Вывести это сообщение.\n");
    cout << endl;
    print_help(36, "-r|--configure [FullObjName] ", "Посылка SystemMessage::ReConfiguration всем объектам (процессам) или заданному по имени (FullObjName).\n");
    print_help(36, "-l|--logrotate [FullObjName] ", "Посылка SystemMessage::LogRotate всем объектам (процессам) или заданному по имени (FullObjName).\n");
    print_help(36, "-p|--oinfo id1@node1,id2@node2,id3,... [userparam]", "Получить информацию об объектах (SimpleInfo). \n");
    print_help(36, "", "userparam - необязательный параметр передаваемый в getInfo() каждому объекту\n");
    print_help(36, "-j|--sinfo id1@node1,id2@node2,id3,...", "Получить информацию о датчиках.\n");
    cout << endl;
    print_help(48, "-x|--setValue id1@node1=val,id2@node2=val2,id3=val3,.. ", "Выставить значения датчиков\n");
    print_help(36, "-g|--getValue id1@node1,id2@node2,id3,id4 ", "Получить значения датчиков.\n");
    cout << endl;
    print_help(36, "-w|--getRawValue id1@node1,id2@node2,id3,.. ", "Получить 'сырое' значение.\n");
    print_help(36, "-y|--getCalibrate id1@node1,id2@node2,id3,.. ", "Получить параметры калибровки.\n");
    print_help(36, "-t|--getTimeChange id1@node1,id2@node2,id3,.. ", "Получить время последнего изменения.\n");
    print_help(36, "-v|--verbose", "Подробный вывод логов.\n");
    print_help(36, "-q|--quiet", "Выводит только результат.\n");
    print_help(36, "-k|--csv", "Вывести результат (getValue) в виде val1,val2,val3...\n");
    print_help(36, "-m|--sendText id1@node1,id2@node2,id3,.. mtype text", "Послать объектам текстовое сообщение text типа mtype\n");
    print_help(36, "-z|--freeze id1@node1=val1,id2@node2=val2,id3=val3,...", "Заморозить указанные датчики и выставить соответствующие значения.\n");
    print_help(36, "-n|--unfreeze id1@node1,id2@node2,id3,...", "Разаморозить указанные датчики.\n");
    cout << endl;
    cout << "Глобальные параметры, которые необходимо передавать через '--'" << endl;
    cout << "-----------------------------------------\n";
    cout << uniset3::Configuration::help() << endl;
    cout << "Example: uniset3-admin arg1 arg2 arg3 -- global_arg1 global_arg2 ..." << endl;
    cout << endl;
}

// --------------------------------------------------------------------------------------
/*!
    \todo Оптимизировать commandToAll, т.к. сейчас НА КАЖДОМ ШАГЕ цикла
        создаётся сообщение и происходит преобразование в TransportMessage.
        TransportMessage можно создать один раз до цикла.
*/
// --------------------------------------------------------------------------------------
static bool verb = false;
static bool quiet = false;
static bool csv = false;

int main(int argc, char** argv)
{
    //  std::ios::sync_with_stdio(false);

    try
    {
        int optindex = 0;
        int opt = 0;

        while(1)
        {
            opt = getopt_long(argc, argv, "hk:eosfur:l:i::x:g:w:y:p:vqz:a:m:n:z:j:", longopts, &optindex);

            if( opt == -1 )
                break;

            switch (opt) //разбираем параметры
            {
                case 'h':    //--help
                    usage();
                    return 0;

                case 'v':
                    verb = true;
                    break;

                case 'q':
                    quiet = true;
                    break;

                case 'c':    //--confile
                    conffile = optarg;
                    break;

                case 'o':    //--omap
                {
                    uniset_init(argc, argv, conffile);
                    return omap();
                }
                break;

                case 'x':    //--setValue
                {
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);
                    const string name = ( optarg ) ? optarg : "";
                    return setValue(name, ui);
                }
                break;

                case 'z':    //--freeze
                {
                    std::string sensors(optarg);
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);
                    return freezeValue(sensors, true, ui);
                }
                break;

                case 'n':    //--unfreeze
                {
                    std::string sensors(optarg);
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);
                    return freezeValue(sensors, false, ui);
                }
                break;

                case 'g':    //--getValue
                case 'k':    //--csv
                {
                    if( opt == 'k' )
                        csv = true;

                    //                    cout<<"(main):received option --getValue='"<<optarg<<"'"<<endl;
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);
                    const string name = ( optarg ) ? optarg : "";
                    return getValue(name, ui);
                }
                break;

                case 'w':    //--getRawValue
                {
                    //                cout<<"(main):received option --getRawValue='"<<optarg<<"'"<<endl;
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);
                    const string name = ( optarg ) ? optarg : "";
                    return getRawValue(name, ui);
                }
                break;

                case 't':    //--getTimeChange
                {
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);
                    const string name = ( optarg ) ? optarg : "";
                    return getTimeChange(name, ui);
                }
                break;

                case 'p':    //--oinfo
                {
                    //                    cout<<"(main):received option --oinfo='"<<optarg<<"'"<<endl;
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);

                    std::string userparam = { "" };

                    // смотрим второй параметр
                    if( checkArg(optind, argc, argv) )
                        userparam = string(argv[optind]);

                    return oinfo(optarg, ui, userparam);
                }
                break;

                case 'j':  //--sinfo
                {
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);
                    return sinfo(optarg, ui);
                }
                break;

                case 'e':    //--exist
                {
                    //                    cout<<"(main):received option --exist"<<endl;
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);

                    verb = true;
                    Command cmd = Exist;
                    auto rep = grpc::CreateChannel(conf->repositoryAddr(), grpc::InsecureChannelCredentials());

                    if( !rep )
                    {
                        cerr << "can't resolve repository" << endl;
                        return 1;
                    }

                    commandToAll(ui, conf->getServicesSection(), rep, (Command)cmd);
                    commandToAll(ui, conf->getControllersSection(), rep, (Command)cmd);
                    commandToAll(ui, conf->getObjectsSection(), rep, (Command)cmd);
                }

                return 0;

                case 's':    //--start
                {
                    //                    cout<<"(main):received option --start"<<endl;
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);

                    Command cmd = StartUp;
                    auto rep = grpc::CreateChannel(conf->repositoryAddr(), grpc::InsecureChannelCredentials());

                    if( !rep )
                    {
                        cerr << "can't resolve repository" << endl;
                        return 1;
                    }

                    commandToAll(ui, conf->getServicesSection(), rep, (Command)cmd);
                    commandToAll(ui, conf->getControllersSection(), rep, (Command)cmd);
                    commandToAll(ui, conf->getObjectsSection(), rep, (Command)cmd);
                }

                return 0;

                case 'r':    //--configure
                {
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);
                    const string name = ( optarg ) ? optarg : "";
                    return configure(name, ui);
                }
                break;

                case 'f':    //--finish
                {
                    //                    cout<<"(main):received option --finish"<<endl;
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);

                    Command cmd = Finish;
                    auto rep = grpc::CreateChannel(conf->repositoryAddr(), grpc::InsecureChannelCredentials());

                    if( !rep )
                    {
                        cerr << "can't resolve repository" << endl;
                        return 1;
                    }

                    commandToAll(ui, conf->getServicesSection(), rep, (Command)cmd);
                    commandToAll(ui, conf->getControllersSection(), rep, (Command)cmd);
                    commandToAll(ui, conf->getObjectsSection(), rep, (Command)cmd);

                    if( verb )
                        cout << "(finish): done" << endl;
                }

                return 0;

                case 'l':    //--logrotate
                {
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);

                    const string name = ( optarg ) ? optarg : "";
                    return logRotate(name, ui);
                }
                break;

                case 'y':    //--getCalibrate
                {
                    //                    cout<<"(main):received option --getCalibrate='"<<optarg<<"'"<<endl;
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);
                    const string name = ( optarg ) ? optarg : "";
                    return getCalibrate(name, ui);
                }
                break;

                case 'u':    //--foldUp
                {
                    //                    cout<<"(main):received option --foldUp"<<endl;
                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);

                    Command cmd = FoldUp;
                    auto rep = grpc::CreateChannel(conf->repositoryAddr(), grpc::InsecureChannelCredentials());

                    if( !rep )
                    {
                        cerr << "can't resolve repository" << endl;
                        return 1;
                    }

                    commandToAll(ui, conf->getServicesSection(), rep, (Command)cmd);
                    commandToAll(ui, conf->getControllersSection(), rep, (Command)cmd);
                    commandToAll(ui, conf->getObjectsSection(), rep, (Command)cmd);
                    //                    cout<<"(foldUp): done"<<endl;
                    return 0;
                }

                case 'm':    //--sendText
                {
                    // смотрим второй параметр
                    if( checkArg(optind, argc, argv) == 0 )
                    {
                        if( !quiet )
                            cerr << "admin(sendText): Unknown 'mtype'. Use: id,name,name2@nodeX mtype text" << endl;

                        return 1;
                    }

                    int mtype = uni_atoi(argv[optind]);
                    const std::string consumers(optarg);
                    ostringstream txt;

                    if( checkArg(optind + 1, argc, argv) == 0 )
                    {
                        if( !quiet )
                            cerr << "admin(sendText): Unknown 'text'. Use: id,name,name2@nodeX mtype text" << endl;

                        return 1;
                    }

                    for( int i = optind + 1; i < argc; i++ )
                    {
                        if( checkArg(i, argc, argv) == 0 )
                            break;

                        txt << " " << argv[i];
                    }

                    auto conf = uniset_init(argc, argv, conffile);
                    UInterface ui(conf);
                    ui.initBackId(uniset3::AdminID);

                    sendText(consumers, ui, txt.str(), mtype);
                    return 0;
                }
                break;

                case '?':
                default:
                {
                    short_usage();
                    return 1;
                }
            }
        }

        return 0;
    }
    catch( std::exception& ex )
    {
        if( !quiet )
            cerr << "exception: " << ex.what() << endl;
    }
    catch(...)
    {
        if( !quiet )
            cerr << "Unknown exception.." << endl;
    }

    return 1;
}

// ==============================================================================================
static bool commandToAll( UInterface& ui, const string& section, std::shared_ptr<grpc::Channel>& rep, Command cmd)
{
    if( verb )
        cout << "\n||=======********  " << section << "  ********=========||\n" << endl;

    uniset3::ios_fmt_restorer ifs(cout);

    cout.setf(ios::left, ios::adjustfield);
    grpc::ClientContext ctx;
    std::shared_ptr<UInterface::ORefInfo> chan;
    uniset3::ObjectRefList lst;
    google::protobuf::StringValue request;
    request.set_value(section);

    std::unique_ptr<URepository_i::Stub> stub(URepository_i::NewStub(rep));

    try
    {
        auto status = stub->list(&ctx, request, &lst);

        if( !status.ok() )
        {
            cerr << "call repository list error: " << status.error_message() << endl;
            return false;
        }

        if( !lst.refs().empty() )
        {
            if( verb )
                cout << "пусто!" << endl;

            return false;
        }
    }
    catch( std::exception& ex )
    {
        cerr << ex.what();
        return false;
    }

    ExistsParams exparam;
    google::protobuf::BoolValue boolResponse;
    google::protobuf::Empty empty;

    uniset3::umessage::MessageHeader header;
    header.set_priority(uniset3::umessage::mpMedium);
    auto ts = uniset3::now_to_uniset_timespec();
    (*header.mutable_ts()) = ts;
    header.set_node(uniset_conf()->getLocalNode());
    header.set_supplier(AdminID);

    for( const auto& o : lst.refs() )
    {
        try
        {
            chan = ui.resolve(o.id());

            if( !chan )
            {
                cerr  << "(getSensorsInfo): call grpc failed. ID=" << o.id() << endl;
                continue;
            }

            grpc::ClientContext octx;
            chan->addMetaData(octx);

            auto oname = uniset3::ObjectIndex::getShortName(uniset_conf()->oind->getMapName(o.id()));

            std::unique_ptr<UniSetObject_i::Stub> obj(UniSetObject_i::NewStub(chan->c));
            uniset3::umessage::SystemMessage msg;
            header.set_consumer(o.id());
            *(msg.mutable_header()) = header;

            switch( cmd )
            {
                case StartUp:
                {
                    msg.set_cmd(uniset3::umessage::SystemMessage::StartUp);
                    auto tm = uniset3::to_transport(msg);

                    auto status = obj->push(&ctx, tm, &empty);

                    if( !status.ok() )
                        cerr << setw(55) << oname << " error: " << status.error_message() << endl;
                    else if( verb )
                        cout << setw(55) << oname << "   <--- start OK" <<   endl;
                }
                break;

                case FoldUp:
                {
                    msg.set_cmd(uniset3::umessage::SystemMessage::FoldUp);
                    auto tm = uniset3::to_transport(msg);

                    auto status = obj->push(&ctx, tm, &empty);

                    if( !status.ok() )
                        cerr << setw(55) << oname << " error: " << status.error_message() << endl;
                    else if( verb )
                        cout << setw(55) << oname << "   <--- foldUp OK" <<   endl;
                }
                break;

                case Finish:
                {
                    msg.set_cmd(uniset3::umessage::SystemMessage::Finish);
                    auto tm = uniset3::to_transport(msg);

                    auto status = obj->push(&ctx, tm, &empty);

                    if( !status.ok() )
                        cerr << setw(55) << oname << " error: " << status.error_message() << endl;
                    else if( verb )
                        cout << setw(55) << oname << "   <--- finish OK" <<   endl;
                }
                break;

                case Exist:
                {
                    exparam.set_id(o.id());
                    auto status = obj->exists(&ctx, exparam, &boolResponse);

                    if ( !status.ok() )
                        cerr << setw(55) << oname << " error: " << status.error_message() << endl;
                    else
                    {
                        if( boolResponse.value() )
                            cout << "(" << setw(6) << o.id() << ")" << setw(55) << oname << "   <--- exist ok\n";
                        else
                            cout << "(" << setw(6) << o.id() << ")" << setw(55) << oname << "   <--- exist NOT OK\n";
                    }
                }
                break;

                case Configure:
                {
                    msg.set_cmd(uniset3::umessage::SystemMessage::ReConfiguration);
                    auto tm = uniset3::to_transport(msg);

                    auto status = obj->push(&ctx, tm, &empty);

                    if( !status.ok() )
                        cerr << setw(55) << oname << " error: " << status.error_message() << endl;
                    else if( verb )
                        cout << setw(55) << oname << "   <--- configure ok\n";
                }
                break;

                case LogRotate:
                {
                    msg.set_cmd(uniset3::umessage::SystemMessage::LogRotate);
                    auto tm = uniset3::to_transport(msg);

                    auto status = obj->push(&ctx, tm, &empty);

                    if( !status.ok() )
                        cerr << setw(55) << oname << " error: " << status.error_message() << endl;
                    else if( verb )
                        cout << setw(55) << oname << "   <--- logrotate ok\n";
                }
                break;

                default:
                {
                    if( !quiet )
                        cout << "неизвестная команда -" << cmd << endl;

                    return false;
                }
            }
        }
        catch( const std::exception& ex )
        {
            if( !quiet )
                cerr << "std::exception: " << ex.what() << endl;
        }
    } // end of for

    return true;
}


// ==============================================================================================
int omap()
{
    uniset3::ios_fmt_restorer ifs(cout);

    try
    {
        cout.setf(ios::left, ios::adjustfield);
        cout << "========================== ObjectsMap  =================================\n";
        uniset_conf()->oind->printMap(cout);
        cout << "==========================================================================\n";
    }
    catch( const uniset3::Exception& ex )
    {
        if( !quiet )
            cerr << " configuration init failed: " << ex << endl;

        return 1;
    }
    catch( const std::exception& ex )
    {
        if( !quiet )
            cerr << "std::exception: " << ex.what() << endl;

        return 1;
    }

    return 0;
}

// --------------------------------------------------------------------------------------
int setValue( const string& args, UInterface& ui )
{
    int err = 0;
    auto conf = ui.getConf();
    auto sl = uniset3::getSInfoList(args, conf);

    if( verb )
        cout << "====== setValue ======" << endl;

    for( auto&& it : sl )
    {
        try
        {
            uniset3::IOType t = conf->getIOType(it.si.id());

            if( verb )
            {
                cout << "  value: " << it.val << endl;
                cout << "   name: (" << it.si.id() << ") " << it.fname << endl;
                cout << " iotype: " << t << endl;
                cout << "   text: " << conf->oind->getTextName(it.si.id()) << "\n\n";
            }

            if( it.si.node() == DefaultObjectId )
                it.si.set_node(conf->getLocalNode());

            switch(t)
            {
                case uniset3::DI:
                case uniset3::DO:
                case uniset3::AI:
                case uniset3::AO:
                    ui.setValue(it.si.id(), it.val, it.si.node(), AdminID);
                    break;

                default:
                    if( !quiet )
                        cerr << "FAILED: Unknown 'iotype' for " << it.fname << endl;

                    err = 1;
                    break;
            }
        }
        catch( const std::exception& ex )
        {
            if( !quiet )
                cerr << "std::exception: " << ex.what() << endl;

            err = 1;
        }
    }

    return err;
}

// --------------------------------------------------------------------------------------
int getValue( const string& args, UInterface& ui )
{
    int err = 0;

    auto conf = ui.getConf();
    auto sl = uniset3::getSInfoList( args, conf );

    if( csv )
        quiet = true;

    if( !quiet )
        cout << "====== getValue ======" << endl;

    size_t num = 0;

    for( auto&& it : sl )
    {
        try
        {
            uniset3::IOType t = conf->getIOType(it.si.id());

            if( !quiet )
            {
                cout << "   name: (" << it.si.id() << ") " << it.fname << endl;
                cout << "   iotype: " << t << endl;
                cout << "   text: " << conf->oind->getTextName(it.si.id()) << "\n\n";
            }

            if( it.si.node() == DefaultObjectId )
                it.si.set_node(conf->getLocalNode());

            switch(t)
            {
                case uniset3::DO:
                case uniset3::DI:
                case uniset3::AO:
                case uniset3::AI:
                    if( !quiet )
                        cout << "  value: " << ui.getValue(it.si.id(), it.si.node()) << endl;
                    else
                    {
                        if( csv )
                        {
                            // т.к. может сработать исключение, а нам надо вывести ','
                            // до числа, то сперва получаем val
                            long val = ui.getValue(it.si.id(), it.si.node());

                            if( num++ > 0 )
                                cout << ",";

                            cout << val;
                        }
                        else
                            cout << ui.getValue(it.si.id(), it.si.node());
                    }

                    break;

                default:
                    if( !quiet )
                        cerr << "FAILED: Unknown 'iotype' for " << it.fname << endl;

                    err = 1;
                    break;
            }
        }
        catch( const std::exception& ex )
        {
            if( !quiet )
                cerr << "std::exception: " << ex.what() << endl;

            err = 1;
        }
    }

    return err;
}
// --------------------------------------------------------------------------------------
int freezeValue( const string& args, bool set, UInterface& ui )
{
    int err = 0;
    auto conf = ui.getConf();
    auto sl = uniset3::getSInfoList(args, conf);

    if( verb )
        cout << "====== " << (set ? "freeze" : "unfreeze") << " ======" << endl;

    for( auto&& it : sl )
    {
        try
        {
            uniset3::IOType t = conf->getIOType(it.si.id());

            if( verb )
            {
                cout << "  value: " << it.val << endl;
                cout << "   name: (" << it.si.id() << ") " << it.fname << endl;
                cout << " iotype: " << t << endl;
                cout << "   text: " << conf->oind->getTextName(it.si.id()) << "\n\n";
            }

            if( it.si.node() == DefaultObjectId )
                it.si.set_node(conf->getLocalNode());

            switch(t)
            {
                case uniset3::DI:
                case uniset3::DO:
                case uniset3::AI:
                case uniset3::AO:
                    ui.freezeValue(it.si, set, it.val, AdminID);
                    break;

                default:
                    if( !quiet )
                        cerr << "FAILED: Unknown 'iotype' for " << it.fname << endl;

                    err = 1;
                    break;
            }
        }
        catch( const std::exception& ex )
        {
            if( !quiet )
                cerr << (set ? "freeze: " : "unfreeze: ") << "std::exception: " << ex.what() << endl;

            err = 1;
        }
    }

    return err;
}
// --------------------------------------------------------------------------------------
int getCalibrate( const std::string& args, UInterface& ui )
{
    int err = 0;
    auto conf = ui.getConf();
    auto sl = uniset3::getSInfoList( args, conf );

    if( !quiet )
        cout << "====== getCalibrate ======" << endl;

    for( auto&& it : sl )
    {
        if( it.si.node() == DefaultObjectId )
            it.si.set_node(conf->getLocalNode());

        try
        {
            if( !quiet )
            {
                cout << "      name: (" << it.si.id() << ") " << it.fname << endl;
                cout << "      text: " << conf->oind->getTextName(it.si.id()) << "\n";
                cout << "калибровка: ";
            }

            uniset3::CalibrateInfo ci = ui.getCalibrateInfo(it.si);

            if( !quiet )
                cout << ci << endl;
            else
                cout << ci;
        }
        catch( const std::exception& ex )
        {
            if( !quiet )
                cerr << "std::exception: " << ex.what() << endl;

            err = 1;
        }
    }

    return err;
}

// --------------------------------------------------------------------------------------
int getRawValue( const std::string& args, UInterface& ui )
{
    int err = 0;
    auto conf = ui.getConf();
    auto sl = uniset3::getSInfoList( args, conf );

    if( !quiet )
        cout << "====== getRawValue ======" << endl;

    for( auto&& it : sl )
    {
        if( it.si.node() == DefaultObjectId )
            it.si.set_node(conf->getLocalNode());

        try
        {
            if( !quiet )
            {
                cout << "   name: (" << it.si.id() << ") " << it.fname << endl;
                cout << "   text: " << conf->oind->getTextName(it.si.id()) << "\n\n";
                cout << "  value: " << ui.getRawValue(it.si) << endl;
            }
            else
                cout << ui.getRawValue(it.si);
        }
        catch( const std::exception& ex )
        {
            if( !quiet )
                cerr << "std::exception: " << ex.what() << endl;

            err = 1;
        }
    }

    return err;
}

// --------------------------------------------------------------------------------------
int getTimeChange( const std::string& args, UInterface& ui )
{
    int err = 0;
    auto conf = ui.getConf();
    auto sl = uniset3::getSInfoList( args, conf );

    if( !quiet )
        cout << "====== getChangedTime ======" << endl;

    for( auto&& it : sl )
    {
        if( it.si.node() == DefaultObjectId )
            it.si.set_node(conf->getLocalNode());

        try
        {
            if( !quiet )
            {
                cout << "   name: (" << it.si.id() << ") " << it.fname << endl;
                cout << "   text: " << conf->oind->getTextName(it.si.id()) << "\n\n";
                cout << ui.getTimeChange(it.si.id(), it.si.node()) << endl;
            }
            else
                cout << ui.getTimeChange(it.si.id(), it.si.node());
        }
        catch( const std::exception& ex )
        {
            if( !quiet )
                cerr << "std::exception: " << ex.what() << endl;

            err = 1;
        }
        catch(...)
        {
            if( !quiet )
                cerr << "Unknown exception.." << endl;

            err = 1;
        }
    }

    return err;
}

// --------------------------------------------------------------------------------------
int logRotate( const string& arg, UInterface& ui )
{
    auto conf = ui.getConf();

    // посылка всем
    if( arg.empty() || arg[0] == '-' )
    {
        auto rep = grpc::CreateChannel(conf->repositoryAddr(), grpc::InsecureChannelCredentials());

        if( !rep )
        {
            cerr << "can't resolve repository" << endl;
            return 1;
        }

        commandToAll(ui, conf->getServicesSection(), rep, (Command)LogRotate);
        commandToAll(ui, conf->getControllersSection(), rep, (Command)LogRotate);
        commandToAll(ui, conf->getObjectsSection(), rep, (Command)LogRotate);
    }
    else // посылка определённому объекту
    {
        auto lst = uniset3::getSInfoList(arg, conf);

        if( lst.empty() )
        {

            if( !quiet )
                cout << "(logrotate): not found ID for name='" << arg << "'" << endl;

            return 1;
        }

        auto s = lst.begin();

        uniset3::umessage::SystemMessage sm;
        sm.mutable_header()->set_priority(uniset3::umessage::mpMedium);
        auto ts = uniset3::now_to_uniset_timespec();
        *(sm.mutable_header()->mutable_ts()) = ts;
        sm.mutable_header()->set_node(uniset_conf()->getLocalNode());
        sm.mutable_header()->set_supplier(AdminID);
        sm.mutable_header()->set_consumer(s->si.id());

        sm.set_cmd(uniset3::umessage::SystemMessage::LogRotate);

        uniset3::umessage::TransportMessage tm = uniset3::to_transport<uniset3::umessage::SystemMessage>(sm);
        ui.send(tm, s->si.node());

        if( verb )
            cout << "\nSend 'LogRotate' to " << arg << " OK.\n";
    }

    return 0;
}

// --------------------------------------------------------------------------------------
int configure( const string& arg, UInterface& ui )
{
    auto conf = ui.getConf();

    // посылка всем
    if( arg.empty() || arg[0] == '-' )
    {
        auto rep = grpc::CreateChannel(conf->repositoryAddr(), grpc::InsecureChannelCredentials());

        if( !rep )
        {
            cerr << "can't resolve repository" << endl;
            return 1;
        }

        commandToAll(ui, conf->getServicesSection(), rep, (Command)Configure);
        commandToAll(ui, conf->getControllersSection(), rep, (Command)Configure);
        commandToAll(ui, conf->getObjectsSection(), rep, (Command)Configure);
    }
    else // посылка определённому объекту
    {
        auto lst = uniset3::getSInfoList(arg, conf);

        if( lst.empty() )
        {

            if( !quiet )
                cout << "(logrotate): not found ID for name='" << arg << "'" << endl;

            return 1;
        }

        auto s = lst.begin();

        uniset3::umessage::SystemMessage sm;
        sm.mutable_header()->set_priority(uniset3::umessage::mpMedium);
        auto ts = uniset3::now_to_uniset_timespec();
        *(sm.mutable_header()->mutable_ts()) = ts;
        sm.mutable_header()->set_node(uniset_conf()->getLocalNode());
        sm.mutable_header()->set_supplier(AdminID);
        sm.mutable_header()->set_consumer(s->si.id());

        sm.set_cmd(uniset3::umessage::SystemMessage::ReConfiguration);
        uniset3::umessage::TransportMessage tm = uniset3::to_transport<uniset3::umessage::SystemMessage>(sm);

        ui.send(tm, s->si.node());

        if( verb )
            cout << "\nSend 'ReConfigure' to " << arg << " OK.\n";
    }

    return 0;
}

// --------------------------------------------------------------------------------------
int oinfo(const string& args, UInterface& ui, const string& userparam )
{
    auto conf = uniset_conf();
    auto sl = uniset3::getObjectsList( args, conf );

    for( auto&& it : sl )
    {
        if( it.node() == DefaultObjectId )
            it.set_node(conf->getLocalNode());

        try
        {
            cout << ui.getObjectInfo(it.id(), userparam, it.node()) << endl;
        }
        catch( const std::exception& ex )
        {
            if( !quiet )
                cerr << "std::exception: " << ex.what() << endl;
        }
        catch(...)
        {
            if( !quiet )
                cerr << "Unknown exception.." << endl;
        }

        cout << endl << endl;
    }

    return 0;
}
// --------------------------------------------------------------------------------------
int sinfo(const string& args, UInterface& ui )
{
    int err = 0;
    auto conf = uniset_conf();
    auto sl = uniset3::getSInfoList(args, conf);

    for( auto&& it : sl )
    {
        try
        {
            // проверка есть ли такой датчик, т.к. тут будет выкинуто исключение
            // если его нет
            uniset3::IOType t = conf->getIOType(it.si.id());

            if( it.si.node() == DefaultObjectId )
                it.si.set_node(conf->getLocalNode());

            auto sinf = ui.getSensorIOInfo(it.si);
#if 0
            uniset3::IOType type;     /*!< тип */
            long priority;                /*!< приоритет уведомления */
            long default_val;             /*!< значение по умолчанию */
            CalibrateInfo ci;             /*!< калибровочные параметры */
#endif
            const int w = 14;
            print_help(w, "id", std::to_string(it.si.id()) + "\n", " ", " : ");
            print_help(w, "node", std::to_string(it.si.node()) + "\n", " ", " : ");
            print_help(w, "value", std::to_string(sinf.value()) + "\n", " ", " : ");
            print_help(w, "real_value", std::to_string(sinf.real_value()) + "\n", " ", " : ");
            print_help(w, "frozen", std::to_string(sinf.frozen()) + "\n", " ", " : ");
            print_help(w, "blocked", std::to_string(sinf.blocked()) + "\n", " ", " : ");

            if( sinf.depend_sid() != DefaultObjectId )
                print_help(w, "depend_sensor", "(" + to_string(sinf.depend_sid()) + ")" + ObjectIndex::getShortName(conf->oind->getMapName(sinf.depend_sid())) + "\n", " ", " : ");

            if( sinf.supplier() == uniset3::AdminID )
                print_help(w, "supplier", "admin\n", " ", " : ");
            else
                print_help(w, "supplier", ObjectIndex::getShortName(conf->oind->getMapName(sinf.supplier())) + "\n", " ", " : ");

            ostringstream ts;
            ts << dateToString(sinf.ts().sec()) << " " << timeToString(sinf.ts().sec()) << "." << sinf.ts().nsec() << "\n";
            print_help(w, "changed", ts.str(), " ", " : ");
        }
        catch( const std::exception& ex )
        {
            if( !quiet )
                cerr << "(sinfo): std::exception: " << ex.what() << endl;

            err = 1;
        }
    }

    return err;
}
// --------------------------------------------------------------------------------------
void sendText( const string& args, UInterface& ui, const string& txt, int mtype )
{
    auto conf = uniset_conf();
    auto sl = uniset3::getObjectsList( args, conf );

    cout << "mtype=" << mtype << " txt: " << txt << endl;

    for( auto&& it : sl )
    {
        if( it.node() == DefaultObjectId )
            it.set_node(conf->getLocalNode());

        try
        {
            ui.sendText(it.id(), txt, it.node());
        }
        catch( const std::exception& ex )
        {
            if( !quiet )
                cerr << "std::exception: " << ex.what() << endl;
        }
        catch(...)
        {
            if( !quiet )
                cerr << "Unknown exception.." << endl;
        }
    }
}

// --------------------------------------------------------------------------------------
void errDoNotResolve( const std::string& oname )
{
    if( verb )
        cerr << oname << ": resolve failed.." << endl;
}
// --------------------------------------------------------------------------------------
char* checkArg( int i, int argc, char* argv[] )
{
    if( i < argc && (argv[i])[0] != '-' )
        return argv[i];

    return 0;
}
// --------------------------------------------------------------------------------------
