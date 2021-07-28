/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include <sstream>
#include "MQTTPublisher.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniSetActivator.h"
#include "Extensions.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::extensions;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
    //  std::ios::sync_with_stdio(false);

    if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
    {
        cout << endl;
        cout << "Usage: uniset2-mqttpulisher --confile configure.xml args1 args2" << endl;
        cout << endl;
        cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
        cout << endl;
        MQTTPublisher::help_print(argc, argv);
        cout << " Global options:" << endl;
        cout << uniset3::Configuration::help() << endl;
        return 0;
    }

    try
    {
        auto conf = uniset_init( argc, argv );

        ObjectId shmID = DefaultObjectId;
        string sID = conf->getArgParam("--smemory-id");

        if( !sID.empty() )
            shmID = conf->getControllerID(sID);
        else
            shmID = getSharedMemoryID();

        if( shmID == DefaultObjectId )
        {
            cerr << sID << "? SharedMemoryID not found in " << conf->getControllersSection() << " section" << endl;
            return 1;
        }

        auto mqtt = MQTTPublisher::init_mqttpublisher(argc, argv, shmID);

        if( !mqtt )
        {
            dcrit << "(mqttpublisher): init failed..." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->add(mqtt);

        act->startup();
        act->run(false);
        return 0;
    }
    catch( std::exception& ex )
    {
        cerr << "(mqttpublisher): " << ex.what() << std::endl;
    }
    catch(...)
    {
        cerr << "(mqttpublisher): catch ..." << std::endl;
    }

    return 1;
}
