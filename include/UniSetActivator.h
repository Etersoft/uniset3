/*
 * Copyright (c) 2021 Pavel Vainerman.
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
 * \brief Активатор объектов
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef UniSetActivator_H_
#define UniSetActivator_H_
// --------------------------------------------------------------------------
#include <memory>
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "UniSetObjectProxy.h"
#include "UniSetManagerProxy.h"
#include "IOControllerProxy.h"
#include "IONotifyControllerProxy.h"
#include "MetricsExporterProxy.h"
//----------------------------------------------------------------------------------------
namespace uniset3
{
    /*! \page pg_Act Активатор объектов
     *
     * Активатор объектов предназначен для запуска, после которого объекты становятся доступны для удалённого
     * вызова.
     **/
    //----------------------------------------------------------------------------------------
    class UniSetActivator;
    typedef std::shared_ptr<UniSetActivator> UniSetActivatorPtr;
    //----------------------------------------------------------------------------------------
    /*! \class UniSetActivator
     *  Создаёт и запускает grpc-сервисы (прокси), которые перенаправляют запросы конкретным объектам
     *  \warning Активатор может быть создан только один. Для его создания используйте код:
     \code
         ...
         auto act = UniSetActivator::Instance()
         ...
    \endcode
    */
    class UniSetActivator:
        public std::enable_shared_from_this<UniSetActivator>
    {
        public:

            static UniSetActivatorPtr Instance();

            virtual ~UniSetActivator();

            bool add( const std::shared_ptr<UniSetObject>& obj );
            void startup();
            bool isExists() const noexcept;

            // запуск системы
            // async = true - асинхронный запуск (создаётся отдельный поток).
            // terminate_control = true - управление процессом завершения (обработка сигналов завершения)
            void run( bool async, bool terminate_control = true );

            // штатное завершение работы
            void shutdown();

            // ожидание завершения (если был запуск run(true))
            void join();

            // прерывание работы
            void terminate();

        protected:

            // уносим в protected, т.к. Activator должен быть только один..
            UniSetActivator();

            static std::shared_ptr<UniSetActivator> inst;

        private:
            void init();
            static void on_finish_timeout();
            static void set_signals( bool set );

            std::atomic_bool active = { false };
            grpc::ServerBuilder builder;
            std::unique_ptr<grpc::Server> server;
            int grpcPort = { 0 };
            std::string grpcHost;
            bool termControl = { true };
            std::string myname;

            mutable uniset3::uniset_rwmutex omutex;
            std::unordered_map<ObjectId, std::shared_ptr<UniSetObject>> objects;
            uniset3::UniSetObjectProxy oproxy;
            uniset3::UniSetManagerProxy mproxy;
            uniset3::IOControllerProxy ioproxy;
            uniset3::IONotifyControllerProxy ionproxy;
            uniset3::MetricsExporterProxy metricsproxy;
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
//----------------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------------
