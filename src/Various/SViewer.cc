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
 *  \brief Программа просмотра состояния датчиков
 *  \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "SViewer.h"
#include "ORepHelpers.h"
#include "UniSetObject_i.hh"
#include "UniSetTypes.h"
#include "ObjectIndex_Array.h"

// --------------------------------------------------------------------------
using namespace uniset3;
using namespace UniversalIO;
using namespace std;
// --------------------------------------------------------------------------
SViewer::SViewer(const string& csec, bool sn):
	csec(csec),
	rep(uniset3::uniset_conf()),
	isShortName(sn)
{
	ui = make_shared<UInterface>(uniset3::uniset_conf());
	ui->setCacheMaxSize(500);
}

SViewer::~SViewer()
{
}
// --------------------------------------------------------------------------
void SViewer::monitor( timeout_t msec )
{
	for(;;)
	{
		view();
		msleep(msec);
	}
}

// --------------------------------------------------------------------------
void SViewer::view()
{
	try
	{
		readSection(csec, "");
	}
	catch( const uniset3::Exception& ex )
	{
		cerr << ex << endl;
	}
}

// ---------------------------------------------------------------------------

void SViewer::readSection( const string& section, const string& secRoot )
{
	ListObjectName lst;
	string curSection;

	try
	{
		if ( secRoot.empty() )
			curSection = section;
		else
			curSection = secRoot + "/" + section;

		//        cout << " read sectionlist ..."<< endl;
		if( !rep.listSections(curSection, &lst, 1000) )
		{
			cout << "(readSection): получен не полный список" << endl;
		}
	}
	catch( const ORepFailed& ex ) {}
	catch(...)
	{
		std::exception_ptr p = std::current_exception();
		cout << "(readSection): get section list: " << (p ? p.__cxa_exception_type()->name() : "catch..") << std::endl;
	}

	if( !lst.empty() )
	{
		for ( ListObjectName::const_iterator li = lst.begin(); li != lst.end(); ++li)
		{
			readSection((*li), curSection);
		}
	}
	else
	{
		const string secName(curSection);
		ListObjectName lstObj;
		ListObjectName::const_iterator li;

		try
		{
			if( !rep.list(secName, &lstObj, 1000) )
				cout << "(readSection): получен не полный список" << endl;
		}
		catch( const ORepFailed& ex )
		{
			cout << "(readSection):.. catch ORepFailed" << endl;
			return;
		}
		catch(...)
		{
			std::exception_ptr p = std::current_exception();
			cout << "(readSection): " << (p ? p.__cxa_exception_type()->name() : "catch..") << std::endl;
			return;
		}

		//        cout << " read objectlist ok."<< endl;

		if ( !lstObj.empty() )
		{
			for ( li = lstObj.begin(); li != lstObj.end(); ++li )
			{
				try
				{
					const string ob(*li);
					const string fname(curSection + "/" + ob);
					ObjectId id = uniset_conf()->oind->getIdByName( fname );

					if( id == DefaultObjectId )
						cout << "(readSection): ID?! для " << fname << endl;
					else
						getInfo(id);
				}
				catch( const uniset3::Exception& ex )
				{
					cout << "(readSection): " << ex << endl;
				}

			}
		}
		else
			cout << " секция " << secRoot << "/" << section << " не содержит объектов " << endl;
	}
}
// ---------------------------------------------------------------------------
void SViewer::getInfo( ObjectId id )
{
	CORBA::Object_var oref;

	try
	{
		if( CORBA::is_nil(oref) )
			oref = ui->resolve(id);

		IONotifyController_i_var ioc = uniset3::_narrow(oref);

		if( CORBA::is_nil(ioc) )
		{
			cout << "(getInfo): nil references" << endl;
			return;
		}


		try
		{
			uniset3::SensorInfoSeq_var amap = ioc->getSensorsMap();
			updateSensors(amap, id);
		}
		catch( const uniset3::NameNotFound& ex ) {}
		catch(...) {}

		try
		{
			uniset3::ThresholdsListSeq_var tlst = ioc->getThresholdsList();
			updateThresholds(tlst, id);
		}
		catch( const uniset3::NameNotFound& ex ) {}
		catch(...) {}

		return;
	}
	catch( const uniset3::Exception& ex )
	{
		cout << "(getInfo):" << ex << endl;
	}
	catch(...)
	{
		std::exception_ptr p = std::current_exception();
		cout << "(getInfo): " << (p ? p.__cxa_exception_type()->name() : "catch..") << std::endl;
	}
}

// ---------------------------------------------------------------------------
void SViewer::updateSensors( uniset3::SensorInfoSeq_var& amap, uniset3::ObjectId oid )
{
	const string owner = ORepHelpers::getShortName(uniset_conf()->oind->getMapName(oid));
	cout << "\n======================================================\n"
		 << ORepHelpers::getShortName(uniset_conf()->oind->getMapName(oid))
		 << "\t Датчики"
		 << "\n------------------------------------------------------"
		 << endl;
	int size = amap->length();

	for(int i = 0; i < size; i++)
	{
		if( amap[i].type == uniset3::AI || amap[i].type == uniset3::DI )
		{
			string name(uniset_conf()->oind->getNameById(amap[i].si.id));

			if( isShortName )
				name = ORepHelpers::getShortName(name);

			string supplier = ORepHelpers::getShortName(uniset_conf()->oind->getMapName(amap[i].supplier));

			if( amap[i].supplier == uniset3::AdminID )
				supplier = "uniset-admin";

			const string txtname( uniset_conf()->oind->getTextName(amap[i].si.id) );
			printInfo( amap[i].si.id, name, amap[i].value, supplier, txtname, (amap[i].type == uniset3::AI ? "AI" : "DI") );
		}
	}

	cout << "------------------------------------------------------\n";

	cout << "\n======================================================\n" << owner;
	cout << "\t Выходы";
	cout << "\n------------------------------------------------------" << endl;

	for(int i = 0; i < size; i++)
	{
		if( amap[i].type == uniset3::AO || amap[i].type == uniset3::DO )
		{
			string name(uniset_conf()->oind->getNameById(amap[i].si.id));

			if( isShortName )
				name = ORepHelpers::getShortName(name);

			string supplier = ORepHelpers::getShortName(uniset_conf()->oind->getMapName(amap[i].supplier));

			if( amap[i].supplier == uniset3::AdminID )
				supplier = "uniset-admin";

			const string txtname( uniset_conf()->oind->getTextName(amap[i].si.id) );
			printInfo( amap[i].si.id, name, amap[i].value, supplier, txtname, (amap[i].type == uniset3::AO ? "AO" : "DO"));
		}
	}

	cout << "------------------------------------------------------\n";

}
// ---------------------------------------------------------------------------
void SViewer::updateThresholds( uniset3::ThresholdsListSeq_var& tlst, uniset3::ObjectId oid )
{
	int size = tlst->length();
	const string owner = ORepHelpers::getShortName(uniset_conf()->oind->getMapName(oid));
	cout << "\n======================================================\n" << owner;
	cout << "\t Пороговые датчики";
	cout << "\n------------------------------------------------------" << endl;

	for(int i = 0; i < size; i++)
	{
		cout << "(" << setw(5) << tlst[i].si.id << ") | ";

		switch( tlst[i].type  )
		{
			case uniset3::AI:
				cout << "AI";
				break;

			case uniset3::AO:
				cout << "AO";
				break;

			default:
				cout << "??";
				break;
		}

		string sname(uniset_conf()->oind->getNameById(tlst[i].si.id));

		if( isShortName )
			sname = ORepHelpers::getShortName(sname);

		cout << " | " << setw(60) << sname << " | " << setw(5) << tlst[i].value << endl;

		int m = tlst[i].tlist.length();

		for( auto k = 0; k < m; k++ )
		{
			uniset3::ThresholdInfo* ti = &tlst[i].tlist[k];
			cout << "\t(" << setw(3) << ti->id << ")  |  " << setw(5) << ti->state << "  |  hi: " << setw(5) << ti->hilimit;
			cout << " | low: " << setw(5) << ti->lowlimit;
			cout << endl;
		}
	}
}
// ---------------------------------------------------------------------------

void SViewer::printInfo(uniset3::ObjectId id, const string& sname, long value, const string& supplier,
						const string& txtname, const string& iotype)
{
	std::ios_base::fmtflags old_flags = cout.flags();
	cout << "(" << setw(5) << id << ")" << " | " << setw(2) << iotype << " | " << setw(60) << sname
		 << "   | " << setw(5) << value << "\t | "
		 << setw(40) << left << supplier << endl; // "\t | " << txtname << endl;
	cout.setf(old_flags);
}
// ---------------------------------------------------------------------------
