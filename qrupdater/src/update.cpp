#include "update.h"

namespace markers {
static QString const selfInstalling = "qru::self";
}

using namespace qrUpdater;

Update::Update(QObject *parent)
	: QObject(parent)
	, mIsInstalled(false)
	, mProcess(nullptr)
{
}

Update::Update(QString const &filePath, QStringList const &args, QString const &version, QObject *parent)
	: QObject(parent)
	, mIsInstalled(false)
	, mProcess(nullptr)
{
	setData(filePath, args, version);
}

void Update::setUrl(QUrl const &link)
{
	mDownloadUrl = link;
}

void Update::setUnitName(QString const &unit)
{
	mModule = unit;
}

void Update::setFilePath(QString const &path)
{
	mFilePath = path;
}

void Update::setData(QString const &filePath, QStringList const &args, QString const &version, QUrl const &link)
{
	mFilePath = filePath;
	mArguments = args;
	mVersion = version;
	mDownloadUrl = link;
	mIsInstalled = false;
}

void Update::clear()
{
	if (QFile::exists(mFilePath)) {
		QFile::remove(mFilePath);
	}
	setData(QString(), QStringList(), QString());
	setUnitName("");
	if (isInstalling()) {
		mProcess->terminate();
	}
	mProcess->deleteLater();
	mProcess = nullptr;
	mIsInstalled = false;
}

void Update::installUpdate()
{
	mProcess = new QProcess(this);
	connect(mProcess, SIGNAL(finished(int, QProcess::ExitStatus))
			, this, SLOT(installationFinished(int, QProcess::ExitStatus)));
	if (!hasSelfInstallMarker()) {
		mProcess->start(mFilePath, mArguments);
	} else {
		mProcess->startDetached(mFilePath, mArguments);
	}
}

bool Update::isEmpty() const
{
	return mFilePath.isEmpty() || !QFile::exists(mFilePath);
}

bool Update::isInstalling() const
{
	return (mProcess != nullptr) && (mProcess->state() != QProcess::NotRunning);
}

bool Update::isInstalled() const
{
	return mIsInstalled;
}

bool Update::hasSelfInstallMarker() const
{
	return unit().contains(markers::selfInstalling);
}

QUrl Update::url() const
{
	return mDownloadUrl;
}

QString Update::unit() const
{
	return mModule;
}

QString Update::filePath() const
{
	return mFilePath;
}

QString Update::fileName() const
{
	return QFileInfo(mFilePath).fileName();
}

QString Update::version() const
{
	return mVersion;
}

QStringList& Update::arguments()
{
	return mArguments;
}

void Update::installationFinished(int exitCode, QProcess::ExitStatus status)
{
	mIsInstalled = true;
	emit installedSuccessfully(status == QProcess::NormalExit);
	Q_UNUSED(exitCode);
}
