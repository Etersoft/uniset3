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
// -----------------------------------------------------------------------------
#ifndef _MBExchange_H_
#define _MBExchange_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include "IONotifyController.h"
#include "UniSetObject.h"
#include "PassiveTimer.h"
#include "DelayTimer.h"
#include "Trigger.h"
#include "Mutex.h"
#include "Calibration.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "ThreadCreator.h"
#include "IOBase.h"
#include "VTypes.h"
#include "MTR.h"
#include "modbus/ModbusClient.h"
#include "LogAgregator.h"
#include "LogServer.h"
#include "LogAgregator.h"
#include "VMonitor.h"
#include "MBConfig.h"
// -----------------------------------------------------------------------------
#ifndef vmonit
#define vmonit( var ) vmon.add( #var, var )
#endif
// -------------------------------------------------------------------------
namespace uniset3
{
    // -----------------------------------------------------------------------------
    /*!
        \par Базовый класс для реализация обмена по протоколу Modbus [RTU|TCP].
    */
    class MBExchange:
        public UniSetObject
    {
        public:
            MBExchange( uniset3::ObjectId objId, uniset3::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                        const std::string& prefix = "mb" );
            virtual ~MBExchange();

            /*! глобальная функция для вывода help-а */
            static void help_print( int argc, const char* const* argv );

            // ----------------------------------
            enum Timer
            {
                tmExchange
            };

            void execute();

            inline std::shared_ptr<LogAgregator> getLogAggregator()
            {
                return loga;
            }
            inline std::shared_ptr<DebugStream> log()
            {
                return mblog;
            }

            virtual ::grpc::Status getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request, ::google::protobuf::StringValue* response) override;

            bool reload( const std::string& confile );

            static uint8_t firstBit( uint16_t mask );

            // offset = firstBit(mask)
            static uint16_t getBits( uint16_t value, uint16_t mask, uint8_t offset );
            // if mask = 0 return value
            static uint16_t setBits( uint16_t value, uint16_t set, uint16_t mask, uint8_t offset );
            // if mask=0 return set
            static uint16_t forceSetBits( uint16_t value, uint16_t set, uint16_t mask, uint8_t offset );

        protected:
            virtual void step();
            virtual void sysCommand( const uniset3::umessage::SystemMessage* msg ) override;
            virtual void sensorInfo( const uniset3::umessage::SensorMessage* sm ) override;
            virtual void timerInfo( const uniset3::umessage::TimerMessage* tm ) override;
            virtual void askSensors( uniset3::UIOCommand cmd );
            virtual void initOutput();
            virtual bool deactivateObject() override;
            virtual bool activateObject() override;
            virtual bool postActivateObjects() override;
            virtual void initIterators();
            virtual void initValues();
            virtual bool reconfigure( const std::shared_ptr<uniset3::UniXML>& xml, const std::shared_ptr<uniset3::MBConfig>& mbconf );

            void firstInitRegisters();
            bool preInitRead( MBConfig::InitList::iterator& p );
            bool initSMValue( ModbusRTU::ModbusData* data, int count, MBConfig::RSProperty* p );
            bool allInitOK;

            virtual std::shared_ptr<ModbusClient> initMB( bool reopen = false ) = 0;

            virtual bool poll();
            bool pollRTU( std::shared_ptr<MBConfig::RTUDevice>& dev, MBConfig::RegMap::iterator& it );

            void updateSM();

            // в функции передаётся итератор,
            // т.к. в них идёт итерирование в случае если запрос в несколько регистров
            void updateRTU(MBConfig::RegMap::iterator& it);
            void updateMTR(MBConfig::RegMap::iterator& it);
            void updateRSProperty( MBConfig::RSProperty* p, bool write_only = false );
            virtual void updateRespondSensors();

            bool isUpdateSM( bool wrFunc, long devMode ) const noexcept;
            bool isPollEnabled( bool wrFunc ) const noexcept;
            bool isSafeMode( std::shared_ptr<MBConfig::RTUDevice>& dev ) const noexcept;

            bool isProcActive() const;
            void setProcActive( bool st );
            bool waitSMReady();

            bool readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec );
            bool initItem( UniXML::iterator& it );
            void initOffsetList();
            std::string initPropPrefix( const std::string& def_prop_prefix );

            xmlNode* cnode = { 0 };
            std::shared_ptr<SMInterface> shm;

            timeout_t initPause = { 3000 };
            uniset3::uniset_rwmutex mutex_start;

            bool force =  { false };        /*!< флаг означающий, что надо сохранять в SM, даже если значение не менялось */
            bool force_out = { false };    /*!< флаг означающий, принудительного чтения выходов */

            PassiveTimer ptHeartBeat;
            uniset3::ObjectId sidHeartBeat = { uniset3::DefaultObjectId };
            long maxHeartBeat = { 10 };
            IOController::IOStateList::iterator itHeartBeat;
            uniset3::ObjectId sidTestSMReady = {uniset3::DefaultObjectId };

            uniset3::ObjectId sidExchangeMode = { uniset3::DefaultObjectId }; /*!< идентификатор для датчика режима работы */
            IOController::IOStateList::iterator itExchangeMode;
            long exchangeMode = { MBConfig::emNone }; /*!< режим работы см. ExchangeMode */

            std::atomic_bool activated = { false };
            std::atomic_bool canceled = { false };
            timeout_t activateTimeout = { 20000 }; // msec
            bool notUseExchangeTimer = { false };

            timeout_t stat_time = { 0 };      /*!< время сбора статистики обмена, 0 - отключена */
            size_t poll_count = { 0 };
            PassiveTimer ptStatistic; /*!< таймер для сбора статистики обмена */
            std::string statInfo = { "" };

            std::shared_ptr<ModbusClient> mb;

            PassiveTimer ptReopen; /*!< таймер для переоткрытия соединения */
            Trigger trReopen;

            PassiveTimer ptInitChannel; /*!< задержка не инициализацию связи */

            std::shared_ptr<LogAgregator> loga;
            std::shared_ptr<DebugStream> mblog;
            std::shared_ptr<LogServer> logserv;
            std::string logserv_host = {""};
            int logserv_port = {0};
            const std::shared_ptr<SharedMemory> ic;

            VMonitor vmon;

            size_t ncycle = { 0 }; /*!< текущий номер цикла опроса */

            std::shared_ptr<uniset3::MBConfig> mbconf;
            uniset3::uniset_rwmutex mutex_conf;

        private:
            MBExchange();

    };
    // --------------------------------------------------------------------------
} // end of namespace uniset3
// -----------------------------------------------------------------------------
#endif // _MBExchange_H_
// -----------------------------------------------------------------------------
