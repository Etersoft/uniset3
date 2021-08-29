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
 * \brief Реализация IONotifyController_i
 * \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef IONotifyController_H_
#define IONotifyController_H_
//---------------------------------------------------------------------------
#include <memory>
#include <unordered_map>
#include <list>
#include <string>
#include <mutex>

#include "UniSetTypes.h"
#include "IOController.h"

//---------------------------------------------------------------------------
namespace uniset3
{
    class IOConfig;
    //---------------------------------------------------------------------------
    /*!
        \page page_IONotifyController Хранение информации о состоянии с уведомлениями об изменении (IONotifyController)

        Класс IONotifyController расширяет набор задач класса IOController.
        Для ознакомления с базовыми функциями см. \ref page_IOController

        Задачи решаемые IONotifyController-ом (\b IONC):
        - \ref sec_NC_AskSensors
        - \ref sec_NC_Consumers
        - \ref sec_NC_Thresholds
        - \ref sec_NC_Depends

        \section sec_NC_AskSensors Механизм заказа датчиков
        Главной задачей класса IONotifyController является уведомление
    объектов (заказчиков) об изменении состояния датчика (входа или выхода).

    Механизм функционирует по следующей логике:
    "заказчики" уведомляют \b IONC об изменении какого именно датчика они хотят получать уведомление,
    после чего, если данный датчик меняет своё состояние, заказчику посылается
    сообщение uniset3::umessage::SensorMessage содержащее информацию о текущем(новом) состоянии датчика,
    времени изменения и т.п. В случае необходимости можно отказаться от уведомления.
    Для заказа датчиков предусмотрен ряд функций. На данный момент рекомендуется
    пользоваться функцией IONotifyController::askSensor.

    \b ConsumerMaxAttempts - максимальное число неудачных попыток послать сообщение "заказчику".
    Настраивается в конфигурационном файле. По умолчанию = 5.

        \section sec_NC_Consumers  Заказчики
    В качестве "заказчиков" могут выступать любые UniSet-объекты (UniSetObject),
    обладающие "обратным адресом" (идентификатором), по которому присылается
    уведомление об изменении состояния. Свой обратный адрес, объекты указывают
    непосредственно при заказе (см. IONotifyController::askSensor).

    Помимо "динамического" заказа во время работы процессов, существует возможность
    задавать список заказчиков на этапе конфигурирования системы ("статический" способ).
    Для этого в конфигурационном файле, в секции \b <sensors> у каждого датчика предусмотрена
    специальная секция \b <consumers>.
    \code
    <sensors>
    ...
    <item name="Sensors1" textname="sensor N1" iotype="AI" ...>
       <consumers>
           <consumer name="TestProc1" type="objects"/>
           <consumer name="TestProc2" type="managers" node="RemoteNode"/>
           ...
       </consumers>
    </item>
    ...
    </sensors>
    \endcode
    "Статический" способ заказа гарантирует, что при перезапуске
    \b IONC список заказчиков будет восстановлен по конфигурационному файлу.

        \section sec_NC_Thresholds Пороговые датчики

        \section sec_NC_Depends Механизм зависимостей между датчиками
        Механизм зависимостей позволяет задать зависимость одного датчика, от другого.
    Например пока "разрешающий" датчик не равен "1", у зависимого держится значение "0".
        Зависимость настраивается в конфигурационном файле, непосредственно у "зависимого датчика".
        <br>Доступные настройки:
      - \b depend            - задаёт имя датчика \b ОТ \b КОТОРОГО зависит данный
      - \b depend_value      - задаёт "разрешающее работу" значение (если поле не задано, считается depend_value="1").
      - \b depend_off_value  - задаёт значение которое будет выставлено для данного датчика, в случае его "блокировки". По умолчанию 0.

    \code
        ...
        <sensors ..>
            ...
            <item id="11" iotype="AI" name="AI11_AS" textname="AI 11" depend="Input4_S" depend_off_value="-50"/>
            ...
        </sensors>
    \endcode
        В данном примере можно увидеть, что датчик AI11_AS зависит от датчика Input4_S и пока Input4_S=0,
    в AI11_AS будет записано значение -50. Как только Input4_S=1 в AI11_AS - появиться его истинное значение.

    \note Следует иметь ввиду, что для \b ЗАВИСИМОГО датчика функция setValue(..) действует как обычно и
    даже если он "заблокирован", значение в него можно сохранять. Оно "появиться" как только снимется блокировка.

    \section sec_NC_Optimization Оптимизация работы со списком "заказчиков"
    Для оптимизации поиска списка заказчиков для конкретного датчика используется поле userdata (void*) у USensorInfo!
    Это опасный и "некрасивый" хак, но он позволяет избавиться от одного лишнего поиска по unordered_map<SensorID,ConsumerList>.
    Суть в том что к датчику через usedata мы привязываем указатель на список заказчиков. Сделано через userdata,
    т.к. сам map "хранится" в IOController и IONotifyController не может поменять тип (в текущей реализации по крайней мере).
    В userdata задействованы два места (см. UserDataID) для списка заказчиков и для списка порогов.
    При этом, чтобы гарантировать корректность работы, cписки заказчиков по тому или иному датчику,
    создаются (см. функцию ask()), но никогда не удаляются, даже если остаются пустыми.
    Это сделано, чтобы сохранённые указатели в userdata, оставались всегда валидными
    (т.к. используются из разных потоков).
    */
    //---------------------------------------------------------------------------
    /*! Реализация IONotifyController_i.
     * см. page_IONotifyController
     */
    class IONotifyController:
        public IOController,
        public IONotifyController_i::Service
    {
        public:

            IONotifyController(const uniset3::ObjectId id, std::shared_ptr<IOConfig> ioconf = nullptr );

            virtual ~IONotifyController();

            virtual ::grpc::Status getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request, ::google::protobuf::StringValue* response) override;
            virtual std::string getStrType() const override;

            // ------  GRPC интерфейс ------
            virtual ::grpc::Status askSensor(::grpc::ServerContext* context, const ::uniset3::AskParams* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status askSensorsSeq(::grpc::ServerContext* context, const ::uniset3::AskSeqParams* request, ::uniset3::IDSeq* response) override;
            // --------------------------------------------
            typedef ::grpc::ServerAsyncReaderWriter<uniset3::umessage::SensorMessage, uniset3::SensorsStreamCmd> RWResponder;
            class AsyncClientSession;
            struct SyncClient:
               public std::enable_shared_from_this<SyncClient>
            {
                SyncClient(IONotifyController* i, AsyncClientSession* s);
                ~SyncClient();

                void readEvent( const uniset3::SensorsStreamCmd& request );
                void pushData( const uniset3::umessage::SensorMessage& r );
                void close();
                bool isClosed() const;

                IONotifyController* nc = { nullptr };
                AsyncClientSession* session = { nullptr };
                std::atomic_bool closed = { false };
            };
            // --------------------------------------------
            /*! Информация о заказчике */
            struct ConsumerInfoExt
            {
                ConsumerInfoExt(const std::shared_ptr<SyncClient>& cli, size_t maxAttemtps = 10 ):
                        client(cli),
                        attempt(maxAttemtps) {}

                ConsumerInfoExt( const uniset3::ConsumerInfo& _ci, size_t maxAttemtps = 10 ):
                    ci(_ci),
                    attempt(maxAttemtps) {}

                std::shared_ptr<SyncClient> client = {nullptr };
                uniset3::ConsumerInfo ci;
                size_t attempt = { 10 };
                size_t lostEvents = { 0 }; // количество потерянных сообщений (не смогли послать)
                size_t smCount = { 0 }; // количество посланных SensorMessage

                ConsumerInfoExt( const ConsumerInfoExt& ) = default;
                ConsumerInfoExt& operator=( const ConsumerInfoExt& ) = default;
                ConsumerInfoExt( ConsumerInfoExt&& ) = default;
                ConsumerInfoExt& operator=(ConsumerInfoExt&& ) = default;
            };

            typedef std::list<ConsumerInfoExt> ConsumerList;

            struct ConsumerListInfo
            {
                ConsumerListInfo(): mut("ConsumerInfoMutex") {}
                ConsumerList clst;
                uniset3::uniset_rwmutex mut;

                ConsumerListInfo( const ConsumerListInfo& ) = delete;
                ConsumerListInfo& operator=( const ConsumerListInfo& ) = delete;
                ConsumerListInfo( ConsumerListInfo&& ) = default;
                ConsumerListInfo& operator=(ConsumerListInfo&& ) = default;
            };

            /*! словарь: датчик -> список потребителей */
            typedef std::unordered_map<uniset3::ObjectId, ConsumerListInfo> AskMap;

            // связь: id датчика --> id порога --> список заказчиков
            // т.к. каждый порог имеет уникальный указатель, используем его в качестве ключа
            typedef std::unordered_map<UThresholdInfo*, ConsumerListInfo> AskThresholdMap;

        protected:
            IONotifyController();
            virtual bool activateObject() override;
            virtual void sensorsRegistration() override;
            virtual void initItem( std::shared_ptr<USensorInfo>& usi, IOController* ic );

            //! посылка информации об изменении состояния датчика (всем или указанному заказчику)
            virtual void send( ConsumerListInfo& lst, const uniset3::umessage::SensorMessage& sm, const uniset3::ConsumerInfo* ci = nullptr );

            //! проверка срабатывания пороговых датчиков
            virtual void checkThreshold( std::shared_ptr<USensorInfo>& usi, bool send = true );
            virtual void checkThreshold( IOController::IOStateList::iterator& li, const uniset3::ObjectId sid, bool send_msg = true );

            //! поиск информации о пороговом датчике
            std::shared_ptr<UThresholdInfo> findThreshold( const uniset3::ObjectId sid, const uniset3::ThresholdId tid );

            /*! чтение dump-файла */
            virtual void readConf();

            std::shared_ptr<IOConfig> restorer;

            void askSyncClient( uniset3::ObjectId sid, uniset3::UIOCommand cmd,
                                const std::shared_ptr<SyncClient>& cli,
                                uniset3::ConsumerInfo _ci );

            virtual void setSyncClient( uniset3::ObjectId sid, int64_t value,
                                   const std::shared_ptr<SyncClient>& cli,
                                   uniset3::ConsumerInfo _ci );

            // функция для работы напрямую с указателем (оптимизация)
            virtual long localSetValue( std::shared_ptr<USensorInfo>& usi,
                                        long value, uniset3::ObjectId sup_id ) override;

            // статистика
            void showStatisticsForConsumer( std::ostringstream& inf, const std::string& consumer );
            void showStatisticsForLostConsumers( std::ostringstream& inf );
            void showStatisticsForConsusmers( std::ostringstream& inf );
            void showStatisticsForConsumersWithLostEvent( std::ostringstream& inf );
            void showStatisticsForSensor( std::ostringstream& inf, const std::string& name );

            //! \warning Оптимизация использует userdata! Это опасно, если кто-то ещё захочет его использовать!
            // идентификаторы данных в userdata (см. USensorInfo::userdata)
            enum UserDataID
            {
                udataConsumerList = 0
            };

        private:

            friend class NCRestorer;
            friend class IONotifyControllerProxy;

            virtual bool initBeforeRunServer( grpc::ServerBuilder &builder ) override;
            virtual bool deactivateAfterStopServer() override;
            //----------------------
            bool addSyncConsumer( ConsumerListInfo& lst, std::shared_ptr<SyncClient> cli );
            bool removeSyncConsumer( ConsumerListInfo& lst, const std::shared_ptr<SyncClient>& cli );
            bool addConsumer( ConsumerListInfo& lst, const uniset3::ConsumerInfo& cons );
            bool removeConsumer( ConsumerListInfo& lst, const uniset3::ConsumerInfo& cons );

            //! обработка заказа
            void ask(AskMap& askLst, const uniset3::ObjectId sid,
                     const uniset3::ConsumerInfo& ci,
                     uniset3::UIOCommand cmd,
                     std::shared_ptr<SyncClient> cli = nullptr );

            AskMap askIOList; /*!< список потребителей по  датчикам */

            /*! замок для блокирования совместного доступа к cписку потребителей датчиков */
            uniset3::uniset_rwmutex askIOMutex;

            sigc::connection conInit;

            int maxAttemtps; /*! максимальное количество попыток послать сообщение заказчику, после чего он будет удалён из списка */
            int sendAttemtps; /*! максимальное количество попыток послать сообщение заказчику во время изменения датчика */

            std::mutex lostConsumersMutex;

            struct LostConsumerInfo
            {
                size_t count = { 0 }; // количество "пропаданий"
                bool lost = { false }; // флаг означающий что "заказчик пропал"
                // lost нужен чтобы в count не увеличивать, на send() по каждому датчику, если заказчик заказывал больше одного датчика)
                // флаг сбрасывается при перезаказе датчика..
            };

            /*! map для хранения информации о заказчиках с которыми была потеряна связь
             * и которые были удалены из списка заказчиков
             */
            std::unordered_map<uniset3::ObjectId, LostConsumerInfo> lostConsumers;

            // ----- Async processing -----
            friend class AsyncClient;
            friend class AsyncClientSession;
            std::unique_ptr<grpc::ServerCompletionQueue> asyncCQ;
            uniset3::IONotifyStreamController_i::AsyncService asyncService;
            std::unique_ptr<grpc::Server> server_;

            std::unordered_map<AsyncClientSession*, std::shared_ptr<SyncClient>> clients;
            std::mutex mutClients;
            std::thread asyncThread;
            void asyncMainLoop();
            std::shared_ptr<SyncClient> newClient(AsyncClientSession* sess);
            void releaseClient(AsyncClientSession* sess);
    };
    // ----------------------------------------------------------------------
} // end of uniset3 namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
