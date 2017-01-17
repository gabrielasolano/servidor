#ifndef INTERFACE_H
#define INTERFACE_H

#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QRegExp>
#include <QDir>
#include <stdio.h>
#include <QIODevice>
#include <QTextStream>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <linux/limits.h>

namespace Ui {
class Interface;
}

class Interface : public QMainWindow
{
  Q_OBJECT

public:
  explicit Interface(QWidget *parent = 0);
  ~Interface();

private slots:
  void on_pushAplicar_clicked();

  void on_pushCancelar_clicked();

  bool check_parameters();

  void create_configuration ();

  bool read_config ();

  void update_interface ();

  pid_t get_pid();

  void on_pushEncerrar_clicked();

  void on_buttonBox_clicked(QAbstractButton *button);

private:
  Ui::Interface *ui;
};

#endif // INTERFACE_H
