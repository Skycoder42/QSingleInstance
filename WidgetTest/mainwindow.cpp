#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	QAction *section = new QAction("seperator", this->ui->menuFile);
	section->setSeparator(true);
	this->ui->menuFile->addAction(section);
	QAction *subMenu = new QAction("menu", this->ui->menuFile);
	subMenu->setMenu(new QMenu(this->ui->menuFile));
	this->ui->menuFile->addAction(subMenu);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_actionTest_triggered()
{
	this->ui->actionTest->deleteLater();
}
