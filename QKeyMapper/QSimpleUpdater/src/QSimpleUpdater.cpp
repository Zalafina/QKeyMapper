/*
 * Copyright (c) 2014-2021 Alex Spataru <https://github.com/alex-spataru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "QSimpleUpdater.h"
#include "Updater.h"
#include <qregularexpression.h>

static QList<QString> URLS;
static QList<Updater *> UPDATERS;

QSimpleUpdater::~QSimpleUpdater()
{
   URLS.clear();

   foreach (Updater *updater, UPDATERS)
      updater->deleteLater();

   UPDATERS.clear();
}

/**
 * Returns the only instance of the class
 */
QSimpleUpdater *QSimpleUpdater::getInstance()
{
   static QSimpleUpdater updater;
   return &updater;
}

bool QSimpleUpdater::compareVersions(const QString &remote, const QString &local)
{
   static QRegularExpression re("v?(\\d+)(?:\\.(\\d+))?(?:\\.(\\d+))?(?:-(\\w+)(?:(\\d+))?)?");
   QRegularExpressionMatch remoteMatch = re.match(remote);
   QRegularExpressionMatch localMatch = re.match(local);

   if (!remoteMatch.hasMatch() || !localMatch.hasMatch())
   {
      // Invalid version format
      return false;
   }

   for (int i = 1; i <= 3; ++i)
   {
      int remoteNum = remoteMatch.captured(i).toInt();
      int localNum = localMatch.captured(i).toInt();

      if (remoteNum > localNum)
         return true;
      else if (localNum > remoteNum)
         return false;
   }

   QString remoteSuffix = remoteMatch.captured(4);
   QString localSuffix = localMatch.captured(4);

   if (remoteSuffix.isEmpty() && !localSuffix.isEmpty())
      // Remote is stable, local is pre-release
      return true;
   if (!remoteSuffix.isEmpty() && localSuffix.isEmpty())
      // Remote is pre-release, local is stable
      return false;
   if (remoteSuffix != localSuffix)
      // Compare suffixes lexicographically
      return remoteSuffix > localSuffix;

   int remoteSuffixNum = remoteMatch.captured(5).toInt();
   int localSuffixNum = localMatch.captured(5).toInt();
   return remoteSuffixNum > localSuffixNum;
}

bool QSimpleUpdater::compareVersionsForQKeyMapper(const QString &remote, const QString &local)
{
    static QRegularExpression re("v?(\\d+)(?:\\.(\\d+))?(?:\\.(\\d+))?(?:-(\\w+)(?:(\\d+))?)?");
    QRegularExpressionMatch remoteMatch = re.match(remote);
    QRegularExpressionMatch localMatch = re.match(local);

    if (!remoteMatch.hasMatch() || !localMatch.hasMatch())
    {
        // Invalid version format
        return false;
    }

    for (int i = 1; i <= 3; ++i)
    {
        int remoteNum = remoteMatch.captured(i).toInt();
        int localNum = localMatch.captured(i).toInt();

        if (remoteNum > localNum)
            return true;
        else if (localNum > remoteNum)
            return false;
    }

    QString remoteSuffix = remoteMatch.captured(4);
    QString localSuffix = localMatch.captured(4);

    if (remoteSuffix.isEmpty() && !localSuffix.isEmpty())
        // Remote is stable, local is pre-release
        return true;
    if (!remoteSuffix.isEmpty() && localSuffix.isEmpty())
        // Remote is pre-release, local is stable
        return false;
    if (remoteSuffix != localSuffix)
        // Compare suffixes lexicographically
        return remoteSuffix > localSuffix;

    int remoteSuffixNum = remoteMatch.captured(5).toInt();
    int localSuffixNum = localMatch.captured(5).toInt();
    return remoteSuffixNum > localSuffixNum;
}

/**
 * Returns \c true if the \c Updater instance registered with the given \a url
 * uses a custom appcast format and/or allows the application to read and
 * interpret the downloaded appcast file
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
bool QSimpleUpdater::usesCustomAppcast(const QString &url) const
{
   return getUpdater(url)->customAppcast();
}

/**
 * Returns \c true if the \c Updater instance registered with the given \a url
 * shall notify the user when an update is available.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
bool QSimpleUpdater::getNotifyOnUpdate(const QString &url) const
{
   return getUpdater(url)->notifyOnUpdate();
}

/**
 * Returns \c true if the \c Updater instance registered with the given \a url
 * shall notify the user when it finishes checking for updates.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
bool QSimpleUpdater::getNotifyOnFinish(const QString &url) const
{
   return getUpdater(url)->notifyOnFinish();
}

/**
 * Returns \c true if the \c Updater instance registered with the given \a url
 * has an update available.
 *
 * \warning You should call \c checkForUpdates() before using this function
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
bool QSimpleUpdater::getUpdateAvailable(const QString &url) const
{
   return getUpdater(url)->updateAvailable();
}

/**
 * Returns \c true if the \c Updater instance registered with the given \a url
 * has the integrated downloader enabled.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
bool QSimpleUpdater::getDownloaderEnabled(const QString &url) const
{
   return getUpdater(url)->downloaderEnabled();
}

/**
 * Returns \c true if the \c Updater instance registered with the given \a url
 * shall try to open the downloaded file.
 *
 * If you want to implement your own way to handle the downloaded file, just
 * bind to the \c downloadFinished() signal and disable the integrated
 * downloader with the \c setUseCustomInstallProcedures() function.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
bool QSimpleUpdater::usesCustomInstallProcedures(const QString &url) const
{
   return getUpdater(url)->useCustomInstallProcedures();
}

/**
 * Returns the URL to open in a web browser of the \c Updater instance
 * registered with the given \a url.
 *
 * \note If the module name is empty, then the \c Updater will use the
 *       application name as its module name.
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
QString QSimpleUpdater::getOpenUrl(const QString &url) const
{
   return getUpdater(url)->openUrl();
}

/**
 * Returns the changelog of the \c Updater instance registered with the given
 * \a url.
 *
 * \warning You should call \c checkForUpdates() before using this function
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
QString QSimpleUpdater::getChangelog(const QString &url) const
{
   return getUpdater(url)->changelog();
}

/**
 * Returns the module name of the \c Updater instance registered with the given
 * \a url.
 *
 * \note If the module name is empty, then the \c Updater will use the
 *       application name as its module name.
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
QString QSimpleUpdater::getModuleName(const QString &url) const
{
   return getUpdater(url)->moduleName();
}

/**
 * Returns the download URL of the \c Updater instance registered with the given
 * \a url.
 *
 * \warning You should call \c checkForUpdates() before using this function
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
QString QSimpleUpdater::getDownloadUrl(const QString &url) const
{
   return getUpdater(url)->downloadUrl();
}

/**
 * Returns the platform key of the \c Updater registered with the given \a url.
 * If you do not define a platform key, the system will assign the following
 * platform key:
 *    - On iOS: \c ios
 *    - On Mac OSX: \c osx
 *    - On Android: \c android
 *    - On GNU/Linux: \c linux
 *    - On Microsoft Windows: \c windows
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
QString QSimpleUpdater::getPlatformKey(const QString &url) const
{
   return getUpdater(url)->platformKey();
}

/**
 * Returns the remote module version of the \c Updater instance registered with
 * the given \a url.
 *
 * \warning You should call \c checkForUpdates() before using this function
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
QString QSimpleUpdater::getLatestVersion(const QString &url) const
{
   return getUpdater(url)->latestVersion();
}

/**
 * Returns the module version of the \c Updater instance registered with the
 * given \a url.
 *
 * \note If the module version is empty, then the \c Updater will use the
 *       application version as its module version.
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
QString QSimpleUpdater::getModuleVersion(const QString &url) const
{
   return getUpdater(url)->moduleVersion();
}

/**
 * Returns the user-agent string used by the updater to communicate with
 * the remote HTTP(S) server.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
QString QSimpleUpdater::getUserAgentString(const QString &url) const
{
   return getUpdater(url)->userAgentString();
}

/**
 * Instructs the \c Updater instance with the registered \c url to download and
 * interpret the update definitions file.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
void QSimpleUpdater::checkForUpdates(const QString &url)
{
   getUpdater(url)->checkForUpdates();
}

void QSimpleUpdater::setDownloadDir(const QString &url, const QString &dir)
{
   getUpdater(url)->setDownloadDir(dir);
}

/**
 * Changes the module \a name of the \c Updater instance registered at the
 * given \a url.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 * \note The module name is used on the user prompts. If the module name is
 *       empty, then the prompts will show the name of the application.
 */
void QSimpleUpdater::setModuleName(const QString &url, const QString &name)
{
   getUpdater(url)->setModuleName(name);
}

/**
 * If \a notify is set to \c true, then the \c Updater instance registered with
 * the given \a url will notify the user when an update is available.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
void QSimpleUpdater::setNotifyOnUpdate(const QString &url, const bool notify)
{
   getUpdater(url)->setNotifyOnUpdate(notify);
}

/**
 * If \a notify is set to \c true, then the \c Updater instance registered with
 * the given \a url will notify the user when it has finished interpreting the
 * update definitions file.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
void QSimpleUpdater::setNotifyOnFinish(const QString &url, const bool notify)
{
   getUpdater(url)->setNotifyOnFinish(notify);
}

/**
 * Changes the platform key of the \c Updater isntance registered at the given
 * \a url.
 *
 * If the platform key is empty, then the system will use the following keys:
 *    - On iOS: \c ios
 *    - On Mac OSX: \c osx
 *    - On Android: \c android
 *    - On GNU/Linux: \c linux
 *    - On Microsoft Windows: \c windows
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
void QSimpleUpdater::setPlatformKey(const QString &url, const QString &platform)
{
   getUpdater(url)->setPlatformKey(platform);
}

/**
 * Changes the module \version of the \c Updater instance registered at the
 * given \a url.
 *
 * \note The module version is used to compare it with the remove version.
 *       If the module name is empty, then the \c Updater instance will use the
 *       application version.
 */
void QSimpleUpdater::setModuleVersion(const QString &url, const QString &version)
{
   getUpdater(url)->setModuleVersion(version);
}

/**
 * If the \a enabled parameter is set to \c true, the \c Updater instance
 * registered with the given \a url will open the integrated downloader
 * if the user agrees to install the update (if any).
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
void QSimpleUpdater::setDownloaderEnabled(const QString &url, const bool enabled)
{
   getUpdater(url)->setDownloaderEnabled(enabled);
}

/**
 * Changes the user-agent string used by the updater to communicate
 * with the remote server
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
void QSimpleUpdater::setUserAgentString(const QString &url, const QString &agent)
{
   getUpdater(url)->setUserAgentString(agent);
}

/**
 * If the \a customAppcast parameter is set to \c true, then the \c Updater
 * will not try to read the network reply from the server, instead, it will
 * emit the \c appcastDownloaded() signal, which allows the application to
 * read and interpret the appcast file by itself.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
void QSimpleUpdater::setUseCustomAppcast(const QString &url, const bool customAppcast)
{
   getUpdater(url)->setUseCustomAppcast(customAppcast);
}

/**
 * If the \a custom parameter is set to \c true, the \c Updater instance
 * registered with the given \a url will not try to open the downloaded file.
 *
 * If you want to implement your own way to handle the downloaded file, just
 * bind to the \c downloadFinished() signal and disable the integrated
 * downloader with the \c setUseCustomInstallProcedures() function.
 *
 * \note If an \c Updater instance registered with the given \a url is not
 *       found, that \c Updater instance will be initialized automatically
 */
void QSimpleUpdater::setUseCustomInstallProcedures(const QString &url, const bool custom)
{
   getUpdater(url)->setUseCustomInstallProcedures(custom);
}

void QSimpleUpdater::setMandatoryUpdate(const QString &url, const bool mandatory_update)
{
   getUpdater(url)->setMandatoryUpdate(mandatory_update);
}

void QSimpleUpdater::setDownloadUserName(const QString &url, const QString &userName)
{
   getUpdater(url)->setDownloadUserName(userName);
}

void QSimpleUpdater::setDownloadPassword(const QString &url, const QString &password)
{
   getUpdater(url)->setDownloadPassword(password);
}

/**
 * Returns the \c Updater instance registered with the given \a url.
 *
 * If an \c Updater instance registered with teh given \a url does not exist,
 * this function will create it and configure it automatically.
 */
Updater *QSimpleUpdater::getUpdater(const QString &url) const
{
   if (!URLS.contains(url))
   {
      Updater *updater = new Updater;
      updater->setUrl(url);

      URLS.append(url);
      UPDATERS.append(updater);

      connect(updater, SIGNAL(checkingFinished(QString)), this, SIGNAL(checkingFinished(QString)));
      connect(updater, SIGNAL(downloadFinished(QString, QString)), this, SIGNAL(downloadFinished(QString, QString)));
      connect(updater, SIGNAL(appcastDownloaded(QString, QByteArray)), this,
              SIGNAL(appcastDownloaded(QString, QByteArray)));
   }

   return UPDATERS.at(URLS.indexOf(url));
}

#if QSU_INCLUDE_MOC
#   include "moc_QSimpleUpdater.cpp"
#endif
