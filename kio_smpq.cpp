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

#include <KComponentData>
#include <KDebug>

#include "kio_smpq.h"

int KDE_EXPORT kdemain(int argc, char * argv[]) {

	QCoreApplication app(argc, argv);
	KComponentData componentData("kio_smb");

	if ( argc != 4 ) {

		kDebug(KIO_SMPQ) << "Usage: kio_smpq protocol domain-socket1 domain-socket2" << endl;
		return -1;

	}

	SMPQSlave slave(argv[1], argv[2], argv[3]);
	slave.dispatchLoop();

	return 0;

}

SMPQSlave::SMPQSlave(const QByteArray &protocol, const QByteArray &pool_socket, const QByteArray &app_socket) : KIO::SlaveBase(protocol, pool_socket, app_socket) {

	//

}

SMPQSlave::~SMPQSlave() {

	//

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

void SMPQSlave::listDir(const KUrl &url) {}
void SMPQSlave::stat(const KUrl &url) {}
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

#include "kio_smpq.moc"
