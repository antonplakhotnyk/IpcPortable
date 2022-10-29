#include <QtWidgets/QApplication>
#include <QtGui/QWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMainWindow>
#include <QtCore/QDebug>
#include "ApplicationStatic.h"

#include <iostream>

#include "ApplicationUnderTest.h"
#include "ApplicationSut.h"
#include "IpcQt_Global.h"



int main(int argc, char* argv[])
{
	ApplicationStatic app(argc, argv);

	Ipc::InitSpecificClient(QStringList{Ipc::c_arg_autotest_server, "localhost"});
	
	ApplicationUnderTest application;
	QMainWindow main_window;
	main_window.show();

	std::cout<<"Test"<<std::endl;

	QApplication::exec();

	return 0;
}
