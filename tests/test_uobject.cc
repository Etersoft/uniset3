#include <catch.hpp>
// --------------------------------------------------------------------------
#include "UniSetObject.h"
#include "MessageType.h"
#include "Configuration.h"
#include "UHelpers.h"
#include "TestUObject.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
/* Простой тест для UniSetObject.
 * Попытка протестировать UniSetObject без зависимостей и только интерфейс.
 * Для доступа к некоторой внутренней информации, пришлось сделать TestObject.
 * Фактически тестовый объект не будет "активирован" как обычно.
 */
// --------------------------------------------------------------------------
shared_ptr<TestUObject> uobj;
// --------------------------------------------------------------------------
void initTest()
{
    REQUIRE( uniset_conf() != nullptr );

    if( !uobj )
    {
        uobj = make_object<TestUObject>("TestUObject1", "TestUObject");
        REQUIRE( uobj != nullptr );
    }
}
// --------------------------------------------------------------------------
static void pushMessage( long id, messages::Priority p )
{
    SensorMessage sm(id, id);
    sm.priority = p;
    sm.consumer = id; // чтобы хоть как-то идентифицировать сообщений, используем поле consumer
    TransportMessage tm( std::move(sm.transport_msg()) );
    uobj->push(tm);
}
// --------------------------------------------------------------------------
TEST_CASE( "UObject: priority messages", "[uobject]" )
{
    initTest();

    /* NOTE: для того чтобы не делать преобразования из VoidMessage в SensorMessage (см. pushMesage)
     * В качестве идентификатора используем поле consumer.
     * Хотя в реальности, оно должно совпадать с id объекта получателя.
     */

    pushMessage(100, messages::mpLow);
    pushMessage(101, messages::mpLow);
    pushMessage(200, messages::mpMedium);
    pushMessage(300, messages::mpHigh);
    pushMessage(301, messages::mpHigh);

    // теперь проверяем что сперва вынули Hi
    // но так же контролируем что порядок извлечения правильный
    // в порядке поступления в очередь
    auto m = uobj->getOneMessage();
    REQUIRE( m->priority == messages::mpHigh );
    REQUIRE( m->consumer == 300 );
    m = uobj->getOneMessage();
    REQUIRE( m->priority == messages::mpHigh );
    REQUIRE( m->consumer == 301 );

    m = uobj->getOneMessage();
    REQUIRE( m->priority == messages::mpMedium );
    REQUIRE( m->consumer == 200 );

    m = uobj->getOneMessage();
    REQUIRE( m->priority == messages::mpLow );
    REQUIRE( m->consumer == 100 );

    pushMessage(201, messages::mpMedium);
    m = uobj->getOneMessage();
    REQUIRE( m->priority == messages::mpMedium );
    REQUIRE( m->consumer == 201 );

    m = uobj->getOneMessage();
    REQUIRE( m->priority == messages::mpLow );
    REQUIRE( m->consumer == 101 );

    REQUIRE( uobj->mqEmpty() == true );
}
// --------------------------------------------------------------------------
