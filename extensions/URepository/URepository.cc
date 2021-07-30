/*
 * Copyright (c) 2020 Pavel Vainerman.
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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include <unistd.h>

#include "unisetstd.h"
#include <Poco/Net/NetException.h>
#include "ujson.h"
#include "IORFile.h"
#include "URepository.h"
#include "Configuration.h"
#include "Exceptions.h"
#include "Debug.h"
#include "UniXML.h"
#include "URepositorySugar.h"
// --------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// --------------------------------------------------------------------------
URepository::URepository( const string& name, int argc, const char* const* argv, const string& prefix ):
    myname(name)
{
    rlog = make_shared<DebugStream>();

    auto logLevels = uniset3::getArgParam("--" + prefix + "log-add-levels", argc, argv, "crit,warn");

    if( !logLevels.empty() )
        rlog->addLevel( Debug::value(logLevels) );

    std::string config = uniset3::getArgParam("--confile", argc, argv, "configure.xml");

    if( config.empty() )
        throw SystemError("Unknown config file");

    std::shared_ptr<UniXML> xml = make_shared<UniXML>();

    try
    {
        cout << myname << "(init): init from " << config << endl;
        xml->open(config);
    }
    catch( std::exception& ex )
    {
        throw ex;
    }

    xmlNode* cnode = xml->findNode(xml->getFirstNode(), "URepository", name);

    if( !cnode )
    {
        ostringstream err;
        err << name << "(init): Not found confnode <URepository name='" << name << "'...>";
        rcrit << err.str() << endl;
        throw uniset3::SystemError(err.str());
    }

    UniXML::iterator it(cnode);

//    UniXML::iterator dirIt = xml->findNode(xml->getFirstNode(), "LockDir");

//    if( !dirIt )
//    {
//        ostringstream err;
//        err << name << "(init): Not found confnode <LockDir name='..'/>";
//        rcrit << err.str() << endl;
//        throw uniset3::SystemError(err.str());
//    }


//    iorfile = make_shared<IORFile>(dirIt.getProp("name"));
//    rinfo << myname << "(init): IOR directory: " << dirIt.getProp("name") << endl;


}
//--------------------------------------------------------------------------------------------
URepository::~URepository()
{
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<URepository> URepository::init_repository( int argc, const char* const* argv, const std::string& prefix )
{
    string name = uniset3::getArgParam("--" + prefix + "name", argc, argv, "URepository");

    if( name.empty() )
    {
        cerr << "(URepository): Unknown name. Use --" << prefix << "name" << endl;
        return nullptr;
    }

    return make_shared<URepository>(name, argc, argv, prefix);
}
// -----------------------------------------------------------------------------
void URepository::help_print()
{
    cout << "Default: prefix='httpresolver'" << endl;
    cout << "--prefix-host ip          - IP на котором слушает http сервер. По умолчанию: 0.0.0.0" << endl;
    cout << "--prefix-port num         - Порт на котором принимать запросы. По умолчанию: 8008" << endl;
    cout << "--prefix-max-queued num   - Размер очереди запросов к http серверу. По умолчанию: 100" << endl;
    cout << "--prefix-max-threads num  - Разрешённое количество потоков для http-сервера. По умолчанию: 3" << endl;
    cout << "--prefix-cors-allow addr  - (CORS): Access-Control-Allow-Origin. Default: *" << endl;
}
// -----------------------------------------------------------------------------
void URepository::run()
{
    pause();
}
// -----------------------------------------------------------------------------
