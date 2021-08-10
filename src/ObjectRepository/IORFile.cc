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
// -----------------------------------------------------------------------------------------
#include <sstream>
#include <fstream>
#include <unistd.h>
#include "IORFile.h"
#include "Exceptions.h"
// -----------------------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// -----------------------------------------------------------------------------------------
//std::string IORFile::makeIOR( const uniset3::ObjectRef& ref )
//{
//    return ref.SerializeAsString();
//}
// -----------------------------------------------------------------------------------------
//uniset3::ObjectRef IORFile::makeRef( const std::string& s )
//{
//    uniset3::ObjectRef ref;
//    if( !ref.ParseFromString(s) )
//        throw ResolveNameError();
//
//    return ref;
//}
// -----------------------------------------------------------------------------------------
IORFile::IORFile( const std::string& dir ):
    iordir(dir)
{

}
// -----------------------------------------------------------------------------------------
IORFile::~IORFile()
{

}
// -----------------------------------------------------------------------------------------
string IORFile::getIOR( const ObjectId id ) const
{
    const string fname( getFileName(id) );
    ifstream ior_file(fname.c_str(), ios::binary);
    string sior;
    ior_file >> sior;
    ior_file.close();
    return sior;
}
// -----------------------------------------------------------------------------------------
void IORFile::setIOR( const ObjectId id, const uniset3::ObjectRef& ref ) const
{
    const string fname( getFileName(id) );
    ofstream ior_file(fname.c_str(), ios::out | ios::trunc | ios::binary);

    if( !ior_file )
        throw ORepFailed("(IORFile): can't open ior-file " + fname);

    if( !ref.SerializeToOstream(&ior_file) )
        throw ORepFailed("(IORFile): can't save ior-file " + fname);

    ior_file.close();
}
// -----------------------------------------------------------------------------------------
uniset3::ObjectRef IORFile::getRef( const ObjectId id ) const
{
    ObjectRef oref;
    const string fname( getFileName(id) );
    ifstream ior_file(fname.c_str(), ios::binary);

    if( !ior_file )
        throw ORepFailed("(IORFile): can't open ior-file " + fname);

    if( !oref.ParseFromIstream(&ior_file) )
        throw ResolveNameError("(IORFile): can't parse oref from file '" + fname + "'");

    return oref;
}
// -----------------------------------------------------------------------------------------
void IORFile::unlinkIOR( const ObjectId id ) const
{
    const string fname( getFileName(id) );
    unlink(fname.c_str());
}
// -----------------------------------------------------------------------------------------
string IORFile::getFileName( const ObjectId id ) const
{
    ostringstream fname;
    fname << iordir << id;
    return fname.str();
}
// -----------------------------------------------------------------------------------------
