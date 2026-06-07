// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick/QQuickItem>
#include <QElapsedTimer>
#include <QVector>
#include <QVarLengthArray>
#include <QHash>
#include <QSet>
#include <QPointer>
#include <private/qquicksprite_p.h>
#include <QAbstractAnimation>
#include <QtQml/qqml.h>
#include <private/qv4util_p.h>
#include <private/qv4global_p.h>
#include <private/qv4staticvalue_p.h>
#include <private/qtquickparticlesglobal_p.h>

QT_BEGIN_NAMESPACE

template<class T, int Prealloc>
class QQuickParticleVarLengthArray: public QVarLengthArray<T, Prealloc>
{
public:
    void insert(const T &element)
    {
        if (!this->contains(element)) {
            this->append(element);
        }
    }

    bool removeOne(const T &element)
    {
        for (int i = 0; i < this->size(); ++i) {
            if (this->at(i) == element) {
                this->remove(i);
                return true;
            }
        }

        return false;
    }
};

class QQuickParticleSystem;
class QQuickParticleAffector;
class QQuickParticleEmitter;
class QQuickParticlePainter;
class QQuickParticleData;
class QQuickParticleSystemAnimation;
class QQuickStochasticEngine;
class QQuickSprite;
class QQuickV4ParticleData;
class QQuickParticleGroup;
class QQuickImageParticle;

struct QQuickParticleDataHeapNode{
    int time;//in ms
    QSet<QQuickParticleData*> data;//Set ptrs instead?
};

class Q_QUICKPARTICLES_EXPORT QQuickParticleDataHeap {
    //Idea is to do a binary heap, but which also stores a set of int,Node* so that if the int already exists, you can
    //add it to the data* list. Pops return the whole list at once.
public:
    QQuickParticleDataHeap();
    void insert(QQuickParticleData* data);
    void insertTimed(QQuickParticleData* data, int time);

    int top();

    bool isEmpty() const { return m_end == 0; }

    QSet<QQuickParticleData*> pop();

    void clear();

    bool contains(QQuickParticleData*);//O(n), for debugging purposes only
private:
    void grow();
    void swap(int, int);
    void bubbleUp(int);
    void bubbleDown(int);
    int m_size;
    int m_end;
    QQuickParticleDataHeapNode m_tmp;
    QVector<QQuickParticleDataHeapNode> m_data;
    QHash<int,int> m_lookups;
};

class Q_QUICKPARTICLES_EXPORT QQuickParticleGroupData {
    class FreeList
    {
    public:
        FreeList() {}

        void resize(int newSize)
        {
            Q_ASSERT(newSize >= 0);
            int oldSize = isUnused.size();
            isUnused.resize(newSize, true);
            if (newSize > oldSize) {
                if (firstUnused == UINT_MAX) {
                    firstUnused = oldSize;
                } else {
                    firstUnused = std::min(firstUnused, unsigned(oldSize));
                }
            } else if (firstUnused >= unsigned(newSize)) {
                firstUnused = UINT_MAX;
            }
        }

        void free(int index)
        {
            isUnused.setBit(index);
            firstUnused = std::min(firstUnused, unsigned(index));
            --allocated;
        }

        int count() const
        { return allocated; }

        bool hasUnusedEntries() const
        { return firstUnused != UINT_MAX; }

        int alloc()
        {
            if (hasUnusedEntries()) {
                int nextFree = firstUnused;
                isUnused.clearBit(firstUnused);
                firstUnused = isUnused.findNext(firstUnused, true, false);
                if (firstUnused >= unsigned(isUnused.size())) {
                    firstUnused = UINT_MAX;
                }
                ++allocated;
                return nextFree;
            } else {
                return -1;
            }
        }

    private:
        QV4::BitVector isUnused;
        unsigned firstUnused = UINT_MAX;
        int allocated = 0;
    };

public: // types
    typedef int ID;
    enum { InvalidID = -1, DefaultGroupID = 0 };

public:
    QQuickParticleGroupData(const QString &name, QQuickParticleSystem* sys);
    ~QQuickParticleGroupData();

    int size() const
    {
        return m_size;
    }

    bool isActive() { return freeList.count() > 0; }

    QString name() const;

    void setSize(int newSize);

    const ID index;
    QQuickParticleVarLengthArray<QQuickParticlePainter*, 4> painters;//TODO: What if they are dynamically removed?

    //TODO: Refactor particle data list out into a separate class
    QVector<QQuickParticleData*> data;
    FreeList freeList;
    QQuickParticleDataHeap dataHeap;
    bool recycle(); //Force recycling round, returns true if all indexes are now reusable

    void initList();
    void kill(QQuickParticleData* d);

    //After calling this, initialize, then call prepareRecycler(d)
    QQuickParticleData* newDatum(bool respectsLimits);

    //TODO: Find and clean up those that don't get added to the recycler (currently they get lost)
    void prepareRecycler(QQuickParticleData* d);

private:
    int m_size;
    QQuickParticleSystem* m_system;
    // Only used in recycle() for tracking of alive particles after latest recycling round
    QVector<QQuickParticleData*> m_latestAliveParticles;
};

struct Color4ub {
    uchar r;
    uchar g;
    uchar b;
    uchar a;
};

class Q_QUICKPARTICLES_EXPORT QQuickParticleData
{
public:
    //Convenience functions for working backwards, because parameters are from the start of particle life
    //If setting multiple parameters at once, doing the conversion yourself will be faster.

    //sets the x accleration without affecting the instantaneous x velocity or position
    void setInstantaneousAX(float ax, QQuickParticleSystem *particleSystem);
    //sets the x velocity without affecting the instantaneous x postion
    void setInstantaneousVX(float vx, QQuickParticleSystem *particleSystem);
    //sets the instantaneous x postion
    void setInstantaneousX(float x, QQuickParticleSystem *particleSystem);
    //sets the y accleration without affecting the instantaneous y velocity or position
    void setInstantaneousAY(float ay, QQuickParticleSystem *particleSystem);
    //sets the y velocity without affecting the instantaneous y postion
    void setInstantaneousVY(float vy, QQuickParticleSystem *particleSystem);
    //sets the instantaneous Y postion
    void setInstantaneousY(float y, QQuickParticleSystem *particleSystem);

    //TODO: Slight caching?
    float curX(QQuickParticleSystem *particleSystem) const;
    float curVX(QQuickParticleSystem *particleSystem) const;
    float curAX() const { return ax; }
    float curAX(QQuickParticleSystem *) const { return ax; } // used by the macros in qquickv4particledata.cpp
    float curY(QQuickParticleSystem *particleSystem) const;
    float curVY(QQuickParticleSystem *particleSystem) const;
    float curAY() const { return ay; }
    float curAY(QQuickParticleSystem *) const { return ay; } // used by the macros in qquickv4particledata.cpp

    int index = 0;
    int systemIndex = -1;

    //General Position Stuff
    float x = 0;
    float y = 0;
    float t = -1;
    float lifeSpan = 0;
    float size = 0;
    float endSize = 0;
    float vx = 0;
    float vy = 0;
    float ax = 0;
    float ay = 0;

    //Painter-specific stuff, now universally shared
    //Used by ImageParticle color mode
    Color4ub color = { 255, 255, 255, 255};
    //Used by ImageParticle deform mode
    float xx = 1;
    float xy = 0;
    float yx = 0;
    float yy = 1;
    float rotation = 0;
    float rotationVelocity = 0;
    uchar autoRotate = 0; // Basically a bool
    //Used by ImageParticle Sprite mode
    float animIdx = 0;
    float frameDuration = 1;
    float frameAt = -1;//Used for duration -1
    float frameCount = 1;
    float animT = -1;
    float animX = 0;
    float animY = 0;
    float animWidth = 1;
    float animHeight = 1;

    QQuickParticleGroupData::ID groupId = 0;

    //Used by ImageParticle data shadowing
    QQuickImageParticle* colorOwner = nullptr;
    QQuickImageParticle* rotationOwner = nullptr;
    QQuickImageParticle* deformationOwner = nullptr;
    QQuickImageParticle* animationOwner = nullptr;

    //Used by ItemParticle
    QQuickItem* delegate = nullptr;
    //Used by custom affectors
    float update = 0;

    void debugDump(QQuickParticleSystem *particleSystem) const;
    bool stillAlive(QQuickParticleSystem *particleSystem) const; //Only checks end, because usually that's all you need and it's a little faster.
    bool alive(QQuickParticleSystem *particleSystem) const;
    float lifeLeft(QQuickParticleSystem *particleSystem) const;

    float curSize(QQuickParticleSystem *particleSystem) const;

    QQuickV4ParticleData v4Value(QQuickParticleSystem *particleSystem);
    void extendLife(float time, QQuickParticleSystem *particleSystem);

    static inline constexpr float EPSILON() noexcept { return 0.001f; }
};

static_assert(std::is_trivially_copyable_v<QQuickParticleData>);
static_assert(std::is_trivially_destructible_v<QQuickParticleData>);

class Q_QUICKPARTICLES_EXPORT QQuickParticleSystem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY emptyChanged)
    QML_NAMED_ELEMENT(ParticleSystem)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickParticleSystem(QQuickItem *parent = nullptr);
    ~QQuickParticleSystem();

    bool isRunning() const
    {
        return m_running;
    }

    int count() const
    {
        return particleCount;
    }

    static const int maxLife = 600000;

Q_SIGNALS:

    void systemInitialized();
    void runningChanged(bool arg);
    void pausedChanged(bool arg);
    void emptyChanged(bool arg);

public Q_SLOTS:
    void start(){setRunning(true);}
    void stop(){setRunning(false);}
    void restart(){setRunning(false);setRunning(true);}
    void pause(){setPaused(true);}
    void resume(){setPaused(false);}

    void reset();
    void setRunning(bool arg);
    void setPaused(bool arg);

    virtual int duration() const { return -1; }


protected:
    //This one only once per frame (effectively)
    void componentComplete() override;

private Q_SLOTS:
    void emittersChanged();
    void loadPainter(QQuickParticlePainter *p);
    void createEngine(); //Not invoked by sprite engine, unlike Sprite uses
    void particleStateChange(int idx);

public:
    //These can be called multiple times per frame, performance critical
    void emitParticle(QQuickParticleData* p, QQuickParticleEmitter *particleEmitter);
    QQuickParticleData *newDatum(
            int groupId, bool respectLimits = true, int sysIdx = -1,
            const QQuickParticleData *cloneFrom = nullptr);
    void finishNewDatum(QQuickParticleData*);
    void moveGroups(QQuickParticleData *d, int newGIdx);
    int nextSystemIndex();

    //This one only once per painter per frame
    int systemSync(QQuickParticlePainter* p);

    //Data members here for ease of related class and auto-test usage. Not "public" API. TODO: d_ptrize
    QSet<QQuickParticleData*> needsReset;
    QVector<QQuickParticleData*> bySysIdx; //Another reference to the data (data owned by group), but by sysIdx
    QQuickStochasticEngine* stateEngine;

    QHash<QString, int> groupIds;
    QVarLengthArray<QQuickParticleGroupData*, 32> groupData;
    int nextFreeGroupId;
    int registerParticleGroupData(const QString &name, QQuickParticleGroupData *pgd);

    //Also only here for auto-test usage
    void updateCurrentTime( int currentTime );
    QQuickParticleSystemAnimation* m_animation;
    bool m_running;
    bool m_debugMode;

    int timeInt;
    bool initialized;
    int particleCount;

    void registerParticlePainter(QQuickParticlePainter* p);
    void registerParticleEmitter(QQuickParticleEmitter* e);
    void finishRegisteringParticleEmitter(QQuickParticleEmitter *e);
    void registerParticleAffector(QQuickParticleAffector* a);
    void registerParticleGroup(QQuickParticleGroup* g);

    static void statePropertyRedirect(QQmlListProperty<QObject> *prop, QObject *value);
    static void stateRedirect(QQuickParticleGroup* group, QQuickParticleSystem* sys, QObject *value);
    bool isPaused() const
    {
        return m_paused;
    }

    bool isEmpty() const
    {
        return m_empty;
    }

private:
    void searchNextFreeGroupId();

private:
    void emitterAdded(QQuickParticleEmitter *e);
    void postProcessEmitters();
    void initializeSystem();
    void initGroups();
    QList<QPointer<QQuickParticleEmitter> > m_emitters;
    QList<QPointer<QQuickParticleAffector> > m_affectors;
    QList<QPointer<QQuickParticlePainter> > m_painters;
    QList<QPointer<QQuickParticlePainter> > m_syncList;
    QList<QQuickParticleGroup*> m_groups;
    int m_nextIndex;
    QSet<int> m_reusableIndexes;
    bool m_componentComplete;

    bool m_paused;
    bool m_allDead;
    bool m_empty;
};

// Internally, this animation drives all the timing. Painters sync up in their updatePaintNode
class QQuickParticleSystemAnimation : public QAbstractAnimation
{
    Q_OBJECT
public:
    QQuickParticleSystemAnimation(QQuickParticleSystem* system)
        : QAbstractAnimation(static_cast<QObject*>(system)), m_system(system)
    { }
protected:
    void updateCurrentTime(int t) override
    {
        m_system->updateCurrentTime(t);
    }

    int duration() const override
    {
        return -1;
    }

private:
    QQuickParticleSystem* m_system;
};

inline void QQuickParticleData::setInstantaneousAX(float ax, QQuickParticleSystem* particleSystem)
{
    float t = (particleSystem->timeInt / 1000.0f) - this->t;
    float t_sq = t * t;
    float vx = (this->vx + t * this->ax) - t * ax;
    float ex = this->x + this->vx * t + 0.5f * this->ax * t_sq;
    float x = ex - t * vx - 0.5f * t_sq * ax;

    this->ax = ax;
    this->vx = vx;
    this->x = x;
}

inline void QQuickParticleData::setInstantaneousVX(float vx, QQuickParticleSystem* particleSystem)
{
    float t = (particleSystem->timeInt / 1000.0f) - this->t;
    float t_sq = t * t;
    float evx = vx - t * this->ax;
    float ex = this->x + this->vx * t + 0.5f * this->ax * t_sq;
    float x = ex - t * evx - 0.5f * t_sq * this->ax;

    this->vx = evx;
    this->x = x;
}

inline void QQuickParticleData::setInstantaneousX(float x, QQuickParticleSystem* particleSystem)
{
    float t = (particleSystem->timeInt / 1000.0f) - this->t;
    float t_sq = t * t;
    this->x = x - t * this->vx - 0.5f * t_sq * this->ax;
}

inline void QQuickParticleData::setInstantaneousAY(float ay, QQuickParticleSystem* particleSystem)
{
    float t = (particleSystem->timeInt / 1000.0f) - this->t;
    float t_sq = t * t;
    float vy = (this->vy + t * this->ay) - t * ay;
    float ey = this->y + this->vy * t + 0.5f * this->ay * t_sq;
    float y = ey - t * vy - 0.5f * t_sq * ay;

    this->ay = ay;
    this->vy = vy;
    this->y = y;
}

inline void QQuickParticleData::setInstantaneousVY(float vy, QQuickParticleSystem* particleSystem)
{
    float t = (particleSystem->timeInt / 1000.0f) - this->t;
    float t_sq = t * t;
    float evy = vy - t * this->ay;
    float ey = this->y + this->vy * t + 0.5f * this->ay * t_sq;
    float y = ey - t*evy - 0.5f * t_sq * this->ay;

    this->vy = evy;
    this->y = y;
}

inline void QQuickParticleData::setInstantaneousY(float y, QQuickParticleSystem *particleSystem)
{
    float t = (particleSystem->timeInt / 1000.0f) - this->t;
    float t_sq = t * t;
    this->y = y - t * this->vy - 0.5f * t_sq * this->ay;
}

inline float QQuickParticleData::curX(QQuickParticleSystem *particleSystem) const
{
    float t = (particleSystem->timeInt / 1000.0f) - this->t;
    float t_sq = t * t;
    return this->x + this->vx * t + 0.5f * this->ax * t_sq;
}

inline float QQuickParticleData::curVX(QQuickParticleSystem *particleSystem) const
{
    float t = (particleSystem->timeInt / 1000.0f) - this->t;
    return this->vx + t * this->ax;
}

inline float QQuickParticleData::curY(QQuickParticleSystem *particleSystem) const
{
    float t = (particleSystem->timeInt / 1000.0f) - this->t;
    float t_sq = t * t;
    return y + vy * t + 0.5f * ay * t_sq;
}

inline float QQuickParticleData::curVY(QQuickParticleSystem *particleSystem) const
{
    float t = (particleSystem->timeInt / 1000.0f) - this->t;
    return vy + t*ay;
}

inline bool QQuickParticleData::stillAlive(QQuickParticleSystem* system) const
{
    if (!system)
        return false;
    return (t + lifeSpan - EPSILON()) > (system->timeInt / 1000.0f);
}

inline bool QQuickParticleData::alive(QQuickParticleSystem* system) const
{
    if (!system)
        return false;
    float st = (system->timeInt / 1000.0f);
    return (t + EPSILON()) < st && (t + lifeSpan - EPSILON()) > st;
}

inline float QQuickParticleData::lifeLeft(QQuickParticleSystem *particleSystem) const
{
    if (!particleSystem)
        return 0.0f;
    return (t + lifeSpan) - (particleSystem->timeInt / 1000.0f);
}

inline float QQuickParticleData::curSize(QQuickParticleSystem *particleSystem) const
{
    if (!particleSystem || lifeSpan == 0.0f)
        return 0.0f;
    return size + (endSize - size) * (1 - (lifeLeft(particleSystem) / lifeSpan));
}

QT_END_NAMESPACE

#endif // PARTICLESYSTEM_H


