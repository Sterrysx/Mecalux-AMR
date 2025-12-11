
#include <QPushButton>


class EliminarRobotButton : public QPushButton{
    Q_OBJECT;

    public:
        EliminarRobotButton(QWidget *parent);

    public slots:

        void modificarSelectedRobot(int robotID);
    signals:
        void eliminarRobot(int robotID);
    private:
        int RobotID;


};