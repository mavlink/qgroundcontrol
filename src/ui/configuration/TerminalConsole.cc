#include "terminalconsole.h"
#include "ui_terminalconsole.h"

TerminalConsole::TerminalConsole(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TerminalConsole)
{
    ui->setupUi(this);
}

TerminalConsole::~TerminalConsole()
{
    delete ui;
}
