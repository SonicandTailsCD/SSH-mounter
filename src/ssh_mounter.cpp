/*
 * SSH Mounter - Simple GUI for SSHFS mounts
 * Copyright (C) 2025 SonicandTailsCD
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 */

#include "ssh_mounter.hpp"
#include "console.hpp"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QEventLoop>
#include <QString>

extern Console console;

const QString newline = "\n";

SSHMounter::SSHMounter(QObject* parent) 
    : QObject(parent), process_(nullptr), state_(MountState::Idle) {
}

void SSHMounter::setState(MountState state) {
    if (state_ != state) {
        state_ = state;
        emit stateChanged(state);
    }
}

bool SSHMounter::checkSSHFSInstalled() {
    QProcess p;
    p.start("which", {"sshfs"});
    p.waitForFinished(2000);
    return p.exitCode() == 0;
}

bool SSHMounter::checkFUSEAvailable() {
    // Check if /dev/fuse exists (Linux)
    return QFile::exists("/dev/fuse") || QFile::exists("/usr/local/bin/sshfs");
}

QString SSHMounter::checkWritePermission(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) {
        // Try to create it
        if (!dir.mkpath(".")) {
            return "Cannot create directory: " + path;
        }
    }
    
    // Test write permission
    QFileInfo info(path);
    if (!info.isWritable()) {
        return "No write permission for: " + path;
    }
    
    return QString(); // Empty = OK
}

bool SSHMounter::mount(const SSHHost& host) {
    if (state_ != MountState::Idle && state_ != MountState::Error) {
        emit mountError("Already busy with another operation");
        return false;
    }
    
    currentHost_ = host;
    setState(MountState::Mounting);
    
    // Validate local path
    QString writeErr = checkWritePermission(host.localPath);
    if (!writeErr.isEmpty()) {
        setState(MountState::Error);
        emit mountError(writeErr);
        return false;
    }
    
    // Build sshfs command
    QString remote = QString("%1@%2:%3")
        .arg(host.user)
        .arg(host.host)
        .arg(host.remotePath);
    
    QStringList args;
    args << remote << host.localPath;
    args << "-p" << QString::number(host.port);

    QString options = "reconnect,ServerAliveInterval=15,ServerAliveCountMax=3,max_conns=16";
    if (!host.usePublicKey) {
        // If not using public key, use password authentication and disable pubkey
        options += ",password_stdin,PubkeyAuthentication=no";
    } else {
        // If using public key, disable password authentication
        options += ",PasswordAuthentication=no";
    }
    args << "-o" << options;
    
    console.log("Mounting: sshfs", args.join(" ").toStdString());
    emit progressMessage("Connecting to " + host.host + "...");
    
    if (process_) {
        delete process_;
    }
    
    process_ = new QProcess(this);
    process_->setProcessChannelMode(QProcess::MergedChannels);
    
    // Create event loop but don't start it yet
    QEventLoop loop;
    connect(process_, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 
            &loop, &QEventLoop::quit);
    connect(process_, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
    
    // Connect our regular handlers
    connect(process_, &QProcess::readyReadStandardOutput, this, &SSHMounter::onProcessOutput);
    connect(process_, &QProcess::readyReadStandardError, this, &SSHMounter::onProcessOutput);
    
    // For password auth, emit password prompt before starting
    if (!host.usePublicKey) {
        emit passwordRequired();
    }
    
    process_->start("sshfs", args);
    
    // Now wait for completion
    loop.exec();    
    if (process_->exitCode() == 0 && process_->exitStatus() == QProcess::NormalExit) {
        setState(MountState::Idle);
        emit mountSuccess();
        console.log("Mount successful:", currentHost_.name.toStdString());
        return true;
    } else {
        setState(MountState::Error);
        QByteArray errors = process_->readAllStandardError();
        QString msg = errors.isEmpty() ? "Mount failed with unknown error" : errors;
        if (process_->error() == QProcess::FailedToStart) {
            msg = "Failed to start sshfs. Is it installed and in your PATH?";
        }
        emit mountError(msg);
        console.log("Mount failed:", msg.toStdString());
        return false;
    }
}

void SSHMounter::unmount(const QString& localPath) {
    if (state_ == MountState::Mounting || state_ == MountState::Unmounting) {
        emit mountError("Already busy with another operation");
        return;
    }
    
    setState(MountState::Unmounting);
    emit progressMessage("Unmounting " + localPath + "...");
    
    if (process_) {
        delete process_;
    }
    
    process_ = new QProcess(this);
    connect(process_, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &SSHMounter::onProcessFinished);
    connect(process_, &QProcess::errorOccurred, this, &SSHMounter::onProcessError);
    
#ifdef Q_OS_MAC
    process_->start("umount", {localPath});
#else
    process_->start("fusermount", {"-u", localPath});
#endif
}

void SSHMounter::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    QString output = process_->readAllStandardOutput();
    QString errors = process_->readAllStandardError();
    
    if (!output.isEmpty()) console.log("stdout:", output.toStdString());
    if (!errors.isEmpty()) console.log("stderr:", errors.toStdString());
    
    if (state_ == MountState::Mounting) {
        if (exitCode == 0 && status == QProcess::NormalExit) {
            setState(MountState::Idle);
            emit mountSuccess();
            console.log("Mount successful:", currentHost_.name.toStdString());
        } else {
            setState(MountState::Error);
            QString msg = errors.isEmpty() ? "Mount failed" : errors;
            emit mountError(msg);
            console.log("Mount failed:", msg.toStdString());
        }
    } else if (state_ == MountState::Unmounting) {
        if (exitCode == 0 && status == QProcess::NormalExit) {
            setState(MountState::Idle);
            emit unmountSuccess();
            console.log("Unmount successful");
        } else {
            setState(MountState::Error);
            QString msg = errors.isEmpty() ? "Unmount failed" : errors;
            emit mountError(msg);
            console.log("Unmount failed:", msg.toStdString());
        }
    }
}

SSHHost SSHMounter::getCurrentHost() {
    return currentHost_;
}

void SSHMounter::onProcessError(QProcess::ProcessError error) {
    QString msg;
    switch (error) {
        case QProcess::FailedToStart:
            msg = "Failed to start sshfs. Is it installed?";
            break;
        case QProcess::Timedout:
            msg = "Operation timed out";
            break;
        default:
            msg = "Process error occurred";
    }
    
    setState(MountState::Error);
    emit mountError(msg);
    console.log("Process error:", msg.toStdString());
}

void SSHMounter::onProcessOutput() {
    QString output = process_->readAllStandardOutput();
    QString errors = process_->readAllStandardError();
    
    console.log("Process output:", output.toStdString());
    console.log("Process errors:", errors.toStdString());

    // Check for various password prompts that SSHFS/SSH might use
    if (output.contains("password", Qt::CaseInsensitive) || 
        errors.contains("password", Qt::CaseInsensitive) ||
        output.contains("Password:") ||
        errors.contains("Password:") ||
        output.contains("password for") ||
        errors.contains("password for")) {
        emit passwordRequired();
    }

    if (output.contains("WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!") || errors.contains("WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!")) {
        emit hostKeyMismatch();
    }
}

void SSHMounter::removeHostKey() {
    if (process_) {
        delete process_;
    }
    
    process_ = new QProcess(this);
    
    QEventLoop loop;
    connect(process_, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), &loop, &QEventLoop::quit);
    connect(process_, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
    
    process_->start("ssh-keygen", {"-R", currentHost_.host});
    loop.exec();
    
    mount(currentHost_);
}

void SSHMounter::supplyPassword(const QString& password) {
    if (process_ && process_->state() == QProcess::Running) {
        process_->write((password + newline).toUtf8());
        process_->closeWriteChannel();
    }
}

void SSHMounter::noPassword() {
    process_->terminate();
    delete process_;
}

#include "ssh_mounter.moc"
