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
#include "Configuration.h"
#include "Extensions.h"
#include "UniSetTypes.h"
#include "IOBase.h"
// -----------------------------------------------------------------------------
using namespace std;
// -----------------------------------------------------------------------------
namespace uniset3
{
    // -----------------------------------------------------------------------------
    IOBase::IOBase():
        stype(uniset3::UnknownIOType),
        cdiagram(nullptr),
        value(0),
        craw(0),
        cprev(0),
        safeval(0),
        defval(0),
        df(1),
        nofilter(false),
        f_median(false),
        f_ls(false),
        f_filter_iir(false),
        ignore(false),
        invert(false),
        noprecision(false),
        calcrop(true),
        debounce_pause(false),
        debounce_state(false),
        ondelay_state(false),
        offdelay_state(false),
        d_id(uniset3::DefaultObjectId),
        d_value(1),
        d_off_value(0),
        d_iotype(uniset3::UnknownIOType),
        t_ai(uniset3::DefaultObjectId),
        t_hilimit(0),
        t_lowlimit(0),
        t_invert(false),
        front(false),
        front_type(ftUnknown),
        front_prev_state(false),
        front_state(false),
        rawdata(false)
    {
        si.set_id(uniset3::DefaultObjectId);
        si.set_node(uniset3::DefaultObjectId);
        cal.set_minraw(0);
        cal.set_maxraw(0);
        cal.set_mincal(0);
        cal.set_maxcal(0);
        cal.set_precision(0);
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, IOBase& inf )
    {
        return os << "(" << inf.si.id() << ")" << uniset_conf()->oind->getMapName(inf.si.id())
               << " default=" << inf.defval
               << " safeval=" << inf.safeval
               << " stype=" << inf.stype
               << " calibration=" << inf.cal
               << " cdiagram=" << ( inf.cdiagram ? inf.cdiagram->getName() : "null" )
               << " value=" << inf.value
               << " craw=" << inf.craw
               << " cprev" << inf.cprev
               << " nofilter=" << inf.nofilter
               << " f_median=" << inf.f_median
               << " f_ls=" << inf.f_ls
               << " f_filter_iir=" << inf.f_filter_iir
               << " ignore=" << inf.ignore
               << " invert=" << inf.invert
               << " noprecision=" << inf.noprecision
               << " calcrop=" << inf.calcrop
               << " debounce_pause=" << inf.debounce_pause
               << " debounce_state=" << inf.debounce_state
               << " ondelay_state=" << inf.ondelay_state
               << " offdelay_state=" << inf.offdelay_state
               << " d_id=" << uniset_conf()->oind->getMapName(inf.d_id)
               << " d_value=" << inf.d_value
               << " d_off_value=" << inf.d_off_value
               << " d_iotype=" << inf.d_iotype
               << " t_ai=" << inf.t_ai
               << " t_invert" << inf.t_invert
               << " t_hilimit" << inf.t_hilimit
               << " t_lowlimit" << inf.t_lowlimit
               << " front=" << inf.front
               << " front_type=" << inf.front_type
               << " front_prev_state=" << inf.front_prev_state
               << " front_state=" << inf.front_state
               << " rawdata=" << inf.rawdata;
    }
    // -----------------------------------------------------------------------------
    IOBase::~IOBase()
    {
        delete cdiagram;
    }
    // -----------------------------------------------------------------------------
    bool IOBase::check_depend( const std::shared_ptr<SMInterface>& shm )
    {
        if( d_id == DefaultObjectId )
            return true;

        if( d_iotype == uniset3::DI || d_iotype == uniset3::DO  )
        {
            if( (bool)shm->localGetValue(d_it, d_id) == (bool)d_value )
                return true;

            return false;
        }

        if( d_iotype == uniset3::AI || d_iotype == uniset3::AO )
        {
            if( shm->localGetValue(d_it, d_id) == d_value )
                return true;

            return false;
        }

        return true;
    }
    // -----------------------------------------------------------------------------
    bool IOBase::check_debounce( bool val )
    {
        // нет защиты от дребезга
        if( ptDebounce.getInterval() <= 0 )
        {
            debounce_state = val;
            return val;
        }

        if( trdebounce.change(val) )
        {
            if( !debounce_pause )
            {
                // засекаем время...
                debounce_pause = true;
                ptDebounce.reset();
            }
        }

        if( debounce_pause && ptDebounce.checkTime() )
        {
            // пауза на дребезг кончилась
            // сохраняем значение
            debounce_state = val;
            debounce_pause = false;
        }

        // возвращаем ТЕКУЩЕЕ, А НЕ НОВОЕ значение
        return debounce_state;
    }
    // -----------------------------------------------------------------------------
    bool IOBase::check_on_delay( bool val )
    {
        // нет задержки на включение
        if( ptOnDelay.getInterval() <= 0 )
        {
            ondelay_state = val;
            return val;
        }

        if( trOnDelay.hi(val) )
            ptOnDelay.reset();

        // обновляем значение только если наступило время
        // или если оно "0"...
        if( !val || ptOnDelay.checkTime() )
            ondelay_state = val;

        // возвращаем ТЕКУЩЕЕ, А НЕ НОВОЕ значение
        return ondelay_state;
    }
    // -----------------------------------------------------------------------------
    bool IOBase::check_off_delay( bool val )
    {
        if( ptOffDelay.getInterval() <= 0 )
        {
            offdelay_state = val;
            return val;
        }

        if( trOffDelay.low(val) )
            ptOffDelay.reset();

        // обновляем значение только если наступило время
        // или если оно "1"...
        if( val || ptOffDelay.checkTime() )
            offdelay_state = val;

        // возвращаем ТЕКУЩЕЕ, А НЕ НОВОЕ значение
        return offdelay_state;
    }
    // -----------------------------------------------------------------------------
    bool IOBase::check_front( bool val )
    {
        if( !front || front_type == ftUnknown )
            return val;

        if( front_type == ft01 )
        {
            if( val && !front_prev_state )
                front_state ^= true;
        }
        else if( front_type == ft10 )
        {
            if( !val && front_prev_state )
                front_state ^= true;
        }

        front_prev_state = val;
        return front_state;
    }
    // -----------------------------------------------------------------------------
    void IOBase::processingAsAI( IOBase* it, long val, const std::shared_ptr<SMInterface>& shm, bool force )
    {
        if( it->stype == uniset3::DI || it->stype == uniset3::DO )
        {
            val = (val ? 1 : 0);
        }
        else
        {
            // проверка зависимости
            if( !it->check_depend(shm) )
                val = it->d_off_value;
            else
            {
                if( !it->nofilter && it->df.size() > 1 )
                {
                    if( it->f_median )
                        val = it->df.median(val);
                    else if( it->f_filter_iir )
                        val = it->df.filterIIR(val);
                    else if( it->f_ls )
                        val = it->df.leastsqr(val);
                    else
                        val = it->df.filterRC(val);
                }

                if( !it->rawdata )
                {
                    if( it->cdiagram )    // задана специальная калибровочная диаграмма
                    {
                        if( it->craw != val )
                        {
                            it->craw = val;
                            val = it->cdiagram->getValue(val, it->calcrop);
                            it->cprev = val;
                        }
                        else
                            val = it->cprev;        // просто передаём предыдущее значение
                    }
                    else
                    {
                        uniset3::CalibrateInfo* cal( &(it->cal) );

                        if( cal->maxraw() != cal->minraw() ) // задана обычная калибровка
                            val = uniset3::lcalibrate(val, cal->minraw(), cal->maxraw(), cal->mincal(), cal->maxcal(), it->calcrop);
                    }

                    if( !it->noprecision && it->cal.precision() != 0 )
                        val = lround( val * pow(10.0, it->cal.precision()) );
                }
            } // end of 'check_depend'

        }

        {
            uniset_rwmutex_wrlock lock(it->val_lock);

            if( force || it->value != val )
            {
                shm->localSetValue( it->ioit, it->si.id(), val, shm->ID() );
                it->value = val;
            }
        }
    }
    // -----------------------------------------------------------------------------
    void IOBase::processingFasAI(IOBase* it, float fval, const std::shared_ptr<SMInterface>& shm, bool force )
    {
        long val = lroundf(fval);

        if( it->stype == uniset3::DI || it->stype == uniset3::DO )
            val = (fval != 0 ? 1 : 0);
        else
        {
            if( it->rawdata )
            {
                val = 0;
                memcpy(&val, &fval, std::min(sizeof(val), sizeof(fval)));
            }
            else if( it->cal.precision() != 0 && !it->noprecision )
                val = lroundf( fval * pow(10.0, it->cal.precision()) );

            // проверка зависимости
            if( !it->check_depend(shm) )
                val = it->d_off_value;
            else
            {
                // Читаем с использованием фильтра...
                if( !it->nofilter )
                {
                    if( it->df.size() > 1 )
                        it->df.add(val);

                    val = it->df.filterRC(val);
                }

                if( !it->rawdata )
                {
                    uniset3::CalibrateInfo* cal( &(it->cal) );

                    if( cal->maxraw() != cal->minraw() ) // задана обычная калибровка
                        val = uniset3::lcalibrate(val, cal->minraw(), cal->maxraw(), cal->mincal(), cal->maxcal(), it->calcrop);
                }
            }
        }

        {
            uniset_rwmutex_wrlock lock(it->val_lock);

            if( force || it->value != val )
            {
                shm->localSetValue( it->ioit, it->si.id(), val, shm->ID() );
                it->value = val;
            }
        }
    }
    // -----------------------------------------------------------------------------
    void IOBase::processingF64asAI(IOBase* it, double fval, const std::shared_ptr<SMInterface>& shm, bool force )
    {
        long val = lroundf(fval);

        if( it->stype == uniset3::DI || it->stype == uniset3::DO )
            val = (fval != 0 ? 1 : 0);
        else
        {
            if( it->rawdata )
            {
                val = 0;
                memcpy(&val, &fval, std::min(sizeof(val), sizeof(fval)));
            }
            else if( it->cal.precision() != 0 && !it->noprecision )
                val = lroundf( fval * pow(10.0, it->cal.precision()) );

            // проверка зависимости
            if( !it->check_depend(shm) )
                val = it->d_off_value;
            else
            {
                // Читаем с использованием фильтра...
                if( !it->nofilter )
                {
                    if( it->df.size() > 1 )
                        it->df.add(val);

                    val = it->df.filterRC(val);
                }

                if( !it->rawdata )
                {
                    uniset3::CalibrateInfo* cal( &(it->cal) );

                    if( cal->maxraw() != cal->minraw() ) // задана обычная калибровка
                        val = uniset3::lcalibrate(val, cal->minraw(), cal->maxraw(), cal->mincal(), cal->maxcal(), it->calcrop);
                }
            }
        }

        {
            uniset_rwmutex_wrlock lock(it->val_lock);

            if( force || it->value != val )
            {
                shm->localSetValue( it->ioit, it->si.id(), val, shm->ID() );
                it->value = val;
            }
        }
    }
    // -----------------------------------------------------------------------------
    void IOBase::processingAsDI( IOBase* it, bool set, const std::shared_ptr<SMInterface>& shm, bool force )
    {
        // проверка зависимости
        if( !it->check_depend(shm) )
            set = (bool)it->d_off_value;
        else if( it->invert )
            set ^= true;

        // Проверяем именно в такой последовательности!
        set = it->check_debounce(set);       // фильтр дребезга

        if(set)
        {
            set = it->check_on_delay(set);  // фильтр на срабатывание
            set = it->check_off_delay(set); // фильтр на отпускание
        }
        else
        {
            set = it->check_off_delay(set); // фильтр на отпускание
            set = it->check_on_delay(set);  // фильтр на срабатывание
        }

        set = it->check_front(set);     // работа по фронту (проверять после debounce_xxx!)

        {
            uniset_rwmutex_wrlock lock(it->val_lock);

            if( force || (bool)it->value != set )
            {
                shm->localSetValue( it->ioit, it->si.id(), (set ? 1 : 0), shm->ID() );
                it->value = set ? 1 : 0;
            }
        }
    }
    // -----------------------------------------------------------------------------
    long IOBase::processingAsAO( IOBase* it, const std::shared_ptr<SMInterface>& shm, bool force )
    {
        // проверка зависимости
        if( !it->check_depend(shm) )
            return it->d_off_value;

        uniset_rwmutex_rlock lock(it->val_lock);
        long val = it->value;

        if( force )
        {
            val = shm->localGetValue(it->ioit, it->si.id());
            it->value = val;
        }

        if( it->rawdata )
            return val;

        if( it->stype == uniset3::AO ||
                it->stype == uniset3::AI )
        {
            if( it->cdiagram )    // задана специальная калибровочная диаграмма
            {
                if( it->cprev != it->value )
                {
                    it->cprev = it->value;
                    val = it->cdiagram->getRawValue(val, it->calcrop);
                    it->craw = val;
                }
                else
                    val = it->craw; // просто передаём предыдущее значение
            }
            else
            {
                // сперва "убираем степень", потом калибруем.. (это обратная последовательность для AsAI)
                if( !it->noprecision && it->cal.precision() != 0 )
                    val = lroundf( (float)it->value / pow(10.0, it->cal.precision()) );

                uniset3::CalibrateInfo* cal = &(it->cal);

                if( cal->maxraw() != cal->minraw() ) // задана калибровка
                {
                    // Калибруем в обратную сторону!!!
                    val = uniset3::lcalibrate(val, cal->mincal(), cal->maxcal(), cal->minraw(), cal->maxraw(), it->calcrop );
                }
            }
        }

        return val;
    }
    // -----------------------------------------------------------------------------
    bool IOBase::processingAsDO( IOBase* it, const std::shared_ptr<SMInterface>& shm, bool force )
    {
        // проверка зависимости
        if( !it->check_depend(shm) )
            return (bool)it->d_off_value;

        uniset_rwmutex_rlock lock(it->val_lock);
        bool set = it->value;

        if( force )
            set = shm->localGetValue(it->ioit, it->si.id()) ? true : false;

        set = it->invert ? !set : set;
        return set;
    }
    // -----------------------------------------------------------------------------
    float IOBase::processingFasAO( IOBase* it, const std::shared_ptr<SMInterface>& shm, bool force )
    {
        // проверка зависимости
        if( !it->check_depend(shm) )
            return (float)it->d_off_value;

        uniset_rwmutex_rlock lock(it->val_lock);
        long val = it->value;

        if( force )
        {
            val = shm->localGetValue(it->ioit, it->si.id());
            it->value = val; // обновим на всякий
        }

        if( it->rawdata )
        {
            float fval = 0;
            memcpy(&fval, &val, std::min(sizeof(val), sizeof(fval)));
            return fval;
        }

        float fval = val;

        if( it->stype == uniset3::AO || it->stype == uniset3::AI )
        {
            uniset3::CalibrateInfo* cal( &(it->cal) );

            if( cal->maxraw() != cal->minraw() ) // задана калибровка
            {
                // Калибруем в обратную сторону!!!
                fval = uniset3::fcalibrate(fval, cal->mincal(), cal->maxcal(), cal->minraw(), cal->maxraw(), it->calcrop );
            }

            if( !it->noprecision && it->cal.precision() != 0 )
                return ( fval / pow(10.0, it->cal.precision()) );
        }
        else // if( it->stype == uniset3::DI || it->stype == uniset3::DO )
            fval = val ? 1.0 : 0.0;

        return fval;
    }
    // -----------------------------------------------------------------------------
    double IOBase::processingF64asAO( IOBase* it, const std::shared_ptr<SMInterface>& shm, bool force )
    {
        // проверка зависимости
        if( !it->check_depend(shm) )
            return (double)it->d_off_value;

        uniset_rwmutex_rlock lock(it->val_lock);
        long val = it->value;

        if( force )
        {
            val = shm->localGetValue(it->ioit, it->si.id());
            it->value = val; // обновим на всякий
        }

        if( it->rawdata )
        {
            double fval = 0;
            memcpy(&fval, &val, std::min(sizeof(val), sizeof(fval)));
            return fval;
        }

        double dval = val;

        if( it->stype == uniset3::AO || it->stype == uniset3::AI )
        {
            uniset3::CalibrateInfo* cal( &(it->cal) );

            if( cal->maxraw() != cal->minraw() ) // задана калибровка
            {
                // Калибруем в обратную сторону!!!
                dval = uniset3::dcalibrate(dval, cal->mincal(), cal->maxcal(), cal->minraw(), cal->maxraw(), it->calcrop );
            }

            if( !it->noprecision && it->cal.precision() != 0 )
                return ( dval / pow(10.0, it->cal.precision()) );
        }
        else // if( it->stype == uniset3::DI || it->stype == uniset3::DO )
            dval = val ? 1.0 : 0.0;

        return dval;
    }
    // -----------------------------------------------------------------------------
    void IOBase::processingThreshold( IOBase* it, const std::shared_ptr<SMInterface>& shm, bool force )
    {
        if( it->t_ai == DefaultObjectId )
            return;

        long val = shm->localGetValue(it->t_ait, it->t_ai);
        bool set = it->value ? true : false;

        //    cout  << "val=" << val << " set=" << set << endl;
        // Проверка нижнего предела
        // значение должно быть меньше lowLimit-чуствительность
        if (it->t_invert)
        {
            if( val <= it->t_lowlimit )
                set = true;
            else if( val >= it->t_hilimit )
                set = false;
        }
        else
        {
            if( val <= it->t_lowlimit )
                set = false;
            else if( val >= it->t_hilimit )
                set = true;
        }

        //    cout  << "thresh: set=" << set << endl;
        processingAsDI(it, set, shm, force);
    }
    // -----------------------------------------------------------------------------
    std::string IOBase::initProp( UniXML::iterator& it, const std::string& prop, const std::string& prefix, bool prefonly, const std::string& defval )
    {
        if( !it.getProp(prefix + prop).empty() )
            return it.getProp(prefix + prop);

        if( prefonly )
            return defval;

        if( !it.getProp(prop).empty() )
            return it.getProp(prop);

        return defval;
    }
    // -----------------------------------------------------------------------------
    int IOBase::initIntProp( UniXML::iterator& it, const std::string& prop, const std::string& prefix, bool prefonly, const int defval )
    {
        string pp(prefix + prop);

        if( !it.getProp(pp).empty() )
            return it.getIntProp(pp);

        if( prefonly )
            return defval;

        if( !it.getProp(prop).empty() )
            return it.getIntProp(prop);

        return defval;
    }
    // -----------------------------------------------------------------------------
    timeout_t IOBase::initTimeoutProp( UniXML::iterator& it, const std::string& prop, const std::string& prefix, bool prefonly, const timeout_t defval )
    {
        string pp(prefix + prop);

        if( !it.getProp(pp).empty() )
            return it.getIntProp(pp);

        if( prefonly )
            return defval;

        if( !it.getProp(prop).empty() )
            return it.getIntProp(prop);

        return defval;
    }
    // -----------------------------------------------------------------------------
    bool IOBase::initItem( IOBase* b, UniXML::iterator& it, const std::shared_ptr<SMInterface>& shm, const std::string& prefix,
                           bool init_prefix_only,
                           std::shared_ptr<DebugStream> dlog, std::string myname,
                           int def_filtersize, float def_filterT, float def_lsparam,
                           float def_iir_coeff_prev, float def_iir_coeff_new )
    {
        auto conf = uniset_conf();
        // Переопределять ID и name - нельзя..
        string sname( it.getProp("name") );

        ObjectId sid = DefaultObjectId;

        if( it.getProp("id").empty() )
            sid = conf->getSensorID(sname);
        else
            sid = it.getPIntProp("id", DefaultObjectId);

        if( sid == DefaultObjectId )
        {
            if( dlog && dlog->is_crit() )
                dlog->crit() << myname << "(readItem): (" << DefaultObjectId << ") Unknown Sensor ID for "
                             << sname << endl;

            return false;
        }

        b->val_lock.setName(sname + "_lock");

        b->si.set_id(sid);
        b->si.set_node(conf->getLocalNode());

        b->nofilter = initIntProp(it, "nofilter", prefix, init_prefix_only);
        b->ignore   = initIntProp(it, "ioignore", prefix, init_prefix_only);
        b->invert   = initIntProp(it, "ioinvert", prefix, init_prefix_only);
        b->defval   = initIntProp(it, "default", prefix, init_prefix_only);
        b->noprecision    = initIntProp(it, "noprecision", prefix, init_prefix_only);
        b->value    = b->defval;
        b->rawdata  = initIntProp(it, "rawdata", prefix, init_prefix_only);

        timeout_t d_msec = initTimeoutProp(it, "debouncedelay", prefix, init_prefix_only, UniSetTimer::WaitUpTime);
        b->ptDebounce.setTiming(d_msec);

        timeout_t d_on_msec = initTimeoutProp(it, "ondelay", prefix, init_prefix_only, UniSetTimer::WaitUpTime);
        b->ptOnDelay.setTiming(d_on_msec);

        timeout_t d_off_msec = initTimeoutProp(it, "offdelay", prefix, init_prefix_only, UniSetTimer::WaitUpTime);
        b->ptOffDelay.setTiming(d_off_msec);

        if( dlog && d_msec != UniSetTimer::WaitUpTime
                && d_on_msec != UniSetTimer::WaitUpTime
                && d_off_msec != UniSetTimer::WaitUpTime )
        {
            dlog->warn() << myname << "(IOBase::readItem): "
                         << " 'debouncedelay' is used in conjunction with the 'ondelay' and 'offdelay'. Sure?"
                         << " [ debouncedelay=" << d_msec
                         << " ondelay=" << d_on_msec
                         << " offdelay=" << d_off_msec
                         << " ]"    << endl;
        }


        b->front = false;
        std::string front_t( initProp(it, "iofront", prefix, init_prefix_only) );

        if( !front_t.empty() )
        {
            if( front_t == "01" )
            {
                b->front = true;
                b->front_type = ft01;
            }
            else if( front_t == "10" )
            {
                b->front = true;
                b->front_type = ft10;
            }
            else
            {
                if( dlog && dlog->is_crit() )
                    dlog->crit() << myname << "(IOBase::readItem): Unknown iofront='" << front_t << "'"
                                 << " for '" << sname << "'.  Must be [ 01, 10 ]." << endl;

                return false;
            }
        }

        std::string ssafe = initProp(it, "safeval", prefix, init_prefix_only);

        b->safevalDefined = !ssafe.empty();

        if( b->safevalDefined )
            b->safeval = uni_atoi(ssafe);

        b->stype = uniset3::getIOType(initProp(it, "iotype", prefix, init_prefix_only));

        if( b->stype == uniset3::UnknownIOType )
        {
            if( dlog && dlog->is_crit() )
                dlog->crit() << myname << "(IOBase::readItem): Unknown iotype=: "
                             << initProp(it, "iotype", prefix, init_prefix_only) << " for " << sname << endl;

            return false;
        }

        string d_txt( initProp(it, "depend", prefix, init_prefix_only) );

        if( !d_txt.empty() )
        {
            b->d_id = conf->getSensorID(d_txt);

            if( b->d_id == DefaultObjectId )
            {
                if( dlog && dlog->is_crit() )
                    dlog->crit() << myname << "(IOBase::readItem): sensor='"
                                 << it.getProp("name") << "' err: "
                                 << " Unknown SensorID for depend='"  << d_txt
                                 << endl;

                return false;
            }

            // по умолчанию срабатывание на "1"
            b->d_value = initProp(it, "depend_value", prefix, init_prefix_only).empty() ? 1 : initIntProp(it, "depend_value", prefix, init_prefix_only);
            b->d_off_value = initIntProp(it, "depend_off_value", prefix, init_prefix_only);
            b->d_iotype = conf->getIOType(b->d_id);
            shm->initIterator(b->d_it);
        }

        b->cal.set_minraw(0);;
        b->cal.set_maxraw(0);
        b->cal.set_mincal(0);
        b->cal.set_maxcal(0);
        b->cal.set_precision(0);
        b->cdiagram = nullptr;
        b->f_median = false;
        b->f_ls = false;
        b->f_filter_iir = false;

        shm->initIterator(b->ioit);

        if( b->stype == uniset3::AI || b->stype == uniset3::AO )
        {
            b->cal.set_minraw(initIntProp(it, "rmin", prefix, init_prefix_only));
            b->cal.set_maxraw(initIntProp(it, "rmax", prefix, init_prefix_only));
            b->cal.set_mincal(initIntProp(it, "cmin", prefix, init_prefix_only));
            b->cal.set_maxcal(initIntProp(it, "cmax", prefix, init_prefix_only));
            b->cal.set_precision(initIntProp(it, "precision", prefix, init_prefix_only));
            b->calcrop = initIntProp(it, "cal_nocrop", prefix, init_prefix_only) ? false : true;

            int f_size     = def_filtersize;
            float f_T     = def_filterT;
            float f_lsparam = def_lsparam;
            int f_median = initIntProp(it, "filtermedian", prefix, init_prefix_only);
            int f_iir = initIntProp(it, "iir_thr", prefix, init_prefix_only);
            float f_iir_coeff_prev = def_iir_coeff_prev;
            float f_iir_coeff_new = def_iir_coeff_new;

            if( f_median > 0 )
            {
                f_size = f_median;
                b->f_median = true;
            }
            else
            {
                if( f_iir > 0 )
                    b->f_filter_iir = true;

                if( !initProp(it, "filtersize", prefix, init_prefix_only).empty() )
                    f_size = initIntProp(it, "filtersize", prefix, init_prefix_only, def_filtersize);
            }

            if( !initProp(it, "filterT", prefix, init_prefix_only).empty() )
            {
                f_T = atof(initProp(it, "filterT", prefix, init_prefix_only).c_str());

                if( f_T < 0 )
                    f_T = 0.0;
            }

            if( !initProp(it, "leastsqr", prefix, init_prefix_only).empty() )
            {
                b->f_ls = true;
                f_lsparam = atof(initProp(it, "leastsqr", prefix, init_prefix_only).c_str());

                if( f_lsparam < 0 )
                    f_lsparam = def_lsparam;
            }

            if( !initProp(it, "iir_coeff_prev", prefix, init_prefix_only).empty() )
                f_iir_coeff_prev = atof(initProp(it, "iir_coeff_prev", prefix, init_prefix_only).c_str());

            if( !initProp(it, "iir_coeff_new", prefix, init_prefix_only).empty() )
                f_iir_coeff_new = atof(initProp(it, "iir_coeff_new", prefix, init_prefix_only).c_str());

            if( b->stype == uniset3::AI )
                b->df.setSettings( f_size, f_T, f_lsparam, f_iir,
                                   f_iir_coeff_prev, f_iir_coeff_new );

            b->df.init(b->defval);

            std::string caldiagram( initProp(it, "caldiagram", prefix, init_prefix_only) );

            if( !caldiagram.empty() )
            {
                b->cdiagram = uniset3::extensions::buildCalibrationDiagram(caldiagram);

                if( !initProp(it, "cal_cachesize", prefix, init_prefix_only).empty() )
                    b->cdiagram->setCacheSize(initIntProp(it, "cal_cachesize", prefix, init_prefix_only));

                if( !initProp(it, "cal_cacheresort", prefix, init_prefix_only).empty() )
                    b->cdiagram->setCacheResortCycle(initIntProp(it, "cal_cacheresort", prefix, init_prefix_only));
            }
        }
        else if( b->stype == uniset3::DI || b->stype == uniset3::DO ) // -V560
        {
            string tai(initProp(it, "threshold_aid", prefix, init_prefix_only));

            if( !tai.empty() )
            {
                b->t_ai = conf->getSensorID(tai);

                if( b->t_ai == DefaultObjectId )
                {
                    if( dlog && dlog->is_crit() )
                        dlog->crit() << myname << "(IOBase::readItem): unknown ID for threshold_ai "
                                     << tai << endl;

                    return false;
                }

                b->t_lowlimit = initIntProp(it, "lowlimit", prefix, init_prefix_only);
                b->t_hilimit = initIntProp(it, "hilimit", prefix, init_prefix_only);
                b->t_invert = initIntProp(it, "threshold_invert", prefix, init_prefix_only);
                shm->initIterator(b->t_ait);
            }
        }

        return true;
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, const IOBase::FrontType& f )
    {
        switch(f)
        {
            case IOBase::ft01:
                os << "(ft01)[0-->1]";
                break;

            case IOBase::ft10:
                os << "(ft10)[1-->0]";
                break;

            case IOBase::ftUnknown:
            default:
                os << "ftUnknown";
                break;

        }

        return os;
    }
    // -----------------------------------------------------------------------------
    IOBase IOBase::make_iobase_copy()
    {
        IOBase b;
        b.si = si;
        b.cal = cal;
        b.stype = stype;
        b.cdiagram = cdiagram;
        b.value = value;
        b.craw = craw;
        b.cprev = cprev;
        b.safeval = safeval;
        b.safevalDefined = safevalDefined;
        b.defval = defval;
        b.df = df;
        b.nofilter = nofilter;
        b.f_median = f_median;
        b.f_ls = f_ls;
        b.f_filter_iir = f_filter_iir;
        b.ignore = ignore;
        b.invert = invert;
        b.noprecision = noprecision;
        b.calcrop = calcrop;
        b.d_id = d_id;
        b.d_value = d_value;
        b.d_off_value = d_off_value;
        b.d_iotype = d_iotype;
        b.t_ai = t_ai;
        b.t_invert = t_invert;
        b.t_hilimit = t_hilimit;
        b.t_lowlimit = t_lowlimit;
        b.front_type = front_type;
        b.front_prev_state = front_prev_state;
        b.front_state = front_state;
        b.rawdata = rawdata;

        b.debounce_pause = debounce_pause;
        b.debounce_state = debounce_state;
        b.ondelay_state = ondelay_state;
        b.offdelay_state = offdelay_state;

        return b;
    }
    // ------------------------------------------------------------------------------------------
    void IOBase::create_from_iobase( const IOBase& b )
    {
        si = b.si;
        cal = b.cal;
        stype = b.stype;
        cdiagram = b.cdiagram;
        value = b.value;
        craw = b.craw;
        cprev = b.cprev;
        safeval = b.safeval;
        safevalDefined = b.safevalDefined;
        defval = b.defval;
        df = b.df;
        nofilter = b.nofilter;
        f_median = b.f_median;
        f_ls = b.f_ls;
        f_filter_iir = b.f_filter_iir;
        ignore = b.ignore;
        invert = b.invert;
        noprecision = b.noprecision;
        calcrop = b.calcrop;
        d_id = b.d_id;
        d_value = b.d_value;
        d_off_value = b.d_off_value;
        d_iotype = b.d_iotype;
        t_ai = b.t_ai;
        t_invert = b.t_invert;
        t_hilimit = b.t_hilimit;
        t_lowlimit = b.t_lowlimit;
        front_type = b.front_type;
        front_prev_state = b.front_prev_state;
        front_state = b.front_state;
        rawdata = b.rawdata;

        debounce_pause = b.debounce_pause;
        debounce_state = b.debounce_state;
        ondelay_state = b.ondelay_state;
        offdelay_state = b.offdelay_state;
    }
    // ------------------------------------------------------------------------------------------
} //  end of namespace uniset3
