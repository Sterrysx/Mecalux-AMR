#include <QComboBox>

class CustomComboBox : public QComboBox {
    Q_OBJECT

public:
    CustomComboBox(QWidget *parent);

public slots:
    void addItem(int value);
    void addItem(QString value);
    void eliminarRobotSlot();
signals:
    void eliminarRobot(int robotID);
};