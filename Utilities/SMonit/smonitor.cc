#include <iostream>
#include <string>
#include "UniSetActivator.h"
#include "Configuration.h"
#include "SMonitor.h"
// -----------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// -----------------------------------------------------------------------------
static int asyncMonitor( const std::list<uniset3::ParamSInfo>& lst );
static int streamMonitor( const std::list<uniset3::ParamSInfo>& lst );
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
    //  std::ios::sync_with_stdio(false);

    try
    {
        if( argc > 1 && ( !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h") ) )
        {
            cout << "Usage: uniset-smonit [ args ] --sid id1@node1,Sensor2@node2,id2,sensorname3,... " << endl
                 << "Args: " << endl
                 << "--name XXX - name for smonit. Default: TestProc (only for remote sensors)" << endl;
            //                 << " --script scriptname \n"
            cout << uniset3::Configuration::help() << endl;
            return 0;
        }

        uniset_init(argc, argv, "configure.xml");

        const string sid = uniset3::getArgParam("--sid", argc, argv, "");
        auto lst = uniset3::getSInfoList(sid, uniset_conf());

        if( lst.empty() || sid.empty() )
        {
            cerr << "Unknown sensors. Use --sid .. or --help" << endl;
            return 1;
        }

        bool useAsync = false;

        for( const auto& s : lst )
        {
            if( s.si.node() != DefaultObjectId )
            {
                useAsync = true;
                break;
            }
        }

        if( useAsync )
            return asyncMonitor(lst);

        return streamMonitor(lst);
    }
    catch( const std::exception& ex )
    {
        cout << "(main): exception: " << ex.what() << endl;
    }
    catch(...)
    {
        cout << "(main): Unknown exception!!" << endl;
    }

    return 1;
}
// ------------------------------------------------------------------------------------------
int asyncMonitor( const std::list<uniset3::ParamSInfo>& lst )
{
    auto conf = uniset_conf();

    ObjectId ID(DefaultObjectId);
    const string name = conf->getArgParam("--name", "TestProc");

    ID = conf->getObjectID(name);

    if( ID == uniset3::DefaultObjectId )
    {
        cerr << "(main): идентификатор '" << name
             << "' не найден в конф. файле!"
             << " в секции " << conf->getObjectsSection() << endl;
        return 1;
    }

    auto act = UniSetActivator::Instance();
    auto smon = make_shared<SMonitor>(ID);
    act->add(smon);

    act->run(false);
    return 0;
}

// ------------------------------------------------------------------------------------------
int streamMonitor( const std::list<uniset3::ParamSInfo>& lst )
{
    UInterface ui;
    auto oref = ui.resolve(lst.begin()->si.id());

    if( !oref->c )
    {
        cerr << "can't resolve server: " << oref->ref.addr() << endl;
        return 1;
    }

    std::unique_ptr<IONotifyStreamController_i::Stub> stub(IONotifyStreamController_i::NewStub(oref->c));

    grpc::ClientContext ctx;
    SensorsStreamCmd request;
    request.set_cmd(uniset3::UIONotify);

    for( const auto& i : lst )
    {
        auto s = request.add_slist();
        s->set_id(i.si.id());
        s->set_val(i.val);
    }

    umessage::SensorMessage reply;
    auto stream = stub->sensorsStream(&ctx);

    if( !stream->Write(request) )
    {
        cerr << "ERROR: failed send command to server " << oref->ref.addr() << endl;
        return 1;
    }

    while( stream->Read(&reply) )
        cout << SMonitor::printEvent(&reply) << endl;

    auto st = stream->Finish();
    cout << "end with status: [" <<  st.error_code() << "]" << st.error_message() << endl;
    return 0;
}
