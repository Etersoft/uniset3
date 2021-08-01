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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------

#include <sstream>
#include <time.h>
#include <unistd.h>
#include <iomanip>

#include "UInterface.h"
#include "IONotifyController.h"
#include "Debug.h"
#include "IOConfig.h"
#include "UHelpers.h"

// ------------------------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::umessage;
// ------------------------------------------------------------------------------------------
IONotifyController::IONotifyController():
    askIOMutex("askIOMutex"),
    trshMutex("trshMutex"),
    maxAttemtps(uniset_conf()->getPIntField("ConsumerMaxAttempts", 10)),
    sendAttemtps(uniset_conf()->getPIntField("ConsumerSendAttempts", 3))
{
    ui->setCacheMaxSize(uniset_conf()->getPIntField("ConsumerMaxCache", 5000));
}

IONotifyController::IONotifyController( ObjectId id, std::shared_ptr<IOConfig> d ):
    IOController(id),
    restorer(d),
    askIOMutex(string(uniset_conf()->oind->getMapName(id)) + "_askIOMutex"),
    trshMutex(string(uniset_conf()->oind->getMapName(id)) + "_trshMutex"),
    maxAttemtps(uniset_conf()->getPIntField("ConsumerMaxAttempts", 10)),
    sendAttemtps(uniset_conf()->getPIntField("ConsumerSendAttempts", 3))
{
    conUndef = signal_change_undefined_state().connect(sigc::mem_fun(*this, &IONotifyController::onChangeUndefinedState));
    conInit = signal_init().connect(sigc::mem_fun(*this, &IONotifyController::initItem));
    ui->setCacheMaxSize(uniset_conf()->getPIntField("ConsumerMaxCache", 5000));
}

IONotifyController::~IONotifyController()
{
    conUndef.disconnect();
    conInit.disconnect();
}
// ------------------------------------------------------------------------------------------
void IONotifyController::showStatisticsForConsumer( ostringstream& inf, const std::string& consumer )
{
    ObjectId consumer_id = uniset_conf()->getObjectID(consumer);

    if( consumer_id == DefaultObjectId )
        consumer_id = uniset_conf()->getControllerID(consumer);

    if( consumer_id == DefaultObjectId )
        consumer_id = uniset_conf()->getServiceID(consumer);

    if( consumer_id == DefaultObjectId )
    {
        inf << "not found consumer '" << consumer << "'" << endl;
        return;
    }

    // Формируем статистику по каждому датчику..
    struct StatInfo
    {
        StatInfo( ObjectId id, const ConsumerInfoExt& c ): inf(c), sid(id) {}

        const ConsumerInfoExt inf;
        ObjectId sid;
    };

    std::list<StatInfo> stat;

    // общее количество SensorMessage полученное этим заказчиком
    size_t smCount = 0;

    {
        // lock askIOMutex

        // выводим информацию по конкретному объекту
        uniset_rwmutex_rlock lock(askIOMutex);

        for( auto&& a : askIOList )
        {
            auto& i = a.second;

            uniset_rwmutex_rlock lock(i.mut);

            if( i.clst.empty() )
                continue;

            // ищем среди заказчиков
            for( const auto& c : i.clst )
            {
                if( c.ci.id() == consumer_id )
                {
                    stat.emplace_back(a.first, c);
                    smCount += c.smCount;
                    break;
                }
            }
        }
    } // unlock askIOMutex

    {
        // выводим информацию по конкретному объекту
        uniset_rwmutex_rlock lock(trshMutex);

        for( auto&& a : askTMap )
        {
            uniset_rwmutex_rlock lock2(a.second.mut);

            for( const auto& c : a.second.clst )
            {
                if( c.ci.id() == consumer_id )
                {
                    if( a.first->sid != DefaultObjectId )
                        stat.emplace_back(a.first->sid, c);
                    else
                        stat.emplace_back(DefaultObjectId, c);

                    smCount += c.smCount;
                    break;
                }
            }
        }
    }

    // печатаем отчёт
    inf << "Statisctic for consumer '" << consumer << "'(smCount=" << smCount << "):"
        << endl
        << "--------------------------------------------"
        << endl;

    if( stat.empty() )
    {
        inf << "NOT FOUND STATISTIC FOR '" << consumer << "'" << endl;
    }
    else
    {
        std::ios_base::fmtflags old_flags = inf.flags();

        inf << std::right;

        auto oind = uniset_conf()->oind;

        for( const auto& s : stat )
        {
            inf << "        " << "(" << setw(6) << s.sid << ") "
                << setw(35) << ObjectIndex::getShortName(oind->getMapName(s.sid))
                << " ["
                << " lostEvents: " << setw(3) << s.inf.lostEvents
                << " attempt: " << setw(3) << s.inf.attempt
                << " smCount: " << setw(5) << s.inf.smCount
                << " ]"
                << endl;
        }

        inf.setf(old_flags);
    }

    inf << "--------------------------------------------" << endl;

}
// ------------------------------------------------------------------------------------------
void IONotifyController::showStatisticsForLostConsumers( ostringstream& inf )
{
    std::lock_guard<std::mutex> lock(lostConsumersMutex);

    if( lostConsumers.empty() )
    {
        inf << "..empty lost consumers list..." << endl;
        return;
    }

    inf << "Statistics 'consumers with lost events':"
        << endl
        << "----------------------------------------"
        << endl;

    auto oind = uniset_conf()->oind;

    for( const auto& l : lostConsumers )
    {
        inf << "        " << "(" << setw(6) << l.first << ") "
            << setw(35) << std::left << ObjectIndex::getShortName(oind->getMapName(l.first))
            << " lostCount=" << l.second.count
            << endl;
    }
}
// ------------------------------------------------------------------------------------------
void IONotifyController::showStatisticsForConsusmers( ostringstream& inf )
{
    uniset_rwmutex_rlock lock(askIOMutex);

    auto oind = uniset_conf()->oind;

    for( auto&& a : askIOList )
    {
        auto& i = a.second;

        uniset_rwmutex_rlock lock(i.mut);

        // отображаем только датчики с "не пустым" списком заказчиков
        if( i.clst.empty() )
            continue;

        inf << "(" << setw(6) << a.first << ")[" << oind->getMapName(a.first) << "]" << endl;

        for( const auto& c : i.clst )
        {
            inf << "        " << "(" << setw(6) << c.ci.id() << ")"
                << setw(35) << ObjectIndex::getShortName(oind->getMapName(c.ci.id()))
                << " ["
                << " lostEvents=" << c.lostEvents
                << " attempt=" << c.attempt
                << " smCount=" << c.smCount
                << "]"
                << endl;
        }
    }
}
// ------------------------------------------------------------------------------------------
void IONotifyController::showStatisticsForConsumersWithLostEvent( ostringstream& inf )
{
    uniset_rwmutex_rlock lock(askIOMutex);

    auto oind = uniset_conf()->oind;
    bool empty = true;

    for( auto&& a : askIOList )
    {
        auto& i = a.second;

        uniset_rwmutex_rlock lock(i.mut);

        // отображаем только датчики с "не пустым" списком заказчиков
        if( i.clst.empty() )
            continue;

        // Т.к. сперва выводится имя датчика, а только потом его заказчики
        // то если надо выводить только тех, у кого есть "потери"(lostEvent>0)
        // для предварительно смотрим список есть ли там хоть один с "потерями", а потом уже выводим

        bool lost = false;

        for( const auto& c : i.clst )
        {
            if( c.lostEvents > 0 )
            {
                lost = true;
                break;
            }
        }

        if( !lost )
            continue;

        empty = false;
        // выводим тех у кого lostEvent>0
        inf << "(" << setw(6) << a.first << ")[" << oind->getMapName(a.first) << "]" << endl;
        inf << "--------------------------------------------------------------------" << endl;

        for( const auto& c : i.clst )
        {
            if( c.lostEvents > 0 )
            {
                inf << "        " << "(" << setw(6) << c.ci.id() << ")"
                    << setw(35) << ObjectIndex::getShortName(oind->getMapName(c.ci.id()))
                    << " ["
                    << " lostEvents=" << c.lostEvents
                    << " attempt=" << c.attempt
                    << " smCount=" << c.smCount
                    << "]"
                    << endl;
            }
        }
    }

    if( empty )
        inf << "...not found consumers with lost event..." << endl;
    else
        inf << "--------------------------------------------------------------------" << endl;
}
// ------------------------------------------------------------------------------------------
void IONotifyController::showStatisticsForSensor( ostringstream& inf, const string& name )
{
    auto conf = uniset_conf();
    auto oind = conf->oind;

    ObjectId sid = conf->getSensorID(name);

    if( sid == DefaultObjectId )
    {
        inf << "..not found ID for sensor '" << name << "'" << endl;
        return;
    }

    ConsumerListInfo* clist = nullptr;

    {
        uniset_rwmutex_rlock lock(askIOMutex);
        auto s = askIOList.find(sid);

        if( s == askIOList.end() )
        {
            inf << "..not found consumers for sensor '" << name << "'" << endl;
            return;
        }

        clist = &(s->second);
    }

    inf << "Statisctics for sensor "
        << "(" << setw(6) << sid << ")[" << name << "]: " << endl
        << "--------------------------------------------------------------------" << endl;

    uniset_rwmutex_rlock lock2(clist->mut);

    for( const auto& c : clist->clst )
    {
        inf << "        (" << setw(6) << c.ci.id() << ")"
            << setw(35) << ObjectIndex::getShortName(oind->getMapName(c.ci.id()))
            << " ["
            << " lostEvents=" << c.lostEvents
            << " attempt=" << c.attempt
            << " smCount=" << c.smCount
            << "]"
            << endl;
    }

    inf << "--------------------------------------------------------------------" << endl;
}
// ------------------------------------------------------------------------------------------
grpc::Status IONotifyController::getType(::grpc::ServerContext* context, const ::uniset3::GetTypeParams* request, ::google::protobuf::StringValue* response)
{
    if( getId() != request->id() )
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

    response->set_value("IONotifyController");
    return ::grpc::Status::OK;
}
// ------------------------------------------------------------------------------------------
grpc::Status IONotifyController::getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request, ::google::protobuf::StringValue* response)
{
    if( request->id() != getId() )
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

    ::google::protobuf::StringValue oinf;
    grpc::Status st = UniSetManager::getInfo(context, request, &oinf);

    if( !st.ok() )
        return st;

    auto& param = request->params();

    ostringstream inf;

    inf << oinf.value() << endl;

    auto oind = uniset_conf()->oind;

    if( param.empty() )
    {
        inf << "-------------------------- lost consumers list [maxAttemtps=" << maxAttemtps << "] ------------------" << endl;
        showStatisticsForLostConsumers(inf);
        inf << "----------------------------------------------------------------------------------" << endl;
    }
    else if( param == "consumers" )
    {
        inf << "------------------------------- consumers list ------------------------------" << endl;
        showStatisticsForConsusmers(inf);
        inf << "-----------------------------------------------------------------------------" << endl << endl;
    }
    else if( param == "lost" )
    {
        inf << "------------------------------- consumers list (lost event)------------------" << endl;
        showStatisticsForConsumersWithLostEvent(inf);
        inf << "-----------------------------------------------------------------------------" << endl << endl;
    }
    else if( !param.empty() )
    {
        auto query = uniset3::explode_str(param, ':');

        if( query.empty() || query.size() == 1 )
            showStatisticsForConsumer(inf, param);
        else if( query.size() > 1 )
        {
            if( query[0] == "consumer" )
                showStatisticsForConsumer(inf, query[1]);
            else if( query[0] == "sensor" )
                showStatisticsForSensor(inf, query[1]);
            else
                inf << "Unknown command: " << param << endl;
        }
    }
    else
    {
        inf << "IONotifyController::UserParam help: " << endl
            << "  Default         - Common info" << endl
            << "  consumers       - Consumers list " << endl
            << "  lost            - Consumers list with lostEvent > 0" << endl
            << "  consumer:name   - Statistic for consumer 'name'" << endl
            << "  sensor:name     - Statistic for sensor 'name'"
            << endl;
    }

    response->set_value(inf.str());
    return grpc::Status::OK;
}

// ------------------------------------------------------------------------------------------
/*!
 *    \param lst - список в который необходимо внести потребителя
 *    \param name - имя вносимого потребителя
 *    \note Добавление произойдёт только если такого потребителя не существует в списке
*/
bool IONotifyController::addConsumer( ConsumerListInfo& lst, const ConsumerInfo& ci )
{
    uniset_rwmutex_wrlock l(lst.mut);

    for( auto&& it :  lst.clst )
    {
        if( it.ci.id() == ci.id() && it.ci.node() == ci.node() )
        {
            // при перезаказе датчиков количество неудачных попыток послать сообщение
            // считаем что "заказчик" опять на связи
            it.attempt = maxAttemtps;

            // выставляем флаг, что заказчик опять "на связи"
            std::lock_guard<std::mutex> lock(lostConsumersMutex);
            auto c = lostConsumers.find(ci.id());

            if( c != lostConsumers.end() )
                c->second.lost = false;

            return false;
        }
    }

    ConsumerInfoExt cinf(ci, maxAttemtps);

    lst.clst.emplace_front( std::move(cinf) );

    // выставляем флаг, что клиент опять "на связи"
    std::lock_guard<std::mutex> lock(lostConsumersMutex);
    auto c = lostConsumers.find(ci.id());

    if( c != lostConsumers.end() )
        c->second.lost = false;

    return true;
}
// ------------------------------------------------------------------------------------------
/*!
 *    \param lst - указатель на список из которого происходит удаление потребителя
 *    \param name - имя удаляемого потребителя
*/
bool IONotifyController::removeConsumer( ConsumerListInfo& lst, const ConsumerInfo& ci )
{
    uniset_rwmutex_wrlock l(lst.mut);

    for( auto li = lst.clst.begin(); li != lst.clst.end(); ++li )
    {
        if( li->ci.id() == ci.id() && li->ci.node() == ci.node()  )
        {
            lst.clst.erase(li);
            return true;
        }
    }

    return false;
}
// ------------------------------------------------------------------------------------------
grpc::Status IONotifyController::askSensor(::grpc::ServerContext* context, const ::uniset3::AskParams* request, ::google::protobuf::Empty* response)
{
    ulog2 << "(askSensor): поступил " << ( request->cmd() == UIODontNotify ? "отказ" : "заказ" ) << " от "
          << "(" << request->ci().id() << ")"
          << uniset_conf()->oind->getNameById(request->ci().id()) << "@" << request->ci().node()
          << " на датчик "
          << uniset_conf()->oind->getNameById(request->sid()) << endl;

    auto li = myioEnd();

    try
    {
        // если такого аналогового датчика нет, здесь сработает исключение...
        localGetValue(li, request->sid());
    }
    catch( IOController::Undefined& ex ) {}

    {
        uniset_rwmutex_wrlock lock(askIOMutex);
        ask(askIOList, request->sid(), request->ci(), request->cmd());
    }

    auto usi = li->second;

    // посылка первый раз состояния
    if( request->cmd() == uniset3::UIONotify || (request->cmd() == UIONotifyFirstNotNull && usi->sinf.value()) )
    {
        ConsumerListInfo* lst = static_cast<ConsumerListInfo*>(usi->getUserData(udataConsumerList));

        if( lst )
        {
            uniset3::uniset_rwmutex_rlock lock(usi->val_lock);
            umessage::SensorMessage smsg( usi->makeSensorMessage(false) );
            send(*lst, smsg, &request->ci());
        }
    }

    return grpc::Status::OK;
}
// ------------------------------------------------------------------------------------------
void IONotifyController::ask( AskMap& askLst, const uniset3::ObjectId sid,
                              const uniset3::ConsumerInfo& cons, uniset3::UIOCommand cmd)
{
    // поиск датчика в списке
    auto askIterator = askLst.find(sid);

    switch (cmd)
    {
        case uniset3::UIONotify: // заказ
        case uniset3::UIONotifyChange:
        case uniset3::UIONotifyFirstNotNull:
        {
            if( askIterator != askLst.end() )
                addConsumer(askIterator->second, cons);
            else
            {
                ConsumerListInfo newlst; // создаем новый список
                addConsumer(newlst, cons);
                askLst.emplace(sid, std::move(newlst));
            }

            break;
        }

        case uniset3::UIODontNotify:     // отказ
        {
            if( askIterator != askLst.end() )
                removeConsumer(askIterator->second, cons);

            break;
        }

        default:
            break;
    }

    if( askIterator == askLst.end() )
        askIterator = askLst.find(sid);

    if( askIterator != askLst.end() )
    {
        auto s = myiofind(sid);

        if( s != myioEnd() )
            s->second->setUserData(udataConsumerList, &(askIterator->second));
        else
            s->second->setUserData(udataConsumerList, nullptr);
    }
}
// ------------------------------------------------------------------------------------------
long IONotifyController::localSetValue( std::shared_ptr<IOController::USensorInfo>& usi,
                                        long value, uniset3::ObjectId sup_id )
{
    // оптимизация:
    // if( !usi ) - не проверяем, т.к. считаем что это внутренние функции и несуществующий указатель передать не могут

    long prevValue = value;
    {
        uniset_rwmutex_rlock lock(usi->val_lock);
        prevValue = usi->sinf.value();
    }

    long curValue = IOController::localSetValue(usi, value, sup_id);

    // Рассылаем уведомления только в случае изменения значения
    // --------
    if( prevValue == curValue )
        return curValue;

    {
        // с учётом того, что параллельно с этой функцией может
        // выполняться askSensor, то
        // посылать сообщение надо "заблокировав" доступ к value...

        uniset3::uniset_rwmutex_rlock lock(usi->val_lock);

        umessage::SensorMessage sm(usi->makeSensorMessage(false));

        try
        {
            if( !usi->sinf.dbignore() )
                logging(sm);
        }
        catch(...) {}

        ConsumerListInfo* lst = static_cast<ConsumerListInfo*>(usi->getUserData(udataConsumerList));

        if( lst )
            send(*lst, sm);
    } // unlock value

    // проверка порогов
    try
    {
        checkThreshold(usi, true);
    }
    catch(...) {}

    return curValue;
}
// -----------------------------------------------------------------------------------------
/*!
    \note В случае зависания в функции push, будут остановлены рассылки другим объектам.
    Возможно нужно ввести своего агента на удалённой стороне, который будет заниматься
    только приёмом сообщений и локальной рассылкой. Lav
*/
void IONotifyController::send( ConsumerListInfo& lst, const uniset3::umessage::SensorMessage& sm, const uniset3::ConsumerInfo* ci  )
{
    umessage::TransportMessage tmsg = to_transport<SensorMessage>(sm);

    uniset_rwmutex_wrlock l(lst.mut);

    for( ConsumerList::iterator li = lst.clst.begin(); li != lst.clst.end(); ++li )
    {
        if( ci )
        {
            if( ci->id() != li->ci.id() || ci->node() != li->ci.node() )
                continue;
        }

        for( int i = 0; i < sendAttemtps; i++ )
        {
            try
            {
                tmsg.mutable_header()->set_consumer(li->ci.id());
                ui->send(tmsg, li->ci.node());
                li->smCount++;
                li->attempt = maxAttemtps; // reinit attempts
                break;
            }
            catch( const std::exception& ex )
            {
                uwarn << myname << "(IONotifyController::send): attempt=" <<  (maxAttemtps - li->attempt + 1) << " "
                      << " from " << maxAttemtps << " "
                      << ex.what()
                      << " for " << uniset_conf()->oind->getNameById(li->ci.id()) << "@" << li->ci.node() << endl;
            }
            catch(...)
            {
                ucrit << myname << "(IONotifyController::send): attempt=" <<  (maxAttemtps - li->attempt + 1) << " "
                      << " from " << maxAttemtps << " "
                      << uniset_conf()->oind->getNameById(li->ci.id()) << "@" << li->ci.node()
                      << " catch..." << endl;
            }

            // фиксируем только после первой попытки послать
            if( i > 0 )
                li->lostEvents++;

            try
            {
                if( maxAttemtps > 0 && --(li->attempt) <= 0 )
                {
                    uwarn << myname << "(IONotifyController::send): ERASE FROM CONSUMERS:  "
                          << uniset_conf()->oind->getNameById(li->ci.id()) << "@" << li->ci.node() << endl;

                    {
                        std::lock_guard<std::mutex> lock(lostConsumersMutex);
                        auto& c = lostConsumers[li->ci.id()];

                        // если уже выставлен флаг что "заказчик" пропал, то не надо увеличивать "счётчик"
                        // видимо мы уже зафиксировали его пропажу на другом датчике...
                        if( !c.lost )
                        {
                            c.count += 1;
                            c.lost = true;
                        }
                    }

                    li = lst.clst.erase(li);
                    --li;
                    break;
                }
            }
            catch( const std::exception& ex )
            {
                uwarn << myname << "(IONotifyController::send): UniSetObject_i::_nil() "
                      << ex.what()
                      << " for " << uniset_conf()->oind->getNameById(li->ci.id()) << "@" << li->ci.node() << endl;
            }
        }
    }
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::activateObject()
{
    // сперва загружаем датчики и заказчиков..
    readConf();
    // а потом уже собственно активация..
    return IOController::activateObject();
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::sensorsRegistration()
{
    for_iolist([this](std::shared_ptr<USensorInfo>& s)
    {
        ioRegistration(s);
    });
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::readConf()
{
    try
    {
        if( restorer )
            initIOList( std::move(restorer->read()) );
    }
    catch( const std::exception& ex )
    {
        // Если дамп не удалось считать, значит что-то не то в configure.xml
        // и безопаснее "вылететь", чем запустится, т.к. часть датчиков не будет работать
        // как ожидается.
        cerr << myname << "(IONotifyController::readConf): " << ex.what() << endl << flush;
        //std::terminate(); // std::abort();
        uterminate();
    }
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::initItem( std::shared_ptr<USensorInfo>& usi, IOController* ic )
{
    if( usi->sinf.type() == uniset3::AI || usi->sinf.type() == uniset3::AO )
        checkThreshold( usi, false );
}
// ------------------------------------------------------------------------------------------
grpc::Status IONotifyController::askThreshold(::grpc::ServerContext* context, const ::uniset3::AskThresholdParams* request, ::google::protobuf::Empty* response)
{
    ulog2 << "(askThreshold): " << ( request->cmd() == UIODontNotify ? "отказ" : "заказ" ) << " от "
          << uniset_conf()->oind->getNameById(request->ci().id()) << "@" << request->ci().node()
          << " на порог tid=" << request->tid()
          << " [" << request->lowlimit() << "," << request->hilimit() << ",invert=" << request->invert() << "]"
          << " для датчика "
          << uniset_conf()->oind->getNameById(request->sid())
          << endl;

    if( request->lowlimit() > request->hilimit() )
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "");

    auto li = myioEnd();

    long val = 0;

    try
    {
        // если такого датчика нет здесь сработает исключение...
        val = localGetValue(li, request->sid());
    }
    catch( const IOController::Undefined& ex ) {}

    {
        // lock
        uniset_rwmutex_wrlock lock(trshMutex);

        auto it = findThreshold(request->sid(), request->tid());

        switch( request->cmd() )
        {
            case uniset3::UIONotify: // заказ
            case uniset3::UIONotifyChange:
            {
                if( !it )
                    it = make_shared<IOController::UThresholdInfo>(request->tid(), request->lowlimit(), request->hilimit(), request->invert());

                it = addThresholdIfNotExist(li->second, it);
                addThresholdConsumer(it, request->ci());

                if( request->cmd() == uniset3::UIONotifyChange )
                    break;

                // посылка первый раз состояния
                umessage::SensorMessage sm(li->second->makeSensorMessage());
                sm.mutable_header()->set_consumer(request->ci().id());
                sm.set_tid(request->tid());

                // Проверка нижнего предела
                if( val <= request->lowlimit() )
                    sm.set_threshold(false);
                // Проверка верхнего предела
                else if( val >= request->hilimit() )
                    sm.set_threshold(true);

                auto clst = askTMap.find(it.get());

                if( clst != askTMap.end() )
                    send(clst->second, sm, &request->ci());
            }
            break;

            case uniset3::UIODontNotify:     // отказ
            {
                if( it )
                    removeThresholdConsumer(li->second, it, request->ci());
            }
            break;

            default:
                break;
        }
    } // unlock trshMutex

    return grpc::Status::OK;
}
// --------------------------------------------------------------------------------------------------------------
std::shared_ptr<IOController::UThresholdInfo>
IONotifyController::addThresholdIfNotExist( std::shared_ptr<USensorInfo>& usi,
        std::shared_ptr<UThresholdInfo>& ti )
{
    uniset3::uniset_rwmutex_wrlock lck(usi->tmut);

    for( auto&& t : usi->thresholds )
    {
        if( ti->tinf.id() == t->tinf.id() )
            return t;
    }

    struct timespec tm = uniset3::now_to_timespec();

    ti->tinf.mutable_ts()->set_sec(tm.tv_sec);

    ti->tinf.mutable_ts()->set_nsec(tm.tv_nsec);

    usi->thresholds.push_back(ti);

    return ti;
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::addThresholdConsumer( std::shared_ptr<IOController::UThresholdInfo>& ti, const ConsumerInfo& ci )
{
    auto i = askTMap.find(ti.get());

    if( i != askTMap.end() )
        return addConsumer(i->second, ci);

    auto ret = askTMap.emplace(ti.get(), ConsumerListInfo());

    return addConsumer( ret.first->second, ci );
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::removeThresholdConsumer( std::shared_ptr<USensorInfo>& usi,
        std::shared_ptr<UThresholdInfo>& ti,
        const uniset3::ConsumerInfo& ci )
{
    uniset_rwmutex_wrlock lck(usi->tmut);

    for( auto&& t : usi->thresholds )
    {
        if( t->tinf.id() == ti->tinf.id() )
        {
            auto it = askTMap.find(ti.get());

            if( it == askTMap.end() )
                return false;

            return removeConsumer(it->second, ci);
        }
    }

    return false;
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::checkThreshold( IOController::IOStateList::iterator& li,
        const uniset3::ObjectId sid,
        bool send_msg )
{
    if( li == myioEnd() )
        li = myiofind(sid);

    if( li == myioEnd() )
        return; // ???

    checkThreshold(li->second, send_msg);
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::checkThreshold( std::shared_ptr<IOController::USensorInfo>& usi, bool send_msg )
{
    uniset_rwmutex_rlock lock(trshMutex);

    auto& ti = usi->thresholds;

    if( ti.empty() )
        return;

    // обрабатываем текущее состояние датчика обязательно "залочив" значение..
    uniset_rwmutex_rlock vlock(usi->val_lock);

    umessage::SensorMessage sm(usi->makeSensorMessage(false));

    // текущее время
    struct timespec tm = uniset3::now_to_timespec();

    {
        uniset_rwmutex_rlock l(usi->tmut);

        for( auto&& it : ti )
        {
            // Используем здесь значение скопированное в sm.value
            // чтобы не делать ещё раз lock на li->second->value
            uniset3::ThresholdState state = it->tinf.state();

            if( !it->tinf.invert() )
            {
                // Если логика не инвертированная, то срабатывание это - выход за зону >= hilimit
                if( sm.value() <= it->tinf.lowlimit()  )
                    state = uniset3::NormalThreshold;
                else if( sm.value() >= it->tinf.hilimit() )
                    state = uniset3::HiThreshold;
            }
            else
            {
                // Если логика инвертированная, то срабатывание это - выход за зону <= lowlimit
                if( sm.value() >= it->tinf.hilimit()  )
                    state = uniset3::NormalThreshold;
                else if( sm.value() <= it->tinf.lowlimit() )
                    state = uniset3::LowThreshold;
            }

            // если ничего не менялось..
            if( it->tinf.state() == state )
                continue;

            it->tinf.set_state(state);
            sm.set_tid(it->tinf.id());

            // если состояние не normal, значит порог сработал,
            // не важно какой.. нижний или верхний (зависит от inverse)
            sm.set_threshold( state != uniset3::NormalThreshold );

            // запоминаем время изменения состояния
            it->tinf.mutable_ts()->set_sec(tm.tv_sec);
            it->tinf.mutable_ts()->set_nsec(tm.tv_nsec);
            *(sm.mutable_sm_ts()) = it->tinf.ts();

            // если порог связан с датчиком, то надо его выставить
            if( it->sid != uniset3::DefaultObjectId )
            {
                try
                {
                    localSetValueIt(it->sit, it->sid, (sm.threshold() ? 1 : 0), usi->sinf.supplier());
                }
                catch( uniset3::Exception& ex )
                {
                    ucrit << myname << "(checkThreshold): " << ex << endl;
                }
            }

            // отдельно посылаем сообщения заказчикам по данному "порогу"
            if( send_msg )
            {
                uniset_rwmutex_rlock lck(trshMutex);
                auto i = askTMap.find(it.get());

                if( i != askTMap.end() )
                    send(i->second, sm);
            }
        }
    }
}
// --------------------------------------------------------------------------------------------------------------
std::shared_ptr<IOController::UThresholdInfo> IONotifyController::findThreshold( const uniset3::ObjectId sid, const uniset3::ThresholdId tid )
{
    auto it = myiofind(sid);

    if( it != myioEnd() )
    {
        auto usi = it->second;
        uniset_rwmutex_rlock lck(usi->tmut);

        for( auto&& t : usi->thresholds )
        {
            if( t->tinf.id() == tid )
                return t;
        }
    }

    return nullptr;
}
// --------------------------------------------------------------------------------------------------------------

grpc::Status IONotifyController::getThresholdInfo(::grpc::ServerContext* context, const ::uniset3::GetThresholdInfoParams* request, ::uniset3::ThresholdInfo* response)
{
    uniset_rwmutex_rlock lock(trshMutex);
    auto it = findThreshold(request->sid(), request->tid());

    if( !it )
    {
        ostringstream err;
        err << myname << "(getThresholds): Not found sensor (" << request->sid() << ") "
            << uniset_conf()->oind->getNameById(request->sid());

        uinfo << err.str() << endl;
        return grpc::Status(grpc::StatusCode::NOT_FOUND, err.str());
    }

    (*response) = it->tinf;
    return grpc::Status::OK;
}
// --------------------------------------------------------------------------------------------------------------
grpc::Status IONotifyController::getThresholds(::grpc::ServerContext* context, const ::uniset3::GetThresholdsParams* request, ::uniset3::ThresholdList* response)
{
    auto it = myiofind(request->id());

    if( it == myioEnd() )
    {
        ostringstream err;
        err << myname << "(getThresholds): Not found sensor (" << request->id() << ") "
            << uniset_conf()->oind->getNameById(request->id());

        uinfo << err.str() << endl;
        return grpc::Status(grpc::StatusCode::NOT_FOUND, err.str());
    }

    auto& usi = it->second;

    try
    {
        *(response->mutable_si()) = usi->sinf.si();
        response->set_value(IOController::localGetValue(usi));
        response->set_type(usi->sinf.type());
    }
    catch( const uniset3::Exception& ex )
    {
        uwarn << myname << "(getThresholds): для датчика "
              << uniset_conf()->oind->getNameById(usi->sinf.si().id())
              << " " << ex << endl;
    }

    uniset_rwmutex_rlock lck(usi->tmut);

    for( const auto& it2 : usi->thresholds )
        *(response->mutable_tlist()->add_thresholds()) = it2->tinf;

    return grpc::Status::OK;
}
// --------------------------------------------------------------------------------------------------------------
grpc::Status IONotifyController::getThresholdsList(::grpc::ServerContext* context, const ::uniset3::GetThresholdsListParams* request, ::uniset3::ThresholdsListSeq* response)
{
    if( getId() != request->id() )
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

    std::list< std::shared_ptr<USensorInfo> > slist;

    // ищем все датчики, у которых не пустой список порогов
    for_iolist([&slist]( std::shared_ptr<USensorInfo>& usi )
    {

        if( !usi->thresholds.empty() )
            slist.push_back(usi);
    });

    if( !slist.empty() )
    {
        for( auto&& it : slist )
        {
            auto t = response->add_thresholds();

            try
            {
                *(t->mutable_si()) = it->sinf.si();
                t->set_value(IOController::localGetValue(it));
                t->set_type(it->sinf.type());
            }
            catch( const std::exception& ex )
            {
                uwarn << myname << "(getThresholdsList): for sid="
                      << uniset_conf()->oind->getNameById(it->sinf.si().id())
                      << " " << ex.what() << endl;
                continue;
            }

            uniset_rwmutex_rlock lck(it->tmut);

            for( const auto& it2 : it->thresholds )
                *(t->mutable_tlist()->add_thresholds()) = it2->tinf;
        }
    }

    return grpc::Status::OK;
}
// -----------------------------------------------------------------------------
void IONotifyController::onChangeUndefinedState( std::shared_ptr<USensorInfo>& usi, IOController* ic )
{
    uniset_rwmutex_rlock vlock(usi->val_lock);
    umessage::SensorMessage sm( usi->makeSensorMessage(false) );

    try
    {
        if( !usi->sinf.dbignore() )
            logging(sm);
    }
    catch(...) {}

    ConsumerListInfo* lst = static_cast<ConsumerListInfo*>(usi->getUserData(udataConsumerList));

    if( lst )
        send(*lst, sm);
}

// -----------------------------------------------------------------------------
grpc::Status IONotifyController::askSensorsSeq(::grpc::ServerContext* context, const ::uniset3::AskSeqParams* request, ::uniset3::IDSeq* response)
{
    AskParams p;
    p.set_cmd(request->cmd());
    *(p.mutable_ci()) = request->ci();

    google::protobuf::Empty empty;

    for( const auto& s : request->ids().ids() )
    {
        try
        {
            p.set_sid(s);
            askSensor(context, &p, &empty );
        }
        catch(...)
        {
            response->add_ids(s);
        }
    }

    return grpc::Status::OK;
}
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
Poco::JSON::Object::Ptr IONotifyController::httpHelp(const Poco::URI::QueryParameters& p)
{
    uniset3::json::help::object myhelp(myname, IOController::httpHelp(p));

    {
        // 'consumers'
        uniset3::json::help::item cmd("consumers", "get consumers list");
        cmd.param("sensor1,sensor2,sensor3", "get consumers for sensors");
        myhelp.add(cmd);
    }

    {
        // 'lost'
        uniset3::json::help::item cmd("lost", "get lost consumers list");
        myhelp.add(cmd);
    }

    return myhelp;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IONotifyController::httpRequest( const string& req, const Poco::URI::QueryParameters& p )
{
    if( req == "consumers" )
        return request_consumers(req, p);

    if( req == "lost" )
        return request_lost(req, p);

    return IOController::httpRequest(req, p);
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IONotifyController::request_consumers( const string& req, const Poco::URI::QueryParameters& p )
{
    Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
    Poco::JSON::Array::Ptr jdata = uniset3::json::make_child_array(json, "sensors");
    auto my = httpGetMyInfo(json);

    auto oind = uniset_conf()->oind;

    std::list<ParamSInfo> slist;

    ConsumerListInfo emptyList;

    if( !p.empty() )
    {
        if( !p[0].first.empty() )
            slist = uniset3::getSInfoList( p[0].first, uniset_conf() );

        if( slist.empty() )
        {
            ostringstream err;
            err << "(request_consumers): Bad request parameter: '" << p[0].first << "'";
            throw uniset3::SystemError(err.str());
        }
    }

    uniset_rwmutex_rlock lock(askIOMutex);

    // Проход по списку заданных..
    if( !slist.empty() )
    {
        for( const auto& s : slist )
        {
            auto a = askIOList.find(s.si.id());

            if( a == askIOList.end() )
                jdata->add( getConsumers(s.si.id(), emptyList, false) );
            else
                jdata->add( getConsumers(a->first, a->second, false) );
        }
    }
    else // Проход по всему списку
    {
        for( auto&& a : askIOList )
        {
            // добавляем только датчики только с непустым списком заказчиков
            auto jret = getConsumers(a.first, a.second, true);

            if( jret )
                jdata->add(jret);
        }
    }

    return json;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IONotifyController::getConsumers(ObjectId sid, ConsumerListInfo& ci, bool ifNotEmpty )
{
    /* Создаём json
     * { {"sensor":
     *            {"id": "xxxx"},
     *            {"name": "xxxx"}
     *   },
     *   "consumers": [
     *            {..consumer1 info },
     *            {..consumer2 info },
     *            {..consumer3 info },
     *            {..consumer4 info }
     *   ]
     * }
     */

    uniset_rwmutex_rlock lock(ci.mut);

    if( ci.clst.empty() && ifNotEmpty )
        return nullptr;

    Poco::JSON::Object::Ptr jret = new Poco::JSON::Object();
    auto oind = uniset_conf()->oind;
    auto jsens = uniset3::json::make_child(jret, "sensor");
    jsens->set("id", sid);
    jsens->set("name", ObjectIndex::getShortName(oind->getMapName(sid)));

    auto jcons = uniset3::json::make_child_array(jret, "consumers");

    for( const auto& c : ci.clst )
    {
        Poco::JSON::Object::Ptr consumer = new Poco::JSON::Object();
        consumer->set("id", c.ci.id());
        consumer->set("name", ObjectIndex::getShortName(oind->getMapName(c.ci.id())));
        consumer->set("node", c.ci.node());
        consumer->set("node_name", oind->getNodeName(c.ci.node()));
        consumer->set("lostEvents", c.lostEvents);
        consumer->set("attempt", c.attempt);
        consumer->set("smCount", c.smCount);
        jcons->add(consumer);
    }

    return jret;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IONotifyController::request_lost( const string& req, const Poco::URI::QueryParameters& p )
{
    Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
    Poco::JSON::Array::Ptr jdata = uniset3::json::make_child_array(json, "lost consumers");
    auto my = httpGetMyInfo(json);

    auto oind = uniset_conf()->oind;

    std::lock_guard<std::mutex> lock(lostConsumersMutex);

    for( const auto& c : lostConsumers )
    {
        Poco::JSON::Object::Ptr jcons = new Poco::JSON::Object();
        jcons->set("id", c.first);
        jcons->set("name",  ObjectIndex::getShortName(oind->getMapName(c.first)));
        jcons->set("lostCount",  c.second.count);
        jcons->set("lost", c.second.lost);
        jdata->add(jcons);
    }

    return json;
}
// -----------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
