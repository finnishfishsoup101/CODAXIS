#include <QApplication>
#include <QPixmap>
#include <QPushButton>
#include <iostream>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow win;
    win.resize(1280, 820);
    win.show();
    QApplication::processEvents();

    bool ok = win.validateAndOpenRepository(QStringLiteral("c:/CODAXIS/CODAXIS"));
    std::cout << "validateAndOpenRepository: " << (ok ? "true" : "false") << "\n";
    QApplication::processEvents();

    // Navigate to Hotspots page
    auto buttons = win.findChildren<QPushButton*>();
    for (auto *b : buttons) {
        if (b->text().contains("Hotspots")) {
            b->click();
            break;
        }
    }
    QApplication::processEvents();

    QPixmap pix(win.size());
    win.render(&pix);
    pix.save(QStringLiteral("C:/Users/asus/.gemini/antigravity-ide/brain/a602f795-b536-4010-89ab-ead225154b63/scratch/hotspots_page.png"));
    std::cout << "Saved hotspots_page.png\n";

    return 0;
}
