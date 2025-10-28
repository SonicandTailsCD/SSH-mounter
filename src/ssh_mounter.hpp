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

#pragma once

#include "ssh_store.hpp"
#include <QObject>
#include <QProcess>

enum class MountState {
    Idle,
    Mounting,
    Mounted,
    Unmounting,
    Error
};

class SSHMounter : public QObject {
    Q_OBJECT
public:
    explicit SSHMounter(QObject* parent = nullptr);

    bool mount(const SSHHost& host);
    void unmount(const QString& localPath);

    // Check system capabilities
    static bool checkSSHFSInstalled();
    static bool checkFUSEAvailable();
    static QString checkWritePermission(const QString& path);

    void setState(MountState state);
    SSHHost getCurrentHost();
    void removeHostKey();

public slots:
    void supplyPassword(const QString& password); // UI calls this
    void noPassword();

signals:
    void stateChanged(MountState state);
    void mountSuccess();
    void mountError(const QString& error);
    void unmountSuccess();
    void passwordRequired(); // Emitted by mounter when password request is detected
    void progressMessage(const QString& msg);
    void hostKeyMismatch();

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void onProcessOutput();

private:
    QProcess* process_;
    MountState state_;
    SSHHost currentHost_;
};
