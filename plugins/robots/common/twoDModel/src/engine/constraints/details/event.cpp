#include "event.h"

using namespace twoDModel::constraints::details;

Event::Event(const QString &id
		, const Condition &condition
		, const Trigger &trigger
		, bool dropsOnFire)
	: mId(id)
	, mIsAlive(false)
	, mCondition(condition)
	, mTrigger(trigger)
	, mDropsOnFire(dropsOnFire)
{
}

QString Event::id() const
{
	return mId;
}

bool Event::isAlive() const
{
	return mIsAlive;
}

void Event::setUp()
{
	mIsAlive = true;
	emit settedUp();
}

void Event::drop()
{
	mIsAlive = false;
	emit dropped();
}

void Event::check()
{
	if (!mIsAlive || !mCondition()) {
		return;
	}

	emit fired();
	mTrigger();
	if (mDropsOnFire) {
		drop();
	}
}

void Event::setCondition(const Condition &condition)
{
	mCondition = condition;
}