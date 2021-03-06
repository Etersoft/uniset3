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
 * \brief Реализация базового (фундаментального) класса для объектов системы
 * (процессов управления, элементов графического интерфейса и т.п.)
 * \author Pavel Vainerman
 */
//---------------------------------------------------------------------------
#ifndef UniSetObject_H_
#define UniSetObject_H_
//--------------------------------------------------------------------------
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include <ostream>
#include <memory>
#include <string>
#include <list>

#include "UniSetTypes.h"
#include "PassiveTimer.h"
#include "Exceptions.h"
#include "UInterface.h"
#include "UniSetObject.grpc.pb.h"
#include "MetricsExporter.grpc.pb.h"
#include "Configurator.grpc.pb.h"
#include "MessageTypes.pb.h"
#include "ThreadCreator.h"
#include "LT_Object.h"
#include "MQMutex.h"

//---------------------------------------------------------------------------
namespace uniset3
{
    //-------------------------------------------------------------------------
    class UniSetActivator;
    class UniSetManager;
    class UniSetObjectProxy;
    //---------------------------------------------------------------------------
    class UniSetObject;
    typedef std::list< std::shared_ptr<UniSetObject> > ObjectsList;     /*!< Список подчиненных объектов */
    //---------------------------------------------------------------------------
    /*! \class UniSetObject
     *  Класс реализует работу uniset-объекта: работа с очередью сообщений, регистрация объекта, инициализация и т.п.
     *  Обработка сообщений ведётся в специально создаваемом потоке.
     *  Для ожидания сообщений используется функция waitMessage(msec), основанная на таймере.
     *  Ожидание прерывается либо по истечении указанного времени, либо по приходу сообщения, при помощи функции
     *  termWaiting() вызываемой из push().
     *  \note Если не будет задан ObjectId(-1), то поток обработки запущен не будет.
     *  Также создание потока можно принудительно отключить при помощи функции void thread(false). Ее необходимо вызвать до активации объекта
     *  (например в конструкторе). При этом ответственность за вызов receiveMessage() и processingMessage() возлагается
     *  на разработчика.
     *
     *  Имеется три очереди сообщений, по приоритетам: Hi, Medium, Low.
     * Соответственно сообщения вынимаются в порядке поступления, но сперва из Hi, потом из Medium, а потом из Low очереди.
     * \warning Если сообщения будут поступать в Hi или Medium очередь быстрее чем они обрабатываются, то до Low сообщений дело может и не дойти.
     *
    */
    class UniSetObject:
        public std::enable_shared_from_this<UniSetObject>,
        public UniSetObject_i::Service,
        public uniset3::metrics::MetricsExporter_i::Service,
        public uniset3::configurator::Configurator_i::Service,
        public LT_Object
    {
        public:
            UniSetObject( uniset3::ObjectId id );
            UniSetObject();
            virtual ~UniSetObject();

            // public grpc interface
            virtual ::grpc::Status getType(::grpc::ServerContext* context, const ::uniset3::GetTypeParams* request, ::google::protobuf::StringValue* response) override;
            virtual ::grpc::Status getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request, ::google::protobuf::StringValue* response) override;
            virtual ::grpc::Status exists(::grpc::ServerContext* context, const ::uniset3::ExistsParams* request, ::google::protobuf::BoolValue* response) override;
            virtual ::grpc::Status push(::grpc::ServerContext* context, const ::uniset3::umessage::TransportMessage* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status metrics(::grpc::ServerContext* context, const ::uniset3::metrics::MetricsParams* request, ::uniset3::metrics::Metrics* response) override;
            virtual ::grpc::Status setParams(::grpc::ServerContext* context, const ::uniset3::configurator::Params* request, ::uniset3::configurator::Params* response) override;
            virtual ::grpc::Status getParams(::grpc::ServerContext* context, const ::uniset3::configurator::Params* request, ::uniset3::configurator::Params* response) override;
            virtual ::grpc::Status loadConfig(::grpc::ServerContext* context, const ::uniset3::configurator::ConfigCmdParams* request, ::grpc::ServerWriter< ::uniset3::configurator::Config>* writer) override;
            virtual ::grpc::Status reloadConfig(::grpc::ServerContext* context, const ::uniset3::configurator::ConfigCmdParams* request, ::google::protobuf::Empty* response) override;

            virtual bool isExists();
            uniset3::ObjectId getId() const;
            std::string getName() const;
            virtual std::string getStrType() const;

            // -------------- вспомогательные --------------
            /*! получить ссылку (на себя) */
            uniset3::ObjectRef getRef() const;
            std::shared_ptr<UniSetObject> get_ptr();

            /*! заказ таймера (вынесена в public, хотя должна была бы быть в protected */
            virtual timeout_t askTimer( uniset3::TimerId timerid, timeout_t timeMS, clock_t ticks = -1,
                                        uniset3::umessage::Priority p = uniset3::umessage::mpHigh ) override;

            friend std::ostream& operator<<(std::ostream& os, UniSetObject& obj );

        protected:

            std::shared_ptr<UInterface> ui; /*!< универсальный интерфейс для работы с другими процессами */
            std::string myname;

            /*! обработка приходящих сообщений */
            virtual void processingMessage( const uniset3::umessage::TransportMessage* msg );

            // конкретные виды сообщений
            virtual void sysCommand( const uniset3::umessage::SystemMessage* sm ) {}
            virtual void sensorInfo( const uniset3::umessage::SensorMessage* sm ) {}
            virtual void timerInfo( const uniset3::umessage::TimerMessage* tm ) {}
            virtual void onTextMessage( const uniset3::umessage::TextMessage* tm ) {}

            /*! Получить сообщение */
            VoidMessagePtr receiveMessage();

            /*! Ожидать сообщения заданное время */
            VoidMessagePtr waitMessage( timeout_t msec = UniSetTimer::WaitUpTime );

            /*! прервать ожидание сообщений */
            void termWaiting();

            /*! текущее количество сообщений в очереди */
            size_t countMessages();

            /*! количество потерянных сообщений */
            size_t getCountOfLostMessages() const;

            //! Активизация объекта (переопределяется для необходимых действий после активизации)
            virtual bool activateObject();

            //! Момент после активации всех объектов
            virtual bool postActivateObjects();

            //! Деактивация объекта (переопределяется для необходимых действий при завершении работы)
            virtual bool deactivateObject();

            //! Деактивация после остановки сервера
            virtual bool deactivateAfterStopServer();

            //! перед стартом grpc-сервера (регистрация сервисов)
            virtual bool initBeforeRunServer( grpc::ServerBuilder& builder );
            //! после старта grpc-сервера (известен адрес)
            virtual bool initAfterRunServer( grpc::ServerBuilder& builder, const std::string& svcAddr );

            // прерывание работы всей программы (с вызовом shutdown)
            void uterminate();

            // управление созданием потока обработки сообщений -------

            /*! запрет(разрешение) создания потока для обработки сообщений */
            void thread( bool create );

            /*! отключение потока обработки сообщений */
            void offThread();

            /*! включение потока обработки сообщений */
            void onThread();

            /*! функция вызываемая из потока */
            virtual void callback();

            // ----- конфигурирование объекта -------
            /*! установка ID объекта */
            void setID(uniset3::ObjectId id);

            /*! установить приоритет для потока обработки сообщений (если позволяют права и система) */
            void setThreadPriority( Poco::Thread::Priority p );

            /*! установка размера очереди сообщений */
            void setMaxSizeOfMessageQueue( size_t s );

            /*! получить размер очереди сообщений */
            size_t getMaxSizeOfMessageQueue() const;

            /*! проверка "активности" объекта */
            bool isActive() const;

            /*! false - завершить работу потока обработки сообщений */
            void setActive( bool set );

        private:

            friend class UniSetManager;
            friend class UniSetObjectProxy;
            friend class IOControllerProxy;
            friend class IONotifyControllerProxy;
            friend class UniSetActivator;

            /*! функция потока */
            void work();

            //! Прямая деактивизация объекта
            bool deactivate();
            //! Непосредственная активизация объекта
            bool activate();
            /* регистрация в репозитории объектов */
            void registration();
            /* удаление ссылки из репозитория объектов     */
            void unregistration();

            void waitFinish();

            void initObject();

            pid_t msgpid = { 0 }; // pid потока обработки сообщений
            bool regOK = { false };
            std::atomic_bool active;

            bool threadcreate;
            std::unique_ptr<UniSetTimer> tmr;
            uniset3::ObjectId myid;
            uniset3::ObjectRef oref;

            /*! замок для блокирования совместного доступа к oRef */
            mutable uniset3::uniset_rwmutex refmutex;

            std::unique_ptr< ThreadCreator<UniSetObject> > thr;

            /*! очереди сообщений в зависимости от приоритета */
            MQMutex mqueueLow;
            MQMutex mqueueMedium;
            MQMutex mqueueHi;

            bool a_working;
            std::mutex    m_working;
            std::condition_variable cv_working;
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
