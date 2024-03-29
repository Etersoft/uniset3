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
#include "IOController.grpc.pb.h"
#include "UniSetTypes.h"
#include "UniSetManager.h"
#include "Configuration.h"
#include "Mutex.h"
#include "DBServer.h"
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
     * (см. также IONotifyController).
    */
    class IOController:
        public UniSetManager,
        public IOController_i::Service
    {
        public:

            IOController( const uniset3::ObjectId id );
            virtual ~IOController();

            virtual ::grpc::Status getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request, ::google::protobuf::StringValue* response) override;
            virtual std::string getStrType() const override;

            void setDBServer( const std::shared_ptr<uniset3::DBServer>& dbserver );

            // ----------------------------------------------------------------
            // Публичный (IDL) интерфейс IOController_i
            // ----------------------------------------------------------------
            virtual ::grpc::Status getValue(::grpc::ServerContext* context, const ::uniset3::GetValueParams* request, ::google::protobuf::Int64Value* response) override;
            virtual ::grpc::Status setValue(::grpc::ServerContext* context, const ::uniset3::SetValueParams* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status freezeValue(::grpc::ServerContext* context, const ::uniset3::FreezeValueParams* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status getIOType(::grpc::ServerContext* context, const ::uniset3::GetIOTypeParams* request, ::uniset3::RetIOType* response) override;
            virtual ::grpc::Status getRawValue(::grpc::ServerContext* context, const ::uniset3::GetRawValueParams* request, ::google::protobuf::Int64Value* response) override;
            virtual ::grpc::Status calibrate(::grpc::ServerContext* context, const ::uniset3::CalibrateParams* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status getCalibrateInfo(::grpc::ServerContext* context, const ::uniset3::GetCalibrateInfoParams* request, ::uniset3::CalibrateInfo* response) override;
            virtual ::grpc::Status getSensorsMap(::grpc::ServerContext* context, const ::uniset3::GetSensorsMapParams* request, ::uniset3::SensorIOInfoSeq* response) override;
            virtual ::grpc::Status getSensorIOInfo(::grpc::ServerContext* context, const ::uniset3::GetSensorIOInfoParams* request, ::uniset3::SensorIOInfo* response) override;
            virtual ::grpc::Status getSensorSeq(::grpc::ServerContext* context, const ::uniset3::GetSensorSeqParams* request, ::uniset3::SensorIOInfoSeq* response) override;
            virtual ::grpc::Status setOutputSeq(::grpc::ServerContext* context, const ::uniset3::SetOutputParams* request, ::uniset3::IDSeq* response) override;
            virtual ::grpc::Status getTimeChange(::grpc::ServerContext* context, const ::uniset3::GetTimeChangeParams* request, ::uniset3::ShortIOInfo* response) override;
            virtual ::grpc::Status getSensors(::grpc::ServerContext* context, const ::uniset3::GetSensorsParams* request, ::uniset3::ShortMapSeq* response) override;

            inline uniset3::SensorInfo SensorInfo( const uniset3::ObjectId sid,
                                                   const uniset3::ObjectId node = uniset3::uniset_conf()->getLocalNode())
            {
                uniset3::SensorInfo si;
                si.set_id(sid);
                si.set_node(node);
                return si;
            };

            uniset3::umessage::Priority getPriority( const uniset3::ObjectId id );

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

            // signal по изменению определённого датчика
            ChangeSignal signal_change_value( uniset3::ObjectId sid );

            // signal по изменению любого датчика
            ChangeSignal signal_change_value();

            // -----------------------------------------------------------------------------------------
            // полнейшее нарушение инкапсуляции
            // но пока, это попытка оптимизировать работу с IOController через указатель.
            // Т.е. работая с датчиками через итераторы..
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

            inline int ioCount() const noexcept
            {
                return ioList.size();
            }

        protected:

            // доступ к элементам через итератор
            // return итоговое значение
            virtual long localSetValueIt( IOStateList::iterator& it, const uniset3::ObjectId sid,
                                          long value, uniset3::ObjectId sup_id );

            virtual long localGetValue( IOStateList::iterator& it, const uniset3::ObjectId sid );

            virtual void localFreezeValueIt( IOController::IOStateList::iterator& li,
                                             uniset3::ObjectId sid,
                                             bool set,
                                             long value,
                                             uniset3::ObjectId sup_id );

            virtual void localFreezeValue( std::shared_ptr<USensorInfo>& usi,
                                           bool set,
                                           long value,
                                           uniset3::ObjectId sup_id );


            // -- работа через указатель ---
            virtual long localSetValue( std::shared_ptr<USensorInfo>& usi, long value, uniset3::ObjectId sup_id );
            long localGetValue( std::shared_ptr<USensorInfo>& usi) ;

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
                         uniset3::umessage::Priority p = uniset3::umessage::mpMedium,
                         long defval = 0, uniset3::CalibrateInfo* ci = 0,
                         uniset3::ObjectId sup_id = uniset3::DefaultObjectId,
                         uniset3::ObjectId depend_sid = uniset3::DefaultObjectId )
            {
                uniset3::SensorIOInfo ai;
                *(ai.mutable_si()) = si;
                ai.set_type(t);
                ai.set_value(v);
                ai.set_priority(p);
                ai.set_default_val(defval);
                ai.set_real_value(v);
                ai.set_blocked(false);
                ai.set_supplier(sup_id);
                ai.set_depend_sid(depend_sid);

                if( ci != 0 )
                    *ai.mutable_ci() = *ci;
                else
                {
                    ai.mutable_ci()->set_minraw(0);
                    ai.mutable_ci()->set_maxraw(0);
                    ai.mutable_ci()->set_mincal(0);
                    ai.mutable_ci()->set_maxcal(0);
                    ai.mutable_ci()->set_precision(0);
                }

                return ai;
            };

            //! сохранение информации об изменении состояния датчика
            virtual void logging( uniset3::umessage::SensorMessage& sm );

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
            friend class IOControllerProxy;

            std::mutex siganyMutex;
            ChangeSignal sigAnyChange;

            InitSignal sigInit;

            IOStateList ioList;    /*!< список с текущим состоянием аналоговых входов/выходов */
            uniset3::uniset_rwmutex ioMutex; /*!< замок для блокирования совместного доступа к ioList */

            bool isPingDBServer;    // флаг связи с DBServer-ом
            uniset3::ObjectId dbserverID = { uniset3::DefaultObjectId };
            std::shared_ptr<uniset3::DBServer> dbserver = { nullptr };
            ::google::protobuf::Empty empty;

            std::mutex loggingMutex; /*!< logging info mutex */

        public:

            /*! Состояние порогового датчика */
            enum ThresholdState
            {
                LowThreshold,       /*!< сработал нижний порог  (значение меньше нижнего) */
                NormalThreshold,    /*!< значение в заданных пределах (не достигли порога) */
                HiThreshold         /*!< сработал верхний порог  (значение больше верхнего) */
            };

            struct UThresholdInfo;
            typedef std::list<std::shared_ptr<UThresholdInfo>> ThresholdExtList;

            struct USensorInfo
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

                uniset3::SensorIOInfo sinf;

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

                long d_value = { 1 }; /*!< разрешающее работу значение датчика от которого зависит данный */
                long d_off_value = { 0 }; /*!< блокирующее значение */
                std::shared_ptr<USensorInfo> d_usi; // shared_ptr на датчик от которого зависит этот.

                // список пороговых датчиков для данного
                uniset3::uniset_rwmutex tmut;
                ThresholdExtList thresholds;

                size_t nchanges = { 0 }; // количество изменений датчика

                long frozen_value = { 0 };

                bool readonly = { false }; // readonly датчик

                // функция обработки информации об изменении состояния датчика, от которого зависит данный
                void checkDepend( std::shared_ptr<USensorInfo>& d_usi, IOController* );

                void init( const uniset3::SensorIOInfo& s );

                inline uniset3::SensorIOInfo makeSensorIOInfo()
                {
                    uniset3::uniset_rwmutex_rlock lock(val_lock);
                    return sinf;
                }

                inline uniset3::umessage::SensorMessage makeSensorMessage( bool with_lock = false )
                {
                    uniset3::umessage::SensorMessage sm;
                    auto header = sm.mutable_header();
                    header->set_priority((uniset3::umessage::Priority)sinf.priority());
                    header->set_node(sinf.si().node()); // uniset_conf()->getLocalNode());
                    header->set_supplier(sinf.supplier());
                    auto ts = uniset3::now_to_uniset_timespec();
                    (*header->mutable_ts()) = ts;

                    sm.set_id(sinf.si().id());
                    sm.set_sensor_type(sinf.type());

                    if( with_lock )
                    {
                        uniset3::uniset_rwmutex_rlock lock(val_lock);
                        sm.set_value(sinf.value());
                        *sm.mutable_sm_ts() = sinf.ts();
                        *sm.mutable_ci() = sinf.ci();
                    }
                    else
                    {
                        sm.set_value(sinf.value());
                        *sm.mutable_sm_ts() = sinf.ts();
                        *sm.mutable_ci() = sinf.ci();
                    }

                    return sm;
                }
            };

            /*! Информация о пороговом значении */
            struct UThresholdInfo
            {
                UThresholdInfo( uniset3::ObjectId _sid, long low, long hi, bool inv ):
                    hilimit(hi),
                    lowlimit(low),
                    state(NormalThreshold),
                    invert(inv),
                    sid(_sid)
                {
                }

                long hilimit;           /*!< верхняя граница срабатывания */
                long lowlimit;          /*!< нижняя граница срабатывания */
                ThresholdState state;
                unsigned long tv_sec;   /*!< время последнего изменения датчика, секунды (clock_gettime(CLOCK_REALTIME) */
                unsigned long tv_nsec;  /*!< время последнего изменения датчика, nanosec (clock_gettime(CLOCK_REALTIME) */
                bool invert;         /*!< инвертированная логика */

                /*! идентификатор дискретного датчика связанного с данным порогом */
                uniset3::ObjectId sid;

                /*! итератор в списке датчиков (для быстрого доступа) */
                IOController::IOStateList::iterator sit;

                inline bool operator== ( const UThresholdInfo& r ) const
                {
                    return ((sid == r.sid) &&
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
