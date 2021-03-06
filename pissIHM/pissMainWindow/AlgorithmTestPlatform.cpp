#include "AlgorithmTestPlatform.h"

/**
 * @brief AlgorithmTestPlatform::AlgorithmTestPlatform
 * @param dispatcher
 * @param appWidth
 * @param appHeight
 */
AlgorithmTestPlatform::AlgorithmTestPlatform(SystemDispatcher* dispatcher,
                                             int appWidth,
                                             int appHeight)
{
    this->dispatcher = dispatcher;
    this->appWidth = appWidth;
    this->appHeight = appHeight;
    this->setFixedHeight(this->appHeight*0.18);
    this->initVariable();
    this->constructIHM();
    this->setConnections();



    ramProgressBar->setValue(40);
    diskProgressBar->setValue(35);
    netProgressBar->setValue(20);

    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof (statex);

    GlobalMemoryStatusEx (&statex);

    //      qDebug()<<statex.dwMemoryLoad;
    //      qDebug()<<statex.ullTotalPhys/DIV;
    //      qDebug()<<statex.ullAvailPhys/DIV;
    //       qDebug()<<statex.ullTotalPageFile/DIV;
    //      qDebug()<<statex.ullAvailPageFile/DIV;
    //      qDebug()<<statex.ullTotalVirtual/DIV;
    //       qDebug()<<statex.ullAvailVirtual/DIV;

    // Show the amount of extended memory available.

    //qDebug()<<statex.ullAvailExtendedVirtual/DIV;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief AlgorithmTestPlatform::setBackgroundColor
//! \param color
//!
void AlgorithmTestPlatform::setBackgroundColor(QString color){
    this->setStyleSheet("background-color:" + color);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief AlgorithmTestPlatform::getCpuUsing
//!
void AlgorithmTestPlatform::getCpuUsing(){

    double cpuValue;
    cpuValue = GetCPULoad() * 100;
    this->cpuProgressBar->setValue(cpuValue);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief AlgorithmTestPlatform::CalculateCPULoad
//! \param idleTicks
//! \param totalTicks
//! \return
//!
float AlgorithmTestPlatform::CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks){
    static unsigned long long previousTotalTicks = 0;
    static unsigned long long previousIdleTicks = 0;

    unsigned long long totalTicksSinceLastTime = totalTicks - previousTotalTicks;
    unsigned long long idleTicksSinceLastTime = idleTicks - previousIdleTicks;


    float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

    previousTotalTicks = totalTicks;
    previousIdleTicks = idleTicks;
    return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief AlgorithmTestPlatform::FileTimeToInt64
//! \param ft
//! \return
//!
unsigned long long AlgorithmTestPlatform::FileTimeToInt64(const FILETIME & ft){
    return (((unsigned long long)(ft.dwHighDateTime)) << 32) | ((unsigned long long)ft.dwLowDateTime);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
//! Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
//! You'll need to call this at regular intervals, since it measures the load between
//! the previous call and the current one.  Returns -1.0 on error.
//!
//! \brief AlgorithmTestPlatform::GetCPULoad
//! \return
//!
float AlgorithmTestPlatform::GetCPULoad(){
    FILETIME idleTime, kernelTime, userTime;
    return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief AlgorithmTestPlatform::initVariable
//!
void AlgorithmTestPlatform::initVariable(){

    this->labelStyleSheet = "border: 1px solid aliceBlue;border-radius: 3px;padding: 2 2px;background-color: transparent; color: red";
    this->progressBarStyleSheet = "QProgressBar {border: 1px solid aliceBlue;border-radius: 3px;}"
                                  "QProgressBar::chunk {background: qlineargradient(x1: 0, y1: 0.5, x2: 1, y2: 0.5, stop: 0 green, stop: 1 white);}";
    this->caracterStyle = new QFont("Helvetica",10,QFont::AnyStyle, false);
    QTimer *timer = new QTimer();
    timer->start(1000);
    this->connect(timer, SIGNAL(timeout()), this, SLOT(getCpuUsing()));
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//!
//!runmTestPlatform::constructIHM
//!
void AlgorithmTestPlatform::constructIHM(){

    this->algorithmTestTitleLabel = new QLabel();
    this->algorithmTestTitleLabel->setFixedHeight(this->appHeight*0.03);
    this->algorithmTestTitleLabel->setFont(*this->caracterStyle);
    this->algorithmTestTitleLabel->setStyleSheet("border: 0px solid gainsboro;border-radius: 0px;background-color: gainsboro; color: AliceBlue");

    this->algorithmTestCommandLabel = new QLabel("System Command");
    this->algorithmTestCommandLabel->setFixedHeight(this->appHeight*0.03);
    this->algorithmTestCommandLabel->setFont(QFont("Helvetica",12,QFont::AnyStyle, true));
    this->algorithmTestCommandLabel->setStyleSheet("border: 0px solid gainsboro;border-radius: 0px;background-color: gainsboro; color: AliceBlue");
    this->algorithmTestCommandLabel->setAlignment(Qt::AlignCenter);

    this->cpuLabel = new QLabel("CPU");
    this->cpuLabel->setFixedSize(appWidth*0.03,appHeight*0.02);
    this->cpuLabel->setFont(*this->caracterStyle);
    this->cpuLabel->setStyleSheet(this->labelStyleSheet);
    this->cpuLabel->setAlignment(Qt::AlignCenter);
    this->ramLabel = new QLabel("RAM");
    this->ramLabel->setFixedSize(appWidth*0.03,appHeight*0.02);
    this->ramLabel->setFont(*this->caracterStyle);
    this->ramLabel->setStyleSheet(this->labelStyleSheet);
    this->ramLabel->setAlignment(Qt::AlignCenter);
    this->diskLable = new QLabel("Disk");
    this->diskLable->setFixedSize(appWidth*0.03,appHeight*0.02);
    this->diskLable->setFont(*this->caracterStyle);
    this->diskLable->setStyleSheet(this->labelStyleSheet);
    this->diskLable->setAlignment(Qt::AlignCenter);
    this->netLable = new QLabel("Net");
    this->netLable->setFixedSize(appWidth*0.03,appHeight*0.02);
    this->netLable->setFont(*this->caracterStyle);
    this->netLable->setStyleSheet(this->labelStyleSheet);
    this->netLable->setAlignment(Qt::AlignCenter);

    this->cpuProgressBar = new QProgressBar();
    this->cpuProgressBar->setOrientation(Qt::Horizontal);
    this->cpuProgressBar->setAlignment(Qt::AlignCenter);
    this->cpuProgressBar->setFixedHeight(appHeight*0.015);
    this->cpuProgressBar->setStyleSheet(this->progressBarStyleSheet);
    this->cpuProgressBar->setMinimum(0);
    this->cpuProgressBar->setMaximum(100);
    this->ramProgressBar = new QProgressBar();
    this->ramProgressBar->setOrientation(Qt::Horizontal);
    this->ramProgressBar->setAlignment(Qt::AlignCenter);
    this->ramProgressBar->setFixedHeight(appHeight*0.015);
    this->ramProgressBar->setStyleSheet(this->progressBarStyleSheet);
    this->ramProgressBar->setMinimum(0);
    this->ramProgressBar->setMaximum(100);
    this->diskProgressBar = new QProgressBar();
    this->diskProgressBar->setOrientation(Qt::Horizontal);
    this->diskProgressBar->setAlignment(Qt::AlignCenter);
    this->diskProgressBar->setFixedHeight(appHeight*0.015);
    this->diskProgressBar->setStyleSheet(this->progressBarStyleSheet);
    this->diskProgressBar->setMinimum(0);
    this->diskProgressBar->setMaximum(100);
    this->netProgressBar = new QProgressBar();
    this->netProgressBar->setOrientation(Qt::Horizontal);
    this->netProgressBar->setAlignment(Qt::AlignCenter);
    this->netProgressBar->setFixedHeight(appHeight*0.015);
    this->netProgressBar->setStyleSheet(this->progressBarStyleSheet);
    this->netProgressBar->setMinimum(0);
    this->netProgressBar->setMaximum(100);

    this->algorithmTestStatusLabel = new QLabel();
    this->algorithmTestStatusLabel->setFixedHeight(this->appHeight*0.08);
    this->algorithmTestStatusLabel->setFont(*this->caracterStyle);
    this->algorithmTestStatusLabel->setStyleSheet("border: 0px solid gainsboro ;border-radius: 0px; background-color: transparent; color: AliceBlue");


    this->systemUsageStatusWidget = new QLabel();
    this->systemUsageStatusWidget->setFixedWidth(this->appWidth*0.13);
    this->systemUsageStatusLayout = new QGridLayout(this->systemUsageStatusWidget);

    this->systemUsageStatusLayout->addWidget(this->algorithmTestTitleLabel, 0, 0);
    this->systemUsageStatusLayout->addWidget(this->cpuLabel, 1, 0);
    this->systemUsageStatusLayout->addWidget(this->ramLabel, 2, 0);
    this->systemUsageStatusLayout->addWidget(this->diskLable, 3, 0);
    this->systemUsageStatusLayout->addWidget(this->netLable, 4, 0);
    this->systemUsageStatusLayout->addWidget(this->algorithmTestStatusLabel, 5, 0);

    this->systemUsageStatusLayout->addWidget(this->algorithmTestCommandLabel, 0, 1);
    this->systemUsageStatusLayout->addWidget(this->cpuProgressBar, 1, 1);
    this->systemUsageStatusLayout->addWidget(this->ramProgressBar, 2, 1);
    this->systemUsageStatusLayout->addWidget(this->diskProgressBar, 3, 1);
    this->systemUsageStatusLayout->addWidget(this->netProgressBar, 4, 1);
    this->systemUsageStatusLayout->setSpacing(0);
    this->systemUsageStatusLayout->setMargin(0);

    systemStatus = new QTextEdit();
    systemStatus->setFont(*this->caracterStyle);
    systemStatus->setStyleSheet("border: 1px solid aliceBlue;border-radius: 0px;background-color: transparent; color: red");

    systemCommand = new QLineEdit();
    systemCommand->setFixedHeight(this->appHeight*0.03);
    systemCommand->setFont(*this->caracterStyle);
    systemCommand->setStyleSheet("border: 1px solid gainsboro ;border-radius: 0px;background-color: transparent; color: red");

    terminalWindow = new QWidget();
    terminalWindowLayout = new QVBoxLayout(terminalWindow);
    terminalWindowLayout->addWidget(systemCommand);
    terminalWindowLayout->addWidget(systemStatus);
    terminalWindowLayout->setSpacing(0);
    terminalWindowLayout->setMargin(0);

    platformLayout = new QHBoxLayout(this);
    platformLayout->addWidget(systemUsageStatusWidget);
    platformLayout->addWidget(terminalWindow);
    platformLayout->setSpacing(0);
    platformLayout->setMargin(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief AlgorithmTestPlatform::setConnections
//!
void AlgorithmTestPlatform::setConnections(){
    this->connect(systemCommand,SIGNAL(returnPressed()),this,SLOT(doParseCommand()));
}

//-----------------------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief AlgorithmTestPlatform::setSystemStatus
//! \param status
//!
void AlgorithmTestPlatform::setSystemStatus(QString status){
    this->systemStatus->setText(status);
}

//----------------------------------------------------------------------------------------
//!
//! \brief AlgorithmTestPlatform::doParseCommand
//!
void AlgorithmTestPlatform::doParseCommand(){

    QString cmd = systemCommand->text();
    setSystemStatus(cmd);
    QString msg;

    if(cmd.contains("vef")){
        if(cmd.contains("mri")){
            QStringList temp = cmd.split(" mri ");
            int id = temp[1].toInt(0,10);
            msg = this->dispatcher->doImageProcessingByMethodType(id, 3, "vef");
        }
        else{
            //!do 2d vessel enhancement
        }
        this->setSystemStatus(msg);
    }
    else if(cmd.contains("check")){
        if(cmd.contains("mri")){
            if(cmd.contains("states")){
                this->getPatientsStatus();
            }
        }
    }
}

//----------------------------------------------------------------------------------------
//!doParseCommand
//! \brief AlgorithmTestPlatform::getPatientsStatus
//!
void AlgorithmTestPlatform::getPatientsStatus(){
    QStringList s = this->dispatcher->getPatientsStatus();
    QString info = "";
    for(int i = 0; i < s.length(); i++){
        info += s[i]+"\n";
    }
    this->setSystemStatus(info);
}
