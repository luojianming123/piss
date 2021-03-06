#ifndef  WIDGET_H
#define WIDGET_H

#define vtkRenderingCore_AUTOINIT 4(vtkInteractionStyle,vtkRenderingFreeType,vtkRenderingFreeTypeOpenGL,vtkRenderingOpenGL)
#define vtkRenderingVolume_AUTOINIT 1(vtkRenderingVolumeOpenGL)

#include <QWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QVTKWidget.h>
#include <QLineEdit>
#include <QTextEdit>
#include <QDesktopWidget>
#include <QPixmap>
#include <QPalette>
#include <QBrush>
#include <QBitmap>
#include <QApplication>
#include <QPoint>
#include <QTime>

#include <QRect>

#include <CPushButton.h>
#include <MRAFileReader.h>
#include <IgssImage.h>
#include <IgssVtkImageConverter.h>
#include <SystemDispatcher.h>
#include <PatientsWidget.h>
#include <ReplaysWidget.h>
#include <SurgerySystemWidget.h>
#include <SurgeryPlanWindow.h>
#include <GuidewareTrackingWindow.h>
#include <AlgorithmTestPlatform.h>
#include <PatientItem.h>
#include <ControlConsoleWindow.h>

#include "SystemOptions.h"


typedef struct{
    int screenIndex;
    QRect rect;
}Screen;

struct MainOptionStates{
    bool patientsWidgetOptionState;
    bool systemWidgetOptionState;
    bool historyWidgetOptionState;
};

/**
 * @brief The pissMainWindow class
 */
class pissMainWindow : public QWidget{
    Q_OBJECT

public:
    void findPatientExisted();
    void initVariable();
    void initVisualizationComponents();
    void constructIHM();
    void setConnections();
    void display();
    void displayMraImage(vtkImageData* input);
    void setSystemDispatcher(SystemDispatcher* systemDispatcher);
    void readImageFileFrom(QString path);
    void readImageFromVtkConvert();
    void setSystemStatus(QString status);
    void drawBackground();
    void globalBackgroundColorSetting();

    AlgorithmTestPlatform* getAlgorithmTestPlatform();

private:

    QString globalBackgroundColor;
    QString globalWorkSpaceColor;

    SystemOptions *systemOptionWindow;
    MainOptionStates mainOptionStates;

    QTime* surgeryTimer;

    QDesktopWidget *desktop;
    Screen *screen;

    QFont *englishCaracterStyle;

    QPixmap *pixmap;

    QVBoxLayout* igssMainWindowLayout;
    QVBoxLayout *controlBoardLayout;
    QHBoxLayout *mainPlatformLayout;
    QGridLayout *statusBarLayout;
    QVBoxLayout *systemInformationBoardWidgetLayout;

    QWidget *mainPlatform;
    QWidget *configurationBoard;
    QWidget *controlBoard;
    QWidget *statusBar;
    QWidget *systemInformationBoardWidget;

    QSpacerItem* controlAreaItem;

    AlgorithmTestPlatform* algorithmTestPlatform;

    QPushButton *displayStatusButton;
    QPushButton *closeButton;
    QPushButton *systemConfigurationButton;

    CPushButton *patientsWidgetOption;
    CPushButton *systemWidgetOption;
    CPushButton *historyWidgetOption;

    QLabel *appIndicationLabel;

    QFrame *widgetOptionContainer;
    QHBoxLayout *widgetOptionContainerLayout;

    QWidget *widgetsContainer;
    QHBoxLayout *widgetsContainerLayout;

    QTabWidget *systemInformationBoard;

    QVTKWidget* mraImageDisplayWindow;

    QString systemInformationBoardStyleSheet;

    PatientsWidget* ecranDesMaladies;
    ReplaysWidget*replaysWidget;
    SurgerySystemWidget* surgerySystemWidget;

    //! component for mra image display
    vtkVolumeRayCastMapper* volumeMapper;
    vtkVolumeRayCastCompositeFunction *compositeFunction;
    vtkVolume *volume;
    vtkSmartPointer<vtkRenderWindow> renderWindow;
    vtkSmartPointer<vtkRenderer> renderer;


    SystemDispatcher* systemDispatcher;
    QString mhdImagePath;
    IgssImage* mraImage;
    IgssImage* igssImage;
    MRAFileReader* mhdFileReader;
    vtkSmartPointer<vtkImageData>vtkImage;
    IgssVtkImageConverter* igssVtkImageConverter;

    SurgeryPlanWindow* surgeryPlanWindow;
    PatientInformationWidget* patientInformationWidget;
    GuidewareTrackingWindow* guidewareTrackingWindow;
    ControlConsoleWindow * controlConsoleWindow;

    int flag;
    int primary_screen_width;
    int primary_screen_height;
    int screen_count;
    int primary_screen;

    QPoint mousePosition;
    QPoint mouseMovingPosition;

    bool buttonGaucheDuSourisClique;
    bool pickInformationBoardButtonIsClicked;

protected:
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private slots:

    void onPatientsWidgetOptionHovered();
    void onPatientsWidgetOptionClicked();
    void onPatientsWidgetOptionReleased();
    void onPatientsWidgetOptionLeaved();

    void onSystemWidgetOptionHovered();
    void onSystemWidgetOptionClicked();
    void onSystemWidgetOptionReleased();
    void onSystemWidgetOptionLeaved();

    void onHistoryWidgetOptionHovered();
    void onHistoryWidgetOptionClicked();
    void onHistoryWidgetOptionReleased();
    void onHistoryWidgetOptionLeaved();

    void closeSystem();
    void configurerLeSysteme();
    void surgeryTerminated();
    void updateIHM();

public:
    pissMainWindow(SystemDispatcher *systemDispatcher);
    //~pissMainWindow();
};

#endif // WIDGET_H
