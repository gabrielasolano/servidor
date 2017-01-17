#include "interface.h"
#include "ui_interface.h"

const QString CONF_FILE = "config.txt";
const QString PID_FILE = "pid.txt";
QString porta;
QString banda;
QString diretorio;

Interface::Interface(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::Interface)
{
  ui->setupUi(this);

  if (read_config())
  {
    update_interface();
  }
}

Interface::~Interface()
{
  delete ui;
}

bool Interface::read_config ()
{
  QString path = QDir::homePath() + "/" + CONF_FILE;
  QFile file(path);

  if(!file.open(QIODevice::ReadOnly))
  {
    QMessageBox::information(0, "Erro", "Nao foi possivel abrir o arquivo de "
                                        "configuracao.");
    return false;
  }

  QTextStream stream(&file);
  stream.operator >>(porta);
  stream.operator >>(diretorio);
  stream.operator >>(banda);

  return true;
}

void Interface::update_interface ()
{
  ui->labelPortaAtual->setText("Porta atual: " + porta);
  ui->labelDiretorioAtual->setText("Diretorio atual: " + diretorio);
  ui->labelBandaAtual->setText("Banda atual: " + banda);
}

void Interface::on_pushAplicar_clicked()
{
  pid_t pid = get_pid();
  if (pid == -1)
  {
    return;
  }

  if (!check_parameters())
  {
    return;
  }

  create_configuration();

  if (kill(pid, SIGUSR1) != 0)
  {
    QMessageBox::information(0, "Erro", "Servidor nao atualizado.");
    return;
  }

  on_pushCancelar_clicked();

  update_interface();
}

pid_t Interface::get_pid()
{
  pid_t pid_servidor;

  QString home = QDir::homePath() + "/" + PID_FILE;
  QFile pid_file(home);

  if (!pid_file.open(QIODevice::ReadOnly))
  {
    QMessageBox::information(0, "Erro", "Nao foi possivel recuperar o PID do "
                                        "servidor");
    return -1;
  }

  QTextStream stream(&pid_file);
  stream.operator >>(pid_servidor);

  return pid_servidor;
}

/*! Cria arquivo de configuracao com a porta, o diretorio raiz e a banda*/
void Interface::create_configuration ()
{
  QString home = QDir::homePath() + "/" + CONF_FILE;

  QFile conf_file(home);

  if (!conf_file.open(QIODevice::WriteOnly))
  {
    QMessageBox::information(0, "Erro", "Nao foi possivel abrir o arquivo de "
                                        "configuracao.");
    return;
  }

  QTextStream stream(&conf_file);
  stream.operator <<(porta + "\n");
  stream.operator <<(diretorio + "\n");
  stream.operator <<(banda + "\n");
}

bool Interface::check_parameters ()
{
  QString pnova = ui->linePorta->text();
  QString dnovo = ui->lineDiretorio->text();
  QString bnova = ui->lineBanda->text();

  QRegExp expr("\\d*");

  if (pnova.size() > 0)
  {
    if ((!expr.exactMatch(pnova)) || (pnova.toLong() <= 0)
        || (pnova.toLong() > 65535))
    {
      QMessageBox::information(0, "Erro", "Insira um valor valido para porta.\n"
                                          "Valores validos: 0 a 65535.");
      return false;
    }
    porta = pnova;
  }

  if (dnovo.size() > 0)
  {
    QDir path;
    if (!path.exists(dnovo))
    {
      QMessageBox::information(0, "Erro", "O caminho informado nao existe.\n");
      return false;
    }
    diretorio = dnovo;
  }

  if (bnova.size() > 0)
  {
    if ((bnova == "n") || (bnova == "N"))
    {
      banda = "0";
      return true;
    }
    else if ((!expr.exactMatch(bnova)) || (bnova.toLong() < 0))
    {
      QMessageBox::information(0, "Erro", "Insira um valor valido para banda.\n"
                                          "Valores validos: maiores que 0.\n"
                                          "Insira [N/n] para nao limitar.");
      return false;
    }
    banda = bnova;
  }
  return true;
}

void Interface::on_pushCancelar_clicked()
{
  ui->lineBanda->setText("");
  ui->lineDiretorio->setText("");
  ui->linePorta->setText("");
}


void Interface::on_pushEncerrar_clicked()
{
  pid_t pid = get_pid();
  if (pid == -1)
  {
    return;
  }
  if (kill(pid, SIGINT) != 0)
  {
    QMessageBox::information(0, "Erro", "Nao foi possivel encerrar o servidor");
    return;
  }
  QMessageBox::information(0, "Encerrado", "Servidor encerrado com sucesso.");
  ui->labelPortaAtual->setText("Porta atual: ");
  ui->labelDiretorioAtual->setText("Diretorio atual: ");
  ui->labelBandaAtual->setText("Banda atual: ");
}

void Interface::on_buttonBox_clicked(QAbstractButton *button)
{
  if(ui->buttonBox->standardButton(button) == QDialogButtonBox::Close)
  {
    this->close();
  }
  else
  {
    if (read_config())
    {
      update_interface();
    }
  }
}
