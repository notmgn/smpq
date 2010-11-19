/*
    kio_smpq.cpp - KDE4 KIO plugin for StormLib MPQ archiving utility
    Copyright (C) 2010  Pali Rohár <pali.rohar@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <QCoreApplication>
#include <QFile>
#include <QString>
#include <QSet>

#include <KComponentData>
#include <KDebug>
#include <kde_file.h>

#include <StormLib.h>

#include "kio_smpq.h"

extern "C" {

int KDE_EXPORT kdemain(int argc, char * argv[]) {

	kDebug(KIO_SMPQ);

	QCoreApplication app(argc, argv);
	KComponentData componentData("kio_smpq");

	if ( argc != 4 ) {

		kDebug(KIO_SMPQ) << "Usage: kio_smpq protocol domain-socket1 domain-socket2";
		return -1;

	}

	SMPQSlave slave(argv[1], argv[2], argv[3]);
	slave.dispatchLoop();

	return 0;

}

}

class SMPQSlavePrivate
{

	public:
		QString fileName;
		HANDLE SArchive;

};

SMPQSlave::SMPQSlave(const QByteArray &protocol, const QByteArray &pool_socket, const QByteArray &app_socket) : KIO::SlaveBase(protocol, pool_socket, app_socket) {

	kDebug(KIO_SMPQ);

	archive = new SMPQSlavePrivate;
	archive->SArchive = NULL;

}

SMPQSlave::~SMPQSlave() {

	kDebug(KIO_SMPQ);

	delete archive;

}

///

void SMPQSlave::openConnection() {}
void SMPQSlave::closeConnection() {}

///

void SMPQSlave::get(const KUrl &url) {}
void SMPQSlave::put(const KUrl &url, int permissions, KIO::JobFlags flags) {}
void SMPQSlave::del(const KUrl &url, bool isfile) {}
void SMPQSlave::copy(const KUrl &src, const KUrl &dest, int permissions, KIO::JobFlags flags) {}
void SMPQSlave::rename(const KUrl &src, const KUrl &dest, KIO::JobFlags flags) {}

///

bool SMPQSlave::parseUrl(const KUrl &url, QString &fileName, QByteArray &archivePath) {

	kDebug(KIO_SMPQ);

	QString path = url.path();

	if ( path.at(path.size() - 1) != '/' )
		path.append('/');

	int pos = 0;
	int nextPos = 0;

	KDE_struct_stat statbuf;

	while ( ( nextPos = path.indexOf('/', pos + 1) ) != -1 ) {

		if ( KDE_stat(QFile::encodeName(path.left(nextPos)), &statbuf) == -1 )
			break;

		pos = nextPos;

	}

	if ( pos == 0 )
		return false;

	fileName = path.left(pos);

	QString temp = path.mid(pos + 1, -1);
	toArchivePath(archivePath, temp);

	return true;

}

bool SMPQSlave::openArchive(const QString &fileName) {

	kDebug(KIO_SMPQ);

	if ( archive->fileName != fileName ) {

		closeArchive();

		if ( ! SFileOpenArchive(fileName.toUtf8(), 0, 0, &archive->SArchive) )
			return false;

		archive->fileName = fileName;

	}

	return true;

}

void SMPQSlave::closeArchive() {

	kDebug(KIO_SMPQ);

	if ( archive->SArchive )
		SFileCloseArchive(archive->SArchive);

	archive->SArchive = NULL;
	archive->fileName.clear();

}

void SMPQSlave::toArchivePath(QByteArray &to, const QString &from) {

#ifdef QT_WS_WIN
	to = from.toUtf8();
#else
	to = from.toUtf8().replace('/', '\\');
#endif

}

void SMPQSlave::fromArchivePath(QString &to, const QByteArray &from) {

#ifdef QT_WS_WIN
	to = QString::fromUtf8(from);
#else
	to = QString::fromUtf8(from).replace('\\', '/');
#endif

}

void SMPQSlave::listDir(const KUrl &url) {

	kDebug(KIO_SMPQ);

	QString fileName;
	QByteArray archivePath;

	if ( ! parseUrl(url, fileName, archivePath) ) {

		error(KIO::ERR_CANNOT_ENTER_DIRECTORY, url.path());
		return;

	}

	if ( ! archivePath.isEmpty() && archivePath.at(archivePath.size() - 1) != '\\' )
		archivePath.append('\\');

	if ( ! openArchive(fileName) ) {

		error(0, ""); // TODO: Better error
		return;

	}

	if ( archivePath.isEmpty() ) {

		KIO::UDSEntry entry;
		entry.insert(KIO::UDSEntry::UDS_NAME, ".");
		entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
		listEntry(entry, false);

	}

	QSet <QString> directories;

	SFILE_FIND_DATA SFileFindData;
	HANDLE SFileFind = SFileFindFirstFile(archive->SArchive, archivePath + '*', &SFileFindData, NULL /*ListFileName*/); // TODO: add listfile

	while ( SFileFind ) {

		QString filePath = SFileFindData.cFileName;
		QString fileName;

		if ( archivePath.isEmpty() )
			fileName = filePath;
		else
			fileName = filePath.mid(archivePath.size(), -1);

		KIO::UDSEntry entry;

		if ( fileName.contains('\\') ) {

			QString dirName = fileName.section('\\', 0, 0);

			if ( directories.contains(dirName) )
				goto next;

			entry.insert(KIO::UDSEntry::UDS_NAME, QFile::decodeName(dirName.toUtf8()));
			entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);

			directories.insert(dirName);

		} else {

			entry.insert(KIO::UDSEntry::UDS_NAME, QFile::decodeName(fileName.toUtf8()));
			entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
			entry.insert(KIO::UDSEntry::UDS_SIZE, SFileFindData.dwFileSize);
//			entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, 0); // TODO: Add time

		}

		listEntry(entry, false);

next:

		if ( ! SFileFindNextFile(SFileFind, &SFileFindData) )
			break;

	}

	listEntry(KIO::UDSEntry(), true);
	finished();

}


void SMPQSlave::stat(const KUrl &url) {

	kDebug(KIO_SMPQ);

	QString fileName;
	QByteArray archivePath;

	if ( ! parseUrl(url, fileName, archivePath) ) {

		error(KIO::ERR_DOES_NOT_EXIST, url.path());
		return;

	}

	if ( archivePath.isEmpty() )
		archivePath = "*";
		
	if ( archivePath.at(archivePath.size() - 1) == '\\' )
		archivePath.append('*');

	if ( ! openArchive(fileName) ) {

		error(0, ""); // TODO: Better error
		return;

	}

	SFILE_FIND_DATA SFileFindData;
	HANDLE SFileFind = SFileFindFirstFile(archive->SArchive, archivePath, &SFileFindData, NULL /*ListFileName*/); // TODO: add listfile

	if ( SFileFind ) {

		KIO::UDSEntry entry;
		entry.insert(KIO::UDSEntry::UDS_NAME, url.path());

		if ( archivePath == SFileFindData.cFileName ) {

			entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
			entry.insert(KIO::UDSEntry::UDS_SIZE, SFileFindData.dwFileSize);
//			entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, 0); // TODO: Add time

		} else {

			entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);

		}

		statEntry(entry);
		finished();

	} else {

		error(KIO::ERR_DOES_NOT_EXIST, url.path());

	}

	SFileFindClose(SFileFind);

}

void SMPQSlave::mkdir(const KUrl &url, int permissions) {}
void SMPQSlave::setModificationTime(const KUrl &url, const QDateTime &mtime) {}
void SMPQSlave::slave_status() {}

///
// KIO::FileJob interface

void SMPQSlave::open(const KUrl &url, QIODevice::OpenMode mode) {}
void SMPQSlave::close() {}
void SMPQSlave::read(KIO::filesize_t size) {}
void SMPQSlave::write(const QByteArray &data) {}
void SMPQSlave::seek(KIO::filesize_t offset) {}
void SMPQSlave::special(const QByteArray &data) {}
