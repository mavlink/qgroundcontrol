/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @brief LinkManager Unit Test
///
///     @author Don Gagne <don@thegagnes.com>

#include "LinkManagerTest.h"
#include "MockLink.h"

UT_REGISTER_TEST(LinkManagerTest)

LinkManagerTest::LinkManagerTest(void) :
    _linkMgr(NULL),
    _multiSpy(NULL)
{
    
}

void LinkManagerTest::init(void)
{
    UnitTest::init();
    
    Q_ASSERT(_linkMgr == NULL);
    Q_ASSERT(_multiSpy == NULL);
    
    _linkMgr = new LinkManager(NULL /* no parent */);
    Q_CHECK_PTR(_linkMgr);
    
    _rgSignals[newLinkSignalIndex] = SIGNAL(newLink(LinkInterface*));
    _rgSignals[linkDeletedSignalIndex] = SIGNAL(linkDeleted(LinkInterface*));
    
    _multiSpy = new MultiSignalSpy();
    QCOMPARE(_multiSpy->init(_linkMgr, _rgSignals, _cSignals), true);
}

void LinkManagerTest::cleanup(void)
{
    Q_ASSERT(_linkMgr);
    Q_ASSERT(_multiSpy);
    
    _linkMgr->_shutdown();
    
    delete _linkMgr;
    delete _multiSpy;
    
    _linkMgr = NULL;
    _multiSpy = NULL;
    
    UnitTest::cleanup();
}


void LinkManagerTest::_add_test(void)
{
    Q_ASSERT(_linkMgr);
    Q_ASSERT(_linkMgr->getLinks().count() == 0);
    
    MockLink* link = new MockLink();
    _linkMgr->_addLink(link);
    
    QList<LinkInterface*> links = _linkMgr->getLinks();
    QCOMPARE(links.count(), 1);
    QCOMPARE(dynamic_cast<MockLink*>(links[0]), link);
}

void LinkManagerTest::_delete_test(void)
{
    Q_ASSERT(_linkMgr);
    Q_ASSERT(_linkMgr->getLinks().count() == 0);
    
    MockLink* link = new MockLink();
    _linkMgr->_addLink(link);
    _linkMgr->_deleteLink(link);
    
    QCOMPARE(_linkMgr->getLinks().count(), 0);
}

void LinkManagerTest::_addSignals_test(void)
{
    Q_ASSERT(_linkMgr);
    Q_ASSERT(_linkMgr->getLinks().count() == 0);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    MockLink* link = new MockLink();
    _linkMgr->_addLink(link);
    
    QCOMPARE(_multiSpy->checkOnlySignalByMask(newLinkSignalMask), true);
    QSignalSpy* spy = _multiSpy->getSpyByIndex(newLinkSignalIndex);
    
    // Check signal argument
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QObject* object = qvariant_cast<QObject *>(signalArgs[0]);
    QVERIFY(object != NULL);
    MockLink* signalLink = qobject_cast<MockLink*>(object);
    QCOMPARE(signalLink, link);
}

void LinkManagerTest::_deleteSignals_test(void)
{
    Q_ASSERT(_linkMgr);
    Q_ASSERT(_linkMgr->getLinks().count() == 0);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    MockLink* link = new MockLink();
    _linkMgr->_addLink(link);
    _multiSpy->clearAllSignals();
    
    _linkMgr->_deleteLink(link);
    
    QCOMPARE(_multiSpy->checkOnlySignalByMask(linkDeletedSignalMask), true);
    QSignalSpy* spy = _multiSpy->getSpyByIndex(linkDeletedSignalIndex);
    
    // Best we can do is check the argument count. We can't check the contents of signal argument
    // because the object has been deleted and any access to the variant to try to cast it back
    // to MockLink* will cause the link to be de-referenced and hence crash.
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
}
