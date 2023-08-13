#include <QtWidgets/QApplication>
#include <QtGui/QWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMainWindow>
#include <QtCore/QDebug>

#include <iostream>

#include "ApplicationUnderTest.h"
#include "Application_Sut.h"
#include "TestabilityGlobalQt.h"
#include "ApplicationStatic.h"



int main(int argc, char* argv[])
{
	ApplicationStatic app(argc, argv);

	TestabilityGlobalQt::InitClient<Application_Sut>(QStringList{TestabilityGlobalQt::c_arg_autotest_server, "localhost"});
	
	ApplicationUnderTest application;
	QMainWindow main_window;
	main_window.show();

	std::cout<<"Test"<<std::endl;

	QApplication::exec();

	return 0;
}
