/****************************************************************************
 *
 *   Copyright (c) 2012-2014 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file uploader.cpp Firmware uploader for PX4IO
 */

#include <sys/types.h>
#include <stdint.h>
//#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef Q_OS_WIN
#include <io.h>
#endif

#include "uploader.h"
#include <QGC.h>

#include <QtGlobal>
#include <QFile>
#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
#include <QJsonDocument>
#include <QJsonObject>
#endif
#include <QTemporaryFile>
#include <QDebug>
#include <QDir>

#undef OK
#define OK 0
#define nullptr NULL

static const quint32 crctab[] =
{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static quint32
crc32(const uint8_t *src, unsigned len, unsigned state)
{
	for (unsigned i = 0; i < len; i++)
		state = crctab[(state ^ src[i]) & 0xff] ^ (state >> 8);
	return state;
}

PX4_Uploader::PX4_Uploader(QSerialPort* port, QObject *parent) : QObject(parent),
    _io_fd(port)
{
    boardNames.append("Zero");
    boardNames.append("One");
    boardNames.append("Two");
    boardNames.append("Three");
    boardNames.append("Four");
    boardNames.append("PX4FMU v1.x");
    boardNames.append("PX4FLOW v1.x");
    boardNames.append("PX4IO v1.x");
    boardNames.append("Eight");
    boardNames.append("PX4 PIXHAWK v1.x");
    boardNames.append("Ten");

    // Create serial port instance

}

PX4_Uploader::~PX4_Uploader()
{
}

void PX4_Uploader::send_app_reboot()
{
    log("Attempting reboot..");

    // Send command to reboot via NSH, then via MAVLink
    // XXX hacky but safe
    // Start NSH
    const char init[] = {0x0d, 0x0d, 0x0d};
    _io_fd->write(init, sizeof(init));

    // Reboot into bootloader
    const char* cmd = "reboot -b\n";
    _io_fd->write(cmd, strlen(cmd));
    _io_fd->write(init, 2);

    // Old reboot command
    const char* cmd_old = "reboot\n";
    _io_fd->write(cmd_old, strlen(cmd_old));
    _io_fd->write(init, 2);

#ifdef Q_OS_WIN
    #pragma warning (push)
    #pragma warning (disable:4309)
#else
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wnarrowing"
#endif

    // Reboot via MAVLink (if enabled)
    // Try system ID 1
    const char mavlink_msg_id1[] = {0xfe,0x21,0x72,0xff,0x00,0x4c,0x00,0x00,0x80,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf6,0x00,0x01,0x00,0x00,0x48,0xf0};
    _io_fd->write(mavlink_msg_id1, sizeof(mavlink_msg_id1));
    // Try system ID 0 (broadcast)
    const char mavlink_msg_id0[] = {0xfe,0x21,0x45,0xff,0x00,0x4c,0x00,0x00,0x80,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf6,0x00,0x00,0x00,0x00,0xd7,0xac};
    _io_fd->write(mavlink_msg_id0, sizeof(mavlink_msg_id0));
    
    if (!_io_fd->waitForBytesWritten(1000)) {
        log("reboot waitForBytesWritten failed");
    }

#ifdef Q_OS_WIN
    #pragma warning (pop)
#else
    #pragma GCC diagnostic pop
#endif
}

int PX4_Uploader::get_bl_info(quint32 &board_id, quint32 &board_rev, quint32 &flash_size, QString &humanReadable, bool &insync)
{
    int ret = -1;

    humanReadable = QString("INFO FAILED");

    /* do the usual program thing - allow for failure */
    for (unsigned retries = 0; retries < 1; retries++) {

        if (retries > 0) {
            log("retrying info...");
            ret = sync();

            if (ret != OK) {
                /* this is immediately fatal */
                log("bootloader not responding (attempting to reset..)");
                send_app_reboot();
				_io_fd->close();
                return -EIO;
            }
        }

        ret = get_info(INFO_BL_REV, bl_rev);

        if (ret == OK) {
            log("found bootloader revision: %d", bl_rev);
        } else {
            log("failed getting bootloader rev.");
        }

        ret = get_info(INFO_BOARD_ID, board_id);

        if (ret == OK) {
            log("found board ID: %d - %s", (int)board_id, (boardNames.size() > (int)board_id) ? boardNames.at(board_id).toStdString().c_str() : "UNKNOWN");
        } else {
            log("failed getting board ID");
        }

        ret = get_info(INFO_BOARD_REV, board_rev);

        if (ret == OK) {
            log("board rev: %d", board_rev);
        } else {
            log("failed getting board rev.");
        }

        ret = get_info(INFO_FLASH_SIZE, flash_size);

        if (ret == OK) {
            log("flash size: %d bytes (%4.1f MiB)", flash_size, flash_size / (1024.0f * 1024.0f));
        } else {
            log("failed getting flash size");
        }

        insync = true;

        humanReadable = QString("%1 rev %2 (%4.1f MiB flash)").arg(boardNames.at(board_id)).arg(board_rev).arg(flash_size / (1024.0f * 1024.0f));
    }

    return ret;
}

int
PX4_Uploader::detect(int &r_board_id)
{
    unsigned int checkBoardMinRev = 0;
    unsigned int checkBoardMaxRev = 99;

    r_board_id = -1;

    int	ret;

    bool insync = false;

    if (!_io_fd->isOpen()) {
        if (!_io_fd->open(QIODevice::ReadWrite)) {
            log("could not open port, error: %d", _io_fd->error());
            return -1;
        }
    }
    log("opened port %s", _io_fd->portName().toStdString().c_str());

    /* look for the bootloader */

    log("scanning for bootloader..");
    ret = sync();

    if (ret != OK) {
        /* this is immediately fatal */
        log("bootloader not responding (attempting to reset..)");
        send_app_reboot();
        _io_fd->close();
        return ret;
    } else {
        log("synced to bootloader");
    }

    /* do the usual program thing - allow for failure */
    for (unsigned retries = 0; retries < 1; retries++) {

        if (!insync) {

            if (retries > 0) {
                log("retrying update...");
                ret = sync();

                if (ret != OK) {
                    /* this is immediately fatal */
                    log("bootloader not responding, please unplug and re-plug board");
                    _io_fd->close();
                    return -EIO;
                } else {
                    insync = true;
                }
            }

            ret = get_info(INFO_BL_REV, bl_rev);

            if (ret == OK) {
                if (bl_rev <= BL_REV) {
                    log("found bootloader revision: %d", bl_rev);
                    if (bl_rev < 3) {
                        log("Bootloader v2 is slow, consider upgrading:\nhttps://pixhawk.ethz.ch/px4/users/bootloader_update");
                    }
                } else {
                    log("found unsupported bootloader revision %d, exiting", bl_rev);
                    _io_fd->close();
                    return -1;
                }
            }

            ret = get_info(INFO_BOARD_ID, board_id);

            if (ret == OK) {
                r_board_id = board_id;
                log("found board ID: %d - %s", static_cast<int>(board_id), (boardNames.size() > static_cast<int>(board_id)) ? boardNames.at(board_id).toStdString().c_str() : "UNKNOWN");
            }

            ret = get_info(INFO_BOARD_REV, board_rev);

            if (ret == OK) {
                // XXX check that flash size deducts the bootloader space
                if (board_rev >= checkBoardMinRev && board_rev <= checkBoardMaxRev) {
                    log("board rev: %d", board_rev);
                } else {
                    log("unsupported board rev (%d), exiting", board_rev);
                    _io_fd->close();
                    return OK;
                }
            }

            ret = get_info(INFO_FLASH_SIZE, flash_size);

            if (ret == OK) {
//                // XXX check that flash size deducts the bootloader space
//                if (flash_size > fw_size) {
//                    log("flash size: %d bytes (%4.1f MiB)", flash_size, flash_size / (1024.0f * 1024.0f));
//                } else {
//                    log("not enough flash size (%d bytes avail, %d needed), exiting", flash_size, fw_size);
//                    _io_fd->close();
//                    return OK;
//                }
            }

        }

        _io_fd->close();

        ret = OK;
        break;
    }

    // Close port
    _io_fd->close();

    return ret;
}

QString PX4_Uploader::getBoardName()
{
    if (board_id < static_cast<quint32>(boardNames.length())) {
        return boardNames[board_id];
    } else {
        return "UNKNOWN";
    }
}

QString PX4_Uploader::getBootloaderName()
{
    return QString("Bootloader rev. %1 (%2 MiB Flash)").arg(bl_rev).arg((float)flash_size / (1024.0f * 1024.0f));
}

int
PX4_Uploader::upload(const QString& filename, int filterId, bool insync)
{
    // magic number, 5 = PX4FMU 1.x
    // if not a .bin, but a .px4 file is loaded
    // this ID is anyway overwritten by the ID stored in the file.
    unsigned int checkBoardId = 5;
    if (filterId > 0)
        checkBoardId = filterId;
    unsigned int checkBoardMinRev = 0;
    unsigned int checkBoardMaxRev = 99;
    bool tempfile = false;

	int	ret;
	size_t fw_size;

    if (!_io_fd->isOpen()) {
        _io_fd->open(QIODevice::ReadWrite | QIODevice::Unbuffered);

        // If still not open, complain and exit
        if (!_io_fd->isOpen()) {
            log("could not open interface");
            return -1;
        }
    } else {
        log("opened port %s", _io_fd->portName().toStdString().c_str());
    }

    /* look for the bootloader */

    log("scanning for bootloader..");
    ret = sync();

    if (ret != OK) {
        /* this is immediately fatal */
        log("bootloader not responding (attempting to reset..)");
        send_app_reboot();
		_io_fd->close();
        return ret;
    } else {
        log("synced to bootloader");
    }

    if (filename.endsWith(".px4")) {
        log("decoding JSON (%s)", filename.toStdString().c_str());

        // Don't trust 5.0.0's JSON support, as it had issues
        // Attempt to decode JSON
        QFile json(filename);
        json.open(QIODevice::ReadOnly | QIODevice::Text);
        QByteArray jbytes = json.readAll();
        QString j8(jbytes);

        int imageSize;

#if QT_VERSION <= QT_VERSION_CHECK(5, 0, 0)

        // BOARD ID
        QStringList decode_list = j8.split("\"board_id\":");
        decode_list = decode_list.last().split(",");
        if (decode_list.count() < 1)
            return -1;
        QString board_id = QString(decode_list.first().toUtf8()).trimmed();
        checkBoardId = board_id.toInt();

        // IMAGE SIZE
        decode_list = j8.split("\"image_size\":");
        decode_list = decode_list.last().split(",");
        if (decode_list.count() < 1)
            return -1;
        QString image_size = QString(decode_list.first().toUtf8()).trimmed();
        imageSize = image_size.toInt();

        // DESCRIPTION
        decode_list = j8.split("\"description\": \"");
        decode_list = decode_list.last().split("\"");
        if (decode_list.count() < 1)
            return -1;
        QString description = QString(decode_list.first().toUtf8()).trimmed();
        log("description: %s", description.toStdString().c_str());
#else
        QJsonDocument doc = QJsonDocument::fromJson(jbytes);

        if (doc.isNull()) {
            // Error, bail out
            log("supplied file is not a valid JSON document");
			_io_fd->close();
            return -1;
        }


        QJsonObject px4 = doc.object();

        imageSize = (int) (px4.value(QString("image_size")).toDouble());

        checkBoardId = (int) (px4.value(QString("board_id")).toDouble());
        QString str = px4.value(QString("description")).toString();
        log("description: %s", str.toStdString().c_str());
        QString identity = px4.value(QString("git_identity")).toString();
        log("GIT identity: %s", identity.toStdString().c_str());
#endif

        log("loaded file for board ID %d", checkBoardId);
        log("uncompressed image size: %d", imageSize);
        if (imageSize == 0) {
            return -200;
        }

        _fw_fd.setFileName(QDir::tempPath() + "/px4upload_" + QGC::groundTimeMilliseconds() + ".bin");
        _fw_fd.open(QIODevice::ReadWrite);

        // XXX Qt's JSON string handling is terribly broken, strings
        // with some length (18K / 25K) are just weirdly cut.
        // The code below works around this by manually 'parsing'
        // for the image string. Since its compressed / checksummed
        // this should be fine.

        // Convert String to QByteArray and unzip it
        QByteArray raw;
        raw.clear();

        qDebug() << "target size:" << imageSize;

        raw.append((unsigned char)((imageSize >> 24) & 0xFF));
        raw.append((unsigned char)((imageSize >> 16) & 0xFF));
        raw.append((unsigned char)((imageSize >> 8) & 0xFF));
        raw.append((unsigned char)((imageSize >> 0) & 0xFF));

        QStringList list = j8.split("\"image\": \"");
        list = list.last().split("\"");

        QByteArray raw64 = list.first().toUtf8();

        raw.append(QByteArray::fromBase64(raw64));
        QByteArray uncompressed = qUncompress(raw);

        QByteArray b = uncompressed;

        log("firmware size: %d bytes, expected %d bytes", b.count(), imageSize);

        if (b.count() == 0 || b.count() != imageSize) {
            log("firmware file invalid, aborting");
			_io_fd->close();
            return -1;
        }

        // pad image to 4-byte length
        while ((b.count() % 4) != 0)
            b.append(static_cast<char>(static_cast<unsigned char>(0xFF)));

        _fw_fd.write(b);
        _fw_fd.seek(0);
        tempfile = true;

    } else if (filename.endsWith(".bin")) {
        // Already a binary file

        // Take some educated guesses based on file name
        if (filename.toLower().contains("px4io")) {
            checkBoardId = 6;
        } else if (filename.toLower().contains("px4flow")) {
            checkBoardId = 7;
        } else if (filename.toLower().contains("px4fmu")) {
            checkBoardId = 5;
        }

        _fw_fd.close();
        _fw_fd.setFileName(filename);
        _fw_fd.open(QIODevice::ReadOnly);

    } else {
        log("unrecognized file ending, exiting.");
        return -1;
    }

    if (!_fw_fd.isOpen()) {
        log("failed to open %s", filename.toStdString().c_str());
        return -ENOENT;
    }

    fw_size = _fw_fd.size();

    log("using firmware from %s", filename.toStdString().c_str());

	/* do the usual program thing - allow for failure */
	for (unsigned retries = 0; retries < 1; retries++) {

        if (!insync) {

            if (retries > 0) {
                log("retrying update...");
                ret = sync();

                if (ret != OK) {
                    /* this is immediately fatal */
                    log("bootloader not responding, please unplug and re-plug board");
					_io_fd->close();
                    return -EIO;
                }
            }

            ret = get_info(INFO_BL_REV, bl_rev);

            if (ret == OK) {
                if (bl_rev <= BL_REV) {
                    log("found bootloader revision: %d", bl_rev);
                    if (bl_rev < 3) {
                        log("Bootloader v2 is slow, consider upgrading:\nhttps://pixhawk.ethz.ch/px4/users/bootloader_update");
                    }
                } else {
                    log("found unsupported bootloader revision %d, exiting", bl_rev);
					_io_fd->close();
                    return -1;
                }
            }

            ret = get_info(INFO_BOARD_ID, board_id);

            if (ret == OK) {
                if (board_id == checkBoardId) {
                    log("found board ID: %d - %s", static_cast<int>(board_id), (boardNames.size() > static_cast<int>(board_id)) ? boardNames.at(board_id).toStdString().c_str() : "UNKNOWN");
                } else {
                    log("found unsupported board ID %d, exiting", board_id);
					_io_fd->close();
                    return -2;
                }
            }

            ret = get_info(INFO_BOARD_REV, board_rev);

            if (ret == OK) {
                // XXX check that flash size deducts the bootloader space
                if (board_rev >= checkBoardMinRev && board_rev <= checkBoardMaxRev) {
                    log("board rev: %d", board_rev);
                } else {
                    log("unsupported board rev (%d), exiting", board_rev);
					_io_fd->close();
                    return OK;
                }
            }

            ret = get_info(INFO_FLASH_SIZE, flash_size);

            if (ret == OK) {
                // XXX check that flash size deducts the bootloader space
                if (flash_size > fw_size) {
                    log("flash size: %d bytes (%4.1f MiB)", flash_size, flash_size / (1024.0f * 1024.0f));
                } else {
                    log("not enough flash size (%d bytes avail, %d needed), exiting", flash_size, fw_size);
					_io_fd->close();
                    return OK;
                }
            }

        }

        /* let's measure how long it takes */\
        quint64 startUploadTime = QGC::groundTimeMilliseconds();

        // All checks passed, erase
		ret = erase();

		if (ret != OK) {
			log("erase failed");
			continue;
        } else {
            emit upgradeProgressChanged(10);
        }

		ret = program(fw_size);

		if (ret != OK) {
			log("program failed");
			continue;
		}

        if (bl_rev <= 2) {
			ret = verify_rev2(fw_size);
        } else if(bl_rev == 3) {
			ret = verify_rev3(fw_size);
        } else {
           // rev 4 and higher still use verify rev3
           ret = verify_rev3(fw_size);
        }

		if (ret != OK) {
            log("verify failed, please reset the board to retry..");
			continue;
		}

		ret = reboot();

		if (ret != OK) {
			log("reboot failed");
			_io_fd->close();
			return ret;
		}
		
        log("update complete (time: %.2f s)", 1e-3f*(float)(QGC::groundTimeMilliseconds() - startUploadTime));

		_io_fd->close();

		ret = OK;
		break;
	}

    _fw_fd.close();
    // Close port
    _io_fd->close();

    if (tempfile) {
        qDebug() << "Deleting" << _fw_fd.fileName();
        if (!_fw_fd.remove()) {
            log("failed removing temporary file");
        }
    }

	return ret;
}

int
PX4_Uploader::recv(uint8_t &c, unsigned timeout)
{
    quint64 startTime = QGC::groundTimeMilliseconds();

    while (_io_fd->bytesAvailable() < 1 && QGC::groundTimeMilliseconds() < (startTime + timeout)) {
        QGC::SLEEP::usleep(500);
    }

    if (_io_fd->bytesAvailable() < 1) {
        //log("poll timeout");
        return -ETIMEDOUT;
    } else {
        QByteArray arr = _io_fd->read(1);
        c = arr.at(0);
    }

    //log("recv 0x%02x", c);

	return OK;
}

int
PX4_Uploader::recv(uint8_t *p, unsigned count)
{
	while (count--) {
        int ret = recv(*p++, 5000);

		if (ret != OK)
			return ret;
	}

	return OK;
}

void
PX4_Uploader::drain()
{
	uint8_t c;
	int ret;

	do {
        ret = recv(c, 1);

		if (ret == OK) {
			//log("discard 0x%02x", c);
		}
	} while (ret == OK);
}

int
PX4_Uploader::send(uint8_t c)
{
    //log("send 0x%02x", c);
    if (_io_fd->write((char*)&c, 1) != 1)
    {
		return -errno;
    } else {
        _io_fd->flush();
    }

    return OK;
}

int
PX4_Uploader::send(uint8_t *p, unsigned count)
{
    if (_io_fd->write((const char*)p, count) != count)
    {
        return -errno;
    } else {
//        log("sent %d bytes", count);
        _io_fd->flush();
    }

	return OK;
}

int
PX4_Uploader::get_sync(unsigned timeout)
{
	uint8_t c[2];
	int ret;

	ret = recv(c[0], timeout);

	if (ret != OK)
		return ret;

	ret = recv(c[1], timeout);

	if (ret != OK)
		return ret;

	if ((c[0] != PROTO_INSYNC) || (c[1] != PROTO_OK)) {
		log("bad sync 0x%02x,0x%02x", c[0], c[1]);
		return -EIO;
	}

	return OK;
}

int
PX4_Uploader::sync()
{
    qDebug() << "DRAIN START";
	drain();
    qDebug() << "DRAIN END";

	/* complete any pending program operation */
	for (unsigned i = 0; i < (PROG_MULTI_MAX + 6); i++)
		send(0);

	send(PROTO_GET_SYNC);
	send(PROTO_EOC);
    qDebug() << "GET SYNC START";
	return get_sync();
}

int
PX4_Uploader::get_info(char param, quint32 &val)
{
	int ret;

	send(PROTO_GET_DEVICE);
	send(param);
	send(PROTO_EOC);

	ret = recv((uint8_t *)&val, sizeof(val));

	if (ret != OK)
		return ret;

	return get_sync();
}

int
PX4_Uploader::erase()
{
	log("erase...");
	send(PROTO_CHIP_ERASE);
	send(PROTO_EOC);
	return get_sync(30000);		/* allow 30s timeout */
}


static int read_with_retry(QFile &fd, void *buf, size_t n)
{
    return fd.read((char*)buf, n);
}

int
PX4_Uploader::program(size_t fw_size)
{
	uint8_t	file_buf[PROG_MULTI_MAX];
    int count;
	int ret;
	size_t sent = 0;

	log("programming %u bytes...", (unsigned)fw_size);

    ret = _fw_fd.seek(0);

	while (sent < fw_size) {
		/* get more bytes to program */
		size_t n = fw_size - sent;
		if (n > sizeof(file_buf)) {
			n = sizeof(file_buf);
		}
        count = read_with_retry(_fw_fd, file_buf, n);

        if (count != (int)n) {
			log("firmware read of %u bytes at %u failed -> %d errno %d", 
			    (unsigned)n,
			    (unsigned)sent,
			    (int)count,
			    (int)errno);
		}

		if (count == 0)
			return OK;

		sent += count;

		if (count < 0)
			return -errno;

        Q_ASSERT((count % 4) == 0);

		send(PROTO_PROG_MULTI);
		send(count);
		send(&file_buf[0], count);
		send(PROTO_EOC);

		ret = get_sync(1000);

		if (ret != OK)
			return ret;

        /* Emit progress bar update */
        if (bl_rev == 2) {
            /* expect slower verify for bootloader rev 2 */
            emit upgradeProgressChanged(10 + (int)(((float)sent/(float)fw_size)*45.0f));
        } else if (bl_rev >= 3) {
            /* expect fast verify for bootloader rev 3 */
            emit upgradeProgressChanged(10 + (int)(((float)sent/(float)fw_size)*80.0f));
        } else {
            log("progress bar update for this bootloader revision not supported");
        }
	}
	return OK;
}

int
PX4_Uploader::verify_rev2(size_t fw_size)
{
	uint8_t	file_buf[4];
    int count;
	int ret;
	size_t sent = 0;

	log("verify...");
    _fw_fd.seek(0);

	send(PROTO_CHIP_VERIFY);
	send(PROTO_EOC);
	ret = get_sync();

	if (ret != OK)
		return ret;

	while (sent < fw_size) {
		/* get more bytes to verify */
		size_t n = fw_size - sent;
		if (n > sizeof(file_buf)) {
			n = sizeof(file_buf);
		}
		count = read_with_retry(_fw_fd, file_buf, n);

        if (count != (int)n) {
			log("firmware read of %u bytes at %u failed -> %d errno %d", 
			    (unsigned)n,
			    (unsigned)sent,
			    (int)count,
			    (int)errno);
		}

		if (count == 0)
			break;

		sent += count;

		if (count < 0)
			return -errno;

        Q_ASSERT((count % 4) == 0);

		send(PROTO_READ_MULTI);
		send(count);
		send(PROTO_EOC);

        // XXX read more than one byte here

        for (int i = 0; i < count; i++) {
			uint8_t c;

            ret = recv(c, 5000);

			if (ret != OK) {
				log("%d: got %d waiting for bytes", sent + i, ret);
				return ret;
			}

			if (c != file_buf[i]) {
				log("%d: got 0x%02x expected 0x%02x", sent + i, c, file_buf[i]);
				return -EINVAL;
			}
		}

		ret = get_sync();

        /* verify with bootloader rev 2 55%...100% */
        emit upgradeProgressChanged(55 + (int)(((float)sent/(float)fw_size)*45.0f));

		if (ret != OK) {
			log("timeout waiting for post-verify sync");
			return ret;
		}
	}

    emit upgradeProgressChanged(100);

	return OK;
}

int
PX4_Uploader::verify_rev3(size_t fw_size_local)
{
	int ret;
	uint8_t	file_buf[4];
    int count;
    quint32 sum = 0;
    quint32 bytes_read = 0;
    quint32 crc = 0;
    quint32 fw_size_remote;
	uint8_t fill_blank = 0xff;

	log("verify...");
    _fw_fd.seek(0);

	ret = get_info(INFO_FLASH_SIZE, fw_size_remote);
	send(PROTO_EOC);

	if (ret != OK) {
		log("could not read firmware size");
		return ret;
	}

	/* read through the firmware file again and calculate the checksum*/
	while (bytes_read < fw_size_local) {
		size_t n = fw_size_local - bytes_read;
		if (n > sizeof(file_buf)) {
			n = sizeof(file_buf);
		}
		count = read_with_retry(_fw_fd, file_buf, n);

        if (count != (int)n) {
			log("firmware read of %u bytes at %u failed -> %d errno %d", 
			    (unsigned)n,
			    (unsigned)bytes_read,
			    (int)count,
			    (int)errno);
		}

		/* set the rest to ff */
		if (count == 0) {
			break;
		}
		/* stop if the file cannot be read */
		if (count < 0)
			return -errno;

		/* calculate crc32 sum */
		sum = crc32((uint8_t *)&file_buf, sizeof(file_buf), sum);

		bytes_read += count;
	}

	/* fill the rest with 0xff */
	while (bytes_read < fw_size_remote) {
		sum = crc32(&fill_blank, sizeof(fill_blank), sum);
		bytes_read += sizeof(fill_blank);
	}

	/* request CRC from IO */
	send(PROTO_GET_CRC);
	send(PROTO_EOC);

	ret = recv((uint8_t*)(&crc), sizeof(crc));

	if (ret != OK) {
		log("did not receive CRC checksum");
		return ret;
	}

	/* compare the CRC sum from the IO with the one calculated */
	if (sum != crc) {
		log("CRC wrong: received: %d, expected: %d", crc, sum);
		return -EINVAL;
	}

    emit upgradeProgressChanged(100);

	return OK;
}

int
PX4_Uploader::reboot()
{
	send(PROTO_REBOOT);
	send(PROTO_EOC);

	return OK;
}

void
PX4_Uploader::log(const char *fmt, ...)
{
	va_list	ap;

    char str[1000];

    int used = sprintf(str, "[PX4 Uploader] ");
	va_start(ap, fmt);
    used = vsprintf(str+used, fmt, ap);
	va_end(ap);
    //printf("\n");
    //fflush(stdout);
    emit upgradeStatusChanged(QString(str));
    qDebug() << QString(str);
}
