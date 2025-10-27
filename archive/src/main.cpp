#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <string>
#include "console.hpp"
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QPainter>
#include <QDebug>

Console console;

// Custom drawing area (you can draw buttons yourself here if you want)
class PaintArea : public QWidget {
    Q_OBJECT
public:
    explicit PaintArea(QWidget* parent = nullptr) : QWidget(parent) {
        setFocusPolicy(Qt::StrongFocus); // accepts keyboard focus
    }

protected:
    void paintEvent(QPaintEvent* /*ev*/) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        // Background
        p.fillRect(rect(), palette().window());

        // Example drawing: rounded rect + text centered
        QRect r = rect().adjusted(10, 10, -10, -10);
        p.setPen(QPen(Qt::black, 2));
        p.setBrush(QColor(0xee, 0xee, 0xff));
        p.drawRoundedRect(r, 8, 8);

        // Centered text
        QString txt = QStringLiteral("Custom drawing area\n(uses Qt double-buffering)");
        p.setPen(Qt::black);
        p.setFont(QFont("Sans", 10));
        p.drawText(r, Qt::AlignCenter, txt);
    }

    // Example of keyboard handling inside the widget
    void keyPressEvent(QKeyEvent* ev) override {
        if (ev->key() == Qt::Key_Space) {
            qDebug() << "Space pressed inside PaintArea";
        } else {
            QWidget::keyPressEvent(ev);
        }
    }
};

// Main window with buttons and the paint area
class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr) : QWidget(parent) {
        auto* mainLayout = new QVBoxLayout(this);

        // Add paint area (grows)
        paintArea = new PaintArea(this);
        paintArea->setMinimumSize(480, 240);
        mainLayout->addWidget(paintArea, /*stretch=*/1);

        // Button row
        auto* row = new QHBoxLayout();
        btnHello = new QPushButton("Hello", this);
        btnQuit  = new QPushButton("Quit", this);

        row->addStretch(1);
        row->addWidget(btnHello);
        row->addSpacing(8);
        row->addWidget(btnQuit);
        row->addStretch(1);

        mainLayout->addLayout(row);

        // Connect signals -> slots (modern syntax)
        connect(btnHello, &QPushButton::clicked, this, &MainWindow::onHello);
        connect(btnQuit,  &QPushButton::clicked, qApp, &QCoreApplication::quit);

        // Keyboard shortcuts & focus policies
        btnHello->setFocusPolicy(Qt::StrongFocus);
        btnQuit->setFocusPolicy(Qt::StrongFocus);
        setWindowTitle("Qt5 Buttons + Custom Paint (Sonic)");
    }

private slots:
    void onHello() {
        // Keep logic here; avoid heavy blocking operations on UI thread
        qDebug() << "Hello pressed";
    }

private:
    PaintArea* paintArea;
    QPushButton* btnHello;
    QPushButton* btnQuit;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    return app.exec();
}

#include "moc_main.cpp"