#include <catch.hpp>
// --------------------------------------------------------------------------
#include "UniSetObject.h"
#include "MessageTypes.pb.h"
#include "Configuration.h"
#include "UHelpers.h"
#include "TestUObject.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::umessage;
// --------------------------------------------------------------------------
/* Простой тест для UniSetObject.
 * Попытка протестировать UniSetObject без зависимостей и только интерфейс.
 * Для доступа к некоторой внутренней информации, пришлось сделать TestObject.
 * Фактически тестовый объект не будет "активирован" как обычно.
 */
// --------------------------------------------------------------------------
shared_ptr<TestUObject> uobj;
// --------------------------------------------------------------------------
static void initTest()
{
    REQUIRE( uniset_conf() != nullptr );

    if( !uobj )
    {
        uobj = make_object<TestUObject>("TestUObject1", "TestUObject");
        REQUIRE( uobj != nullptr );
    }
}
// --------------------------------------------------------------------------
static void pushMessage( long id, umessage::Priority p )
{
    SensorMessage sm = makeSensorMessage(id, id, uniset3::AI);
    sm.mutable_header()->set_priority(p);
    sm.mutable_header()->set_consumer(id); // чтобы хоть как-то идентифицировать сообщений, используем поле consumer
    auto tm = to_transport<SensorMessage>(sm);

    grpc::ServerContext ctx;
    google::protobuf::Empty empty;
    uobj->push(&ctx, &tm, &empty);
}
// --------------------------------------------------------------------------
TEST_CASE( "UObject: priority umessage", "[uobject]" )
{
    initTest();

    /* NOTE: для того чтобы не делать преобразования из VoidMessage в SensorMessage (см. pushMesage)
     * В качестве идентификатора используем поле consumer.
     * Хотя в реальности, оно должно совпадать с id объекта получателя.
     */

    pushMessage(100, umessage::mpLow);
    pushMessage(101, umessage::mpLow);
    pushMessage(200, umessage::mpMedium);
    pushMessage(300, umessage::mpHigh);
    pushMessage(301, umessage::mpHigh);

    // теперь проверяем что сперва вынули Hi
    // но так же контролируем что порядок извлечения правильный
    // в порядке поступления в очередь
    auto m = uobj->getOneMessage();
    REQUIRE( m->priority() == umessage::mpHigh );
    REQUIRE( m->consumer() == 300 );
    m = uobj->getOneMessage();
    REQUIRE( m->priority() == umessage::mpHigh );
    REQUIRE( m->consumer() == 301 );

    m = uobj->getOneMessage();
    REQUIRE( m->priority() == umessage::mpMedium );
    REQUIRE( m->consumer() == 200 );

    m = uobj->getOneMessage();
    REQUIRE( m->priority() == umessage::mpLow );
    REQUIRE( m->consumer() == 100 );

    pushMessage(201, umessage::mpMedium);
    m = uobj->getOneMessage();
    REQUIRE( m->priority() == umessage::mpMedium );
    REQUIRE( m->consumer() == 201 );

    m = uobj->getOneMessage();
    REQUIRE( m->priority() == umessage::mpLow );
    REQUIRE( m->consumer() == 101 );

    REQUIRE( uobj->mqEmpty() == true );
}
// --------------------------------------------------------------------------
