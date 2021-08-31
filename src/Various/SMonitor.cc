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
// ------------------------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include <cmath>
#include "SMonitor.h"
#include "Configuration.h"
#include "UniSetTypes.h"
// ------------------------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// ------------------------------------------------------------------------------------------
SMonitor::SMonitor():
    script("")
{
}

SMonitor::SMonitor(ObjectId id):
    UniSetObject(id),
    script("")
{
    const string sid(uniset_conf()->getArgParam("--sid"));

    lst = uniset3::getSInfoList(sid, uniset_conf());

    if( lst.empty() )
        throw SystemError("Не задан список датчиков (--sid)");

    script = uniset_conf()->getArgParam("--script");
}

SMonitor::SMonitor( uniset3::ObjectId id, const std::list<uniset3::ParamSInfo>& _lst ):
    UniSetObject(id),
    script(""),
    lst(_lst)
{

}

SMonitor::~SMonitor()
{
}
// ------------------------------------------------------------------------------------------
void SMonitor::sysCommand( const uniset3::umessage::SystemMessage* sm )
{
    switch(sm->cmd())
    {
        case umessage::SystemMessage::StartUp:
        {
            for( auto&& it : lst )
            {
                if( it.si.node() == DefaultObjectId )
                    it.si.set_node(uniset_conf()->getLocalNode());

                try
                {
                    if( it.si.id() != DefaultObjectId )
                        ui->askRemoteSensor(it.si.id(), uniset3::UIONotify, it.si.node());
                }
                catch( const std::exception& ex )
                {
                    cerr << myname << ":(askSensor): " << ex.what() << endl;
                    uterminate();
                }
                catch(...)
                {
                    std::exception_ptr p = std::current_exception();
                    cerr << myname << ": " << (p ? p.__cxa_exception_type()->name() : "FAIL ask sensors..") << std::endl;
                    uterminate();
                }
            }
        }
        break;

        case umessage::SystemMessage::FoldUp:
        case umessage::SystemMessage::Finish:
            break;

        case umessage::SystemMessage::WatchDog:
            break;

        default:
            break;
    }
}
// ------------------------------------------------------------------------------------------
std::string SMonitor::printEvent( const uniset3::umessage::SensorMessage* sm )
{
    auto conf = uniset_conf();
    ostringstream s;

    string s_sup("");

    if( sm->header().supplier() == uniset3::AdminID )
        s_sup = "uniset-admin";
    else
        s_sup = ObjectIndex::getShortName(conf->oind->getMapName(sm->header().supplier()));

    s << "(" << setw(6) << sm->id() << "):"
      << "[(" << std::right << setw(5) << sm->header().supplier() << ")"
      << std::left << setw(20) << s_sup <<  "] "
      << std::right << setw(8) << timeToString(sm->sm_ts().sec(), ":")
      << "(" << setw(6) << sm->sm_ts().nsec() << "): "
      << std::right << setw(45) << conf->oind->getMapName(sm->id())
      << "    value:" << std::right << setw(9) << sm->value()
      << "    fvalue:" << std::right << setw(12) << ( (float)sm->value() / pow(10.0, sm->ci().precision()) ) << endl;

    return s.str();
}
// ------------------------------------------------------------------------------------------
void SMonitor::sensorInfo( const umessage::SensorMessage* si )
{
    cout << printEvent(si) << endl;

    if( !script.empty() )
    {
        ostringstream cmd;

        // если задан полный путь или путь начиная с '.'
        // то берём как есть, иначе прибавляем bindir из файла настроек
        if( script[0] == '.' || script[0] == '/' )
            cmd << script;
        else
            cmd << uniset_conf()->getBinDir() << script;

        cmd << " " << si->id() << " " << si->value() << " " << si->sm_ts().sec() << " " << si->sm_ts().nsec();

        int ret = system(cmd.str().c_str());
        int res = WEXITSTATUS(ret);

        if( res != 0 )
            cerr << "run script '" << cmd.str() << "' failed.." << endl;

        //        if( WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
        //        {
        //            cout << "finish..." << endl;
        //        }
    }
}
// ------------------------------------------------------------------------------------------
