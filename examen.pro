TEMPLATE    = app
QT         += opengl

INCLUDEPATH +=  /usr/include/glm
INCLUDEPATH += ./Model

FORMS += MyForm.ui

HEADERS += SimuladorGLWidget.h \
           MyForm.h \
           AfegirRobotButton.h \
           CustomComboBox.h

SOURCES += main.cpp \
        ./Model/model.cpp \
        SimuladorGLWidget.cpp \
        MyForm.cpp \
        AfegirRobotButton.cpp \
        CustomComboBox.cpp
