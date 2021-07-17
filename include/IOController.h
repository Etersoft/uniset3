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
 * \brief Реализация IOController_i
 * \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef IOController_H_
#define IOController_H_
//---------------------------------------------------------------------------
#include <unordered_map>
#include <list>
#include <limits>
#include <sigc++/sigc++.h>
#include "IOController_i.hh"
#include "UniSetTypes.h"
#include "UniSetManager.h"
#include "Configuration.h"
#include "Mutex.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    /*! Реализация интерфейса IOController-а
     * Важной особенностью данной реализации является то, что
     * список входов/выходов (ioList) формируется один раз во время создания объекта
     * и не меняется (!) в процессе работы. На этом построены некоторые оптимизации!
     * Поэтому неизменность ioList во время всей жизни объекта должна гарантироваться.
     * В частности, очень важной является структура USensorInfo, а также userdata,
     * которые используются для "кэширования" (сохранения) указателей на специальные данные.
     * (см. также IONotifyContoller).
    */
    class IOController:
        public UniSetManager,
        public POA_IOController_i
    {
        public:

            IOController( const std::string& name, const std::string& section );
            IOController( const uniset3::ObjectId id );
            virtual ~IOController();

            virtual uniset3::ObjectType getType() override
            {
                return uniset3::ObjectType("IOController");
            }

            virtual uniset3::SimpleInfo* getInfo( const char* userparam = "" ) override;

            // ----------------------------------------------------------------
            // Публичный (IDL) интерфейс IOController_i
            // ----------------------------------------------------------------

            virtual CORBA::Long getValue( uniset3::ObjectId sid ) override;

            virtual void setValue( uniset3::ObjectId sid, CORBA::Long value,
                                   uniset3::ObjectId sup_id = uniset3::DefaultObjectId ) override;
            virtual void setUndefinedState( uniset3::ObjectId sid,
                                            CORBA::Boolean undefined,
                                            uniset3::ObjectId sup_id = uniset3::DefaultObjectId ) override;

            virtual void freezeValue( uniset3::ObjectId sid,
                                      CORBA::Boolean set,
                                      CORBA::Long value,
                                      uniset3::ObjectId sup_id = uniset3::DefaultObjectId ) override;

            virtual uniset3::SensorInfoSeq* getSensorSeq( const uniset3::IDSeq& lst ) override;
            virtual uniset3::IDSeq* setOutputSeq( const uniset3::OutSeq& lst, uniset3::ObjectId sup_id ) override;

            //     ----------------------------------------------------------------
            virtual uniset3::IOType getIOType( uniset3::ObjectId sid ) override;

            virtual uniset3::SensorInfoSeq* getSensorsMap() override;
            virtual uniset3::SensorIOInfo getSensorIOInfo( uniset3::ObjectId sid ) override;

            virtual CORBA::Long getRawValue( uniset3::ObjectId sid ) override;
            virtual void calibrate( uniset3::ObjectId sid,
                                    const uniset3::CalibrateInfo& ci,
                                    uniset3::ObjectId adminId ) override;

            uniset3::CalibrateInfo getCalibrateInfo( uniset3::ObjectId sid ) override;

            inline uniset3::SensorInfo SensorInfo( const uniset3::ObjectId sid,
                    const uniset3::ObjectId node = uniset3::uniset_conf()->getLocalNode())
            {
                uniset3::SensorInfo si;
                si.id = sid;
                si.node = node;
                return si;
            };

            uniset3::Message::Priority getPriority( const uniset3::ObjectId id );

            virtual uniset3::ShortIOInfo getTimeChange( const uniset3::ObjectId id ) override;

            virtual uniset3::ShortMapSeq* getSensors() override;

#ifndef DISABLE_REST_API
            // http API
            virtual Poco::JSON::Object::Ptr httpHelp( const Poco::URI::QueryParameters& p ) override;
            virtual Poco::JSON::Object::Ptr httpRequest( const std::string& req, const Poco::URI::QueryParameters& p ) override;
#endif

        public:

            // предварительное объявление..
            struct USensorInfo;
            typedef std::unordered_map<uniset3::ObjectId, std::shared_ptr<USensorInfo>> IOStateList;

            static const long not_specified_value = { std::numeric_limits<long>::max() };

            // ================== Доступные сигналы =================
            /*!
            // \warning  В сигнале напрямую передаётся указатель на внутреннюю структуру!
            // Это не очень хорошо, с точки зрения "архитектуры", но оптимальнее по быстродействию!
            // необходимо в обработчике не забывать использовать uniset_rwmutex_wrlock(val_lock) или uniset_rwmutex_rlock(val_lock)
            */
            typedef sigc::signal<void, std::shared_ptr<USensorInfo>&, IOController*> ChangeSignal;
            typedef sigc::signal<void, std::shared_ptr<USensorInfo>&, IOController*> ChangeUndefinedStateSignal;

            // signal по изменению определённого датчика
            ChangeSignal signal_change_value( uniset3::ObjectId sid );

            // signal по изменению любого датчика
            ChangeSignal signal_change_value();

            // сигналы по изменению флага "неопределённое состояние" (обрыв датчика например)
            ChangeUndefinedStateSignal signal_change_undefined_state( uniset3::ObjectId sid );
            ChangeUndefinedStateSignal signal_change_undefined_state();
            // -----------------------------------------------------------------------------------------
            // полнейшее нарушение инкапсуляции
            // но пока, это попытка оптимизировать работу с IOController через указатель.
            // Т.е. работая с датчиками через итераторы..
#if 1
            inline IOStateList::iterator ioBegin()
            {
                return ioList.begin();
            }
            inline IOStateList::iterator ioEnd()
            {
                return ioList.end();
            }
            inline IOStateList::iterator find( uniset3::ObjectId k )
            {
                return ioList.find(k);
            }
#endif
            inline int ioCount() const noexcept
            {
                return ioList.size();
            }

        protected:

            // доступ к элементам через итератор
            // return итоговое значение
            virtual long localSetValueIt( IOStateList::iterator& it, const uniset3::ObjectId sid,
                                          CORBA::Long value, uniset3::ObjectId sup_id );

            virtual long localGetValue( IOStateList::iterator& it, const uniset3::ObjectId sid );

            /*! функция выставления признака неопределённого состояния для аналоговых датчиков
                // для дискретных датчиков необходимости для подобной функции нет.
                // см. логику выставления в функции localSaveState
            */
            virtual void localSetUndefinedState( IOStateList::iterator& it, bool undefined,
                                                 const uniset3::ObjectId sid );

            virtual void localFreezeValueIt( IOController::IOStateList::iterator& li,
                                             uniset3::ObjectId sid,
                                             CORBA::Boolean set,
                                             CORBA::Long value,
                                             uniset3::ObjectId sup_id );

            virtual void localFreezeValue( std::shared_ptr<USensorInfo>& usi,
                                           CORBA::Boolean set,
                                           CORBA::Long value,
                                           uniset3::ObjectId sup_id );


            // -- работа через указатель ---
            virtual long localSetValue( std::shared_ptr<USensorInfo>& usi, CORBA::Long value, uniset3::ObjectId sup_id );
            long localGetValue( std::shared_ptr<USensorInfo>& usi) ;

#ifndef DISABLE_REST_API
            // http API
            virtual Poco::JSON::Object::Ptr request_get( const std::string& req, const Poco::URI::QueryParameters& p );
            virtual Poco::JSON::Object::Ptr request_sensors( const std::string& req, const Poco::URI::QueryParameters& p );
            void getSensorInfo( Poco::JSON::Array::Ptr& jdata, std::shared_ptr<USensorInfo>& s, bool shortInfo = false );
#endif

            // переопределяем для добавления вызова регистрации датчиков
            virtual bool deactivateObject() override;
            virtual bool activateObject() override;

            /*! Начальная инициализация (выставление значений) */
            virtual void activateInit();

            /*! регистрация датчиков, за информацию о которых отвечает данный IOController */
            virtual void sensorsRegistration() {};
            /*! удаление из репозитория датчиков за информацию о которых отвечает данный IOController */
            virtual void sensorsUnregistration();

            typedef sigc::signal<void, std::shared_ptr<USensorInfo>&, IOController*> InitSignal;

            // signal по изменению определённого датчика
            InitSignal signal_init();

            /*! регистрация датчика в репозитории */
            void ioRegistration(std::shared_ptr<USensorInfo>& usi );

            /*! разрегистрация датчика */
            void ioUnRegistration( const uniset3::ObjectId sid );

            // ------------------------------
            inline uniset3::SensorIOInfo
            SensorIOInfo(long v, uniset3::IOType t, const uniset3::SensorInfo& si,
                         uniset3::Message::Priority p = uniset3::Message::Medium,
                         long defval = 0, uniset3::CalibrateInfo* ci = 0,
                         uniset3::ObjectId sup_id = uniset3::DefaultObjectId,
                         uniset3::ObjectId depend_sid = uniset3::DefaultObjectId )
            {
                uniset3::SensorIOInfo ai;
                ai.si = si;
                ai.type = t;
                ai.value = v;
                ai.priority = p;
                ai.default_val = defval;
                ai.real_value = v;
                ai.blocked = false;
                ai.supplier = sup_id;
                ai.depend_sid = depend_sid;

                if( ci != 0 )
                    ai.ci = *ci;
                else
                {
                    ai.ci.minRaw = 0;
                    ai.ci.maxRaw = 0;
                    ai.ci.minCal = 0;
                    ai.ci.maxCal = 0;
                    ai.ci.precision = 0;
                }

                return ai;
            };

            //! сохранение информации об изменении состояния датчика
            virtual void logging( uniset3::SensorMessage& sm );

            //! сохранение состояния всех датчиков в БД
            virtual void dumpToDB();

            IOController();

            // доступ к списку c изменением только для своих
            IOStateList::iterator myioBegin();
            IOStateList::iterator myioEnd();
            IOStateList::iterator myiofind( uniset3::ObjectId id );

            void initIOList( const IOStateList&& l );

            typedef std::function<void(std::shared_ptr<USensorInfo>&)> UFunction;
            // функция работает с mutex
            void for_iolist( UFunction f );

        private:
            friend class NCRestorer;
            friend class SMInterface;

            std::mutex siganyMutex;
            ChangeSignal sigAnyChange;

            std::mutex siganyundefMutex;
            ChangeSignal sigAnyUndefChange;
            InitSignal sigInit;

            IOStateList ioList;    /*!< список с текущим состоянием аналоговых входов/выходов */
            uniset3::uniset_rwmutex ioMutex; /*!< замок для блокирования совместного доступа к ioList */

            bool isPingDBServer;    // флаг связи с DBServer-ом
            uniset3::ObjectId dbserverID = { uniset3::DefaultObjectId };

            std::mutex loggingMutex; /*!< logging info mutex */

        public:

            struct UThresholdInfo;
            typedef std::list<std::shared_ptr<UThresholdInfo>> ThresholdExtList;

            struct USensorInfo:
                public uniset3::SensorIOInfo
            {
                USensorInfo( const USensorInfo& ) = delete;
                const USensorInfo& operator=(const USensorInfo& ) = delete;
                USensorInfo( USensorInfo&& ) = default;
                USensorInfo& operator=(USensorInfo&& ) = default;

                USensorInfo();
                virtual ~USensorInfo() {}

                USensorInfo(uniset3::SensorIOInfo& r);
                USensorInfo(uniset3::SensorIOInfo* r);
                USensorInfo(const uniset3::SensorIOInfo& r);

                USensorInfo& operator=(uniset3::SensorIOInfo& r);
                const USensorInfo& operator=(const uniset3::SensorIOInfo& r);
                USensorInfo& operator=(uniset3::SensorIOInfo* r);

                // Дополнительные (вспомогательные поля)
                uniset3::uniset_rwmutex val_lock; /*!< флаг блокирующий работу со значением */

                // userdata (универсальный, но небезопасный способ расширения информации связанной с датчиком)
                static const size_t MaxUserData = 4;
                void* userdata[MaxUserData] = { nullptr, nullptr, nullptr, nullptr }; /*!< расширение для возможности хранения своей информации */
                uniset3::uniset_rwmutex userdata_lock; /*!< mutex для работы с userdata */

                void* getUserData( size_t index );
                void setUserData( size_t index, void* data );

                // сигнал для реализации механизма зависимостей..
                // (все зависимые датчики подключаются к нему (см. NCRestorer::init_depends_signals)
                uniset3::uniset_rwmutex changeMutex;
                ChangeSignal sigChange;

                uniset3::uniset_rwmutex undefMutex;
                ChangeUndefinedStateSignal sigUndefChange;

                long d_value = { 1 }; /*!< разрешающее работу значение датчика от которого зависит данный */
                long d_off_value = { 0 }; /*!< блокирующее значение */
                std::shared_ptr<USensorInfo> d_usi; // shared_ptr на датчик от которого зависит этот.

                // список пороговых датчиков для данного
                uniset3::uniset_rwmutex tmut;
                ThresholdExtList thresholds;

                size_t nchanges = { 0 }; // количество изменений датчика

                long undef_value = { not_specified_value }; // значение для "неопределённого состояния датчика"
                long frozen_value = { 0 };

                // функция обработки информации об изменении состояния датчика, от которого зависит данный
                void checkDepend( std::shared_ptr<USensorInfo>& d_usi, IOController* );

                void init( const uniset3::SensorIOInfo& s );

                inline uniset3::SensorIOInfo makeSensorIOInfo()
                {
                    uniset3::uniset_rwmutex_rlock lock(val_lock);
                    uniset3::SensorIOInfo s(*this);
                    return s;
                }

                inline uniset3::SensorMessage makeSensorMessage( bool with_lock = false )
                {
                    uniset3::SensorMessage sm;
                    sm.id           = si.id;
                    sm.node         = si.node; // uniset_conf()->getLocalNode()?
                    sm.sensor_type  = type;
                    sm.priority     = (uniset3::Message::Priority)priority;

                    // лочим только изменяемые поля
                    if( with_lock )
                    {
                        uniset3::uniset_rwmutex_rlock lock(val_lock);
                        sm.value        = value;
                        sm.sm_tv.tv_sec    = tv_sec;
                        sm.sm_tv.tv_nsec   = tv_nsec;
                        sm.ci           = ci;
                        sm.supplier     = supplier;
                        sm.undefined    = undefined;
                    }
                    else
                    {
                        sm.value        = value;
                        sm.sm_tv.tv_sec    = tv_sec;
                        sm.sm_tv.tv_nsec   = tv_nsec;
                        sm.ci           = ci;
                        sm.supplier     = supplier;
                        sm.undefined    = undefined;
                    }

                    return sm;
                }
            };

            /*! Информация о пороговом значении */
            struct UThresholdInfo:
                public uniset3::ThresholdInfo
            {
                UThresholdInfo( uniset3::ThresholdId tid, CORBA::Long low, CORBA::Long hi, bool inv,
                                uniset3::ObjectId _sid = uniset3::DefaultObjectId ):
                    sid(_sid),
                    invert(inv)
                {
                    id       = tid;
                    hilimit  = hi;
                    lowlimit = low;
                    state    = uniset3::NormalThreshold;
                }

                /*! идентификатор дискретного датчика связанного с данным порогом */
                uniset3::ObjectId sid;

                /*! итератор в списке датчиков (для быстрого доступа) */
                IOController::IOStateList::iterator sit;

                /*! инверсная логика */
                bool invert;

                inline bool operator== ( const ThresholdInfo& r ) const
                {
                    return ((id == r.id) &&
                            (hilimit == r.hilimit) &&
                            (lowlimit == r.lowlimit) &&
                            (invert == r.invert) );
                }

                UThresholdInfo( const UThresholdInfo& ) = delete;
                UThresholdInfo& operator=( const UThresholdInfo& ) = delete;
                UThresholdInfo( UThresholdInfo&& ) = default;
                UThresholdInfo& operator=(UThresholdInfo&& ) = default;
            };
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
