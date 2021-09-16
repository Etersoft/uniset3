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
//--------------------------------------------------------------------------------
/*! \file
 *  \brief Программа просмотра состояния датчиков
 *  \author Pavel Vainerman
 */
//--------------------------------------------------------------------------------
#ifndef _SVIEWER_H
#define _SVIEWER_H
//--------------------------------------------------------------------------------
#include <string>
#include <memory>
#include "IOController.pb.h"
#include "UInterface.h"
#include "PassiveTimer.h"
//--------------------------------------------------------------------------------
namespace uniset3
{
    class SViewer
    {
        public:

            explicit SViewer(const std::string& ControllersSection, bool isShortName = true);
            virtual ~SViewer();

            void view();
            void monitor( timeout_t msec = 500 );

            static void printInfo(uniset3::ObjectId id,
                                  const std::string& sname,
                                  long value,
                                  const std::string& supplier,
                                  const std::string& txtname, const std::string& iotype);


        protected:
            void readSection(const std::string& sec, const std::string& secRoot);
            void getSensorsInfo(uniset3::ObjectId iocontrollerID);

            virtual void updateSensors( uniset3::SensorIOInfoSeq& smap, uniset3::ObjectId oid );

            const std::string csec;
            std::shared_ptr<UInterface> ui;

        private:
            bool isShortName = { true };

    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
