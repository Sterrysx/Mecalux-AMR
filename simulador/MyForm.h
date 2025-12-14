#include "ui_MyForm.h"

class MyForm : public QWidget
{
  Q_OBJECT

  public:
    MyForm (QWidget *parent=0);
  
  private slots:
    void onConnectBackendClicked();
  
  private:
    Ui::MyForm ui;
};
