#ifndef TestUObject_H_
#define TestUObject_H_
// -------------------------------------------------------------------------
#include "UniSetObject.h"
// -------------------------------------------------------------------------
/*! Специальный тестовый объект для тестирования класса UniSetObject
 * Для наглядности и простоты все функции объявлены здесь же в h-файле
*/
class TestUObject:
    public uniset3::UniSetObject
{
    public:

        TestUObject( uniset3::ObjectId id, xmlNode* cnode ):
            uniset3::UniSetObject(id) {}

        virtual ~TestUObject() {};

        // специальные функции для проведения тестирования
        inline uniset3::VoidMessagePtr getOneMessage()
        {
            return receiveMessage();
        }

        inline bool mqEmpty()
        {
            return (countMessages() == 0);
        }

    protected:
        TestUObject() {};
};
// -------------------------------------------------------------------------
#endif // TestUObject_H_
// -------------------------------------------------------------------------
