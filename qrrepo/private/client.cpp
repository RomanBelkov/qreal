#include "client.h"

#include <QtCore/QDebug>

#include "../../qrkernel/exception/exception.h"
#include "singleXmlSerializer.h"

using namespace qReal;
using namespace qrRepo;
using namespace qrRepo::details;

Client::Client(QString const &workingFile)
		: mWorkingFile(workingFile)
		, mSerializer(workingFile)
{
	init();
	loadFromDisk();
}


void Client::init()
{
	mObjects.insert(Id::rootId(), new LogicalObject(Id::rootId()));
	mObjects[Id::rootId()]->setProperty("name", Id::rootId().toString());
}

Client::~Client()
{
	mSerializer.clearWorkingDir();

	foreach (Id id, mObjects.keys()) {
		delete mObjects[id];
	}
}

IdList Client::findElementsByName(QString const &name, bool sensitivity, bool regExpression) const
{
	Qt::CaseSensitivity caseSensitivity = sensitivity ? Qt::CaseSensitive : Qt::CaseInsensitive;

	QRegExp *regExp = new QRegExp(name, caseSensitivity);
	IdList result;

	if (regExpression){
		foreach (Object *element, mObjects.values()) {
			if (element->property("name").toString().contains(*regExp)
					&& !isLogicalId(mObjects.key(element))) {
				result.append(mObjects.key(element));
			}
		}
	} else {
		foreach (Object *element, mObjects.values()) {
			if (element->property("name").toString().contains(name, caseSensitivity)
					&& !isLogicalId(mObjects.key(element))) {
				result.append(mObjects.key(element));
			}
		}
	}

	return result;
}

qReal::IdList Client::elementsByProperty(QString const &property, bool sensitivity
		, bool regExpression) const
{
	IdList result;

	foreach (Object *element, mObjects.values()) {
		if ((element->hasProperty(property, sensitivity, regExpression))
				&& (!isLogicalId(mObjects.key(element)))) {
			result.append(mObjects.key(element));
		}
	}

	return result;
}

qReal::IdList Client::elementsByPropertyContent(QString const &propertyValue, bool sensitivity
		, bool regExpression) const
{
	Qt::CaseSensitivity caseSensitivity;

	if (sensitivity) {
		caseSensitivity = Qt::CaseSensitive;
	} else {
		caseSensitivity = Qt::CaseInsensitive;
	}

	QRegExp *regExp = new QRegExp(propertyValue, caseSensitivity);
	IdList result;

	foreach (Object *element, mObjects.values()) {
		QMapIterator<QString, QVariant> iterator = element->propertiesIterator();
		if (regExpression) {
			while (iterator.hasNext()) {
				if (iterator.next().value().toString().contains(*regExp)) {
					result.append(mObjects.key(element));
					break;
				}
			}
		} else {
			while (iterator.hasNext()) {
				if (iterator.next().value().toString().contains(propertyValue, caseSensitivity)) {
					result.append(mObjects.key(element));
					break;
				}
			}
		}
	}

	return result;
}

void Client::replaceProperties(qReal::IdList const &toReplace, QString const value, QString const newValue)
{
	foreach (qReal::Id currentId, toReplace) {
		mObjects[currentId]->replaceProperties(value, newValue);
	}
}

IdList Client::children(Id const &id) const
{
	if (mObjects.contains(id)) {
		return mObjects[id]->children();
	} else {
		throw Exception("Client: Requesting children of nonexistent object " + id.toString());
	}
}

Id Client::parent(Id const &id) const
{
	if (mObjects.contains(id)) {
		return mObjects[id]->parent();
	} else {
		throw Exception("Client: Requesting parents of nonexistent object " + id.toString());
	}
}

Id Client::cloneObject(const qReal::Id &id)
{
	Object *result = mObjects[id]->clone(mObjects);
	return result->id();
}

void Client::setParent(Id const &id, Id const &parent)
{
	if (mObjects.contains(id)) {
		if (mObjects.contains(parent)) {
			mObjects[id]->setParent(parent);
			if (!mObjects[parent]->children().contains(id))
				mObjects[parent]->addChild(id);
		} else {
			throw Exception("Client: Adding nonexistent parent " + parent.toString() + " to  object " + id.toString());
		}
	} else {
		throw Exception("Client: Adding parent " + parent.toString() + " to nonexistent object " + id.toString());
	}
}

void Client::addChild(const Id &id, const Id &child)
{
	addChild(id, child, Id());
}

void Client::addChild(const Id &id, const Id &child, Id const &logicalId)
{
	if (mObjects.contains(id)) {
		if (!mObjects[id]->children().contains(child))
			mObjects[id]->addChild(child);

		if (mObjects.contains(child)) { // should we move element?
			mObjects[child]->setParent(id);
		} else {
			Object * const object = logicalId == Id()
					? static_cast<Object *>(new LogicalObject(child, id))
					: static_cast<Object *>(new GraphicalObject(child, id, logicalId))
					;

			mObjects.insert(child, object);
		}
	} else {
		throw Exception("Client: Adding child " + child.toString() + " to nonexistent object " + id.toString());
	}
}

void Client::stackBefore(qReal::Id const &id, qReal::Id const &child, qReal::Id const &sibling) {
	if(!mObjects.contains(id)) {
		throw Exception("Client: Moving child " + child.toString() + " of nonexistent object " + id.toString());
	}

	if(!mObjects.contains(child)) {
		throw Exception("Client: Moving nonexistent child " + child.toString());
	}

	if(!mObjects.contains(sibling)) {
		throw Exception("Client: Stacking before nonexistent child " + sibling.toString());
	}

	mObjects[id]->stackBefore(child, sibling);
}

void Client::removeParent(const Id &id)
{
	if (mObjects.contains(id)) {
		Id const parent = mObjects[id]->parent();
		if (mObjects.contains(parent)) {
			mObjects[id]->removeParent();
			mObjects[parent]->removeChild(id);
		} else {
			throw Exception("Client: Removing nonexistent parent " + parent.toString() + " from object " + id.toString());
		}
	} else {
		throw Exception("Client: Removing parent from nonexistent object " + id.toString());
	}
}

void Client::removeChild(const Id &id, const Id &child)
{
	if (mObjects.contains(id)) {
		if (mObjects.contains(child)) {
			mObjects[id]->removeChild(child);
		} else {
			throw Exception("Client: removing nonexistent child " + child.toString() + " from object " + id.toString());
		}
	} else {
		throw Exception("Client: removing child " + child.toString() + " from nonexistent object " + id.toString());
	}
}

void Client::setProperty(const Id &id, QString const &name, const QVariant &value ) const
{
	if (mObjects.contains(id)) {
		// see Object::property() for details
//		Q_ASSERT(mObjects[id]->hasProperty(name)
//				 ? mObjects[id]->property(name).userType() == value.userType()
//				 : true);
		mObjects[id]->setProperty(name, value);
	} else {
		throw Exception("Client: Setting property of nonexistent object " + id.toString());
	}
}

void Client::copyProperties(const Id &dest, const Id &src)
{
	mObjects[dest]->copyPropertiesFrom(*mObjects[src]);
}

QMap<QString, QVariant> Client::properties(Id const &id)
{
	return mObjects[id]->properties();
}

void Client::setProperties(Id const &id, QMap<QString, QVariant> const &properties)
{
	mObjects[id]->setProperties(properties);
}

QVariant Client::property( const Id &id, QString const &name ) const
{
	if (mObjects.contains(id)) {
		return mObjects[id]->property(name);
	} else {
		throw Exception("Client: Requesting property of nonexistent object " + id.toString());
	}
}

void Client::removeProperty( const Id &id, QString const &name )
{
	if (mObjects.contains(id)) {
		return mObjects[id]->removeProperty(name);
	} else {
		throw Exception("Client: Removing property of nonexistent object " + id.toString());
	}
}

bool Client::hasProperty(const Id &id, QString const &name, bool sensitivity, bool regExpression) const
{
	if (mObjects.contains(id)) {
		return mObjects[id]->hasProperty(name, sensitivity, regExpression);
	} else {
		throw Exception("Client: Checking the existence of a property '" + name + "' of nonexistent object " + id.toString());
	}
}

void Client::setBackReference(Id const &id, Id const &reference) const
{
	if (mObjects.contains(id)) {
		if (mObjects.contains(reference)) {
			mObjects[id]->setBackReference(reference);
		} else {
			throw Exception("Client: setting nonexistent back reference " + reference.toString()
							+ " to object " + id.toString());
		}
	} else {
		throw Exception("Client: setting back reference of nonexistent object " + id.toString());
	}
}

void Client::removeBackReference(Id const &id, Id const &reference) const
{
	if (mObjects.contains(id)) {
		if (mObjects.contains(reference)) {
			mObjects[id]->removeBackReference(reference);
		} else {
			throw Exception("Client: removing nonexistent back reference " + reference.toString()
							+ " of object " + id.toString());
		}
	} else {
		throw Exception("Client: removing back reference of nonexistent object " + id.toString());
	}
}

void Client::setTemporaryRemovedLinks(Id const &id, QString const &direction, qReal::IdList const &linkIdList)
{
	if (mObjects.contains(id)) {
		mObjects[id]->setTemporaryRemovedLinks(direction, linkIdList);
	} else {
		throw Exception("Client: Setting temporaryRemovedLinks of nonexistent object " + id.toString());
	}
}

IdList Client::temporaryRemovedLinksAt(Id const &id, QString const &direction) const
{
	if (mObjects.contains(id)) {
		return mObjects[id]->temporaryRemovedLinksAt(direction);
	} else {
		throw Exception("Client: Requesting temporaryRemovedLinks of nonexistent object " + id.toString());
	}
}

IdList Client::temporaryRemovedLinks(Id const &id) const
{
	if (mObjects.contains(id)) {
		return mObjects[id]->temporaryRemovedLinks();
	} else {
		throw Exception("Client: Requesting temporaryRemovedLinks of nonexistent object " + id.toString());
	}
}

void Client::removeTemporaryRemovedLinks(Id const &id)
{
	if (mObjects.contains(id)) {
		return mObjects[id]->removeTemporaryRemovedLinks();
	} else {
		throw Exception("Client: Removing temporaryRemovedLinks of nonexistent object " + id.toString());
	}
}

void Client::loadFromDisk()
{
	mSerializer.loadFromDisk(mObjects);
	addChildrenToRootObject();
}

void Client::importFromDisk(QString const &importedFile)
{
	mSerializer.setWorkingFile(importedFile);
	loadFromDisk();
	mSerializer.setWorkingFile(mWorkingFile);
}

void Client::addChildrenToRootObject()
{
	foreach (Object *object, mObjects.values()) {
		if (object->parent() == Id::rootId()) {
			if (!mObjects[Id::rootId()]->children().contains(object->id()))
				mObjects[Id::rootId()]->addChild(object->id());
		}
	}
}

IdList Client::idsOfAllChildrenOf(Id id) const
{
	IdList result;
	result.clear();
	result.append(id);
	IdList list = mObjects[id]->children();
	foreach(Id const &childId, list)
		result.append(idsOfAllChildrenOf(childId));
	return result;
}

QList<Object*> Client::allChildrenOf(Id id) const
{
	QList<Object*> result;
	result.append(mObjects[id]);
	foreach(Id const &childId, mObjects[id]->children())
		result.append(allChildrenOf(childId));
	return result;
}

QList<Object*> Client::allChildrenOfWithLogicalId(Id id) const
{
	QList<Object*> result;
	result.append(mObjects[id]);

	// along with each ID we also add its logical ID.

	foreach(Id const &childId, mObjects[id]->children())
		result << allChildrenOf(childId)
				<< allChildrenOf(logicalId(childId));
	return result;
}

bool Client::exist(const Id &id) const
{
	return (mObjects[id] != NULL);
}

void Client::saveAll() const
{
	mSerializer.saveToDisk(mObjects.values());
}

void Client::save(IdList list) const
{
	QList<Object*> toSave;
	foreach(Id const &id, list)
		toSave.append(allChildrenOf(id));

	mSerializer.saveToDisk(toSave);
}

void Client::saveWithLogicalId(qReal::IdList list) const
{
	QList<Object*> toSave;
	foreach(Id const &id, list)
		toSave.append(allChildrenOfWithLogicalId(id));

	mSerializer.saveToDisk(toSave);
}

void Client::saveDiagramsById(QHash<QString, IdList> const &diagramIds)
{
	QString const currentWorkingFile = mWorkingFile;
	foreach (QString const &savePath, diagramIds.keys()) {
		qReal::IdList diagrams = diagramIds[savePath];
		setWorkingFile(savePath);
		qReal::IdList elementsToSave;
		foreach (qReal::Id const &id, diagrams) {
			elementsToSave += idsOfAllChildrenOf(id);
			// id is a graphical ID for this diagram
			// we have to add logical diagram ID
			// to this list manually
			elementsToSave += logicalId(id);
		}
		saveWithLogicalId(elementsToSave);
	}
	setWorkingFile(currentWorkingFile);
}

void Client::remove(IdList list) const
{
	foreach(Id const &id, list) {
		qDebug() << id.toString();
		mSerializer.removeFromDisk(id);
	}
}

void Client::remove(const qReal::Id &id)
{
	if (mObjects.contains(id)) {
		delete mObjects[id];
		mObjects.remove(id);
	} else {
		throw Exception("Client: Trying to remove nonexistent object " + id.toString());
	}
}

void Client::setWorkingFile(QString const &workingFile)
{
	mSerializer.setWorkingFile(workingFile);
	mWorkingFile = workingFile;
}

void Client::exportToXml(QString const &targetFile) const
{
	SingleXmlSerializer::exportToXml(targetFile, mObjects);
}

QString Client::workingFile() const
{
	return mWorkingFile;
}

void Client::printDebug() const
{
	qDebug() << mObjects.size() << " objects in repository";
	foreach (Object *object, mObjects.values()) {
		qDebug() << object->id().toString();
		qDebug() << "Children:";
		foreach (Id id, object->children())
			qDebug() << id.toString();
		qDebug() << "Parent:";
		qDebug() << object->parent().toString();
		qDebug() << "============";
	}
}

void Client::exterminate()
{
	printDebug();
	mObjects.clear();
	//serializer.clearWorkingDir();
	mSerializer.saveToDisk(mObjects.values());
	init();
	printDebug();
}

void Client::open(QString const &saveFile)
{
	mObjects.clear();
	init();
	mSerializer.setWorkingFile(saveFile);
	loadFromDisk();
}

qReal::IdList Client::elements() const
{
	return mObjects.keys();
}

bool Client::isLogicalId(qReal::Id const &elem) const
{
	return mObjects[elem]->isLogicalObject();
}

qReal::Id Client::logicalId(qReal::Id const &elem) const
{
	GraphicalObject const * const graphicalObject = dynamic_cast<GraphicalObject *>(mObjects[elem]);
	if (!graphicalObject) {
		throw Exception("Trying to get logical id from non-graphical object");
	}

	return graphicalObject->logicalId();
}

QMapIterator<QString, QVariant> Client::propertiesIterator(qReal::Id const &id) const
{
	return mObjects[id]->propertiesIterator();
}

void Client::createGraphicalPart(qReal::Id const &id, int partIndex)
{
	GraphicalObject * const graphicalObject = dynamic_cast<GraphicalObject *>(mObjects[id]);
	if (!graphicalObject) {
		throw Exception("Trying to create graphical part for non-graphical object");
	}

	graphicalObject->createGraphicalPart(partIndex);
}

QVariant Client::graphicalPartProperty(qReal::Id const &id, int partIndex, QString const &propertyName) const
{
	GraphicalObject * const graphicalObject = dynamic_cast<GraphicalObject *>(mObjects[id]);
	if (!graphicalObject) {
		throw Exception("Trying to obtain graphical part property for non-graphical item");
	}

	return graphicalObject->graphicalPartProperty(partIndex, propertyName);
}

void Client::setGraphicalPartProperty(
		qReal::Id const &id
		, int partIndex
		, QString const &propertyName
		, QVariant const &value
		)
{
	GraphicalObject * const graphicalObject = dynamic_cast<GraphicalObject *>(mObjects[id]);
	if (!graphicalObject) {
		throw Exception("Trying to obtain graphical part property for non-graphical item");
	}

	graphicalObject->setGraphicalPartProperty(partIndex, propertyName, value);
}
