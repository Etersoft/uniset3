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
 *  \author Pavel Vainerman
 *    \brief базовые типы и вспомогательные функции библиотеки UniSet
*/
// --------------------------------------------------------------------------
#ifndef UniSetTypes_H_
#define UniSetTypes_H_
// --------------------------------------------------------------------------
#include <memory>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <list>
#include <vector>
#include <limits>
#include <ostream>
#include <chrono>
#include <thread>
#include <regex>

#include "UniSetTypes.pb.h"
#include "IOController.pb.h"
#include "MessageTypes.pb.h"
#include "MetricsExporter.pb.h"
#include "Configurator.pb.h"
#include "Mutex.h"
#include "UniXML.h"
#include "PassiveTimer.h" // for typedef timeout_t
// -----------------------------------------------------------------------------------------
/*! Задержка в миллисекундах */
inline void msleep( uniset3::timeout_t m )
{
    std::this_thread::sleep_for(std::chrono::milliseconds(m));
}

/*! Определения базовых (вспомогательные типы данных, константы, полезные функции) */
namespace uniset3
{
    typedef int64_t ObjectId;
    typedef int64_t ThresholdId;
    typedef int64_t TimerId;
    typedef std::string ObjectType;
    typedef std::list<NodeInfo> ListOfNode;
    typedef uint64_t KeyType;    /*!< уникальный ключ объекта */

    const std::string MetaDataServiceId = {"service_id"};

    class Configuration;
    // ---------------------------------------------------------------
    // Вспомогательные типы данных и константы

    /*! Запрещенные для использования в именах объектов символы */
    const char BadSymbols[] = {'.', '/'};

    /*! Проверка на наличие недопустимых символов
     * Запрещенные символы см. uniset3::BadSymbols[]
     * \return Если не найдено запрещённых символов то будет возвращен 0, иначе найденный символ
     */
    char checkBadSymbols(const std::string& str);

    /*! Получение запрещенных символов в виде строки '.', '/', и т.д. */
    std::string BadSymbolsToStr();


    /* hash32("DefaultObjectId") = 122387491 */
    const ObjectId DefaultObjectId = -1;    /*!< Идентификатор объекта по умолчанию */
    const ThresholdId DefaultThresholdId = -1;      /*!< идентификатор порогов по умолчанию */
    const ThresholdId DefaultTimerId = -1;      /*!< идентификатор таймера по умолчанию */

    const ObjectId AdminID = -2; /*!< сервисный идентификатор используемый утилитой admin */

    /*! генератор уникального положительного ключа
     *  Уникальность гарантируется только для пары значений id и node.
     *  \todo Желательно продумать что-нибудь с использованием хэш.
     *  \warning Уникальность не гарантирована, возможны коллизии
    */
    KeyType key( const uniset3::ObjectId id, const uniset3::ObjectId node );
    KeyType key( const uniset3::SensorInfo& si );

    uint64_t hash64( const std::string& str ) noexcept;
    uint64_t hash64( const char* buf, size_t sz ) noexcept;
    uint32_t hash32( const std::string& str ) noexcept;
    uint32_t hash32( const char* buf, size_t sz ) noexcept;

    typedef std::list<std::string> ListObjectName;    /*!< Список объектов типа ObjectName */

    uniset3::IOType getIOType( const std::string& s ) noexcept;
    std::string iotype2str( const uniset3::IOType& t ) noexcept;
    std::ostream& operator<<( std::ostream& os, const uniset3::IOType t );
    std::ostream& operator<<( std::ostream& os, const uniset3::ShortIOInfo& s );
    std::ostream& operator<<( std::ostream& os, const uniset3::ObjectRef& o);

    /*! Команды для управления лампочками */
    enum LampCommand
    {
        lmpOFF      = 0,    /*!< выключить */
        lmpON       = 1,    /*!< включить */
        lmpBLINK    = 2,    /*!< мигать */
        lmpBLINK2   = 3,    /*!< мигать */
        lmpBLINK3   = 4     /*!< мигать */
    };

    const long ChannelBreakValue = std::numeric_limits<long>::max();

    class IDList
    {
        public:

            IDList( const std::vector<std::string>& v );
#if __cplusplus >= 201703L
            IDList( const std::vector<std::string_view>& v );
#endif
            IDList();
            ~IDList();

            void add( ObjectId id );
            void del( ObjectId id );

            inline size_t size() const noexcept
            {
                return lst.size();
            }
            inline bool empty() const noexcept
            {
                return lst.empty();
            }

            std::list<ObjectId> getList() const noexcept;
            const std::list<ObjectId>& ref() const noexcept;

            // за освобождение выделенной памяти
            // отвечает вызывающий!
            uniset3::IDSeq getIDSeq() const;

            //
            ObjectId getFirst() const noexcept;
            ObjectId node;    // узел, на котором находятся датчики

        private:
            std::list<ObjectId> lst;
    };

    /*! Информация об имени объекта */
    struct ObjectInfo
    {
        ObjectId id = { DefaultObjectId };        /*!< идентификатор */
        std::string repName = { "" };      /*!< текстовое имя для регистрации в репозитории */
        std::string textName = { "" };     /*!< текстовое имя */
        std::string secName = { "" };      /*!< имя секции */
        std::string name = { "" };     /*!< имя */
        xmlNode* xmlnode = { nullptr };

        inline bool operator < ( const ObjectInfo& o ) const
        {
            return (id < o.id);
        }
    };

    // ---------------------------------------------------------------
    // Различные преобразования

    //! Преобразование строки в число (воспринимает префикс 0, как 8-ное, префикс 0x, как 16-ное, минус для отриц. чисел)
    int uni_atoi( const char* str ) noexcept;
    int uni_atoi( const std::string& str ) noexcept;
#if __cplusplus >= 201703L
    int uni_atoi_sv( std::string_view str ) noexcept;
#endif

    char* uni_strdup( const std::string& src );

    std::string timeToString(time_t tm = time(0), const std::string& brk = ":") noexcept; /*!< Преобразование времени в строку HH:MM:SS */
    std::string dateToString(time_t tm = time(0), const std::string& brk = "/") noexcept; /*!< Преобразование даты в строку DD/MM/YYYY */

    std::chrono::high_resolution_clock::time_point deadline_msec( uint64_t msec ) noexcept;

    struct timeval to_timeval( const std::chrono::system_clock::duration& d ); /*!< конвертирование std::chrono в posix timeval */
    struct timespec to_timespec( const std::chrono::system_clock::duration& d ); /*!< конвертирование std::chrono в posix timespec */
    struct timespec now_to_timespec(); /*!< получение текущего времени */

    uniset3::Timespec to_uniset_timespec( const std::chrono::system_clock::duration& d );
    uniset3::Timespec now_to_uniset_timespec(); /*!< получение текущего времени */
    uniset3::Timespec to_uniset_timespec(struct timespec ts);
    bool equal(const uniset3::Timespec& ts1, const uniset3::Timespec& ts2) noexcept;
    int64_t timespec_to_nanosec( const struct timespec& ts );
    int64_t uniset_timespec_to_nanosec( const uniset3::Timespec& ts );

    /*! Разбивка строки по указанному символу */
    IDList split_by_id( const std::string& str, char sep = ',' );
    std::vector<std::string> split( const std::string& str, char sep = ',' );
#if __cplusplus >= 201703L
    IDList split_by_id( std::string_view str, char sep = ',' );
    std::vector<std::string_view> split_sv( std::string_view str, char sep = ',' );
#endif

    struct ParamSInfo
    {
        uniset3::SensorInfo si;
        long val;
        std::string fname; // fullname id@node or id
    };

    /*! Функция разбора строки вида: id1@node1=val1,id2@node2=val2,...
       Если '=' не указано, возвращается val=0
       Если @node не указано, возвращается node=DefaultObjectId */
    std::list<ParamSInfo> getSInfoList( const std::string& s, std::shared_ptr<uniset3::Configuration> conf = nullptr );
#if __cplusplus >= 201703L
    std::list<ParamSInfo> getSInfoList_sv( std::string_view s, std::shared_ptr<uniset3::Configuration> conf = nullptr );
#endif

    /*! Функция разбора строки вида: id1@node1,id2@node2,...
      Если @node не указано, возвращается node=DefaultObjectId */
    std::list<uniset3::ConsumerInfo> getObjectsList( const std::string& s, std::shared_ptr<uniset3::Configuration> conf = nullptr );

    /*! проверка является текст в строке - числом..
     * \warning Числом будет считаться только строка ПОЛНОСТЬЮ состоящая из чисел.
     * Т.е. например "-10" или "100.0" или "10 000" - числом считаться не будут!
    */
    bool is_digit( const std::string& s ) noexcept;
#if __cplusplus >= 201703L
    bool is_digit_sv( std::string_view s ) noexcept;
#endif

    /*! замена всех вхождений подстроки
     * \param src - исходная строка
     * \param from - подстрока которая ищется (для замены)
     * \param to - строка на которую будет сделана замена
    */
    std::string replace_all( const std::string& src, const std::string& from, const std::string& to );
    // ---------------------------------------------------------------
    // Работа с командной строкой

    /*! Получение параметра командной строки
        \param name - название параметра
        \param defval - значение, которое будет возвращено, если параметр не найден
        \warning Поиск ведётся с первого аргумента, а не с нулевого!
    */
    inline std::string getArgParam( const std::string& name,
                                    int _argc, const char* const* _argv,
                                    const std::string& defval = "" ) noexcept
    {
        for( int i = 1; i < (_argc - 1) ; i++ )
        {
            if( name == _argv[i] )
                return _argv[i + 1];
        }

        return defval;
    }

    /*! получить значение, если пустое, то defval, если defval="" return defval2 */
    inline std::string getArg2Param(const std::string& name,
                                    int _argc, const char* const* _argv,
                                    const std::string& defval, const std::string& defval2 = "") noexcept
    {
        std::string s(uniset3::getArgParam(name, _argc, _argv, ""));

        if( !s.empty() )
            return s;

        if( !defval.empty() )
            return defval;

        return defval2;
    }

    inline int getArgInt( const std::string& name,
                          int _argc, const char* const* _argv,
                          const std::string& defval = "" ) noexcept
    {
        return uni_atoi(getArgParam(name, _argc, _argv, defval));
    }

    inline int getArgPInt( const std::string& name,
                           int _argc, const char* const* _argv,
                           const std::string& strdefval, int defval ) noexcept
    {
        std::string param = uniset3::getArgParam(name, _argc, _argv, strdefval);

        if( param.empty() && strdefval.empty() )
            return defval;

        return uniset3::uni_atoi(param);
    }


    /*! Проверка наличия параметра в командной строке
        \param name - название параметра
        \param _argc - argc
        \param _argv - argv
        \return Возвращает -1, если параметр не найден.
            Или позицию параметра, если найден.

        \warning Поиск ведётся с первого аргумента, а не с нулевого!
    */
    inline int findArgParam( const std::string& name, int _argc, const char* const* _argv )
    {
        for( int i = 1; i < _argc; i++ )
        {
            if( name == _argv[i] )
                return i;
        }

        return -1;
    }

    // ---------------------------------------------------------------
    // Калибровка

    std::ostream& operator<<( std::ostream& os, const uniset3::CalibrateInfo& c );

    // Функции калибровки значений
    // raw      - преобразуемое значение
    // rawMin   - минимальная граница исходного диапазона
    // rawMax   - максимальная граница исходного диапазона
    // calMin   - минимальная граница калиброванного диапазона
    // calMin   - минимальная граница калиброванного диапазона
    // limit    - обрезать итоговое значение по границам
    float fcalibrate(float raw, float rawMin, float rawMax, float calMin, float calMax, bool limit = true );
    long lcalibrate(long raw, long rawMin, long rawMax, long calMin, long calMax, bool limit = true );
    double dcalibrate(double raw, double rawMin, double rawMax, double calMin, double calMax, bool limit = true );

    // установка значения в нужный диапазон
    long setinregion(long raw, long rawMin, long rawMax);
    // установка значения вне диапазона
    long setoutregion(long raw, long rawMin, long rawMax);

    // Всякие helper-ы
    // ---------------------------------------------------------------
    umessage::MessageHeader makeMessageHeader( uniset3::umessage::Priority p = uniset3::umessage::mpMedium );
    umessage::SystemMessage makeSystemMessage(umessage::SystemMessage::Command cmd = umessage::SystemMessage::Unknown);
    umessage::SensorMessage makeSensorMessage(ObjectId sid, long value, uniset3::IOType type);
    umessage::TimerMessage makeTimerMessage(int tid = uniset3::DefaultTimerId, uniset3::umessage::Priority p = uniset3::umessage::mpMedium);
    umessage::ConfirmMessage makeConfirmMessage(ObjectId sensor_id,
            const double& sensor_value,
            const uniset3::Timespec& sensor_time,
            const uniset3::Timespec& confirm_time,
            uniset3::umessage::Priority prior = uniset3::umessage::mpMedium);

    umessage::TextMessage makeTextMessage( const std::string& msg,
                                           int mtype,
                                           const uniset3::Timespec& tm,
                                           const uniset3::ProducerInfo& pi,
                                           uniset3::umessage::Priority prior = uniset3::umessage::mpMedium,
                                           ObjectId cons = uniset3::DefaultObjectId );
    umessage::TextMessage makeTextMessage();
    // ---------------------------------------------------------------
    uniset3::metrics::Metric createMetric( const std::string& name, const double val, const std::string& description = "");
    uniset3::metrics::Metric createMetric( const std::string& name, const std::string& val, const std::string& description = "");
    uniset3::configurator::ParamValue createParamValue( const double val );
    uniset3::configurator::ParamValue createParamValue( const std::string& val );
    // ---------------------------------------------------------------


    bool file_exists( const std::string& filename );
    bool directory_exists( const std::string& path );

    // Проверка xml-узла на соответствие <...f_prop="f_val">,
    // если не задано f_val, то проверяется, что просто f_prop!=""
    bool check_filter( UniXML::iterator& it, const std::string& f_prop, const std::string& f_val = "" ) noexcept;
    bool check_filter_re( UniXML::iterator& it, const std::string& f_prop, const std::regex& re ) noexcept;

    // RAII для флагов форматирования ostream..
    class ios_fmt_restorer
    {
        public:
            ios_fmt_restorer( std::ostream& s ):
                os(s), f(nullptr)
            {
                f.copyfmt(s);
            }

            ~ios_fmt_restorer()
            {
                os.copyfmt(f);
            }

            ios_fmt_restorer( const ios_fmt_restorer& ) = delete;
            ios_fmt_restorer& operator=( const ios_fmt_restorer& ) = delete;

        private:
            std::ostream& os;
            std::ios f;
    };

    // -----------------------------------------------------------------------------------------
} // end of namespace uniset3
// -----------------------------------------------------------------------------------------
inline bool operator==(const struct timespec& r1, const struct timespec& r2)
{
    return (r1.tv_sec == r2.tv_sec && r1.tv_nsec == r2.tv_nsec);
}
inline bool operator!=(const struct timespec& r1, const struct timespec& r2)
{
    return !(operator==(r1, r2));
}
#endif
