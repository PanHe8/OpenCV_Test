#include "MainWindow.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFile>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  app.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));
  MainWindow window;
  window.show();
  const QStringList args = app.arguments();
  const bool smoke = args.contains(QStringLiteral("--smoke-test"));
  QString path = QDir(QCoreApplication::applicationDirPath())
                   .filePath(QStringLiteral("images/FPC1_mountPickTrain.bmp"));
  const int imageArg = args.indexOf(QStringLiteral("--image"));
  if (imageArg >= 0 && imageArg + 1 < args.size())
    path = args[imageArg + 1];
  QTimer::singleShot(0, &window, [&window, &app, path, smoke] {
    const bool loaded = window.loadImage(path);
    if (loaded) {
      if (smoke) {
        // 通过按钮验证源码中的 clicked 信号连接。
        auto* detectButton =
          window.findChild<QPushButton*>(QStringLiteral("detectButton"));
        if (detectButton)
          detectButton->click();
      }
    }
    if (smoke) {
      const QString dir = QCoreApplication::applicationDirPath();
      auto* editor =
        window.findChild<QPlainTextEdit*>(QStringLiteral("resultEdit"));
      QFile logFile(dir + QStringLiteral("/smoke-log.txt"));
      const bool logSaved = editor && logFile.open(QIODevice::WriteOnly) &&
        logFile.write(editor->toPlainText().toUtf8()) >= 0;
      const bool report = loaded && logSaved &&
        editor->toPlainText().contains(QStringLiteral("识别完成")) &&
        window.saveReport(dir + QStringLiteral("/smoke-results.csv"));
      // 等待事件循环完成布局后，再保存界面截图。
      QTimer::singleShot(250, &window, [&window, &app, dir, report] {
        const bool captured =
          window.grab().save(dir + QStringLiteral("/smoke-window.png"));
        app.exit(report && captured ? 0 : 1);
      });
    }
  });
  return app.exec();
}
