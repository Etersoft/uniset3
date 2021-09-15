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
#ifndef IOBase_H_
#define IOBase_H_
// -----------------------------------------------------------------------------
#include <string>
#include <memory>
#include "PassiveTimer.h"
#include "Trigger.h"
#include "Mutex.h"
#include "DigitalFilter.h"
#include "Calibration.h"
#include "IOController.h"
#include "SMInterface.h"
// -------------------------------------------------------------------------
namespace uniset3
{
    // -----------------------------------------------------------------------------
    /*! Свойства переменной в/в */
    struct IOBase
    {
        static const int DefaultSubdev  = -1;
        static const int DefaultChannel = -1;
        static const int DefaultCard = -1;

        // т.к. IOBase содержит rwmutex с запрещённым конструктором копирования
        // приходится здесь тоже объявлять разрешенными только операции "перемещения"
        IOBase( const IOBase& r ) = delete;
        IOBase& operator=(const IOBase& r) = delete;

        IOBase( IOBase&& r ) = default;
        IOBase& operator=(IOBase&& r) = default;

        ~IOBase();
        IOBase();

        bool check_debounce( bool val );    /*!< реализация фильтра против дребезга */
        bool check_on_delay( bool val );    /*!< реализация задержки на включение */
        bool check_off_delay( bool val );   /*!< реализация задержки на отключение */
        bool check_front( bool val );       /*!< реализация срабатывания по фронту сигнала */
        bool check_depend( const std::shared_ptr<SMInterface>& shm ); /*!< проверка разрешения(зависимости) от другого датчика */

        uniset3::SensorInfo si;
        uniset3::IOType stype;           /*!< тип канала (DI,DO,AI,AO) */
        uniset3::CalibrateInfo cal;   /*!< калибровочные параметры */
        Calibration* cdiagram;               /*!< специальная калибровочная диаграмма */

        long value;     /*!< текущее значение */
        long craw;      /*!< текущее 'сырое' значение до калибровки */
        long cprev;     /*!< предыдущее значение после калибровки */
        long safeval;   /*!< безопасное значение */
        long defval;    /*!< состояние по умолчанию (при запуске) */
        bool safevalDefined = { false }; /*!< флаг, означающий что safeval задан (можно использовать) */

        DigitalFilter df;   /*!< реализация программного фильтра */
        bool nofilter;      /*!< отключение фильтра */
        bool f_median;      /*!< признак использования медианного фильтра */
        bool f_ls;          /*!< признак использования адаптивного фильтра по методу наименьших квадратов */
        bool f_filter_iir;  /*!< признак использования рекурсивного фильтра */

        bool ignore;    /*!< игнорировать при опросе */
        bool invert;    /*!< инвертированная логика */
        bool noprecision;
        bool calcrop;   /*!< обрезать значения по границам при калибровке. Default: true */

        PassiveTimer ptDebounce;    /*!< таймер на дребезг */
        PassiveTimer ptOnDelay;     /*!< задержка на срабатывание */
        PassiveTimer ptOffDelay;    /*!< задержка на отпускание */

        Trigger trOnDelay;
        Trigger trOffDelay;
        Trigger trdebounce;

        bool debounce_pause;
        bool debounce_state;    /*!< значение для фильтра антидребезга */
        bool ondelay_state;     /*!< значение для задержки включения */
        bool offdelay_state;    /*!< значение для задержки отключения */

        // Зависимость (d - depend)
        uniset3::ObjectId d_id;  /*!< идентификатор датчика, от которого зависит данный */
        IOController::IOStateList::iterator d_it; /*! итератор на датчик от которого зависит данный */
        long d_value; /*!< разрешающее работу значение датчика от которого зависит данный */
        long d_off_value; /*!< блокирующее значение */
        uniset3::IOType d_iotype;

        // Порог
        uniset3::ObjectId t_ai; /*!< если данный датчик дискретный,
                                        и является пороговым, то в данном поле
                                        хранится идентификатор аналогового датчика
                                        с которым он связан */
        long t_hilimit;        /*!< верхняя граница срабатывания */
        long t_lowlimit;       /*!< нижняя граница срабатывания */
        bool t_invert;         /*!< инвертированная логика */

        IOController::IOStateList::iterator t_ait; // итератор для аналогового датчика

        // Работа по фронтам сигнала
        enum FrontType
        {
            ftUnknown,
            ft01,      // срабатывание на переход "0-->1"
            ft10       // срабатывание на переход "1-->0"
        };

        friend std::ostream& operator<<( std::ostream& os, const FrontType& f );

        bool front; // флаг работы по фронту
        FrontType front_type;
        bool front_prev_state;
        bool front_state;

        bool rawdata; // флаг для сохранения данный в таком виде в каком они пришли (4байта просто копируются в long). Актуально для Vtypes::F4.

        IOController::IOStateList::iterator ioit;
        uniset3::uniset_rwmutex val_lock;     /*!< блокировка на время "работы" со значением */

        IOBase make_iobase_copy();
        void create_from_iobase( const IOBase& b );

        friend std::ostream& operator<<(std::ostream& os, const IOBase& inf );

        static void processingF64asAI( IOBase* it, double new_val, const std::shared_ptr<SMInterface>& shm, bool force );
        static void processingFasAI( IOBase* it, float new_val, const std::shared_ptr<SMInterface>& shm, bool force );
        static void processingAsAI( IOBase* it, long new_val, const std::shared_ptr<SMInterface>& shm, bool force );
        static void processingAsDI( IOBase* it, bool new_set, const std::shared_ptr<SMInterface>& shm, bool force );
        static long processingAsAO( IOBase* it, const std::shared_ptr<SMInterface>& shm, bool force );
        static float processingFasAO( IOBase* it, const std::shared_ptr<SMInterface>& shm, bool force );
        static double processingF64asAO( IOBase* it, const std::shared_ptr<SMInterface>& shm, bool force );
        static bool processingAsDO( IOBase* it, const std::shared_ptr<SMInterface>& shm, bool force );
        static void processingThreshold( IOBase* it, const std::shared_ptr<SMInterface>& shm, bool force );

        /*! \param initPrefixOnly - TRUE - инициализировать только свойства с prefix (или брать значения по умолчанию).
                                    FALSE - сперва искать свойство с prefix, если не найдено брать без prefix.
        */
        static bool initItem( IOBase* b, UniXML::iterator& it, const std::shared_ptr<SMInterface>& shm,
                              const std::string& prefix, bool init_prefix_only,
                              std::shared_ptr<DebugStream> dlog = nullptr, std::string myname = "",
                              int def_filtersize = 0, float def_filterT = 0.0,
                              float def_lsparam = 0.2, float def_iir_coeff_prev = 0.5,
                              float def_iir_coeff_new = 0.5 );


        // helpes
        static std::string initProp( UniXML::iterator& it, const std::string& prop, const std::string& prefix, bool prefonly, const std::string& defval = "" );
        static int initIntProp( UniXML::iterator& it, const std::string& prop, const std::string& prefix, bool prefonly, const int defval = 0 );
        static timeout_t initTimeoutProp( UniXML::iterator& it, const std::string& prop, const std::string& prefix, bool prefonly, const timeout_t defval);
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset3
// -----------------------------------------------------------------------------
#endif // IOBase_H_
// -----------------------------------------------------------------------------
