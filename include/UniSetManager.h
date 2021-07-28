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
 * \brief Реализация интерфейса менеджера объектов.
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef UniSetManager_H_
#define UniSetManager_H_
// --------------------------------------------------------------------------
#include <memory>
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "UniSetManager.grpc.pb.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    //---------------------------------------------------------------------------
    class UniSetActivator;

    class UniSetManager;
    typedef std::list< std::shared_ptr<UniSetManager> > UniSetManagerList;
    //---------------------------------------------------------------------------
    /*! \class UniSetManager
     *    \par
     *    Содержит в себе функции управления объектами. их регистрации и т.п.
     *    Создается менеджер объектов, после чего вызывается initObjects()
     *    для инициализации объектов которыми управляет
     *    данный менеджер...
     *    Менеджер в свою очередь сам является объектом и обладает всеми его свойствами
     *
     *     Для пересылки сообщения всем подчиненным объектам используется
     *        функция UniSetManager::broadcast(const TransportMessage& msg)
     *    \par
     *     У базового менеджера имеются базовые три функции см. UniSetManager_i.
     *    \note Только при вызове функции UniSetManager::broadcast() происходит
     *        формирование сообщения всем подчиненным объектам... Если команда происходит
     *    при помощи push, то пересылки всем подчинённым объектам не происходит...
     *
     *
    */
    class UniSetManager:
        public UniSetObject,
        public UniSetManager_i::Service
    {
        public:
            UniSetManager( uniset3::ObjectId id );

            virtual ~UniSetManager();


            // ------  функции объявленые в интерфейсе(IDL) ------
            virtual ::grpc::Status getType(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::StringValue* response) override;
            virtual ::grpc::Status broadcast(::grpc::ServerContext* context, const ::uniset3::umessage::TransportMessage* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status getObjectsInfo(::grpc::ServerContext* context, const ::uniset3::ObjectsInfoParams* request, ::uniset3::SimpleInfoSeq* response) override;

            // --------------------------
            std::shared_ptr<UniSetManager> get_mptr();
            virtual bool add( const std::shared_ptr<UniSetObject>& obj );
            virtual bool remove( const std::shared_ptr<UniSetObject>& obj );
            // --------------------------
            size_t objectsCount() const;    // количество подчинённых объектов
            // ---------------

        protected:

            UniSetManager();

            virtual bool addManager( const std::shared_ptr<UniSetManager>& mngr );
            virtual bool removeManager( const std::shared_ptr<UniSetManager>& mngr );
            virtual bool addObject( const std::shared_ptr<UniSetObject>& obj );
            virtual bool removeObject( const std::shared_ptr<UniSetObject>& obj );

            enum OManagerCommand { deactiv, activ, initial };
            friend std::ostream& operator<<( std::ostream& os, uniset3::UniSetManager::OManagerCommand& cmd );

            // работа со списком объектов
            void objects(OManagerCommand cmd);
            // работа со списком менеджеров
            void managers(OManagerCommand cmd);

            void initGRPC( const std::weak_ptr<UniSetManager>& rmngr );

            //! \note Переопределяя, не забывайте вызвать базовую
            virtual bool activateObject() override;
            //! \note Переопределяя, не забывайте вызвать базовую
            virtual bool deactivateObject() override;

            const std::shared_ptr<UniSetObject> findObject( const std::string& name ) const;
            const std::shared_ptr<UniSetManager> findManager( const std::string& name ) const;

            // рекурсивный поиск по всем объектам
            const std::shared_ptr<UniSetObject> deepFindObject( const std::string& name ) const;

            // рекурсивное наполнение списка объектов
            void getAllObjectsList(std::vector<std::shared_ptr<UniSetObject> >& vec, size_t lim = 1000 );

            typedef UniSetManagerList::iterator MListIterator;

            // Функции для работы со списками подчинённых объектов
            // ---------------
            typedef std::function<void(const std::shared_ptr<UniSetObject>&)> OFunction;
            void apply_for_objects( OFunction f );

            typedef std::function<void(const std::shared_ptr<UniSetManager>&)> MFunction;
            void apply_for_managers( MFunction f );

        private:

            UniSetManagerList mlist;
            ObjectsList olist;

            mutable uniset3::uniset_rwmutex olistMutex;
            mutable uniset3::uniset_rwmutex mlistMutex;
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
#endif
