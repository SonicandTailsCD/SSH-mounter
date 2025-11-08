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

#include "console.hpp"
#include "ssh_store.hpp"
#include "ssh_mounter.hpp"

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QMessageBox>
#include <QCheckBox>
#include <QTimer>
#include <QPainter>
#include <QPropertyAnimation>
#include <QFileDialog>
#include <QCloseEvent>
#include <QInputDialog>
#include <cmath>

Console console;

// Animated spinner widget
class SpinnerWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int rotation READ rotation WRITE setRotation)
public:
    SpinnerWidget(QWidget* parent = nullptr) : QWidget(parent), rotation_(0) {
        setFixedSize(24, 24);
        animation_ = new QPropertyAnimation(this, "rotation", this);
        animation_->setDuration(1000);
        animation_->setStartValue(0);
        animation_->setEndValue(360);
        animation_->setLoopCount(-1);
    }
    
    void start() { animation_->start(); update(); }
    void stop() { animation_->stop(); rotation_ = 0; update(); }
    
    int rotation() const { return rotation_; }
    void setRotation(int r) { rotation_ = r; update(); }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(width()/2, height()/2);
        p.rotate(rotation_);
        
        for (int i = 0; i < 8; ++i) {
            int alpha = 255 - (i * 30);
            QPen pen(QColor(100, 100, 255, alpha));
            pen.setWidth(3);
            pen.setCapStyle(Qt::RoundCap);
            p.setPen(pen);
            p.drawLine(0, -8, 0, -4);
            p.rotate(45);
        }
    }
    
private:
    int rotation_;
    QPropertyAnimation* animation_;
};

// Host edit dialog
class HostDialog : public QDialog {
public:
    HostDialog(QWidget* parent = nullptr, const SSHHost* host = nullptr) 
        : QDialog(parent) {
        setWindowTitle(host ? "Edit Host" : "Add Host");
        auto* layout = new QFormLayout(this);
        
        nameEdit_ = new QLineEdit(this);
        userEdit_ = new QLineEdit(this);
        hostEdit_ = new QLineEdit(this);
        portSpin_ = new QSpinBox(this);
        portSpin_->setRange(1, 65535);
        portSpin_->setValue(22);
        remoteEdit_ = new QLineEdit(this);
        localEdit_ = new QLineEdit(this);
        pubkeyCheck_ = new QCheckBox("Use Public Key Authentication", this);
        
        if (host) {
            nameEdit_->setText(host->name);
            userEdit_->setText(host->user);
            hostEdit_->setText(host->host);
            portSpin_->setValue(host->port);
            remoteEdit_->setText(host->remotePath);
            localEdit_->setText(host->localPath);
            pubkeyCheck_->setChecked(host->usePublicKey);
        }
        
        layout->addRow("Name:", nameEdit_);
        layout->addRow("User:", userEdit_);
        layout->addRow("Host:", hostEdit_);
        layout->addRow("Port:", portSpin_);
        layout->addRow("Remote Path:", remoteEdit_);
        layout->addRow("", pubkeyCheck_);
        
        auto* localLayout = new QHBoxLayout();
        localLayout->addWidget(localEdit_);
        auto* browseBtn = new QPushButton("Browse", this);
        localLayout->addWidget(browseBtn);
        layout->addRow("Local Path:", localLayout);
        
        connect(browseBtn, &QPushButton::clicked, [this](){
            QString dir = QFileDialog::getExistingDirectory(this, "Select Mount Point");
            if (!dir.isEmpty()) localEdit_->setText(dir);
        });
        
        auto* btnBox = new QHBoxLayout();
        auto* okBtn = new QPushButton("OK", this);
        auto* cancelBtn = new QPushButton("Cancel", this);
        btnBox->addStretch();
        btnBox->addWidget(okBtn);
        btnBox->addWidget(cancelBtn);
        layout->addRow(btnBox);
        
        connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    }
    
    SSHHost getHost() const {
        SSHHost h;
        h.name = nameEdit_->text();
        h.user = userEdit_->text();
        h.host = hostEdit_->text();
        h.port = portSpin_->value();
        h.remotePath = remoteEdit_->text();
        h.localPath = localEdit_->text();
        h.usePublicKey = pubkeyCheck_->isChecked();
        return h;
    }
    
private:
    QLineEdit* nameEdit_;
    QLineEdit* userEdit_;
    QLineEdit* hostEdit_;
    QSpinBox* portSpin_;
    QLineEdit* remoteEdit_;
    QLineEdit* localEdit_;
    QCheckBox* pubkeyCheck_;
};

// Main window
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow() {
        setWindowTitle("SSH Mounter");
        setMinimumSize(600, 400);
        
        auto* central = new QWidget(this);
        setCentralWidget(central);
        auto* mainLayout = new QVBoxLayout(central);
        
        // Status bar
        statusLabel_ = new QLabel("Ready", this);
        auto* statusLayout = new QHBoxLayout();
        statusLayout->addWidget(statusLabel_);
        spinner_ = new SpinnerWidget(this);
        spinner_->hide();
        statusLayout->addWidget(spinner_);
        statusLayout->addStretch();
        mainLayout->addLayout(statusLayout);
        
        // Host list
        hostList_ = new QListWidget(this);
        mainLayout->addWidget(hostList_, 1);
        
        // Buttons
        auto* btnLayout = new QHBoxLayout();
        addBtn_ = new QPushButton("Add Host", this);
        editBtn_ = new QPushButton("Edit", this);
        removeBtn_ = new QPushButton("Remove", this);
        mountBtn_ = new QPushButton("Mount", this);
        unmountBtn_ = new QPushButton("Unmount", this);
        
        btnLayout->addWidget(addBtn_);
        btnLayout->addWidget(editBtn_);
        btnLayout->addWidget(removeBtn_);
        btnLayout->addStretch();
        btnLayout->addWidget(mountBtn_);
        btnLayout->addWidget(unmountBtn_);
        mainLayout->addLayout(btnLayout);

        store_ = new SSHStore(this);
        mounter_ = new SSHMounter(this);
        process_ = nullptr;
        mounts_ = nullptr;
        
        checkSystemRequirements();
        
        if (!store_->load()) {
            QMessageBox::warning(this, "Error", "Failed to load hosts");
        }
        refreshHostList();
        
        // Connect signals
        connect(addBtn_, &QPushButton::clicked, this, &MainWindow::addHost);
        connect(editBtn_, &QPushButton::clicked, this, &MainWindow::editHost);
        connect(removeBtn_, &QPushButton::clicked, this, &MainWindow::removeHost);
        connect(mountBtn_, &QPushButton::clicked, this, &MainWindow::mountHost);
        connect(unmountBtn_, &QPushButton::clicked, this, &MainWindow::unmountHost);
        connect(hostList_, &QListWidget::currentRowChanged, this, &MainWindow::onClickHost);
        connect(mounter_, &SSHMounter::stateChanged, this, &MainWindow::onMountStateChanged);
        connect(mounter_, &SSHMounter::mountSuccess, this, &MainWindow::onMountSuccess);
        connect(mounter_, &SSHMounter::mountError, this, &MainWindow::onMountError);
        connect(mounter_, &SSHMounter::progressMessage, this, &MainWindow::textHandler);
        connect(mounter_, &SSHMounter::passwordRequired, this, &MainWindow::onPasswordRequired);
        connect(mounter_, &SSHMounter::hostKeyMismatch, this, &MainWindow::onHostKeyMismatch);
        
        console.log("Application started");
    }
    
protected:
    void closeEvent(QCloseEvent* event) override {
        if (!store_->save()) {
            int ret = QMessageBox::warning(this, "Save Error", 
                "Failed to save hosts. Quit anyway?",
                QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::No) {
                event->ignore();
                return;
            }
        }
        console.log("Application closed");
        event->accept();
    }
    
private slots:
    void addHost() {
        HostDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            SSHHost host = dlg.getHost();
            store_->addHost(host);
            refreshHostList();
            showCheckmark("Host added ✓");
        }
    }
    
    void textHandler(const QString &text) {
        MainWindow::statusLabel_->setText(text);
        if (text.contains("Connecting")) {
            spinner_->show();
        }
        else spinner_->hide();
    }

    void editHost() {
        int idx = hostList_->currentRow();
        if (idx < 0) return;
        
        SSHHost host = store_->getHosts()[idx];
        HostDialog dlg(this, &host);
        if (dlg.exec() == QDialog::Accepted) {
            store_->updateHost(idx, dlg.getHost());
            refreshHostList();
            showCheckmark("Host updated ✓");
        }
    }

    void onClickHost(int currentRow) {
        if (!store_) return;
        if (currentRow < 0) return;
        SSHHost host = store_->getHosts()[currentRow];
        if (!mounts_) mountListUpdate();
        bool mounted = false;
        for (const QString& mount : *mounts_) {
            QString text = QString("%1@%2:%3").arg(host.user).arg(host.host).arg(host.remotePath);
            if (mount.contains(text)) {
                mounted = true;
                break;
            }
        };
        if (mounted) {
            mountBtn_->hide();
            unmountBtn_->show();
        }
        else {
            mountBtn_->show();
            unmountBtn_->hide();
        }
    }

    void onPasswordRequired() {
        bool ok;
        int id = hostList_->currentRow();
        if (id < 0) return;

        SSHHost host = store_->getHosts()[id];

        QString password = QInputDialog::getText(this, QString("Login to %1@%2").arg(host.user).arg(host.host), QString("Authentication is required to SSH into %1@%2").arg(host.user).arg(host.host), QLineEdit::Password, QString(), &ok);

        if (ok && !password.isEmpty()) {
            mounter_->supplyPassword(password);
        }
        else {
            statusLabel_->setText("Cancelled.");
            mounter_->setState(MountState::Idle);
            mounter_->noPassword();
        }
    }

    void onHostKeyMismatch() {
        int ret = QMessageBox::warning(this, "Host Key Mismatch", 
            "The host key for " + mounter_->getCurrentHost().host + " has changed!\n"
            "This could be a sign of a man-in-the-middle attack.\n\n"
            "Do you want to remove the old key and reconnect?",
            QMessageBox::Yes | QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            mounter_->removeHostKey();
        }
        else {
            statusLabel_->setText("Cancelled.");
            mounter_->setState(MountState::Idle);
        }
    }
    
    void removeHost() {
        int idx = hostList_->currentRow();
        if (idx < 0) return;
        
        store_->removeHost(idx);
        refreshHostList();
        showCheckmark("Host removed ✓");
    }
    
    void mountHost() {
        int idx = hostList_->currentRow();
        if (idx < 0) {
            QMessageBox::warning(this, "Error", "Please select a host");
            return;
        }
        
        SSHHost host = store_->getHosts()[idx];
        if (mounter_->mount(host)) mountListUpdate();
    }
    
    void unmountHost() {
        int idx = hostList_->currentRow();
        if (idx < 0) {
            QMessageBox::warning(this, "Error", "Please select a host");
            return;
        }
        
        SSHHost host = store_->getHosts()[idx];
        mounter_->unmount(host.localPath);
        mountListUpdate();
    }
    
    void onMountStateChanged(MountState state) {
        bool busy = (state == MountState::Mounting || state == MountState::Unmounting);
        
        if (busy) {
            spinner_->start();
        } else {
            spinner_->stop();
            spinner_->hide();
        }
        
        // Disable buttons during operations
        mountBtn_->setEnabled(!busy);
        unmountBtn_->setEnabled(!busy);
        addBtn_->setEnabled(!busy);
        editBtn_->setEnabled(!busy);
        removeBtn_->setEnabled(!busy);
    }
    
    void mountListUpdate() {
        if (process_) delete process_;
        if (mounts_) delete mounts_;
        mounts_ = new QStringList();
        process_ = new QProcess(this);
        QEventLoop loop;
        connect(process_, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), &loop, &QEventLoop::quit);
        connect(process_, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
        connect(process_, &QProcess::readyReadStandardOutput, this, &MainWindow::onProcessOutput);
        connect(process_, &QProcess::readyReadStandardError, this, &MainWindow::onProcessOutput);
    
        process_->start("mount");
        loop.exec();
    }

    void onProcessOutput() {
        QString output = process_->readAllStandardOutput();
        QString errors = process_->readAllStandardError();
        if (errors.contains("not found")) {
            mounts_->append("none");
        }
        else {
            for (const QString& mount : output.split("\n")) {
                if (mount.trimmed() != "") mounts_->append(mount);
            }
        };
    }

    void onMountSuccess() {
        showCheckmark("Mounted successfully ✓");
        mountBtn_->hide();
        unmountBtn_->show();
    }
    
    void onMountError(const QString& error) {
        statusLabel_->setText("Error: " + error);
        QMessageBox::critical(this, "Mount Error", error);
    }
    
    void showCheckmark(const QString& msg) {
        statusLabel_->setText(msg);
        QTimer::singleShot(2000, [this](){ statusLabel_->setText("Ready"); mounter_->setState(MountState::Idle); });
    }
    
    void refreshHostList() {
        hostList_->clear();
        for (const auto& host : store_->getHosts()) {
            QString text = QString("%1 (%2@%3)")
                .arg(host.name)
                .arg(host.user)
                .arg(host.host);
            hostList_->addItem(text);
        }
    }
    
    void checkSystemRequirements() {
        QStringList issues;
        
        if (!SSHMounter::checkSSHFSInstalled()) {
            issues << "sshfs is not installed";
        }
        if (!SSHMounter::checkFUSEAvailable()) {
            issues << "FUSE is not available";
        }
        
        if (!issues.isEmpty()) {
            QMessageBox::warning(this, "System Requirements", 
                "Some requirements are missing:\n" + issues.join("\n") +
                "\n\nThe application may not work correctly.");
        }
    }
    
private:
    QListWidget* hostList_;
    QPushButton* addBtn_;
    QPushButton* editBtn_;
    QPushButton* removeBtn_;
    QPushButton* mountBtn_;
    QPushButton* unmountBtn_;
    QLabel* statusLabel_;
    SpinnerWidget* spinner_;
    QProcess* process_;
    SSHStore* store_;
    SSHMounter* mounter_;
    QStringList* mounts_;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow win;
    win.show();
    console.info("[INFO] ", CLR_RESET, "Application started.");
    return app.exec();
}

#include "main.moc"
