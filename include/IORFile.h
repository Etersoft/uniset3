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
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef IORFile_H_
#define IORFile_H_
// --------------------------------------------------------------------------
#include <string>
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
namespace uniset3
{
    /*! Класс работы с файлами содержащими IOR объекта */
    class IORFile
    {
        public:

            IORFile( const std::string& iordir );
            ~IORFile();

            std::string getIOR( const ObjectId id ) const;
            uniset3::ObjectRef getRef( const ObjectId id ) const;
            void setIOR( const ObjectId id, const uniset3::ObjectRef& ref ) const; // throw ORepFailed
            void unlinkIOR( const ObjectId id ) const;

            std::string getFileName( const ObjectId id ) const;

        private:
            const std::string iordir;
    };
    // -----------------------------------------------------------------------------------------
}    // end of namespace
// -----------------------------------------------------------------------------------------
#endif
