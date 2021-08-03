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
    /*! Реализация grpc-интерфейса UniSetManager_i
     * Позволяет управлять группой объектов добавляемых функцией add().
    */
    class UniSetManager:
        public UniSetObject,
        public UniSetManager_i::Service
    {
        public:
            UniSetManager( uniset3::ObjectId id );

            virtual ~UniSetManager();

            // ------  функции объявленые в интерфейсе(IDL) ------
            virtual ::grpc::Status broadcast(::grpc::ServerContext* context, const ::uniset3::umessage::TransportMessage* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status getObjectsInfo(::grpc::ServerContext* context, const ::uniset3::ObjectsInfoParams* request, ::uniset3::SimpleInfoSeq* response) override;
            virtual std::string getStrType() const override;
            // --------------------------
            std::shared_ptr<UniSetManager> get_mptr();
            virtual bool add( const std::shared_ptr<UniSetObject>& obj );
            virtual bool remove( const std::shared_ptr<UniSetObject>& obj );
            // --------------------------
            size_t objectsCount() const;    // количество подчинённых объектов
            // ---------------

        protected:

            UniSetManager();

            friend class UniSetManagerProxy;
            virtual bool init( const std::string& svcAddr );

            //! \note Переопределяя, не забывайте вызвать базовую
            virtual bool activateObject() override;
            //! \note Переопределяя, не забывайте вызвать базовую
            virtual bool deactivateObject() override;

            const std::shared_ptr<UniSetObject> findObject( const std::string& name ) const;

            // рекурсивный поиск по всем объектам
            const std::shared_ptr<UniSetObject> deepFindObject( const std::string& name ) const;

            // рекурсивное наполнение списка объектов
            void getAllObjectsList(std::vector<std::shared_ptr<UniSetObject> >& vec, size_t lim = 1000 );

        private:

            std::list< std::shared_ptr<UniSetObject> > olist;
            mutable uniset3::uniset_rwmutex olistMutex;
    };
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
#endif
