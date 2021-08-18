#include "Configuration.h"
#include "HttpAPIGateway.h"
// --------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    //  std::ios::sync_with_stdio(false);

    try
    {
        if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
        {
            cout << endl;
            cout << "Usage: uniset3-api-gateway args1 args2" << endl;
            cout << endl;
            cout << "--confile filename - configuration file. Default: configure.xml" << endl;
            cout << endl;
            HttpAPIGateway::help_print();
            return 0;
        }

        auto conf = uniset_init(argc, argv);
        auto api = HttpAPIGateway::init_apigateway(argc, argv);

        if( !api )
            return 1;

        api->run();
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(HttpAPIGateway::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(HttpAPIGateway::main): catch ..." << endl;
    }

    return 1;
}
