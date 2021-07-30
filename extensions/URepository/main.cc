#include "Configuration.h"
#include "URepository.h"
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
            cout << "Usage: uniset3-urepository args1 args2" << endl;
            cout << endl;
            cout << "--confile filename - configuration file. Default: configure.xml" << endl;
            cout << endl;
            URepository::help_print();
            return 0;
        }

        auto repository = URepository::init_repository(argc, argv);

        if( !repository )
            return 1;

        repository->run();
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(URepository::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(URepository::main): catch ..." << endl;
    }

    return 1;
}
