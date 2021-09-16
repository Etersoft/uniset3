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
#include <queue>
#include <grpcpp/grpcpp.h>
#include <grpcpp/alarm.h>

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
    maxAttemtps(uniset_conf()->getPIntField("ConsumerMaxAttempts", 10)),
    sendAttemtps(uniset_conf()->getPIntField("ConsumerSendAttempts", 3))
{
    ui->setCacheMaxSize(uniset_conf()->getPIntField("ConsumerMaxCache", 5000));
}

IONotifyController::IONotifyController( ObjectId id, std::shared_ptr<IOConfig> d ):
    IOController(id),
    restorer(d),
    askIOMutex(string(uniset_conf()->oind->getMapName(id)) + "_askIOMutex"),
    maxAttemtps(uniset_conf()->getPIntField("ConsumerMaxAttempts", 10)),
    sendAttemtps(uniset_conf()->getPIntField("ConsumerSendAttempts", 3))
{
    conInit = signal_init().connect(sigc::mem_fun(*this, &IONotifyController::initItem));
    ui->setCacheMaxSize(uniset_conf()->getPIntField("ConsumerMaxCache", 5000));
}

IONotifyController::~IONotifyController()
{
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
std::string IONotifyController::getStrType() const
{
    return "IONotifyController";
}
// ------------------------------------------------------------------------------------------
grpc::Status IONotifyController::getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request, ::google::protobuf::StringValue* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getInfo): Deadline exceeded or Client cancelled, abandoning.");
    }

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
        auto query = uniset3::split(param, ':');

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
bool IONotifyController::addSyncConsumer( ConsumerListInfo& lst, std::shared_ptr<SyncClient> cli )
{
    uniset_rwmutex_wrlock l(lst.mut);

    for( auto&& it :  lst.clst )
    {
        if(it.client && it.client.get() == cli.get() )
        {
            // при перезаказе датчиков количество неудачных попыток послать сообщение
            // считаем что "заказчик" опять на связи
            it.attempt = maxAttemtps;
            return false;
        }
    }

    ConsumerInfoExt cinf(cli, maxAttemtps);
    lst.clst.emplace_front( std::move(cinf) );
    return true;
}
// ------------------------------------------------------------------------------------------
bool IONotifyController::removeSyncConsumer( ConsumerListInfo& lst, const std::shared_ptr<SyncClient>& cli )
{
    uniset_rwmutex_wrlock l(lst.mut);

    for( auto li = lst.clst.begin(); li != lst.clst.end(); ++li )
    {
        if(li->client && li->client.get() == cli.get() )
        {
            lst.clst.erase(li);
            return true;
        }
    }

    return false;
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
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(askSensor): Deadline exceeded or Client cancelled, abandoning.");
    }

    ulog2 << "(askSensor): поступил " << ( request->cmd() == UIODontNotify ? "отказ" : "заказ" ) << " от "
          << "(" << request->ci().id() << ")"
          << uniset_conf()->oind->getNameById(request->ci().id()) << "@" << request->ci().node()
          << " на датчик "
          << uniset_conf()->oind->getNameById(request->sid()) << endl;

    auto li = myioEnd();

    // если такого аналогового датчика нет, здесь сработает исключение...
    localGetValue(li, request->sid());

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
bool IONotifyController::getSensor( uniset3::ObjectId sid, uniset3::umessage::SensorMessage& sm )
{
    auto li = find(sid);
    if( li == ioEnd() )
        return false;

    sm = li->second->makeSensorMessage(true);
    return true;
}
// ------------------------------------------------------------------------------------------
void IONotifyController::setSyncClient( uniset3::ObjectId sid, int64_t value,
                                        const std::shared_ptr<SyncClient>& cli,
                                        uniset3::ConsumerInfo ci )
{
    auto li = ioEnd();

    try
    {
        localSetValueIt( li, sid, value, ci.id() );
        return;
    }
    catch( std::exception& ex )
    {
        ulog4 << "(setSyncClient): " << ex.what() << endl;
    }
}
// ------------------------------------------------------------------------------------------
void IONotifyController::askSyncClient( uniset3::ObjectId sid, uniset3::UIOCommand cmd,
                                        const std::shared_ptr<SyncClient>& cli,
                                        uniset3::ConsumerInfo ci )
{
    ulog2 << "(askSyncClient): поступил " << ( cmd == UIODontNotify ? "отказ" : "заказ" ) << " от "
          << "(" << ci.id() << ")"
          << " на датчик "
          << uniset_conf()->oind->getNameById(sid) << endl;

    auto li = myioEnd();

    if( ci.id() == 0 )
        ci.set_id(DefaultObjectId);

    if( ci.node() == 0 )
        ci.set_node(DefaultObjectId);

    // если такого аналогового датчика нет, здесь сработает исключение...
    try
    {
        localGetValue(li, sid);
    }
    catch( std::exception& ex )
    {
        // cli->pushError(smsg);
        return;
    }

    {
        uniset_rwmutex_wrlock lock(askIOMutex);
        ask(askIOList, sid, ci, cmd, cli);
    }

    auto usi = li->second;

    // посылка первый раз состояния
    if( cmd == uniset3::UIONotify || (cmd == UIONotifyFirstNotNull && usi->sinf.value()) )
    {
        uniset3::uniset_rwmutex_rlock lock(usi->val_lock);
        umessage::SensorMessage smsg( usi->makeSensorMessage(false) );
        cli->pushData(smsg);
    }
}
// ------------------------------------------------------------------------------------------
void IONotifyController::ask( AskMap& askLst, const uniset3::ObjectId sid,
                              const uniset3::ConsumerInfo& cons,
                              uniset3::UIOCommand cmd,
                              std::shared_ptr<SyncClient> cli )
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
            {
                if( cli )
                    addSyncConsumer(askIterator->second, cli);
                else
                    addConsumer(askIterator->second, cons);
            }
            else
            {
                ConsumerListInfo newlst; // создаем новый список

                if( cli )
                    addSyncConsumer(newlst, cli);
                else
                    addConsumer(newlst, cons);

                askLst.emplace(sid, std::move(newlst));
            }

            break;
        }

        case uniset3::UIODontNotify:     // отказ
        {
            if( askIterator != askLst.end() )
            {
                if( cli )
                    removeSyncConsumer(askIterator->second, cli);
                else
                    removeConsumer(askIterator->second, cons);
            }

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

        if( !ci && li->client )
        {
            if( li->client->isClosed() )
            {
                li->client = nullptr;
                li = lst.clst.erase(li);

                if( li == lst.clst.end() )
                    li--;

                continue;
            }

            li->client->pushData(sm);
            continue;
        }

        for( int i = 0; i < sendAttemtps; i++ )
        {
            try
            {
                tmsg.set_consumer(li->ci.id());
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
    bool ret = IOController::activateObject();
    asyncThread = std::thread([this]()
    {
        asyncMainLoop();
    });

    return ret;
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
    uniset_rwmutex_rlock lock(usi->tmut);
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
            ThresholdState state = it->state;

            if( !it->invert )
            {
                // Если логика не инвертированная, то срабатывание это - выход за зону >= hilimit
                if( sm.value() <= it->lowlimit  )
                    state = NormalThreshold;
                else if( sm.value() >= it->hilimit )
                    state = HiThreshold;
            }
            else
            {
                // Если логика инвертированная, то срабатывание это - выход за зону <= lowlimit
                if( sm.value() >= it->hilimit )
                    state = NormalThreshold;
                else if( sm.value() <= it->lowlimit )
                    state = LowThreshold;
            }

            // если ничего не менялось..
            if( it->state == state )
                continue;

            it->state = state;

            // если состояние не normal, значит порог сработал,
            // не важно какой.. нижний или верхний (зависит от inverse)
            bool isThreshold = ( state != NormalThreshold );

            // запоминаем время изменения состояния
            it->tv_sec = tm.tv_sec;
            it->tv_nsec = tm.tv_nsec;
            sm.mutable_sm_ts()->set_sec(tm.tv_sec);
            sm.mutable_sm_ts()->set_nsec(tm.tv_nsec);

            // если порог связан с датчиком, то надо его выставить
            if( it->sid != uniset3::DefaultObjectId )
            {
                try
                {
                    localSetValueIt(it->sit, it->sid, (isThreshold ? 1 : 0), usi->sinf.supplier());
                }
                catch( uniset3::Exception& ex )
                {
                    ucrit << myname << "(checkThreshold): " << ex << endl;
                }
            }
        }
    }
}
// --------------------------------------------------------------------------------------------------------------
std::shared_ptr<IOController::UThresholdInfo> IONotifyController::findThreshold( const uniset3::ObjectId sid, const uniset3::ObjectId t_sid )
{
    auto it = myiofind(sid);

    if( it != myioEnd() )
    {
        auto usi = it->second;
        uniset_rwmutex_rlock lck(usi->tmut);

        for( const auto& t : usi->thresholds )
        {
            if( t->sid == t_sid )
                return t;
        }
    }

    return nullptr;
}
// --------------------------------------------------------------------------------------------------------------
grpc::Status IONotifyController::askSensorsSeq(::grpc::ServerContext* context, const ::uniset3::AskSeqParams* request, ::uniset3::IDSeq* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(askSensorsSeq): Deadline exceeded or Client cancelled, abandoning.");
    }

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
// ************* Async processing **************
// -----------------------------------------------------------------------------
bool IONotifyController::initBeforeRunServer( grpc::ServerBuilder& builder )
{
    bool ret = IOController::initBeforeRunServer(builder);
    builder.RegisterService(static_cast<IONotifyStreamController_i::Service*>(&asyncService));
    asyncCQ = builder.AddCompletionQueue();
    return ret;
}
// -----------------------------------------------------------------------------
bool IONotifyController::deactivateAfterStopServer()
{
    bool ret = IOController::deactivateAfterStopServer();
    asyncCQ->Shutdown();
    asyncThread.join();
    return ret;
}
// -----------------------------------------------------------------------------
std::shared_ptr<IONotifyController::SyncClient> IONotifyController::newClient(AsyncClientSession* sess )
{
    std::lock_guard<std::mutex> l(mutClients);
    ulog4 << myname << "(async): ADD NEW CLIENT: " << sess << endl;
    auto cli = std::make_shared<SyncClient>(this, sess);
    clients[sess] = cli;
    return cli;
}
// -----------------------------------------------------------------------------
void IONotifyController::releaseClient( AsyncClientSession* sess )
{
    std::lock_guard<std::mutex> l(mutClients);
    auto i = clients.find(sess);

    if( i != clients.end() )
    {
        ulog4 << myname << "(async): REMOVE CLIENT: " << i->first << endl;
        i->second->close();
        i->second = nullptr;
        clients.erase(i);
    }
}
// -----------------------------------------------------------------------------
struct TagData
{
    enum class Event
    {
        start,
        read,
        write,
        data_timer,
        close
    };

    IONotifyController::AsyncClientSession* handler;
    Event evt;
};
// -----------------------------------------------------------------------------
struct TagSet
{
    TagSet(IONotifyController::AsyncClientSession* self)
        : on_start{self, TagData::Event::start},
          on_read{self, TagData::Event::read},
          on_write{self, TagData::Event::write},
          on_data_timer{self, TagData::Event::data_timer},
          on_close{self, TagData::Event::close} {}

    TagData on_start;
    TagData on_read;
    TagData on_write;
    TagData on_data_timer;
    TagData on_close;
};
// -----------------------------------------------------------------------------
IONotifyController::SyncClient::SyncClient(IONotifyController* i, AsyncClientSession* s):
    nc(i), session(s)
{

}
// -----------------------------------------------------------------------------
void IONotifyController::SyncClient::readEvent( const uniset3::SensorsStreamCmd& request )
{
    ulog4 << "[" << session << "](readEvent): cmd=" << request.cmd() << endl;

    if( request.cmd() == UIOSet )
    {
        for( const auto& s : request.slist() )
            nc->setSyncClient(s.id(), s.val(), shared_from_this(), request.ci());
    }
    else if( request.cmd() == UIOGet )
    {
        uniset3::umessage::SensorMessage sm;
        for( const auto& s : request.slist() )
        {
            if(nc->getSensor(s.id(), sm) )
                pushData(sm);
        }
    }
    else
    {
        for( const auto& s : request.slist() )
            nc->askSyncClient(s.id(), request.cmd(), shared_from_this(), request.ci());
    }
}
// -----------------------------------------------------------------------------
class IONotifyController::AsyncClientSession
{
    public:
        AsyncClientSession(IONotifyController* i, uniset3::IONotifyStreamController_i::AsyncService* srv, grpc::ServerCompletionQueue* scq)
            : tags(this), service(srv), impl(i), cq(scq), responder(&ctx)
        {
            service->RequestsensorsStream(&ctx, &responder, cq, cq, &tags.on_start);
            wait_data_timer = make_unique<grpc::Alarm>();
            num++;
        }

        ~AsyncClientSession()
        {
            ulog4 << "AsyncClientSession[" << this << "] destroy.. [" << --num << "]" << endl;
        }

        static std::chrono::system_clock::time_point deadline_seconds(int s)
        {
            return std::chrono::system_clock::now() + std::chrono::seconds(s);
        }

        bool isClosed() const
        {
            return closed;
        }

        void shutdown()
        {
            ulog4 << "AsyncClientSession[" << this << "] ON SHUTDOWN..." << endl;
            on_close();
        }

        void on_close()
        {
            if( deleted )
                return;

            deleted = true;
            closed = true;
            wait_data_timer->Cancel();
            impl->releaseClient(this);
            cli = nullptr;
            // delete this;
        }

        void on_create( bool ok )
        {
            if( closed )
                return;

            if( !ok )
            {
                on_close();
                return;
            }

            ulog4 << "AsyncClientSession[" << this << "] ON CREATE...[" << num << "]" << endl;
            cli = impl->newClient(this);
            new AsyncClientSession(impl, service, cq);
            responder.Read(&request, &tags.on_read);
            reset_timer();
        }

        void on_read( bool ok )
        {
            if( !ok )
            {
                if( closed )
                    return;

                ulog4 << "AsyncClientSession[" << this << "] ON CLOSE READ..." << endl;
                closed = true;
                wait_data_timer->Cancel();
                responder.Finish(grpc::Status::OK, &tags.on_close);
                return;
            }

            ulog4 << "AsyncClientSession[" << this << "] ON READ..." << endl;
            responder.Read(&request, &tags.on_read);
            cli->readEvent(request);
        }

        void on_data_timer(bool ok)
        {
            if( closed )
                return;

            if( !ok ) // timer was break (new data pushed)
                on_write(true);
            else
            {
                ulog4 << "AsyncClientSession[" << this << "] ON WAIT WRITE DATA TIMEOUT..." << endl;
                reset_timer();
            }
        }

        void reset_timer()
        {
            wait_data_timer->Set(cq, deadline_seconds(wait_data_time_in_seconds), &tags.on_data_timer);
        }

        void on_write(bool ok)
        {
            if( closed )
                return;

            if( !ok )
            {
                ulog4 << "AsyncClientSession[" << this << "] ON CLOSE WRITE..." << endl;
                closed = true;
                wait_data_timer->Cancel();
                responder.Finish(grpc::Status::OK, &tags.on_close);
                return;
            }

            ulog4 << "AsyncClientSession[" << this << "] ON WRITE..." << endl;
            std::lock_guard<std::mutex> lk(wr_queue_mutex);

            if( wr_queue.empty() )
            {
                ulog4 << "AsyncClientSession[" << this << "] write: no new data.." << endl;
                reset_timer();
                return;
            }

            auto reply = wr_queue.front();
            ulog4 << "AsyncClientSession[" << this << "] write: sid=" << reply.id() << " value=" << reply.value() << endl;
            responder.Write(reply, wr_options, &tags.on_write);
            wr_queue.pop();
        }

        void push_data( const uniset3::umessage::SensorMessage& r )
        {
            if( deleted || closed )
                return;

            std::lock_guard<std::mutex> lk(wr_queue_mutex);
            wr_queue.push(r);

            if( wait_data_timer )
                wait_data_timer->Cancel();
        }

    private:
        static atomic_int64_t num;
        std::atomic_bool closed = {false};
        std::atomic_bool deleted = {false};
        TagSet tags;
        uniset3::IONotifyStreamController_i::AsyncService* service;
        std::unique_ptr<grpc::Alarm> wait_data_timer;
        size_t wait_data_time_in_seconds = { 15*60*60 };

        IONotifyController* impl;
        std::shared_ptr<SyncClient> cli;
        grpc::ServerCompletionQueue* cq;
        grpc::ServerContext ctx;
        IONotifyController::RWResponder responder;
        grpc::WriteOptions wr_options;

        uniset3::SensorsStreamCmd request;
        std::queue<uniset3::umessage::SensorMessage> wr_queue;
        std::mutex wr_queue_mutex;
};
// -----------------------------------------------------------------------------
atomic_int64_t IONotifyController::AsyncClientSession::num;
// -----------------------------------------------------------------------------
void IONotifyController::SyncClient::close()
{
    closed = true;
}
// -----------------------------------------------------------------------------
void IONotifyController::SyncClient::pushData( const uniset3::umessage::SensorMessage& r )
{
    if( !closed )
        session->push_data(r);
}
// -----------------------------------------------------------------------------
IONotifyController::SyncClient::~SyncClient()
{
    ulog4 << "AsyncClientSession[" << this << "] delete session.." << endl;
    delete session;
    session = nullptr;
}
// -----------------------------------------------------------------------------
bool IONotifyController::SyncClient::isClosed() const
{
    return closed;
}
// -----------------------------------------------------------------------------
void IONotifyController::asyncMainLoop()
{
    new AsyncClientSession(this, &asyncService, asyncCQ.get());
    void* raw_tag;
    bool ok;

    while( true )
    {
        raw_tag = nullptr;
        bool ret = asyncCQ->Next(&raw_tag, &ok);
        TagData* tag = reinterpret_cast<TagData*>(raw_tag);

        if( !ret )
        {
            if( tag )
                tag->handler->shutdown();

            break;
        }

        if( tag->evt == TagData::Event::read )
            tag->handler->on_read(ok);
        else if( tag->evt == TagData::Event::data_timer )
            tag->handler->on_data_timer(ok);
        else if( tag->evt == TagData::Event::write )
            tag->handler->on_write(ok);
        else if( tag->evt == TagData::Event::close )
            tag->handler->on_close();
        else if( tag->evt == TagData::Event::start )
            tag->handler->on_create(ok);
    }
}
// -----------------------------------------------------------------------------
#if 0
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
