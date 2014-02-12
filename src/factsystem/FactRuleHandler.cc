#include "FactSystem.h"
#include "lua.hpp"
#include "stdlib.h"
#include <QDebug>
#include <QFile>
#include <QApplication>
#include "UASInterface.h"

static UASInterface* _uas = NULL;   // gross but it works to get the rest of the facvt system to the C routines

/// Allocator for Lua
extern "C" void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud;  (void)osize;  /* not used */
    if (nsize == 0) {
        free(ptr);
        return NULL;
    }
    else
        return realloc(ptr, nsize);
}

extern "C" int l_getFactValue(lua_State* L)
{
    Q_ASSERT(_uas);
    FactHandler& factHandler = _uas->getFactHandler();
    
    const char* providerFactId = lua_tostring(L, 1);
    
    Fact::Provider_t    provider;
    QString             factId;
    
    if (!Fact::parseProviderFactIdReference(providerFactId, provider, factId)) {
        lua_pushstring(L, QString("lua.getFactValue(%1) - unknown provider id").arg(providerFactId).toAscii().constData());
        lua_error(L);
    }
    if (!factHandler.containsFact(provider, factId)) {
        lua_pushstring(L, QString("lua.getFactValue(%1) - unknown fact id").arg(providerFactId).toAscii().constData());
        lua_error(L);
    }

    Fact& fact= factHandler.getFact(provider, factId);
    
    qDebug() << QString("getFactValue factId(%1) provider(%2) value(%3)").arg(factId).arg(provider).arg(fact.value().toString());

    lua_pushnumber(L, fact.value().toDouble());
    return 1;
}

extern "C" int l_getFactDefault(lua_State* L)
{
    Q_ASSERT(_uas);
    FactHandler& factHandler = _uas->getFactHandler();
    
    const char* providerFactId = lua_tostring(L, 1);
    
    Fact::Provider_t    provider;
    QString             factId;
    
    if (!Fact::parseProviderFactIdReference(providerFactId, provider, factId)) {
        lua_pushstring(L, QString("lua.getFactDefault(%1) - unknown provider id").arg(providerFactId).toAscii().constData());
        lua_error(L);
    }
    if (!factHandler.containsFact(provider, factId)) {
        lua_pushstring(L, QString("lua.getFactDefault(%1) - unknown fact id or missing meta data").arg(providerFactId).toAscii().constData());
        lua_error(L);
    }

    Fact& fact = factHandler.getFact(provider, factId);
    
    if (!fact.hasDefaultValue()) {
        lua_pushstring(L, QString("lua.getFactDefault(%1) - no default value for fact").arg(providerFactId).toAscii().constData());
        lua_error(L);
    }
    
    qDebug() << QString("getFactDefault factId(%1) provider(%2) default(%3)").arg(factId).arg(provider).arg(fact.defaultValue());
    
    lua_pushnumber(L, fact.defaultValue());
    return 1;
}

extern "C" int l_isFactNotAtDefault(lua_State* L)
{
    Q_ASSERT(_uas);
    FactHandler& factHandler = _uas->getFactHandler();
    
    const char* providerFactId = lua_tostring(L, 1);
    
    Fact::Provider_t    provider;
    QString             factId;
    
    if (!Fact::parseProviderFactIdReference(providerFactId, provider, factId)) {
        lua_pushstring(L, QString("lua.isFactNotAtDefault(%1) - unknown provider id").arg(providerFactId).toAscii().constData());
        lua_error(L);
    }
    if (!factHandler.containsFact(provider, factId)) {
        lua_pushstring(L, QString("lua.isFactNotAtDefault(%1) - unknown fact id").arg(providerFactId).toAscii().constData());
        lua_error(L);
    }
    
    Fact& fact = factHandler.getFact(provider, factId);
    
    if (!fact.hasDefaultValue()) {
        lua_pushstring(L, QString("lua.isFactNotAtDefault(%1) - no default value for fact").arg(providerFactId).toAscii().constData());
        lua_error(L);
    }

    qDebug() << QString("isFactNotDefault factId(%1) provider(%2) value(%3) default(%4)").arg(factId).arg(provider).arg(fact.value().toString()).arg(fact.defaultValue());
    
    lua_pushboolean(L, fact.value() != fact.defaultValue());
    return 1;
}

extern "C" int l_qDebug(lua_State* L)
{
    const char* msg = lua_tostring(L, 1);
    
    qDebug() << msg;
    
    return 0;
}

FactRuleHandler::FactRuleHandler(QObject* parent) :
    QObject(parent)
{
    
}

void FactRuleHandler::setup(UASInterface* uas)
{
    _uas = uas;
}

/// Evaluates the Lua rules and signals the rule facts
void FactRuleHandler::evaluateRules(void)
{
    Q_ASSERT(_uas);
    FactHandler& factHandler = _uas->getFactHandler();
    
    // Load the rule file into memory
    QFile ruleFile(qApp->applicationDirPath() + _ruleFile);
    qDebug() << ruleFile.fileName();
    Q_ASSERT(ruleFile.exists());
    bool bRet = ruleFile.open(QIODevice::ReadOnly);
    Q_ASSERT(bRet == true);
    QTextStream rulesStream(&ruleFile);
    QString rules = rulesStream.readAll();
    
    // Start up Lua
    lua_State *L = lua_newstate(l_alloc, NULL);
    luaopen_base(L);
    
    // Push in our C Functions
    lua_pushcfunction(L, l_getFactDefault);
    lua_setglobal(L, "getFactDefault");
    lua_pushcfunction(L, l_getFactValue);
    lua_setglobal(L, "getFactValue");
    lua_pushcfunction(L, l_isFactNotAtDefault);
    lua_setglobal(L, "isFactNotAtDefault");
    lua_pushcfunction(L, l_qDebug);
    lua_setglobal(L, "qDebug");
    
    // Evaluate the rules
    if (luaL_loadstring(L, rules.toAscii().constData()) || lua_pcall(L, 0, 0, 0)) {
        // FIXME: This should not be an assert but some sort of error reporting mechanism
        Q_ASSERT_X(false, "FactRuleHandler::evaluateRules", lua_tostring(L, -1));
    }
    
    // Each rule fact should have a corresponding global variable from the lua context
    QStringList     rawFactIds;
    QList<float>    values;
    
    QStringListIterator i = factHandler.factIdIterator(Fact::ruleProvider);
    while (i.hasNext()) {
        QString factId = i.next();
        
        lua_getglobal(L, factId.toAscii().constData());
        double value = lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        rawFactIds += factId;
        values += value;
    }
    
    lua_close(L);

    emit evaluatedRuleData(_uas->getUASID(), rawFactIds, values);
}