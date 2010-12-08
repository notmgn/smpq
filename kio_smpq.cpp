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
#include <QByteArray>
#include <QVarLengthArray>
#include <QString>
#include <QSet>

#include <KComponentData>
#include <KDebug>
#include <KMimeType>
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
		SMPQSlavePrivate() : SArchive(NULL), flags(0), SFile(NULL) { }

		HANDLE SArchive;
		QString archive;
		unsigned int flags;

		HANDLE SFile;
		QByteArray file;
		KUrl url;

};

SMPQSlave::SMPQSlave(const QByteArray &protocol, const QByteArray &pool_socket, const QByteArray &app_socket) : KIO::SlaveBase(protocol, pool_socket, app_socket) {

	kDebug(KIO_SMPQ);

	p = new SMPQSlavePrivate;

}

SMPQSlave::~SMPQSlave() {

	kDebug(KIO_SMPQ);

	delete p;

}

bool SMPQSlave::parseUrl(const KUrl &url, QString &fileName, QByteArray &archivePath) {

	kDebug(KIO_SMPQ);

	QString path = url.path();

	bool appended = false;

	if ( path.at(path.size() - 1) != '/' ) {

		path.append('/');
		appended = true;

	}

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

	if ( appended )
		path.chop(1);

	fileName = path.left(pos);

	toArchivePath(archivePath, path.mid(pos + 1, -1));

	return true;

}

bool SMPQSlave::openArchive(const QString &archive, unsigned int flags) {

	kDebug(KIO_SMPQ);

	if ( p->archive != archive || p->flags != flags || ! p->SArchive ) {

		closeArchive();

		if ( ! SFileOpenArchive(archive.toUtf8(), 0, flags, &p->SArchive) )
			return false;

		p->archive = archive;
		p->flags = flags;

	}

	return true;

}

void SMPQSlave::closeArchive() {

	kDebug(KIO_SMPQ);

	if ( p->SArchive )
		SFileCloseArchive(p->SArchive);

	p->archive.clear();
	p->SArchive = NULL;
	p->flags = 0;

	p->file.clear();
	p->SFile = NULL;

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

///

void SMPQSlave::openConnection() {}
void SMPQSlave::closeConnection() {}

///

void SMPQSlave::get(const KUrl &url) {

	kDebug(KIO_SMPQ);

	QString fileName;
	QByteArray archivePath;

	if ( ! parseUrl(url, fileName, archivePath) ) {

		error(KIO::ERR_DOES_NOT_EXIST, url.path());
		return;

	}

	if ( ! openArchive(fileName, MPQ_OPEN_READ_ONLY) ) {

		error(0, ""); // TODO: Better error
		return;

	}

	SFILE_FIND_DATA SFileFindData;
	HANDLE SFileFind = SFileFindFirstFile(p->SArchive, archivePath, &SFileFindData, NULL /*ListFileName*/); // TODO: add listfile

	if ( ! SFileFind ) {

		error(0, ""); // TODO: Better error
		return;

	}

	HANDLE SFile = NULL;
	QByteArray SFileName = SFileFindData.cFileName;

	SFileFindClose(SFileFind);

	// Skip internal MPQ files
	if ( SFileName == "(listfile)" || SFileName == "(signature)" || SFileName == "(attributes)" ) {

		error(0, ""); // TODO: Better error
		return;

	}

	if ( ! SFileOpenFileEx(p->SArchive, SFileName, 0, &SFile) ) {

		error(0, ""); // TODO: Better error
		return;

	}

	int eof = 0;
	size_t bytes = 1024;
	QVarLengthArray <char> buffer(bytes);

	SFileReadFile(SFile, buffer.data(), buffer.size(), &bytes, NULL);

	QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytes);
	KMimeType::Ptr fileMimeType = KMimeType::findByNameAndContent(url.fileName(), fileData);
	mimeType(fileMimeType->name());

	int low = 0;
	int high = 0;
	SFileSetFilePointer(SFile, low, &high, FILE_BEGIN);

	buffer.resize(0x10000);

	while ( 1 ) {

		if ( ! SFileReadFile(SFile, buffer.data(), buffer.size(), &bytes, NULL) ) {

			eof = GetLastError() == ERROR_HANDLE_EOF;

			if ( ! eof ) {

				SFileCloseFile(SFile);
				error(0, ""); // TODO: Better error
				return;

			}

		}

		data(QByteArray::fromRawData(buffer.data(), bytes));

		if ( eof )
			break;
	
	}

	SFileCloseFile(SFile);

	data(QByteArray());

	finished();

}

void SMPQSlave::put(const KUrl &url, int permissions, KIO::JobFlags flags) {}

void SMPQSlave::del(const KUrl &url, bool isfile) {

	kDebug(KIO_SMPQ);

	QString fileName;
	QByteArray archivePath;

	if ( ! parseUrl(url, fileName, archivePath) ) {

		error(KIO::ERR_DOES_NOT_EXIST, url.path());
		return;

	}

	if ( ! openArchive(fileName) ) {

		error(0, ""); // TODO: Better error
		return;

	}

	if ( ! SFileRemoveFile(p->SArchive, archivePath, SFILE_OPEN_FROM_MPQ) ) {

		error(0, "");  // TODO: Better error
		return;

	}

	SFileCompactArchive(p->SArchive, NULL, 0);
	SFileFlushArchive(p->SArchive);

	finished();

}

void SMPQSlave::copy(const KUrl &src, const KUrl &dest, int permissions, KIO::JobFlags flags) {}

void SMPQSlave::rename(const KUrl &src, const KUrl &dest, KIO::JobFlags flags) {

	kDebug(KIO_SMPQ);

	QString srcFileName;
	QByteArray srcArchivePath;

	QString destFileName;
	QByteArray destArchivePath;

	if ( ! parseUrl(src, srcFileName, srcArchivePath) ) {

		error(KIO::ERR_DOES_NOT_EXIST, src.path());
		return;

	}

	if ( ! parseUrl(dest, destFileName, destArchivePath) ) {

		error(KIO::ERR_DOES_NOT_EXIST, dest.path());
		return;

	}

	if ( srcFileName != destFileName ) {

		error(KIO::ERR_UNSUPPORTED_ACTION, "");
		return;

	}

	if ( srcArchivePath.isEmpty() || srcArchivePath.at(srcArchivePath.size() - 1) == '\\' ) {

		error(0, ""); // TODO: Implement rename directory
		return;

	}

	if ( ! openArchive(srcFileName) ) {

		error(0, ""); // TODO: Better error
		return;

	}

	SFILE_FIND_DATA SFileFindData;
	HANDLE SFileFind = SFileFindFirstFile(p->SArchive, destArchivePath, &SFileFindData, NULL /*ListFileName*/); // TODO: add listfile

	if ( SFileFind && ! ( flags & KIO::Overwrite ) ) {

		SFileFindClose(SFileFind);
		error(KIO::ERR_FILE_ALREADY_EXIST, destArchivePath);
		return;

	}

	if ( SFileFind && ( flags & KIO::Overwrite ) ) {

		if ( ! SFileRemoveFile(p->SArchive, destArchivePath, SFILE_OPEN_FROM_MPQ) ) {

			error(0, "");  // TODO: Better error
			return;

		}

		SFileCompactArchive(p->SArchive, NULL, 0);

	}

	if ( ! SFileRenameFile(p->SArchive, srcArchivePath, destArchivePath) ) {

		error(0, ""); // TODO: Better error
		return;

	}

	SFileFlushArchive(p->SArchive);
	finished();

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

	QSet <QByteArray> directories;

	SFILE_FIND_DATA SFileFindData;
	HANDLE SFileFind = SFileFindFirstFile(p->SArchive, archivePath + '*', &SFileFindData, NULL /*ListFileName*/); // TODO: add listfile

	while ( SFileFind ) {

		QByteArray filePath = SFileFindData.cFileName;
		QByteArray fileName;

		if ( archivePath.isEmpty() )
			fileName = filePath;
		else
			fileName = filePath.mid(archivePath.size(), -1);

		if ( fileName.contains('\\') ) {

			QByteArray dirName = fileName.split('\\').first();

			if ( ! directories.contains(dirName) ) {

				KIO::UDSEntry entry;
				entry.insert(KIO::UDSEntry::UDS_NAME, QFile::decodeName(dirName));
				entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
				listEntry(entry, false);

				directories.insert(dirName);

			}

		} else {

			KIO::UDSEntry entry;
			entry.insert(KIO::UDSEntry::UDS_NAME, QFile::decodeName(fileName));
			entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
			entry.insert(KIO::UDSEntry::UDS_SIZE, SFileFindData.dwFileSize);
//			entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, 0); // TODO: Add time
			listEntry(entry, false);

		}

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
	HANDLE SFileFind = SFileFindFirstFile(p->SArchive, archivePath, &SFileFindData, NULL /*ListFileName*/); // TODO: add listfile

	if ( ! SFileFind )
		SFileFind = SFileFindFirstFile(p->SArchive, archivePath + "\\*", &SFileFindData, NULL /*ListFileName*/); // TODO: add listfile

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

void SMPQSlave::mkdir(const KUrl &url, int permissions) { finished(); }

///

void SMPQSlave::setModificationTime(const KUrl &url, const QDateTime &mtime) {}
void SMPQSlave::slave_status() {}

///
// KIO::FileJob interface

void SMPQSlave::open(const KUrl &url, QIODevice::OpenMode mode) {

	kDebug(KIO_SMPQ);

	QString fileName;
	QByteArray archivePath;

	if ( ! parseUrl(url, fileName, archivePath) ) {

		error(KIO::ERR_DOES_NOT_EXIST, url.path());
		return;

	}

	unsigned int flags = 0;

	if ( mode & QIODevice::ReadOnly )
		flags = MPQ_OPEN_READ_ONLY;

	if ( ! openArchive(fileName, flags) ) {

		error(0, ""); // TODO: Better error
		return;

	}

	if ( ! SFileOpenFileEx(p->SArchive, archivePath, 0, &p->SFile) ) {

		error(0, ""); // TODO: Better error
		return;

	}

	p->file = archivePath;
	p->url = url;

	if ( mode & QIODevice::ReadOnly ) {

		size_t bytes = 1024;
		QVarLengthArray <char> buffer(bytes);

		if ( ! SFileReadFile(p->SFile, buffer.data(), buffer.size(), &bytes, NULL) ) {

			if ( GetLastError() != ERROR_HANDLE_EOF ) {

				error(KIO::ERR_COULD_NOT_READ, p->url.prettyUrl());
				close();
				return;

			}

		}

		QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytes);
		KMimeType::Ptr fileMimeType = KMimeType::findByNameAndContent(url.fileName(), fileData);
		mimeType(fileMimeType->name());

		int low = 0;
		int high = 0;
		SFileSetFilePointer(p->SFile, low, &high, FILE_BEGIN);

	}

}

void SMPQSlave::close() {

	SFileCloseFile(p->SFile);

	if ( p->SFile )
		p->SFile = NULL;

	p->file.clear();
	p->url.clear();

}

void SMPQSlave::read(KIO::filesize_t size) {

	size_t bytes = size;
	QVarLengthArray <char> buffer(bytes);

	if ( ! SFileReadFile(p->SFile, buffer.data(), buffer.size(), &bytes, NULL) ) {

		if ( GetLastError() != ERROR_HANDLE_EOF ) {

			error(KIO::ERR_COULD_NOT_READ, p->url.prettyUrl());
			return;

		}

		data(QByteArray::fromRawData(buffer.data(), bytes));
		data(QByteArray());

	} else {

		data(QByteArray::fromRawData(buffer.data(), bytes));

	}

}

void SMPQSlave::write(const QByteArray &data) {}

void SMPQSlave::seek(KIO::filesize_t offset) {

	// TODO: signed or unsigned? SFileSetFilePointer needs LONG
	int low = offset;
	int high = offset >> 32;

	if ( SFileSetFilePointer(p->SFile, low, &high, FILE_BEGIN) == SFILE_INVALID_SIZE ) {

			error(KIO::ERR_COULD_NOT_SEEK, p->url.prettyUrl());
			return;

	}

	position(((KIO::filesize_t)high << 31) | low);

}

void SMPQSlave::special(const QByteArray &data) {}
