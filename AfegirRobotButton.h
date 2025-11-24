
#include <QPushButton>


class AfegirRobotButton : public QPushButton{
    Q_OBJECT;

    public:
        AfegirRobotButton(QWidget *parent);

    public slots:

        void modificarPosicioY(int newY);
        void modificarPosicioX(int newX);
        void crearRobotSlot();
    signals:
        void crearRobot(int x, int y);
        
    private:
        int x;
        int y;

};