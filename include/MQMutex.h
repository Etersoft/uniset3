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
#ifndef MQMutex_H_
#define MQMutex_H_
//--------------------------------------------------------------------------
#include <deque>
#include <list>
#include <memory>
#include "MessageTypes.pb.h"
#include "Mutex.h"
//--------------------------------------------------------------------------
namespace uniset3
{
    //--------------------------------------------------------------------------
    typedef std::shared_ptr<uniset3::umessage::TransportMessage> VoidMessagePtr;
    //--------------------------------------------------------------------------

    /*! \class MQMutex
     * Простая "многопоточная" очередь сообщений с использованием std::mutex.
     * Максимальное ограничение на размер очереди сообщений задаётся функцией setMaxSizeOfMessageQueue().
     *
     * Контроль переполнения очереди осуществляется в push
     * Если очередь переполняется, то сообщения ТЕРЯЮТСЯ!
     * При помощи функции setLostStrategy() можно установить стратегию что терять
     * lostNewData - в случае переполнения теряются новые данные (т.е. не будут помещаться в очередь)
     * lostOldData - в случае переполнения очереди, старые данные затираются новыми.
     *
    */
    class MQMutex
    {
        public:
            MQMutex( size_t qsize = 2000 );

            /*! поместить сообщение в очередь */
            void push( const VoidMessagePtr& msg );

            /*! Извлечь сообщение из очереди
             * \return не валидный shatred_ptr(nullptr) если сообщений нет
             */
            VoidMessagePtr top() noexcept;

            size_t size();
            bool empty();

            // ----- Настройки  -----
            // неявно подразумевается, что всё настраивается до первого использования
            // ----------------------
            void setMaxSizeOfMessageQueue( size_t s ) noexcept;
            size_t getMaxSizeOfMessageQueue() const noexcept;

            /*! Стратегия при переполнении */
            enum LostStrategy
            {
                lostOldData, // default
                lostNewData
            };

            void setLostStrategy( LostStrategy s ) noexcept;

            // ---- Статистика ----
            /*! максимальное количество которое было в очереди сообщений */
            inline size_t getMaxQueueMessages() const noexcept
            {
                return stMaxQueueMessages;
            }

            /*! количество потерянных сообщений */
            inline size_t getCountOfLostMessages() const noexcept
            {
                return stCountOfLostMessages;
            }

        protected:

        private:

            //typedef std::queue<VoidMessagePtr> MQueue;
            typedef std::deque<VoidMessagePtr> MQueue;

            MQueue mqueue;
            std::mutex qmutex;

            LostStrategy lostStrategy = { lostOldData };

            /*! размер очереди сообщений (при превышении происходит очистка) */
            size_t SizeOfMessageQueue = { 2000 };

            // статистическая информация
            size_t stMaxQueueMessages = { 0 };    /*!< Максимальное число сообщений хранившихся в очереди */
            size_t stCountOfLostMessages = { 0 };    /*!< количество переполнений очереди сообщений */
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
