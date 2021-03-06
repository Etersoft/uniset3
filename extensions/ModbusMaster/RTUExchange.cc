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
#include <cmath>
#include <sstream>
#include "Extensions.h"
#include "RTUExchange.h"
#include "modbus/MBLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::extensions;
// -----------------------------------------------------------------------------
RTUExchange::RTUExchange(uniset3::ObjectId objId, uniset3::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
                         const std::string& prefix_ ):
    MBExchange(objId, shmId, ic, prefix_),
    mbrtu(0),
    defSpeed(ComPort::ComSpeed38400),
    use485F(false),
    transmitCtl(false),
    rs_pre_clean(false)
{
    if( objId == DefaultObjectId )
        throw uniset3::SystemError("(RTUExchange): objId=-1?!! Use --" + mbconf->prefix + "-name" );

    auto conf = uniset_conf();

    // префикс для "свойств" - по умолчанию
    mbconf->prop_prefix = "";

    // если задано поле для "фильтрации"
    // то в качестве префикса используем его
    if( !mbconf->s_field.empty() )
        mbconf->prop_prefix = mbconf->s_field + "_";

    // если "принудительно" задан префикс
    // используем его.
    {
        string p("--" + mbconf->prefix + "-set-prop-prefix");
        string v = conf->getArgParam(p, "");

        if( !v.empty() && v[0] != '-' )
            mbconf->prop_prefix = v;
        // если параметр всё-таки указан, считаем, что это попытка задать "пустой" префикс
        else if( findArgParam(p, conf->getArgc(), conf->getArgv()) != -1 )
            mbconf->prop_prefix = "";
    }

    mbinfo << myname << "(init): prop_prefix=" << mbconf->prop_prefix << endl;

    UniXML::iterator it(cnode);
    // ---------- init RS ----------
    devname    = conf->getArgParam("--" + mbconf->prefix + "-dev", it.getProp("device"));

    if( devname.empty() )
        throw uniset3::SystemError(myname + "(RTUExchange): Unknown device..." );

    string speed = conf->getArgParam("--" + mbconf->prefix + "-speed", it.getProp("speed"));

    if( speed.empty() )
        speed = "38400";

    use485F = conf->getArgInt("--" + mbconf->prefix + "-use485F", it.getProp("use485F"));
    transmitCtl = conf->getArgInt("--" + mbconf->prefix + "-transmit-ctl", it.getProp("transmitCtl"));
    defSpeed = ComPort::getSpeed(speed);

    mbconf->sleepPause_msec = conf->getArgPInt("--" + mbconf->prefix + "-sleepPause-usec", it.getProp("slepePause"), 100);

    rs_pre_clean = conf->getArgInt("--" + mbconf->prefix + "-pre-clean", it.getProp("pre_clean"));

    if( shm->isLocalwork() )
        mbconf->loadConfig(conf->getConfXML(), conf->getXMLSensorsSection());
    else
        ic->addReadItem( sigc::mem_fun(this, &RTUExchange::readItem) );

    initMB(false);

    if( dlog()->is_info() )
        MBConfig::printMap(mbconf->devices);
}
// -----------------------------------------------------------------------------
void RTUExchange::help_print( int argc, const char* const* argv )
{
    cout << "Default: prefix='rs'" << endl;
    MBExchange::help_print(argc, argv);
    //    cout << " Настройки протокола RS: " << endl;
    cout << "--prefix-dev devname  - файл устройства" << endl;
    cout << "--prefix-speed        - Скорость обмена (9600,19920,38400,57600,115200)." << endl;
    cout << "--prefix-my-addr      - адрес текущего узла" << endl;
    cout << "--prefix-recv-timeout - Таймаут на ожидание ответа." << endl;
    cout << "--prefix-pre-clean    - Очищать буфер перед каждым запросом" << endl;
    cout << "--prefix-sleepPause-usec - Таймаут на ожидание очередного байта" << endl;
}
// -----------------------------------------------------------------------------
RTUExchange::~RTUExchange()
{
    //    delete mbrtu;
}
// -----------------------------------------------------------------------------
std::shared_ptr<ModbusClient> RTUExchange::initMB( bool reopen )
{
    if( !file_exists(devname) )
    {
        if( mbrtu )
        {
            mb = nullptr;
            mbrtu = nullptr;
        }

        return mbrtu;
    }

    if( mbrtu )
    {
        if( !reopen )
            return mbrtu;

        mbrtu = nullptr;
        mb = nullptr;
    }

    try
    {
        mbrtu = std::make_shared<ModbusRTUMaster>(devname, use485F, transmitCtl);

        if( defSpeed != ComPort::ComSpeed0 )
            mbrtu->setSpeed(defSpeed);

        auto l = loga->create(myname + "-exchangelog");
        mbrtu->setLog(l);

        if( ic )
            ic->logAgregator()->add(loga);

        if( mbconf->recv_timeout > 0 )
            mbrtu->setTimeout(mbconf->recv_timeout);

        mbrtu->setSleepPause(mbconf->sleepPause_msec);
        mbrtu->setAfterSendPause(mbconf->aftersend_pause);

        mbinfo << myname << "(init): dev=" << devname << " speed=" << ComPort::getSpeed( mbrtu->getSpeed() ) << endl;
    }
    catch( const std::exception& ex )
    {
        mbrtu = nullptr;
        mbwarn << myname << "(init): " << ex.what() << endl;
    }
    catch(...)
    {
        mbrtu = nullptr;
        mbinfo << myname << "(init): catch...." << endl;
    }

    mb = mbrtu;
    return mbrtu;
}
// -----------------------------------------------------------------------------
void RTUExchange::step()
{
    try
    {
        if( sidExchangeMode != DefaultObjectId && force )
            exchangeMode = shm->localGetValue(itExchangeMode, sidExchangeMode);
    }
    catch(...) {}

    try
    {
        poll();
    }
    catch( std::exception& ex )
    {
        mbwarn << myname << "(step): poll error:  " << ex.what() << endl;
    }

    try
    {
        MBExchange::step();
    }
    catch( std::exception& ex )
    {
        mbwarn << myname << "(step): MBExchange::step error:  " << ex.what() << endl;
    }
}
// -----------------------------------------------------------------------------
bool RTUExchange::poll()
{
    if( !mb )
    {
        mb = initMB(false);

        if( !isProcActive() )
            return false;

        updateSM();
        allInitOK = false;
        return false;
    }

    if( !allInitOK )
        firstInitRegisters();

    if( !isProcActive() )
        return false;

    ncycle++;
    bool allNotRespond = true;
    ComPort::Speed s = mbrtu->getSpeed();

    for( auto it1 : mbconf->devices )
    {
        auto d = it1.second;

        if( d->mode_id != DefaultObjectId && d->mode == MBConfig::emSkipExchange )
            continue;

        if( d->speed != s )
        {
            s = d->speed;
            mbrtu->setSpeed(d->speed);
        }

        d->prev_numreply.store(d->numreply);


        dlog3 << myname << "(poll): ask addr=" << ModbusRTU::addr2str(d->mbaddr)
              << " regs=" << d->pollmap.size() << endl;

        for( auto&& m : d->pollmap )
        {
            if( m.first != 0 && (ncycle % m.first) != 0 )
                continue;

            auto rmap = m.second;

            for( auto&& it = rmap->begin(); it != rmap->end(); ++it )
            {
                try
                {
                    if( d->dtype == MBConfig::dtRTU || d->dtype == MBConfig::dtMTR )
                    {
                        if( rs_pre_clean )
                            mb->cleanupChannel();

                        if( pollRTU(d, it) )
                        {
                            d->numreply++;
                            allNotRespond = false;
                        }
                    }
                }
                catch( ModbusRTU::mbException& ex )
                {
                    dlog3 << myname << "(poll): FAILED ask addr=" << ModbusRTU::addr2str(d->mbaddr)
                          << " reg=" << ModbusRTU::dat2str(it->second->mbreg)
                          << " for sensors: ";
                    mbconf->print_plist(dlog()->level3(), it->second->slst);
                    dlog()->level3(false) << " err: " << ex << endl;
                }

                if( it == rmap->end() )
                    break;

                if( !isProcActive() )
                    return false;
            }
        }
    }

    // update SharedMemory...
    updateSM();

    // check thresholds
    for( auto&& t : mbconf->thrlist )
    {
        if( !isProcActive() )
            return false;

        IOBase::processingThreshold(&t, shm, force);
    }

    if( trReopen.hi(allNotRespond) )
        ptReopen.reset();

    if( allNotRespond && ptReopen.checkTime() )
    {
        mbwarn << myname << ": REOPEN timeout..(" << ptReopen.getInterval() << ")" << endl;

        mb = initMB(true);
        ptReopen.reset();
    }

    //    printMap(rmap);
    return !allNotRespond;
}
// -----------------------------------------------------------------------------
std::shared_ptr<RTUExchange> RTUExchange::init_rtuexchange(int argc, const char* const* argv, uniset3::ObjectId icID,
        const std::shared_ptr<SharedMemory>& ic, const std::string& prefix )
{
    auto conf = uniset_conf();

    string name = conf->getArgParam("--" + prefix + "-name", "RTUExchange1");

    if( name.empty() )
    {
        cerr << "(rtuexchange): Unknown 'name'. Use --" << prefix << "-name" << endl;
        return 0;
    }

    ObjectId ID = conf->getObjectID(name);

    if( ID == uniset3::DefaultObjectId )
    {
        cerr << "(rtuexchange): Not found ID for '" << name
             << "'!"
             << " in section <" << conf->getObjectsSection() << ">" << endl;
        return 0;
    }

    dinfo << "(rtuexchange): name = " << name << "(" << ID << ")" << endl;
    return make_shared<RTUExchange>(ID, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
