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
 * \brief Универсальный интерфейс для взаимодействия с объектами системы
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef UInterface_H_
#define UInterface_H_
// ---------------------------------------------------------------------------
#include <memory>
#include <string>
#include <atomic>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <grpcpp/grpcpp.h>
#include "Exceptions.h"
#include "UniSetTypes.h"
#include "ObjectIndex.h"
#include "URepository.grpc.pb.h"
#include "IOController.pb.h"
#include "MessageTypes.pb.h"
#include "Configuration.h"
#ifndef DISABLE_REST_API
#include "UHttpClient.h"
#endif
// -----------------------------------------------------------------------------------------
namespace uniset3
{
    /*!
     * \class UInterface
     * Универсальный интерфейс для взаимодействия между объектами (процессами).
     * По сути является "фасадом" к реализации механизма взаимодействия
     * в libuniset (основанном на CORBA) Хотя до конца скрыть CORBA-у пока не удалось.
     * Для увеличения производительности в функции встроен cache обращений...
     *
     * См. также \ref UniversalIOControllerPage
    */
    class UInterface
    {
        public:

            UInterface( const uniset3::ObjectId backid, const std::shared_ptr<uniset3::ObjectIndex> oind = nullptr );
            UInterface( const std::shared_ptr<uniset3::Configuration>& uconf = uniset3::uniset_conf() );
            ~UInterface();

            // ---------------------------------------------------------------
            // Работа с датчиками

            //! Получение состояния датчика
            long getValue (const uniset3::ObjectId id, const uniset3::ObjectId node) const;
            long getValue ( const uniset3::ObjectId id ) const;
            long getRawValue( const uniset3::SensorInfo& si );

            //! Выставление состояния датчика
            void setValue ( const uniset3::ObjectId id, long value, const uniset3::ObjectId node, uniset3::ObjectId sup_id = uniset3::DefaultObjectId ) const;
            void setValue ( const uniset3::ObjectId id, long value ) const;
            void setValue ( const uniset3::SensorInfo& si, long value, const uniset3::ObjectId supplier ) const;

            //! Получение состояния для списка указанных датчиков
            uniset3::SensorIOInfoSeq getSensorSeq( const uniset3::IDList& lst );

            //! Получение состояния информации о датчике
            uniset3::SensorIOInfo getSensorIOInfo( const uniset3::SensorInfo& si );

            /*! Изменения состояния списка входов/выходов
                \return Возвращает список не найденных идентификаторов */
            uniset3::IDSeq setOutputSeq( const uniset3::OutSeq& lst, uniset3::ObjectId sup_id );

            // ---------------------------------------------------------------
            // Заказ датчиков

            //! Универсальный заказ информации об изменении датчика
            void askSensor( const uniset3::ObjectId id, uniset3::UIOCommand cmd,
                            uniset3::ObjectId backid = uniset3::DefaultObjectId ) const;

            void askRemoteSensor( const uniset3::ObjectId id, uniset3::UIOCommand cmd, const uniset3::ObjectId node,
                                  uniset3::ObjectId backid = uniset3::DefaultObjectId ) const;

            //! Заказ по списку
            uniset3::IDSeq askSensorsSeq( const uniset3::IDList& lst, uniset3::UIOCommand cmd,
                                          uniset3::ObjectId backid = uniset3::DefaultObjectId );
            // ------------------------------------------------------

            // установка неопределённого состояния
            void setUndefinedState( const uniset3::SensorInfo& si, bool undefined, uniset3::ObjectId supplier );

            // заморозка значения (выставить указанный value и не менять)
            void freezeValue( const uniset3::SensorInfo& si, bool set, long value, uniset3::ObjectId supplier = uniset3::DefaultObjectId );
            // ---------------------------------------------------------------
            // Калибровка... пороги...

            //! калибровка
            void calibrate(const uniset3::SensorInfo& si,
                           const uniset3::CalibrateInfo& ci,
                           uniset3::ObjectId adminId = uniset3::DefaultObjectId );

            uniset3::CalibrateInfo getCalibrateInfo( const uniset3::SensorInfo& si );

            //! Заказ информации об изменении порога
            void askThreshold( const uniset3::ObjectId sensorId, const uniset3::ThresholdId tid,
                               uniset3::UIOCommand cmd,
                               long lowLimit, long hiLimit, bool invert = false,
                               uniset3::ObjectId backid = uniset3::DefaultObjectId ) const;

            void askRemoteThreshold( const uniset3::ObjectId sensorId, const uniset3::ObjectId node,
                                     const uniset3::ThresholdId thresholdId, uniset3::UIOCommand cmd,
                                     long lowLimit, long hiLimit, bool invert = false,
                                     uniset3::ObjectId backid = uniset3::DefaultObjectId ) const;

            uniset3::ThresholdInfo getThresholdInfo( const uniset3::SensorInfo& si, const uniset3::ThresholdId tid ) const;
            uniset3::ThresholdInfo getThresholdInfo( const uniset3::ObjectId sid, const uniset3::ThresholdId tid ) const;

            // ---------------------------------------------------------------
            // Вспомогательные функции

            uniset3::IOType getIOType(const uniset3::ObjectId id, uniset3::ObjectId node) const;
            uniset3::IOType getIOType(const uniset3::ObjectId id) const;

            // read from xml (only for xml!) т.е. без удалённого запроса
            uniset3::IOType getConfIOType( const uniset3::ObjectId id ) const noexcept;

            // Получение типа объекта..
            uniset3::ObjectType getType(const uniset3::ObjectId id, const uniset3::ObjectId node) const;
            uniset3::ObjectType getType(const uniset3::ObjectId id) const;

            //! Время последнего изменения датчика
            uniset3::ShortIOInfo getTimeChange( const uniset3::ObjectId id, const uniset3::ObjectId node ) const;

            //! Информация об объекте
            std::string getObjectInfo( const uniset3::ObjectId id, const std::string& params, const uniset3::ObjectId node ) const;
            std::string apiRequest( const uniset3::ObjectId id, const std::string& query, const uniset3::ObjectId node ) const;

            //! Получить список датчиков
            uniset3::ShortMapSeq getSensors( const uniset3::ObjectId id,
                                             const uniset3::ObjectId node = uniset3::uniset_conf()->getLocalNode() );

            uniset3::SensorIOInfoSeq getSensorsMap( const uniset3::ObjectId id,
                                                    const uniset3::ObjectId node = uniset3::uniset_conf()->getLocalNode() );

            uniset3::ThresholdsListSeq getThresholdsList( const uniset3::ObjectId id,
                    const uniset3::ObjectId node = uniset3::uniset_conf()->getLocalNode() );
            // ---------------------------------------------------------------
            // Работа с репозиторием

            /*! регистрация объекта в репозитории
             *  throw(uniset3::ORepFailed)
             */
            void registered(const uniset3::ObjectRef oRef, bool force = false) const;

            // throw(uniset3::ORepFailed)
            void unregister(const uniset3::ObjectId id);

            std::shared_ptr<grpc::Channel> resolve( const uniset3::ObjectId id ) const;

            // throw(uniset3::ResolveNameError, uniset3::TimeOut);
            std::shared_ptr<grpc::Channel> resolve(const uniset3::ObjectId id, const uniset3::ObjectId node) const;

            // Проверка доступности объекта или датчика
            bool isExists( const uniset3::ObjectId id ) const noexcept;
            bool isExists( const uniset3::ObjectId id, const uniset3::ObjectId node ) const noexcept;

            //! used for check 'isExist' \deprecated! Use waitReadyWithCancellation(..)
            bool waitReady( const uniset3::ObjectId id, int msec, int pause = 5000,
                            const uniset3::ObjectId node = uniset3::uniset_conf()->getLocalNode() ) noexcept;

            //! used for check 'getValue'
            bool waitWorking( const uniset3::ObjectId id, int msec, int pause = 3000,
                              const uniset3::ObjectId node = uniset3::uniset_conf()->getLocalNode() ) noexcept;

            bool waitReadyWithCancellation( const uniset3::ObjectId id, int msec, std::atomic_bool& cancelFlag, int pause = 5000,
                                            const uniset3::ObjectId node = uniset3::uniset_conf()->getLocalNode() ) noexcept;

            // ---------------------------------------------------------------
            // Работа с ID, Name

            /*! получение идентификатора объекта по имени */
            inline uniset3::ObjectId getIdByName( const std::string& name ) const noexcept
            {
                return oind->getIdByName(name);
            }

            /*! получение имени по идентификатору объекта */
            inline std::string getNameById( const uniset3::ObjectId id ) const noexcept
            {
                return oind->getNameById(id);
            }

            inline uniset3::ObjectId getNodeId( const std::string& fullname ) const noexcept
            {
                return oind->getNodeId(fullname);
            }

            inline std::string getTextName( const uniset3::ObjectId id ) const noexcept
            {
                return oind->getTextName(id);
            }

            // ---------------------------------------------------------------
            // Получение указателей на вспомогательные классы.
            inline std::shared_ptr<uniset3::ObjectIndex> getObjectIndex() noexcept
            {
                return oind;
            }
            inline std::shared_ptr<uniset3::Configuration> getConf() noexcept
            {
                return uconf;
            }
            // ---------------------------------------------------------------
            // Посылка сообщений

            /*! посылка сообщения msg объекту name на узел node */
            void send(const uniset3::umessage::TransportMessage& msg, uniset3::ObjectId node = uniset3::DefaultObjectId);
            void sendText(const uniset3::umessage::TextMessage& msg, uniset3::ObjectId node = uniset3::DefaultObjectId);
            void sendText(const uniset3::ObjectId name, const std::string& text, int mtype, const uniset3::ObjectId node = uniset3::DefaultObjectId );

            // ---------------------------------------------------------------
            // Вспомогательный класс для кэширования ссылок на удалённые объекты

            inline void setCacheMaxSize( size_t newsize ) noexcept
            {
                rcache.setMaxSize(newsize);
            }

            /*! Кэш ссылок на объекты */
            class CacheOfResolve
            {
                public:
                    CacheOfResolve( size_t maxsize, size_t cleancount = 20 ):
                        MaxSize(maxsize), minCallCount(cleancount)  {} ;
                    ~CacheOfResolve() {};

                    //  throw(uniset3::NameNotFound, uniset3::SystemError)
                    std::shared_ptr<grpc::Channel> resolve( const uniset3::ObjectId id, const uniset3::ObjectId node ) const;

                    void cache(const uniset3::ObjectId id, const uniset3::ObjectId node, std::shared_ptr<grpc::Channel>& chan ) const;
                    void erase( const uniset3::ObjectId id, const uniset3::ObjectId node ) const noexcept;

                    inline void setMaxSize( size_t ms ) noexcept
                    {
                        MaxSize = ms;
                    };

                protected:
                    CacheOfResolve() {};

                private:

                    bool clean() noexcept;       /*!< функция очистки кэш-а от старых ссылок */
                    inline void clear() noexcept /*!< удаление всей информации */
                    {
                        uniset3::uniset_rwmutex_wrlock l(cmutex);
                        mcache.clear();
                    };

                    struct Item
                    {
                        Item( std::shared_ptr<grpc::Channel>& c ): chan(c), ncall(0) {}
                        Item(): chan(nullptr), ncall(0) {}

                        std::shared_ptr<grpc::Channel> chan;
                        size_t ncall; // счётчик обращений

                        bool operator<( const CacheOfResolve::Item& rhs ) const
                        {
                            return this->ncall > rhs.ncall;
                        }
                    };

                    typedef std::unordered_map<uniset3::KeyType, Item> CacheMap;
                    mutable CacheMap mcache;
                    mutable uniset3::uniset_rwmutex cmutex;
                    size_t MaxSize = { 20 };      /*!< максимальный размер кэша */
                    size_t minCallCount = { 20 }; /*!< минимальное количество вызовов, меньше которого ссылка считается устаревшей */
            };

            void initBackId( uniset3::ObjectId backid );

        protected:
            std::string set_err(const std::string& pre, const uniset3::ObjectId id, const uniset3::ObjectId node) const;
            std::string httpResolve( const uniset3::ObjectId id, const uniset3::ObjectId node ) const;
            std::shared_ptr<grpc::Channel> resolveRepository( uniset3::ObjectId node ) const;

        private:
            void init();

            mutable std::shared_ptr<grpc::Channel> rep;
            uniset3::ObjectId myid;
            CacheOfResolve rcache;
            std::shared_ptr<uniset3::ObjectIndex> oind;
            std::shared_ptr<uniset3::Configuration> uconf;
#ifndef DISABLE_REST_API
            mutable UHttp::UHttpClient resolver;
#endif
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
