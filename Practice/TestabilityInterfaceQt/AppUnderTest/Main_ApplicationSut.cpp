#include <QtWidgets/QApplication>
#include <QtGui/QWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMainWindow>
#include <QtCore/QDebug>
#include "ApplicationWithEvents.h"

#include <iostream>

#include "ApplicationUnderTest.h"
#include "ApplicationSut.h"
#include "IpcQt_Global.h"



int main(int argc, char* argv[])
{
	ApplicationWithEvents app(argc, argv);

	Ipc::InitClient("localhost", 2233);
	
	ApplicationUnderTest application;
	QMainWindow main_window;
	main_window.show();

	std::cout<<"Test"<<std::endl;

	QApplication::exec();

	return 0;
}
