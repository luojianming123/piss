#include "SurgeryPlanWindow.h"

SurgeryPlanWindow::SurgeryPlanWindow(QRect rect,
                                     QTime* surgeryTime,
                                     SystemDispatcher* systemDispatcher,
                                     AlgorithmTestPlatform *algorithmTestPlatform) : QWidget()
{
    this->rect = rect;
    this->x = rect.x();
    this->y = rect.y();

    this->appWidth = rect.width();
    this->appHeight = rect.height();
    this->systemDispatcher = systemDispatcher;
    this->algorithmTestPlatform = algorithmTestPlatform;
    this->surgeryTime = surgeryTime;

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);// | Qt::WindowStaysOnTopHint);
    this->setWindowOpacity(1);
    this->setMouseTracking(true);
    this->setAutoFillBackground(true);

    defaultFolderIcon = new QIcon(":/images/folder.png");
    fileUnloadedIcon = new QIcon(":/images/fileunloaded.png");
    fileLoadedIcon = new QIcon(":/images/fileloaded.png");
    defaultTitleIcon = new QIcon(":/images/title.png");
    font = new  QFont("Helvetica", 8, QFont::AnyStyle, true);

    this->constructPatientInformationWidget();
    this->constructCprAnalyseWidget();
    this->constructControlBar();
    this->regroupAllComponents();

    this->initialisation();
    this->setConnections();
    this->drawBackground();

    //this->MraConnections = vtkEventQtSlotConnect::New();

    //! update coords as we move through the window
    //this->MraConnections->Connect(patientMRAImage->GetRenderWindow()->GetInteractor(), vtkCommand::MouseMoveEvent, this, SLOT(updateCoords(vtkObject*)));
    //this->MraConnections->Connect(patientMRAImage->GetRenderWindow()->GetInteractor(), vtkCommand::LeftButtonPressEvent, this, SLOT(onImageMouseLeftButtonPressed(vtkObject*)));
    //this->MraConnections->Connect(patientMRAImage->GetRenderWindow()->GetInteractor(), vtkCommand::LeftButtonReleaseEvent, this, SLOT(onImageMouseLeftButtonReleased(vtkObject*)));
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::setStartTime
//! \param start_time
//!
void SurgeryPlanWindow::setStartTime(int start_time){
    this->start_time = start_time;
    this->timer->start(1000);
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::update
//!
void SurgeryPlanWindow::update(){

    this->patientHandling->doImageFileLecture();

    //! wait lecture finished
    do{
        this->algorithmTestPlatform->setSystemStatus("unfinished");
    }while(!this->patientHandling->readFinished());

    //this->algorithmTestPlatform->setSystemStatus("finished");

    this->updatePatientPersonelInformation();
    this->displayPatientMRAImage();
    this->updatePatientMRAImageStatistics();
    this->updatePatientMRAImageHistogram();
    this->initialRendering();

    this->loadVesselsExisted();
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::loadVesselsExisted
//!
void SurgeryPlanWindow::loadVesselsExisted(){
    QString path = this->patientHandling->getCenterLineFolderPath();
    QDir recoredDir(path);
    QStringList allFiles = recoredDir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);//(QDir::Filter::Files,QDir::SortFlag::NoSort)

    if(allFiles.size()>0){
        for(int i=0 ; i<allFiles.size(); i++){
            QTreeWidgetItem *vesselItem = new QTreeWidgetItem(vesselsFolder,QStringList(QString(allFiles.at(i))));
            vesselItem->setIcon(0, *this->fileUnloadedIcon);
            vesselItem->setFlags(vesselItem->flags()| Qt::ItemIsUserCheckable);
            vesselItem->setCheckState(0, Qt::Unchecked);
        }
    }
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::displayCenterLine
//!
void SurgeryPlanWindow::displayCenterLine(){

    int pointCount = this->patientHandling->getCenterLinePointsCount();

    if(pointCount > 0){
        poly->GetPointIds()->SetNumberOfIds(pointCount);

        for(int i = 0; i < pointCount; i++){
            poly->GetPointIds()->SetId(i,i);
        }

        this->grid->SetPoints(this->patientHandling->getCenterLinePoints());
        this->grid->InsertNextCell(poly->GetCellType(),poly->GetPointIds());
        this->mapperreferencePath->SetInputData(grid);
        this->actorreferencePath->SetMapper(mapperreferencePath);
        this->actorreferencePath->GetProperty()->SetColor(1,0,0);
        this->actorreferencePath->GetProperty()->SetOpacity(0.3);
        this->actorreferencePath->GetProperty()->SetPointSize(0.89);
        this->renderer->AddActor(actorreferencePath);
    }
    else{
        this->algorithmTestPlatform->setSystemStatus("no centerline folder found");
    }
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::updatePatientMRAImage
//!
void SurgeryPlanWindow::displayPatientMRAImage(){
    display(this->patientHandling->getMraImageToBeDisplayed());
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::display
//! \param imgToBeDisplayed
//!
void SurgeryPlanWindow::display(vtkImageData *imgToBeDisplayed){

    //!---------------------------------------------------------------
    //! volume data visualization
    //!---------------------------------------------------------------
    //fixedPointVolumeRayCastMapper
    //this->volumeMapper->SetVolumeRayCastFunction(compositeFunction);
    this->volumeMapper->SetInputData(imgToBeDisplayed);
    this->volumeMapper->SetBlendModeToMaximumIntensity();
    this->volume->SetMapper(volumeMapper) ;
    this->volume->SetProperty(volumeProperty);

    this->renderer->AddVolume(volume);
    this->renderer->SetBackground(58.0/255, 89.0/255, 92.0/255);

    this->renderWindow->AddRenderer(renderer);

    this->patientMRAImage->SetRenderWindow(renderWindow);

    QString humaBodyDataPath = this->systemDispatcher->getImageCenterPath()+"human_body.obj";
    vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();
    reader->SetFileName(humaBodyDataPath.toLocal8Bit().data());
    reader->Update();
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(reader->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    vtkOrientationMarkerWidget* axes = vtkOrientationMarkerWidget::New();
    axes->SetOrientationMarker(actor);
    vtkRenderWindowInteractor *iren = patientMRAImage->GetRenderWindow()->GetInteractor();
    axes->SetDefaultRenderer(this->renderer);
    axes->SetInteractor(iren);
    axes->EnabledOn();
    axes->InteractiveOn();

    this->renderer->ResetCamera();
    this->renderWindow->Render();
    this->patientMRAImage->update();
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::setPatientHandling
//! \param patientHandling
//!
void SurgeryPlanWindow::setPatientHandling(Patient *patientHandling){
    this->patientHandling = patientHandling;
    this->curveReformationWindow->setPatientHandling(patientHandling);
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::displayWindow
//!
void SurgeryPlanWindow::displayWindow(){
    this->move(this->x, this->y);
    this->resize(this->appWidth, this->appHeight);
    this->show();
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::displaySize
//!
void SurgeryPlanWindow::displaySize(){
    this->show();
    this->resize(appWidth, appHeight);
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::drawBackground
//!
void SurgeryPlanWindow::drawBackground(){
    pixmap = new QPixmap(":/images/background_darkBlue.png");
    QPalette p =  this->palette();

    p.setBrush(QPalette::Background, QBrush(pixmap->scaled(QSize(appWidth, appHeight), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));

    this->setPalette(p);
    this->setMask(pixmap->mask());
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::setConnections
//!
void SurgeryPlanWindow::setConnections(){

    // switch the volume rendering options
    this->connect(opacityTransferChoice, SIGNAL(toggled(bool)), this, SLOT(opacityTransformationStateChanged(bool)));
    this->connect(colorTransferChoice, SIGNAL(toggled(bool)), this, SLOT(colorTransformationStateChanged(bool)));
    this->connect(gradientTransferChoice, SIGNAL(toggled(bool)), this, SLOT(gradientTransformationStateChanged(bool)));

    // actions of the mouse's pointer on the plotting board
    this->connect(transformationPlottingBoard, SIGNAL(mouseDoubleClick(QMouseEvent*)), this, SLOT(generateNewTransformationPoint(QMouseEvent*)));
    this->connect(transformationPlottingBoard, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(transformPointTracking(QMouseEvent*)));
    this->connect(transformationPlottingBoard, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(lockTargetPoint(QMouseEvent*)));
    this->connect(transformationPlottingBoard, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(positioningTransformPoint(QMouseEvent*)));

    // ....
    this->connect(this->originalOption, SIGNAL(mouseHover()), this, SLOT(originalOptionHovered()));
    this->connect(this->originalOption, SIGNAL(mouseLeftButtonClicked()), this, SLOT(originalOptionClicked()));
    this->connect(this->originalOption, SIGNAL(mouseLeftButtonReleased()), this, SLOT(originalOptionReleased()));
    this->connect(this->originalOption, SIGNAL(mouseLeaved()), this, SLOT(originalOptionLeaved()));

    this->connect(this->transparentBrainOption, SIGNAL(mouseHover()), this, SLOT(transparentBrainOptionHovered()));
    this->connect(this->transparentBrainOption, SIGNAL(mouseLeftButtonClicked()), this, SLOT(transparentBrainOptionClicked()));
    this->connect(this->transparentBrainOption, SIGNAL(mouseLeftButtonReleased()), this, SLOT(transparentBrainOptionReleased()));
    this->connect(this->transparentBrainOption, SIGNAL(mouseLeaved()), this, SLOT(transparentBrainOptionLeaved()));

    this->connect(this->greyMatterOption, SIGNAL(mouseHover()), this, SLOT(greyMatterOptionHovered()));
    this->connect(this->greyMatterOption, SIGNAL(mouseLeftButtonClicked()), this, SLOT(greyMatterOptionClicked()));
    this->connect(this->greyMatterOption, SIGNAL(mouseLeftButtonReleased()), this, SLOT(greyMatterOptionReleased()));
    this->connect(this->greyMatterOption, SIGNAL(mouseLeaved()), this, SLOT(greyMatterOptionLeaved()));

    this->connect(this->whiteMatterOption, SIGNAL(mouseHover()), this, SLOT(whiteMatterOptionHovered()));
    this->connect(this->whiteMatterOption, SIGNAL(mouseLeftButtonClicked()), this, SLOT(whiteMatterOptionClicked()));
    this->connect(this->whiteMatterOption, SIGNAL(mouseLeftButtonReleased()), this, SLOT(whiteMatterOptionReleased()));
    this->connect(this->whiteMatterOption, SIGNAL(mouseLeaved()), this, SLOT(whiteMatterOptionLeaved()));

    this->connect(this->vesselOption, SIGNAL(mouseHover()), this, SLOT(vesselOptionHovered()));
    this->connect(this->vesselOption, SIGNAL(mouseLeftButtonClicked()), this, SLOT(vesselOptionClicked()));
    this->connect(this->vesselOption, SIGNAL(mouseLeftButtonReleased()), this, SLOT(vesselOptionReleased()));
    this->connect(this->vesselOption, SIGNAL(mouseLeaved()), this, SLOT(vesselOptionLeaved()));

    this->connect(this->interventionalRouteOption, SIGNAL(mouseHover()), this, SLOT(interventionalRouteOptionHovered()));
    this->connect(this->interventionalRouteOption, SIGNAL(mouseLeftButtonClicked()), this, SLOT(interventionalRouteOptionClicked()));
    this->connect(this->interventionalRouteOption, SIGNAL(mouseLeftButtonReleased()), this, SLOT(interventionalRouteOptionReleased()));
    this->connect(this->interventionalRouteOption, SIGNAL(mouseLeaved()), this, SLOT(interventionalRouteOptionLeaved()));

    this->connect(this->imageConfigurationButton, SIGNAL(mouseLeftButtonClicked()), this, SLOT(displayConfigurationBoard()));
    //this->connect(this->sugeryEndnessButton, SIGNAL(clicked()), this, SLOT(stopSurgery()));
    this->connect(this->curveReformationButton,SIGNAL(mouseLeftButtonClicked()),this,SLOT(displayCurveReformatwionWindow()));
    this->connect(this->quitSurgeryPlanButton, SIGNAL(clicked()), this, SLOT(closeSurgeryPlanWindow()));
    this->connect(timer, SIGNAL(timeout()), this, SLOT(showTime()));
    this->connect(centerlineTreeWidget, SIGNAL(customContextMenuRequested(const QPoint&)),this, SLOT(showContextMenu(const QPoint &)));
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::showContextMenu
//! \param pos
//!
void SurgeryPlanWindow::showContextMenu(const QPoint &pos){

    QTreeWidgetItem* vesselItem = centerlineTreeWidget->itemAt(pos);

    if(vesselItem == NULL){
        return;
    }

    vesselHandlingName = vesselItem->text(0);
    if(vesselHandlingName == "centerlines"){

    }
    else if(vesselHandlingName.contains("reference")||vesselHandlingName.contains("vessel")){
        QMenu* vesselsManipulationOptions = new QMenu(centerlineTreeWidget);
        QAction* loadAction = vesselsManipulationOptions->addAction("load");
        QAction* unloadAction =  vesselsManipulationOptions->addAction("unload");
        QAction* deleteAction =  vesselsManipulationOptions->addAction("delete");

        connect(loadAction,SIGNAL(triggered()),this,SLOT(loadVesselAction()));
        connect(unloadAction,SIGNAL(triggered()),this,SLOT(unloadVesselAction()));
        connect(deleteAction,SIGNAL(triggered()),this,SLOT(deleteVesselAction()));

        vesselsManipulationOptions->exec(QCursor::pos());
    }
    else{

    }

}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::loadVesselAction
//!
void SurgeryPlanWindow::loadVesselAction(){
    //vesselHandlingName
    patientHandling->loadVesselByPath(vesselHandlingName);
   this->displayVesselse();
   this->displayCpr();
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::unloadVesselAction
//!
void SurgeryPlanWindow::unloadVesselAction(){
    //qDebug()<<"u failure load file";

}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief deleteVesselAction
//!
void SurgeryPlanWindow::deleteVesselAction(){

}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::stopSurgery
//!
void SurgeryPlanWindow::stopSurgery(){
    this->close();
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::updateCoords
//! \param obj
//!
void SurgeryPlanWindow::updateCoords(vtkObject* obj){
  // get interactor
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
  // get event position
  int event_pos[2];
  iren->GetEventPosition(event_pos);
  // update label
  QString str;
  str.sprintf("x=%d : y=%d", event_pos[0], event_pos[1]);
  qDebug()<<str;
  //coord->setText(str);
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::updatePatientMRAImageStatistics
//!
void SurgeryPlanWindow::updatePatientMRAImageStatistics(){

     QStringList statisticsList = patientHandling->getMriStatisticsList();

     grayscaleValueNumberLineEdit->setText(QString::number(patientHandling->getMraImageToBeDisplayed()->GetNumberOfPoints()));
     grayscaleValueMaximumValueLineEdit->setText(statisticsList[0]);
     grayscaleValueMinimumValueLineEdit->setText(statisticsList[1]);
     grayscaleValueMeanLineEdit->setText(statisticsList[2]);
     grayscaleValueMedianLineEdit->setText(statisticsList[3]);
     grayscaleValueStandardDeviationLineEdit->setText(statisticsList[4]);

}


//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::initialRendering
//!
void SurgeryPlanWindow::initialRendering(){

    int max = this->patientHandling->getMaximumGrayscaleValue();
    int min = this->patientHandling->getMinimumGrayscaleValue();
    int interval = max - min;

    //! doit etre calibrer selons les differentes valeurs maximaux
    this->opacityTransferChoice->setChecked(true);
    this->generateNewOpacityPoint(min ,               0.0);
    if(max  != 1){
        this->generateNewOpacityPoint(min + 0.3*interval, 0.0);
        this->generateNewOpacityPoint(min + 0.6*interval, 1.8);
    }
    this->generateNewOpacityPoint(max,                1.8);

    this->colorTransferChoice->setChecked(true);
    this->generateInitColorPoints(min,                4);
    this->generateInitColorPoints(max,                4);

    /*this->generateInitColorPoints(min,                4);
    this->generateInitColorPoints(min + 0.3*interval, 1);
    this->generateInitColorPoints(min + 0.5*interval, 2);
    this->generateInitColorPoints(min + 0.7*interval, 3);
    this->generateInitColorPoints(max,                4);*/

    this->gradientTransferChoice->setChecked(true);
    this->generateNewGradientPoint(min,                2.0);
    if(max  != 1){
        this->generateNewGradientPoint(min + 0.4*interval, 2.0);
        this->generateNewGradientPoint(min + 0.6*interval, 0.9);
        this->generateNewGradientPoint(min + 0.8*interval, 0.73);
    }
    this->generateNewGradientPoint(max,                0.1);
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::transformationStateChanged
//!
void SurgeryPlanWindow::opacityTransformationStateChanged(bool state){
    transferOptionStates.opacityTransferOptionChoosen = state;
    if(transferOptionStates.opacityTransferOptionChoosen){
        this->updatePatientMRAImageTransformationCurves();
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::ColorTransformationStateChanged
//! \param state
//!
void SurgeryPlanWindow::colorTransformationStateChanged(bool state){
    transferOptionStates.colorTransferOptionChoosen = state;
    if(transferOptionStates.colorTransferOptionChoosen){
        this->updatePatientMRAImageTransformationCurves();
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::GradientTransformationStateChanged
//! \param state
//!
void SurgeryPlanWindow::gradientTransformationStateChanged(bool state){
    transferOptionStates.gradientTransferOptionChoosen = state;
    if(transferOptionStates.gradientTransferOptionChoosen){
        this->updatePatientMRAImageTransformationCurves();
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::process
//!
void SurgeryPlanWindow::process(){

}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::updatePatientPersonelInformation
//!
void SurgeryPlanWindow::updatePatientPersonelInformation(){

}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::updatePatientPersonelPhoto
//!
void SurgeryPlanWindow::updatePatientPersonelPhoto(){
    /*
    QString p = patientHandling->getPersonnelInfoPath() + patientHandling->getName() + ".png";
    photoWidget->setPixmap(QPixmap(p));
    photoWidget->setAutoFillBackground(true);
    photoWidget->setScaledContents(true);
    photoWidget->update();*/
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::updatePatientMRAImageHistogram
//!
void SurgeryPlanWindow::updatePatientMRAImageHistogram(){

    QVector<HistogramPoint*> frequencies = patientHandling->getMriHistogramfrequencies();

    int index = histogramPlottingBoard->addCurve("Histogram", "grayscale value", "", "cyan", 3);
    histogramPlottingBoard->doHistogramPlotting(index,frequencies);

}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::opacityPointTracking
//!
void SurgeryPlanWindow::opacityPointTracking(){

    //!---------
    if(!this->transformationPlottingBoard_Clicked){
            //! -------------
            int index = this->patientHandling->findPointInTolerentArea(this->transformationPlottingBoard_AbscissaValue, this->transformationPlottingBoard_OrdinateValue, "opacity");
            if(index  == -1){
                transformationPlottingBoard->setCursor(Qt::ArrowCursor);
            }
            else if(index>=0){
                transformationPlottingBoard->setCursor(Qt::PointingHandCursor);
                currentGrayScaleValueLineEdit->setText(QString::number(this->patientHandling->getGrayScaleValueByIndex(index, "opacity")));
                currentTransformationValueLineEdit->setText(QString::number(this->patientHandling->getOpacityValueByIndex(index)));
                currentTransformationValueLabel->setText("opacity: ");
            }
    }
    else{
            //! ---------------
            if(lockingTransformationPointIndex >= 0){
                this->transformationPlottingBoard->setCursor(Qt::PointingHandCursor);
                this->patientHandling->updateOpacityPoint(lockingTransformationPointIndex, int(this->transformationPlottingBoard_AbscissaValue),  this->transformationPlottingBoard_OrdinateValue);
                this->transformationPlottingBoard->doTransformationPlotting(0,this->patientHandling->getOpacityTransferPoints());
                this->updatePatientMRAImage();
            }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::colorPointTracking
//!
void SurgeryPlanWindow::colorPointTracking(){

    //!---------
    if(!this->transformationPlottingBoard_Clicked){
        //! In grayscale -> color mode choosen, while the mouse's pointer moving through the plotting board, fetch the current color while the mouse's  pointer over a color point
        int index = this->patientHandling->findPointInTolerentArea(this->transformationPlottingBoard_AbscissaValue, this->transformationPlottingBoard_OrdinateValue, "color");
        if(index  == -1){
            transformationPlottingBoard->setCursor(Qt::ArrowCursor);
        }
        else if(index>=0){
            transformationPlottingBoard->setCursor(Qt::PointingHandCursor);
            currentGrayScaleValueLineEdit->setText(QString::number(this->patientHandling->getGrayScaleValueByIndex(index, "color")));
            currentTransformationValueLineEdit->setText("("+QString::number(this->patientHandling->getRedValueByIndex(index)) +
                                                        ", " + QString::number(this->patientHandling->getGreenValueByIndex(index)) +
                                                        ", " + QString::number(this->patientHandling->getBlueValueByIndex(index)) + ")");

            currentTransformationValueLineEdit->setStyleSheet("background-color: rgb("+
                                                              QString::number(this->patientHandling->getRedValueByIndex(index)*255) +
                                                              ", " + QString::number(this->patientHandling->getGreenValueByIndex(index)*255) +
                                                              ", " + QString::number(this->patientHandling->getBlueValueByIndex(index)*255) + ")"
                                                              ";color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px;");
        }
    }
    else{
        //qDebug()<<lockingTransformationPointIndex;
        if(lockingTransformationPointIndex >= 0){
            this->transformationPlottingBoard->setCursor(Qt::PointingHandCursor);
            this->patientHandling->updateColorPoint(lockingTransformationPointIndex, int(this->transformationPlottingBoard_AbscissaValue));
            this->transformationPlottingBoard->doColorTransformationPlotting(this->patientHandling->getColorTransferPoints());
            this->updatePatientMRAImage();
        }
    }

}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::gradientPointTracking
//!
void SurgeryPlanWindow::gradientPointTracking(){

    //!---------
    if(!this->transformationPlottingBoard_Clicked){
        int index = this->patientHandling->findPointInTolerentArea(this->transformationPlottingBoard_AbscissaValue, this->transformationPlottingBoard_OrdinateValue, "gradient");
        if(index  == -1){
            transformationPlottingBoard->setCursor(Qt::ArrowCursor);
        }
        else if(index>=0){
            transformationPlottingBoard->setCursor(Qt::PointingHandCursor);
            currentGrayScaleValueLineEdit->setText(QString::number(this->patientHandling->getGrayScaleValueByIndex(index, "gradient")));
            currentTransformationValueLineEdit->setText(QString::number(this->patientHandling->getGradientValueByIndex(index)));
            currentTransformationValueLabel->setText("gradient: ");
        }
    }
    else{

        if(lockingTransformationPointIndex >= 0){
            this->transformationPlottingBoard->setCursor(Qt::PointingHandCursor);
            this->patientHandling->updateGradientPoint(lockingTransformationPointIndex, int(this->transformationPlottingBoard_AbscissaValue), this->transformationPlottingBoard_OrdinateValue);
            //this->transformationPlottingBoard->clearGraphs();
            this->transformationPlottingBoard->doTransformationPlotting(0,this->patientHandling->getGradientTransferPoints());
            this->updatePatientMRAImage();
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::tranformPointTracking
//!
void SurgeryPlanWindow::transformPointTracking(QMouseEvent*event){

    this->transformationPlottingBoard_AbscissaValue = transformationPlottingBoard->xAxis->pixelToCoord(event->pos().x());
    this->transformationPlottingBoard_OrdinateValue = transformationPlottingBoard->yAxis->pixelToCoord(event->pos().y());
    if(transferOptionStates.opacityTransferOptionChoosen){
       this->opacityPointTracking();
    }
    else if(transferOptionStates.colorTransferOptionChoosen){
       this->colorPointTracking();
    }
    else{
       this->gradientPointTracking();
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::lockTargetPoint
//! \param event
//!
void SurgeryPlanWindow::lockTargetPoint(QMouseEvent* event){


    this->transformationPlottingBoard_AbscissaValue = transformationPlottingBoard->xAxis->pixelToCoord(event->pos().x());
    this->transformationPlottingBoard_OrdinateValue = transformationPlottingBoard->yAxis->pixelToCoord(event->pos().y());
    if(event->button() == Qt::LeftButton) {
        this->transformationPlottingBoard_Clicked = true;
        if(transferOptionStates.opacityTransferOptionChoosen){
            lockingTransformationPointIndex = this->patientHandling->findPointInTolerentArea(this->transformationPlottingBoard_AbscissaValue, this->transformationPlottingBoard_OrdinateValue,  "opacity");
        }
        else if(transferOptionStates.colorTransferOptionChoosen){
            lockingTransformationPointIndex = this->patientHandling->findPointInTolerentArea(this->transformationPlottingBoard_AbscissaValue, this->transformationPlottingBoard_OrdinateValue,  "color");
        }
        else{
            lockingTransformationPointIndex = this->patientHandling->findPointInTolerentArea(this->transformationPlottingBoard_AbscissaValue,  this->transformationPlottingBoard_OrdinateValue, "gradient");
        }
    }
    else{

    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::positioningTransformPoint
//! \param event
//!
void SurgeryPlanWindow::positioningTransformPoint(QMouseEvent* event){
    if(event->button() == Qt::LeftButton) {
        this->transformationPlottingBoard_AbscissaValue = transformationPlottingBoard->xAxis->pixelToCoord(event->pos().x());
        this->transformationPlottingBoard_OrdinateValue = transformationPlottingBoard->yAxis->pixelToCoord(event->pos().y());
        this->transformationPlottingBoard_Clicked = false;
        this->lockingTransformationPointIndex = -1;
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::generateNewTransformationPoint
//! \param event
//!
void SurgeryPlanWindow::generateNewTransformationPoint(QMouseEvent* event){
    this->transformationPlottingBoard_AbscissaValue = transformationPlottingBoard->xAxis->pixelToCoord(event->pos().x());
    this->transformationPlottingBoard_OrdinateValue = transformationPlottingBoard->yAxis->pixelToCoord(event->pos().y());

    if(event->button() == Qt::LeftButton) {
        if(transferOptionStates.opacityTransferOptionChoosen){
            this->generateNewOpacityPoint(this->transformationPlottingBoard_AbscissaValue,  this->transformationPlottingBoard_OrdinateValue);
        }
        else if(transferOptionStates.colorTransferOptionChoosen){
            this->generateNewColorPoints(this->transformationPlottingBoard_AbscissaValue,  this->transformationPlottingBoard_OrdinateValue);
        }
        else{
            this->generateNewGradientPoint(this->transformationPlottingBoard_AbscissaValue,  this->transformationPlottingBoard_OrdinateValue);
        }
    }
    else{

    }

}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::generateNewOpacityPoint
//! \param abscissa
//! \param ordinate
//!
void SurgeryPlanWindow::generateNewOpacityPoint(double abscissa, double ordinate){

    TransferPoint*opacityPoint = new TransferPoint();
    opacityPoint->setAbscissaValue(abscissa);
    opacityPoint->setOrdinateValue(ordinate);
    this->patientHandling->setOpacityTransferPoint(opacityPoint);
    if(transferOptionStates.opacityTransferOptionChoosen){
        this->transformationPlottingBoard->doTransformationPlotting(0,this->patientHandling->getOpacityTransferPoints());
    }
    this->updatePatientMRAImage();
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::generateNewColorPoints
//! \param abscissa
//! \param ordinate
//!
void SurgeryPlanWindow::generateNewColorPoints(double abscissa, double ordinate){
    ColorPoint * colorPoint =  new ColorPoint();

    //TODO: display a widget for color chosen
    colorChooseWindow->exec();

    if(colorChooseWindow->colorChosen()){
        colorPoint->setAbscissaValue(abscissa);

        colorPoint->setRedValue(1.0*colorChooseWindow->getRedValue()/255);
        colorPoint->setGreenValue(1.0*colorChooseWindow->getGreenValue()/255);
        colorPoint->setBlueValue(1.0*colorChooseWindow->getBlueValue()/255);
        this->patientHandling->setColorTransferPoint(colorPoint);
        if(transferOptionStates.colorTransferOptionChoosen){
            this->transformationPlottingBoard->doColorTransformationPlotting(this->patientHandling->getColorTransferPoints());
        }
        this->updatePatientMRAImage();
    }

}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::generateNewGradientPoint
//! \param abscissa
//! \param ordinate
//!
void SurgeryPlanWindow::generateNewGradientPoint(double abscissa, double ordinate){
    TransferPoint*opacityPoint = new TransferPoint();
    opacityPoint->setAbscissaValue(abscissa);
    opacityPoint->setOrdinateValue(ordinate);
    this->patientHandling->setGradientTransferPoint(opacityPoint);
     if(transferOptionStates.gradientTransferOptionChoosen){
        this->transformationPlottingBoard->doTransformationPlotting(0,this->patientHandling->getGradientTransferPoints());
     }
    this->updatePatientMRAImage();
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief generateNewColorPoint
//! \param abscissa
//! \param ordinate
//! \param color
//!
void SurgeryPlanWindow::generateInitColorPoints(double abscissa, int colorCount){
    QColor *color =  new QColor(255, 255, 255);
    //QColor *color =  new QColor(255, colorCount*51, 0);
    ColorPoint *colorPoint = new ColorPoint();
    colorPoint->setAbscissaValue(abscissa);
    colorPoint->setBlueValue(1.0*color->blue()/255);
    colorPoint->setRedValue(1.0*color->red()/255);
    colorPoint->setGreenValue(1.0*color->green()/255);
    this->patientHandling->setColorTransferPoint(colorPoint);
    if(transferOptionStates.colorTransferOptionChoosen){
        this->transformationPlottingBoard->doColorTransformationPlotting(this->patientHandling->getColorTransferPoints());
    }
   this->updatePatientMRAImage();
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::updatePatientMRAImage
//!
void SurgeryPlanWindow::updatePatientMRAImage(){
    if(transferOptionStates.opacityTransferOptionChoosen){
        this->opacityTransferFunction->RemoveAllPoints();
        QVector<TransferPoint*> points = this->patientHandling->getOpacityTransferPoints();
        for(unsigned char cpt = 0; cpt < points.size(); cpt++){
            this->opacityTransferFunction->AddPoint(int(points[cpt]->getAbscissaValue()), points[cpt]->getOrdinateValue());
        }
        this->volumeProperty->SetScalarOpacity(opacityTransferFunction);
    }
    else if(transferOptionStates.colorTransferOptionChoosen){
        this->colorTransferFunction->RemoveAllPoints();
        QVector<ColorPoint*> points = this->patientHandling->getColorTransferPoints();
        for(unsigned char cpt = 0; cpt < points.size(); cpt++){
            this->colorTransferFunction->AddRGBPoint(int(points[cpt]->getAbscissaValue()), points[cpt]->getRedValue(), points[cpt]->getGreenValue(), points[cpt]->getBlueValue());
        }
        volumeProperty->SetColor(colorTransferFunction);
    }
    else{
        this->gradientTransferFunction->RemoveAllPoints();
        QVector<TransferPoint*> points = this->patientHandling->getGradientTransferPoints();
        for(unsigned char cpt = 0; cpt < points.size(); cpt++){
            this->gradientTransferFunction->AddPoint(int(points[cpt]->getAbscissaValue()), points[cpt]->getOrdinateValue());
        }
        this->volumeProperty->SetGradientOpacity(gradientTransferFunction);
    }
    //this->volume->SetProperty(volumeProperty); modified 2015/11/25
    this->patientMRAImage->update();
}



//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::updatePatientMRAImageTransformationCurves
//!
void SurgeryPlanWindow::updatePatientMRAImageTransformationCurves(){
    if(transformationPlottingBoard->getCurveNumber()>0){
        transformationPlottingBoard->clearGraphs();
    }

    transformationPlottingBoard->setAbscissaRange(this->patientHandling->getMriStatisticsList()[1].toInt(0,10),
                                                  this->patientHandling->getMriStatisticsList()[0].toInt(0,10)*1.1 );

    if(transferOptionStates.opacityTransferOptionChoosen){
        currentTransformationValueLabel->setText("Opacity:  ");
        currentTransformationValueLineEdit->setStyleSheet("QLineEdit {background-color:transparent; color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue;}");
        transformationPlottingBoard->setOrdinateRange(-0.5, 2.5);
        transformationPlottingBoard->legendSetting(false);
        transferCurveIndex.opacityTransferCurveIndex = transformationPlottingBoard->addCurve("grayValue->opacity", "grayscale value", "", "gainsboro", 7);
        this->transformationPlottingBoard->doTransformationPlotting(0,this->patientHandling->getOpacityTransferPoints());
    }
    else if(transferOptionStates.colorTransferOptionChoosen){
        currentTransformationValueLabel->setText("Color:    ");
        transformationPlottingBoard->setOrdinateRange(-0.5, 2.5);
         transformationPlottingBoard->legendSetting(true);
        transferCurveIndex.colorTransferCurveIndex = transformationPlottingBoard->addCurve("grayValue->color", "grayscale value", "", "gainsboro", 7);
        this->transformationPlottingBoard->doColorTransformationPlotting(this->patientHandling->getColorTransferPoints());
    }
    else{
        currentTransformationValueLabel->setText("Gradient: ");
        currentTransformationValueLineEdit->setStyleSheet("QLineEdit {background-color:transparent; color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue;}");
        transformationPlottingBoard->legendSetting(false);
        transferCurveIndex.gradientTransferCurveIndex = transformationPlottingBoard->addCurve("grayValue->gradient", "grayscale value", "", "gainsboro", 7);
        this->transformationPlottingBoard->doTransformationPlotting(0,this->patientHandling->getGradientTransferPoints());
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief Patient::removeCurveBy
//! \param index
//!
void SurgeryPlanWindow::removeCurveBy(int index){
    transformationPlottingBoard->removeGraph(index);
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::constructControlBar
//!
void SurgeryPlanWindow::constructControlBar(){
    controlBar = new QFrame();
    controlBar->setFixedHeight(int(appHeight*0.025));
    //controlBar->setStyleSheet("background-color: AliceBlue");

    controlBarLayout = new QHBoxLayout(controlBar);

    imageConfigurationButton = new CPushButton();
    imageConfigurationButton->setIcon(QIcon(":/images/image_configuration.png"));
    imageConfigurationButton->setFixedSize(QSize(int(appHeight*0.025), int(appHeight*0.025)));
    imageConfigurationButton->setIconSize(QSize(int(appHeight*0.025), int(appHeight*0.025)));
    imageConfigurationButton->setFlat(true);

    imageUpdateButton = new CPushButton();
    imageUpdateButton->setIcon(QIcon(":/images/image_update.png"));
    imageUpdateButton->setFixedSize(QSize(int(appHeight*0.025), int(appHeight*0.025)));
    imageUpdateButton->setIconSize(QSize(int(appHeight*0.025), int(appHeight*0.025)));
    imageUpdateButton->setFlat(true);

    curveReformationButton = new CPushButton();
    curveReformationButton->setIcon(QIcon(":/images/lala.png"));
    curveReformationButton->setFixedSize(QSize(int(appHeight*0.025),int(appHeight*0.025)));
    curveReformationButton->setIconSize(QSize(int(appHeight*0.025),int(appHeight*0.025)));
    curveReformationButton->setFlat(true);

    beginTherapyButton = new CPushButton();
    beginTherapyButton->setIcon(QIcon(":/images/begin_therapy.png"));
    beginTherapyButton->setFixedSize(QSize(int(appHeight*0.025), int(appHeight*0.025)));
    beginTherapyButton->setIconSize(QSize(int(appHeight*0.025), int(appHeight*0.025)));
    beginTherapyButton->setFlat(true);
    spacer_controlbar = new QSpacerItem(int(appHeight*0.025), 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    controlBarLayout->addWidget(imageConfigurationButton);
    controlBarLayout->addWidget(imageUpdateButton);
    controlBarLayout->addWidget(curveReformationButton);
    controlBarLayout->addItem(spacer_controlbar);
    controlBarLayout->addWidget(beginTherapyButton);
    controlBarLayout->setSpacing(0);
    controlBarLayout->setMargin(0);
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! Initialisation des composant necessaire pour l'affichage des images tridimensionelle
//!
//! \brief SurgeryPlanWindow::initialisation
//!
void SurgeryPlanWindow::initialisation(){
    //fixedPointVolumeRayCastMapper = vtkFixedPointVolumeRayCastMapper::New();
    //! for volume data visualization
    volumeMapper = vtkFixedPointVolumeRayCastMapper::New();
    compositeFunction = vtkVolumeRayCastCompositeFunction::New();
    volume = vtkVolume::New();
    renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderer = vtkSmartPointer<vtkRenderer>::New();

    opacityTransferFunction=vtkPiecewiseFunction::New();
    gradientTransferFunction = vtkPiecewiseFunction::New();
    colorTransferFunction = vtkColorTransferFunction::New();

    volumeProperty=vtkVolumeProperty::New();
    volumeProperty->ShadeOff();
    volumeProperty->SetInterpolationTypeToLinear();

    //! for centerline visualization
    actorreferencePath = vtkActor::New();
    mapperreferencePath = vtkDataSetMapper::New();
    grid = vtkUnstructuredGrid::New();
    poly = vtkPolyVertex::New();

    imageOptionStates.originalOptionState = false;
    imageOptionStates.transparentBrainOptionState = false;
    imageOptionStates.greyMatterOptionState = false;
    imageOptionStates.whiteMatterOptionState = false;
    imageOptionStates.vesselOptionState = false;
    imageOptionStates.interventionalRouteOptionState = false;

    transformationPlottingBoard_Clicked = false;
    lockingTransformationPointIndex = -1;

    this->transformationPlottingBoard_AbscissaValue = 0.0;
    this->transformationPlottingBoard_OrdinateValue = 0.0;

    this->patientWidgetConfigurationBoard = new PatientWidgetConfigurationBoard(this->patientHandling);
    this->colorChooseWindow = new ColorChooseWindow();

    this->curveReformationWindow = new CurveReformationWindow(this->rect.width(), this->rect.height());
    this->timer = new QTimer(this);
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! Construct the widget which contains all the information about the patient in order to prepare the therapy
//!
//! \brief SurgeryPlanWindow::constructPatientInformationWidget
//!
void SurgeryPlanWindow::constructPatientInformationWidget(){


    this->elapsedTimeLabel = new QLCDNumber();
    this->elapsedTimeLabel->setStyleSheet("background-color: transparent; color: Gainsboro");
    this->elapsedTimeLabel->setDigitCount(8);
    this->elapsedTimeLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));

    this->surgeryPlanWindowLabel = new QLabel();
    this->surgeryPlanWindowLabel->setText("Surgery Plan");
    this->surgeryPlanWindowLabel->setStyleSheet("QLabel{background-color: transparent; color: Gainsboro}");
    this->surgeryPlanWindowLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    this->surgeryPlanWindowLabel->setAlignment(Qt::AlignLeft);

    patientImageInformationWidget = new QWidget();
    patientImageInformationWidget->setFixedHeight(int(appHeight*0.03));
    patientImageInformationWidgetLayout = new QHBoxLayout(patientImageInformationWidget);
    patientImageInformationWidgetLayout->addWidget(surgeryPlanWindowLabel);
    //! TODO to be customerized
    //patientImageInformationWidgetLayout->addWidget(elapsedTimeLabel);
    patientImageInformationWidgetLayout->setSpacing(0);
    patientImageInformationWidgetLayout->setMargin(0);

    imageStatisticsWidget = new QWidget();
    imageStatisticsWidget->setFixedHeight(appHeight*0.15);
    imageStatisticsWidgetLayout = new QGridLayout(imageStatisticsWidget);

    grayscaleValueNumberLabel = new QLabel("Total Number: ");
    grayscaleValueMeanLabel = new QLabel("Mean: ");
    grayscaleValueMedianLabel = new QLabel("Median: ");
    grayscaleValueMaximumValueLabel = new QLabel("Maximum Value: ");
    grayscaleValueMinimumValueLabel = new QLabel("Minimum Value: ");
    grayscaleValueStandardDeviationLabel = new QLabel("Standard Deviation: ");

    grayscaleValueNumberLineEdit = new QLineEdit();
    grayscaleValueMeanLineEdit = new QLineEdit();
    grayscaleValueMedianLineEdit = new QLineEdit();
    grayscaleValueMaximumValueLineEdit = new QLineEdit();
    grayscaleValueMinimumValueLineEdit = new QLineEdit();
    grayscaleValueStandardDeviationLineEdit = new QLineEdit();

    grayscaleValueNumberLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, false));
    grayscaleValueMeanLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, false));
    grayscaleValueMedianLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, false));
    grayscaleValueMaximumValueLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, false));
    grayscaleValueMinimumValueLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, false));
    grayscaleValueStandardDeviationLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, false));

    grayscaleValueNumberLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    grayscaleValueMeanLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    grayscaleValueMedianLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    grayscaleValueMaximumValueLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    grayscaleValueMinimumValueLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    grayscaleValueStandardDeviationLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));

    grayscaleValueNumberLineEdit->setReadOnly(true);
    grayscaleValueMeanLineEdit->setReadOnly(true);
    grayscaleValueMedianLineEdit->setReadOnly(true);
    grayscaleValueMaximumValueLineEdit->setReadOnly(true);
    grayscaleValueMinimumValueLineEdit->setReadOnly(true);
    grayscaleValueStandardDeviationLineEdit->setReadOnly(true);

    grayscaleValueNumberLabel->setStyleSheet("color: AliceBlue");
    grayscaleValueMeanLabel->setStyleSheet("color: AliceBlue");
    grayscaleValueMedianLabel->setStyleSheet("color: AliceBlue");
    grayscaleValueMaximumValueLabel->setStyleSheet("color: AliceBlue");
    grayscaleValueMinimumValueLabel->setStyleSheet("color: AliceBlue");
    grayscaleValueStandardDeviationLabel->setStyleSheet("color: AliceBlue");

    grayscaleValueNumberLineEdit->setStyleSheet("QLineEdit {background-color:transparent; color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue;}");
    grayscaleValueMeanLineEdit->setStyleSheet("QLineEdit {background-color:transparent; color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue; }");
    grayscaleValueMedianLineEdit->setStyleSheet("QLineEdit {background-color:transparent; color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue; }");
    grayscaleValueMaximumValueLineEdit->setStyleSheet("QLineEdit {background-color:transparent; color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue; }");
    grayscaleValueMinimumValueLineEdit->setStyleSheet("QLineEdit {background-color:transparent; color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue; }");
    grayscaleValueStandardDeviationLineEdit->setStyleSheet("QLineEdit {background-color:transparent; color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue; }");

    imageStatisticsWidgetLayout->addWidget(grayscaleValueNumberLabel, 0, 0);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueNumberLineEdit, 0, 1);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueMaximumValueLabel, 1, 0);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueMaximumValueLineEdit, 1, 1);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueMinimumValueLabel, 2, 0);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueMinimumValueLineEdit, 2, 1);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueMeanLabel, 3, 0);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueMeanLineEdit, 3, 1);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueMedianLabel, 4, 0);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueMedianLineEdit, 4, 1);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueStandardDeviationLabel, 5, 0);
    imageStatisticsWidgetLayout->addWidget(grayscaleValueStandardDeviationLineEdit, 5, 1);
    imageStatisticsWidgetLayout->setSpacing(0);

    histogramPlottingBoard = new PlottingBoard();
    histogramPlottingBoard->setFixedHeight(appHeight*0.2);
    histogramPlottingBoard->legendSetting(true);

    histogramGroupBox = new QGroupBox();
    histogramGroupBox->setStyleSheet("QGroupBox{background-color:transparent; color: Aliceblue; border: 1px solid gray; border-radius: 0px;margin-top: 1ex;} ");
    histogramGroupBox->setTitle("Image Statistics");
    histogramGroupBox->setFont(QFont("Helvetica", 8, QFont::AnyStyle, false));
    histogramGroupBoxLayout = new QVBoxLayout(histogramGroupBox);
    histogramGroupBoxLayout->addWidget(imageStatisticsWidget);
    histogramGroupBoxLayout->addWidget(histogramPlottingBoard);

    histogramGroupBoxLayout->setSpacing(0);
    histogramGroupBoxLayout->setContentsMargins(0,15,0,0);

    //!-------------------------------------
    volumeRenderingGroupBox = new QGroupBox();
    volumeRenderingGroupBox->setStyleSheet("QGroupBox{background-color:transparent; color: Aliceblue; border: 1px solid gray; border-radius: 0px;margin-top: 1ex;} ");
    volumeRenderingGroupBox->setTitle("Volume Rendering");
    volumeRenderingGroupBox->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    volumeRenderingGroupBoxLayout = new QVBoxLayout(volumeRenderingGroupBox);

    transferChoicesWidget = new QWidget();
    transferChoicesLayout = new QHBoxLayout(transferChoicesWidget);

    transferChoiceLabel = new QLabel("transfer grayscale value to: ");
    opacityTransferChoice = new QRadioButton("opacity");
    colorTransferChoice = new QRadioButton("color");
    gradientTransferChoice = new QRadioButton("gradient");
    transferChoicesWidgetSpacer = new QSpacerItem(0,10, QSizePolicy::Expanding, QSizePolicy::Expanding);

    opacityTransferChoice->setFixedWidth(70);
    colorTransferChoice->setFixedWidth(60);
    gradientTransferChoice->setFixedWidth(70);

    transferChoiceLabel->setStyleSheet("color: AliceBlue");
    opacityTransferChoice->setStyleSheet("color: AliceBlue");
    colorTransferChoice->setStyleSheet("color: AliceBlue");
    gradientTransferChoice->setStyleSheet("color: AliceBlue");

    transferChoiceLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    opacityTransferChoice->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    colorTransferChoice->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    gradientTransferChoice->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));

    transferChoicesLayout->addWidget(transferChoiceLabel);
    transferChoicesLayout->addWidget(opacityTransferChoice);
    transferChoicesLayout->addWidget(colorTransferChoice);
    transferChoicesLayout->addWidget(gradientTransferChoice);
    transferChoicesLayout->addItem(transferChoicesWidgetSpacer);
    transferChoicesLayout->setSpacing(0);

    transformationPlottingBoard = new PlottingBoard();

    transformationIndicationBar = new QWidget();
    transformationIndicationBarLayout = new QHBoxLayout(transformationIndicationBar);
    currentGrayScaleValueLabel = new QLabel("grascale value: ");
    currentTransformationValueLabel = new QLabel("opacity: ");
    transformationButton = new CPushButton(this);

    transformationButton->setIcon(QIcon(":/images/transformation.png"));
    transformationButton->setFixedSize(QSize(60, 30));
    transformationButton->setIconSize(QSize(53, 23));
    transformationButton->setFlat(true);

    currentGrayScaleValueLineEdit = new QLineEdit();
    currentTransformationValueLineEdit = new QLineEdit();
    currentGrayScaleValueLabel->setStyleSheet("color: AliceBlue");
    currentTransformationValueLabel->setStyleSheet("color: AliceBlue");
    currentGrayScaleValueLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    currentTransformationValueLabel->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));

    grayscaleValueNumberLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    grayscaleValueMeanLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    grayscaleValueMedianLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    grayscaleValueMaximumValueLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    grayscaleValueMinimumValueLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));
    grayscaleValueStandardDeviationLineEdit->setFont(QFont("Segoe UI", 8, QFont::AnyStyle, true));

    currentGrayScaleValueLineEdit->setReadOnly(true);
    currentTransformationValueLineEdit->setReadOnly(true);
    currentGrayScaleValueLineEdit->setStyleSheet("QLineEdit {background-color:transparent; color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue;}");
    currentTransformationValueLineEdit->setStyleSheet("QLineEdit {background-color:transparent; color:AliceBlue; border: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue;}");

    transformationIndicationBarLayout->addWidget(currentGrayScaleValueLabel);
    transformationIndicationBarLayout->addWidget(currentGrayScaleValueLineEdit);
    transformationIndicationBarLayout->addWidget(transformationButton);
    transformationIndicationBarLayout->addWidget(currentTransformationValueLabel);
    transformationIndicationBarLayout->addWidget(currentTransformationValueLineEdit);
    transformationIndicationBarLayout->setSpacing(0);

    transformationPlottingBoard->setFixedHeight(appHeight*0.2);
    transformationPlottingBoard->legendSetting(false);

    volumeRenderingGroupBoxLayout->addWidget(transferChoicesWidget);
    volumeRenderingGroupBoxLayout->addWidget(transformationPlottingBoard);
    volumeRenderingGroupBoxLayout->addWidget(transformationIndicationBar);
    volumeRenderingGroupBoxLayout->setSpacing(0);

    patientMRAImageOptionWidgetSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

    this->patientMRAImageOptionWidget =  new QWidget();
    this->patientMRAImageOptionWidget->setFixedWidth(int(appWidth*0.3));
    this->patientMRAImageOptionWidgetLayout = new QVBoxLayout(patientMRAImageOptionWidget);
    this->patientMRAImageOptionWidgetLayout->addWidget(patientImageInformationWidget);
    this->patientMRAImageOptionWidgetLayout->addWidget(histogramGroupBox);
    this->patientMRAImageOptionWidgetLayout->addWidget(volumeRenderingGroupBox);
    this->patientMRAImageOptionWidgetLayout->setSpacing(0);
    this->patientMRAImageOptionWidgetLayout->setMargin(0);

    //!--------------------------------------------------------------------------------------------------------
    //! six item where you could select different image of patient's MRA SurgeryPlanWindowmage
    //!--------------------------------------------------------------------------------------------------------
    this->originalOption = new CPushButton();
    this->originalOption->setText("Original Image");
    this->originalOption->setFixedSize(QSize(150, int(appHeight*0.03)));
    this->originalOption->setFixedWidth(150);
    this->originalOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    this->originalOption->setFlat(true);
    this->originalOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));

    this->transparentBrainOption = new CPushButton();
    this->transparentBrainOption->setText("Transparent Brain");
    this->transparentBrainOption->setFixedSize(QSize(150, int(appHeight*0.03)));
    this->transparentBrainOption->setFixedWidth(150);
    this->transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    this->transparentBrainOption->setFlat(true);
    this->transparentBrainOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));

    this->greyMatterOption = new CPushButton();
    this->greyMatterOption->setText("White Matter");
    this->greyMatterOption->setFixedSize(QSize(150, int(appHeight*0.03)));
    this->greyMatterOption->setFixedWidth(150);
    this->greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    this->greyMatterOption->setFlat(true);
    this->greyMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));

    this->whiteMatterOption = new CPushButton();
    this->whiteMatterOption->setText("Grey Matter");
    this->whiteMatterOption->setFixedSize(QSize(150, int(appHeight*0.03)));
    this->whiteMatterOption->setFixedWidth(150);
    this->whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    this->whiteMatterOption->setFlat(true);
    this->whiteMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));

    this->vesselOption = new CPushButton();
    this->vesselOption->setText("Vessel Tree");
    this->vesselOption->setFixedSize(QSize(150, int(appHeight*0.03)));
    this->vesselOption->setFixedWidth(150);
    this->vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    this->vesselOption->setFlat(true);
    this->vesselOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));

    this->interventionalRouteOption = new CPushButton();
    this->interventionalRouteOption->setText("Interventional Route");
    this->interventionalRouteOption->setFixedSize(QSize(150, int(appHeight*0.03)));
    this->interventionalRouteOption->setFixedWidth(appWidth*0.16);
    this->interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    this->interventionalRouteOption->setFlat(true);
    this->interventionalRouteOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));

    this->patientMRAImageOptionLabel = new QLabel();
    //patientMRAImageOptionLabel->setFixedWidth(320);

    this->quitSurgeryPlanButton = new QPushButton("X");
    this->quitSurgeryPlanButton->setFixedSize(int(appHeight*0.03), int(appHeight*0.03));
    this->quitSurgeryPlanButton->setStyleSheet("background-color:transparent");

    //--------------------------------------------------------------------------------------------------------
    //patient MRA image  widget and its horizonal layout (the widget which is used to display MRA image)
    //--------------------------------------------------------------------------------------------------------
    this->patientMRAImageOption = new QWidget();
    this->patientMRAImageOption->setFixedHeight(int(appHeight*0.03));
    this->patientMRAImageOptionLayout = new QHBoxLayout(this->patientMRAImageOption);
    this->patientMRAImageOptionLayout->addWidget(this->originalOption);
    this->patientMRAImageOptionLayout->addWidget(this->transparentBrainOption);
    this->patientMRAImageOptionLayout->addWidget(this->greyMatterOption);
    this->patientMRAImageOptionLayout->addWidget(this->whiteMatterOption);
    this->patientMRAImageOptionLayout->addWidget(this->vesselOption);
    this->patientMRAImageOptionLayout->addWidget(this->interventionalRouteOption);
    this->patientMRAImageOptionLayout->addWidget(this->patientMRAImageOptionLabel);
    this->patientMRAImageOptionLayout->addWidget(this->quitSurgeryPlanButton);
    this->patientMRAImageOptionLayout->setSpacing(1);
    this->patientMRAImageOptionLayout->setMargin(0);

    //--------------------------------------------------------------------------------------------------------
    //patient MRA image  widget and its horizonal layout (the widget which is used to display MRA image)
    //--------------------------------------------------------------------------------------------------------
    this->patientMRAImageWidget = new QWidget();
    this->patientMRAImageWidgetLayout = new QHBoxLayout(this->patientMRAImageWidget);
    this->patientMRAImage = new QVTKWidget();

    //patientMRAImageManipulation = new QWidget();
    this->patientMRAImageWidgetLayout->addWidget(this->patientMRAImage);
    this->patientMRAImageWidgetLayout->setSpacing(0);
    this->patientMRAImageWidgetLayout->setMargin(0);

    //--------------------------------------------------------------------------------------------------------
    //patient MRA image configuration widget and its vertical layout
    //--------------------------------------------------------------------------------------------------------
    this->patientMRAImageConfigurationWidget =  new QWidget();
    this->patientMRAImageConfigurationWidgetLayout = new QVBoxLayout(this->patientMRAImageConfigurationWidget);
    this->patientMRAImageConfigurationWidgetLayout->addWidget(this->patientMRAImageOption);
    this->patientMRAImageConfigurationWidgetLayout->addWidget(this->patientMRAImageWidget);
    this->patientMRAImageConfigurationWidgetLayout->setSpacing(0);
    this->patientMRAImageConfigurationWidgetLayout->setMargin(0);

    //--------------------------------------------------------------------------------------------------------
    //patient clinical widgets container(the bigest widget) and its horizonal layout
    //--------------------------------------------------------------------------------------------------------
    this->patientClinicalWidgetsContainer = new QFrame();
    this->patientClinicalWidgetsContainerLayout = new QHBoxLayout(this->patientClinicalWidgetsContainer);
    this->patientClinicalWidgetsContainerLayout->addWidget(this->patientMRAImageOptionWidget);
    this->patientClinicalWidgetsContainerLayout->addWidget(this->patientMRAImageConfigurationWidget);
    this->patientClinicalWidgetsContainerLayout->setSpacing(2);
    this->patientClinicalWidgetsContainerLayout->setMargin(2);

    this->patientInformationWidget = new QFrame();
    this->patientInformationLayout = new QVBoxLayout(this->patientInformationWidget);
    this->patientInformationLayout->addWidget(this->patientClinicalWidgetsContainer);
    this->patientInformationLayout->setSpacing(1);
    this->patientInformationLayout->setMargin(0);
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//!  the widget which contains all the information about the patient in order to prepare the therapy
//!
//! \brief SurgeryPlanWindow::constructDoctorInformationWidget
//!
void SurgeryPlanWindow::constructCprAnalyseWidget(){
    cprAnalyseWidget =  new QFrame();
    cprAnalyseWidget->setFixedHeight(int(appHeight*0.27));
    cprAnalyseWidget->setStyleSheet("background-color:transparent; color:AliceBlue; border-top: 1px solid Gray;border-radius: 0px;padding: 0 8px; selection-background-color: darkAliceBlue");
    cprAnalyseWidgetLayout = new QHBoxLayout(cprAnalyseWidget);

    centerlineTreeWidget = new QTreeWidget();
    QString centerlineTreeWidgetStyle = "QTreeWidget{show-decoration-selected:2}" \
                                        "QTreeWidget::item {border: 1px solid #d9d9d9;border-top-color: transparent;border-bottom-color: transparent;}" \
                                        "QTreeWidget::item:hover {background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #e7effd, stop: 1 #cbdaf1),border: 1px solid #bfcde4;}" \
                                        "QTreeWidget::item:selected {border: 1px solid #567dbc;}" \
                                        "QTreeWidget::item:selected:active{background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6ea1f1, stop: 1 #567dbc)}" \
                                        "QTreeWidget::item:selected:!active {background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6b9be8, stop: 1 #577fbf)}";

    centerlineTreeWidget->setStyleSheet(centerlineTreeWidgetStyle);
    centerlineTreeWidget->setHeaderHidden(1);
    centerlineTreeWidget->setFixedSize(appWidth*0.3,appHeight*0.27);
    centerlineTreeWidget->setStyleSheet("background-color:transparent");
    centerlineTreeWidget->setColumnCount(1);
    centerlineTreeWidget->setHeaderLabel(tr("Centerline Choose"));
    centerlineTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    vesselsFolder = new QTreeWidgetItem(centerlineTreeWidget);
    vesselsFolder->setText(0, "centerlines");
    vesselsFolder->setFlags(Qt::ItemIsEnabled);
    vesselsFolder->setIcon(0, *this->defaultFolderIcon);
    vesselsFolder->setFlags(vesselsFolder->flags()| Qt::ItemIsUserCheckable);
    vesselsFolder->setCheckState(0, Qt::Unchecked);
    centerlineTreeWidget->expandAll();

    centerLineVTKWidget = new QVTKWidget();
    centerLineVTKWidget->setFixedSize(appWidth*0.2,appHeight*0.27);

    cprOutcomingVTKWidget = new QVTKWidget();
    cprOutcomingVTKWidget->setFixedSize(appWidth*0.2,appHeight*0.27);
    flyThroughWidget = new QWidget();
    flyThroughWidget->setFixedSize(appWidth*0.3,appHeight*0.27);

    cprAnalyseWidgetLayout->addWidget(centerlineTreeWidget);
    cprAnalyseWidgetLayout->addWidget(centerLineVTKWidget);
    cprAnalyseWidgetLayout->addWidget(cprOutcomingVTKWidget);
    cprAnalyseWidgetLayout->addWidget(flyThroughWidget);
    cprAnalyseWidgetLayout->setMargin(0);
    cprAnalyseWidgetLayout->setSpacing(0);
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::displayVesselse
//!
void SurgeryPlanWindow::displayVesselse(){
    singleVesselPoints = patientHandling->getVesselByName(vesselHandlingName);

    vtkPolyVertex *singleVesselPoly = vtkPolyVertex::New();
    vtkUnstructuredGrid *singleVesselgrid = vtkUnstructuredGrid::New();
    vtkDataSetMapper *singleVesselMapper =  vtkDataSetMapper::New();
    vtkActor *singleVesselactor = vtkActor::New();
    vtkRenderer *singleVesselRenderer = vtkRenderer::New();
    vtkRenderWindow *singleVesselRenderwindow =vtkRenderWindow::New();

    singleVesselPoly->GetPointIds()->SetNumberOfIds(singleVesselPoints->GetNumberOfPoints());
    for(int i = 0;i<singleVesselPoints->GetNumberOfPoints();i++){
        singleVesselPoly->GetPointIds()->SetId(i,i);
    }
    singleVesselgrid->SetPoints(singleVesselPoints);
    singleVesselgrid->InsertNextCell(singleVesselPoly->GetCellType(),singleVesselPoly->GetPointIds());
    singleVesselMapper->SetInputData(singleVesselgrid);
    singleVesselactor->SetMapper(singleVesselMapper);
    singleVesselactor->GetProperty()->SetColor(1,0,0);
    singleVesselactor->GetProperty()->SetOpacity(0.1);
    singleVesselactor->GetProperty()->SetPointSize(0.39);
    singleVesselRenderer->AddActor(singleVesselactor);
    singleVesselRenderer->SetBackground(55.0/255, 85.0/255, 95.0/255);
    singleVesselRenderwindow->AddRenderer(singleVesselRenderer);
    centerLineVTKWidget->SetRenderWindow(singleVesselRenderwindow);
    centerLineVTKWidget->update();
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::displayCpr
//! \param input
//!
void SurgeryPlanWindow::displayCpr(){
    cprmapper = vtkPolyDataMapper::New();
    cpractor = vtkActor::New();
    cprRenderer = vtkRenderer::New();
    cprRenderwindow = vtkRenderWindow::New();

    cprMath();
    cprmapper->SetInputConnection(pCut->GetOutputPort());
    cpractor->SetMapper(cprmapper);
    cprRenderer->AddActor(cpractor);
    cprRenderer->SetBackground(55.0/255, 85.0/255, 95.0/255);
    cprRenderwindow->AddRenderer(cprRenderer);
    cprOutcomingVTKWidget->SetRenderWindow(cprRenderwindow);
    cprOutcomingVTKWidget->update();
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::sfunctionSource
//!
void SurgeryPlanWindow::sfunctionSource(){
    xSpline = vtkSCurveSpline::New();
    ySpline = vtkSCurveSpline::New();
    zSpline = vtkSCurveSpline::New();
    spline = vtkParametricSpline::New();
    functionSource = vtkParametricFunctionSource::New();

    reslicer = vtkSplineDrivenImageSlicer::New();
    append = vtkImageAppend::New();
    cprmapper1 = vtkFixedPointVolumeRayCastMapper::New();
    cprcolorTranFun = vtkColorTransferFunction::New();
    cprVolumeproperty = vtkVolumeProperty::New();
    cprPieceFun = vtkPiecewiseFunction::New();
    pPlane = vtkPlane::New();
    pCut = vtkCutter::New();
    m_pShift = vtkImageShiftScale::New();
    cprvolume = vtkVolume::New();

    spline->SetXSpline(xSpline);
    spline->SetYSpline(ySpline);
    spline->SetZSpline(zSpline);
    spline->SetPoints(singleVesselPoints);
    functionSource->SetParametricFunction(spline);
    functionSource->Update();
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::cprMath
//!
void SurgeryPlanWindow::cprMath(){

    sfunctionSource();
    reslicer->SetInputData(patientHandling->getMraImageToBeDisplayed());
    reslicer->SetPathConnection(functionSource->GetOutputPort());
    reslicer->SetSliceSpacing( 0.2,0.1 );
    reslicer->SetSliceThickness( .8 );
    reslicer->SetSliceExtent(40,200 );
    reslicer->SetOffsetPoint( 30 );
    int nbPoints = functionSource->GetOutput()->GetNumberOfPoints();
    for( int ptId = 0; ptId < nbPoints; ptId++)
    {
        reslicer->SetOffsetPoint(ptId);
        reslicer->Update();
         vtkImageData *tempSlice = vtkImageData::New();
        tempSlice->DeepCopy( reslicer->GetOutput( 0 ));
        append->AddInputData(tempSlice);
    }
    append->SetAppendAxis(2);
    append->Update();

    cprmapper1->SetInputConnection(append->GetOutputPort());
    cprcolorTranFun->AddRGBSegment(0,1,1,1,255,1,1,1);
    cprPieceFun->AddSegment(0,0,3000,1);
    cprPieceFun->AddPoint(20,0.2);
    cprPieceFun->AddPoint(80,0.5);
    cprPieceFun->AddPoint(120,0.7);
    cprPieceFun->AddPoint(200,0.9);
    cprPieceFun->ClampingOff();
    cprmapper1->SetBlendModeToMaximumIntensity();
    cprVolumeproperty->SetColor(cprcolorTranFun);
    cprVolumeproperty->SetScalarOpacity(cprPieceFun);
    cprVolumeproperty->SetInterpolationTypeToLinear();
    cprVolumeproperty->ShadeOff();
    cprvolume->SetProperty(cprVolumeproperty);
    cprvolume->SetMapper(cprmapper1);

    double range[2];
    patientHandling->getMraImageToBeDisplayed()->GetScalarRange(range);
    m_pShift->SetShift(-1.0*range[0]);
    m_pShift->SetScale(255.0/(range[1]-range[0]));
    m_pShift->SetOutputScalarTypeToUnsignedChar();
    m_pShift->SetInputConnection(append->GetOutputPort());
    m_pShift->ReleaseDataFlagOff();
    m_pShift->Update();

    pPlane->SetOrigin(cprvolume->GetCenter());
    pPlane->SetNormal(1,1,0);
    pCut->SetCutFunction(pPlane);
    pCut->SetInputConnection(m_pShift->GetOutputPort());
    pCut->Update();
}


//!--------------------------------------------------------------------------------------------------------------------------------
//!
//!
//! \brief SurgeryPlanWindow::regroupAllComponents
//!
void SurgeryPlanWindow::regroupAllComponents(){

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(patientInformationWidget);
    mainLayout->addWidget(cprAnalyseWidget);
    mainLayout->addWidget(controlBar);
    mainLayout->setSpacing(1);
    mainLayout->setContentsMargins(0,0,0,0);
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief CanalyserMainWindow::createRandomVtkImageData
//!
void SurgeryPlanWindow::createRandomVtkImageData(){
    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    image->SetDimensions(448,448,128);
    image->SetSpacing(0.5, 0.5, 0.8);
    image->AllocateScalars(VTK_UNSIGNED_SHORT,1);

    int dims[3];
    image->GetDimensions(dims);
    for(int i = 0; i < dims[0]; i++){
      for(int j = 0; j < dims[1]; j++){
          for(int k = 0; k < dims[2]; k++){
            unsigned short* pixel = static_cast<unsigned short*>(image->GetScalarPointer(i,j,k));
            *pixel = 2000;
          }
        }
      }
     display(image);
}

//!--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::originalOptionHovered
//!
void SurgeryPlanWindow::originalOptionHovered(){
    originalOption->setStyleSheet("border: 1px solid grey;  border-radius: 0px; background-color: gainsboro;  min-width: 0px; color: AliceBlue");
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::originalOptionClicked
//!
void SurgeryPlanWindow::originalOptionClicked(){
    originalOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: AliceBlue  "  );
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::originalOptionReleased
//!
void SurgeryPlanWindow::originalOptionReleased(){

    originalOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: AliceBlue  "  );
    transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );

    originalOption->setFont(QFont("Segoe UI",10, QFont::DemiBold, true));
    transparentBrainOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));
    greyMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    whiteMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    vesselOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    interventionalRouteOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));

    imageOptionStates.originalOptionState = true;
    imageOptionStates.transparentBrainOptionState = false;
    imageOptionStates.greyMatterOptionState = false;
    imageOptionStates.whiteMatterOptionState = false;
    imageOptionStates.vesselOptionState = false;
    imageOptionStates.interventionalRouteOptionState = false;
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::originalOptionLeaved
//!
void SurgeryPlanWindow::originalOptionLeaved(){
    if(imageOptionStates.originalOptionState){
        originalOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: AliceBlue");
    }
    else{
        originalOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey " );
    }

}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::transparentBrainOptionHovered
//!
void SurgeryPlanWindow::transparentBrainOptionHovered(){
    transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: gainsboro;  min-width: 0px; color: AliceBlue  "  );
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::transparentBrainOptionClicked
//!
void SurgeryPlanWindow::transparentBrainOptionClicked(){
    transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: AliceBlue  "  );
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::transparentBrainOptionReleased
//!
void SurgeryPlanWindow::transparentBrainOptionReleased(){
    originalOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: aliceblue  "  );
    greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );

    originalOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));
    transparentBrainOption->setFont(QFont("Segoe UI",10, QFont::DemiBold, true));
    greyMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    whiteMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    vesselOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    interventionalRouteOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));

    imageOptionStates.originalOptionState = false;
    imageOptionStates.transparentBrainOptionState = true;
    imageOptionStates.greyMatterOptionState = false;
    imageOptionStates.whiteMatterOptionState = false;
    imageOptionStates.vesselOptionState = false;
    imageOptionStates.interventionalRouteOptionState = false;
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::transparentBrainOptionLeaved
//!
void SurgeryPlanWindow::transparentBrainOptionLeaved(){
    if(imageOptionStates.transparentBrainOptionState){
        transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: AliceBlue");
    }
    else{
        transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey " );
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::greyMatterOptionHovered
//!
void SurgeryPlanWindow::greyMatterOptionHovered(){
    greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: gainsboro;  min-width: 0px; color: AliceBlue  "  );
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::greyMatterOptionClicked
//!
void SurgeryPlanWindow::greyMatterOptionClicked(){
    greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: AliceBlue  "  );

}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::greyMatterOptionReleased
//!
void SurgeryPlanWindow::greyMatterOptionReleased(){
    originalOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: aliceblue  "  );
    whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );

    originalOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));
    transparentBrainOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));
    greyMatterOption->setFont(QFont("Segoe UI", 10, QFont::DemiBold, true));
    whiteMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    vesselOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    interventionalRouteOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));

    imageOptionStates.originalOptionState = false;
    imageOptionStates.transparentBrainOptionState = false;
    imageOptionStates.greyMatterOptionState = true;
    imageOptionStates.whiteMatterOptionState = false;
    imageOptionStates.vesselOptionState = false;
    imageOptionStates.interventionalRouteOptionState = false;
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::greyMatterOptionLeaved
//!
void SurgeryPlanWindow::greyMatterOptionLeaved(){
    if(imageOptionStates.greyMatterOptionState){
        greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: AliceBlue");
    }
    else{
        greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey " );
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::whiteMatterOptionHovered
//!
void SurgeryPlanWindow::whiteMatterOptionHovered(){
    whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: gainsboro;  min-width: 0px; color: AliceBlue  "  );
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::whiteMatterOptionClicked
//!
void SurgeryPlanWindow::whiteMatterOptionClicked(){
    whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: AliceBlue  "  );
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::whiteMatterOptionReleased
//!
void SurgeryPlanWindow::whiteMatterOptionReleased(){
    originalOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: aliceblue  "  );
    vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );

    originalOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));
    transparentBrainOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));
    greyMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    whiteMatterOption->setFont(QFont("Segoe UI", 10, QFont::DemiBold, true));
    vesselOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    interventionalRouteOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));

    imageOptionStates.originalOptionState = false;
    imageOptionStates.transparentBrainOptionState = false;
    imageOptionStates.greyMatterOptionState = false;
    imageOptionStates.whiteMatterOptionState = true;
    imageOptionStates.vesselOptionState = false;
    imageOptionStates.interventionalRouteOptionState = false;
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::whiteMatterOptionLeaved
//!
void SurgeryPlanWindow::whiteMatterOptionLeaved(){
    if(imageOptionStates.whiteMatterOptionState){
        whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: AliceBlue");
    }
    else{
        whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey " );
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::vesselOptionHovered
//!
void SurgeryPlanWindow::vesselOptionHovered(){
    vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: gainsboro;  min-width: 0px; color: AliceBlue  "  );
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::vesselOptionClicked
//!
void SurgeryPlanWindow::vesselOptionClicked(){
    vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: AliceBlue  "  );
}

#include <QThread>
//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::vesselOptionReleased
//!
void SurgeryPlanWindow::vesselOptionReleased(){
    originalOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: aliceblue  "  );
    interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );

    originalOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));
    transparentBrainOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));
    greyMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    whiteMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    vesselOption->setFont(QFont("Segoe UI", 10, QFont::DemiBold, true));
    interventionalRouteOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));

    imageOptionStates.originalOptionState = false;
    imageOptionStates.transparentBrainOptionState = false;
    imageOptionStates.greyMatterOptionState = false;
    imageOptionStates.whiteMatterOptionState = false;
    imageOptionStates.vesselOptionState = true;
    imageOptionStates.interventionalRouteOptionState = false;
}


void SurgeryPlanWindow::getVesselEnhancedImage(){
    qDebug()<<enhancedImage->GetDataDimension();
}


//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::vesselOptionLeaved
//!
void SurgeryPlanWindow::vesselOptionLeaved(){
    if(imageOptionStates.vesselOptionState){
        vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: AliceBlue");
    }
    else{
        vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey " );
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::interventionalRouteOptionHovered
//!
void SurgeryPlanWindow::interventionalRouteOptionHovered(){
     interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: gainsboro;  min-width: 0px; color: AliceBlue  "  );
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::interventionalRouteOptionClicked
//!
void SurgeryPlanWindow::interventionalRouteOptionClicked(){
     interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: AliceBlue  "  );
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::interventionalRouteOptionReleased
//!
void SurgeryPlanWindow::interventionalRouteOptionReleased(){
    originalOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    transparentBrainOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    greyMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    whiteMatterOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    vesselOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey  "  );
    interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: aliceblue  "  );

    originalOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));
    transparentBrainOption->setFont(QFont("Segoe UI",10, QFont::AnyStyle, true));
    greyMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    whiteMatterOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    vesselOption->setFont(QFont("Segoe UI", 10, QFont::AnyStyle, true));
    interventionalRouteOption->setFont(QFont("Segoe UI", 10, QFont::DemiBold, true));

    imageOptionStates.originalOptionState = false;
    imageOptionStates.transparentBrainOptionState = false;
    imageOptionStates.greyMatterOptionState = false;
    imageOptionStates.whiteMatterOptionState = false;
    imageOptionStates.vesselOptionState = false;
    imageOptionStates.interventionalRouteOptionState = true;

    displayCenterLine();

}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::interventionalRouteOptionLeaved
//!
void SurgeryPlanWindow::interventionalRouteOptionLeaved(){
    if(imageOptionStates.interventionalRouteOptionState){
        interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: grey;  min-width: 0px; color: AliceBlue");
    }
    else{
        interventionalRouteOption->setStyleSheet( "border: 1px solid grey;  border-radius: 0px; background-color: transparent;  min-width: 0px; color: grey " );
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::displayConfigurationBoard
//!
void SurgeryPlanWindow::displayConfigurationBoard(){
    this->patientWidgetConfigurationBoard->display(QCursor::pos());
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::displayCurveReformatwionWindow
//!
void SurgeryPlanWindow::displayCurveReformatwionWindow(){
    this->curveReformationWindow->display();
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::closeSurgeryPlanWindow
//!
void SurgeryPlanWindow::closeSurgeryPlanWindow(){
    //this->close();
    this->display(this->patientHandling->getMraImageToBeDisplayed());
}

//--------------------------------------------------------------------------------------------------------------------------------
//!
//! \brief SurgeryPlanWindow::showTime
//!
void SurgeryPlanWindow::showTime(){
    this->current_time = this->surgeryTime->elapsed();
    int t = this->current_time - this->start_time;
    int hour = t/3600000;
    int minute = t/60000 - hour*60;
    int second = t/1000 - minute*60 - hour*3600;

//    if(t < 86400000){
//        hour = t/3600000;
//        minute = t/60000 - hour*60;
//        second = t/1000 - minute*60 - hour*3600;
//    }
//    else {
//        day = day +1;
//        t = t-86400000*day;
//        hour = t/3600000;
//        minute = t/60000 - hour*60;
//        second = t/1000 - minute*60 - hour*3600;
//    }
    if(hour < 10 && minute < 10 &&second < 10)
        this->elapsedTimeLabel->display("0" + QString::number(hour) + ":" + "0" + QString::number(minute) + ":" + "0" + QString::number(second));
    else if(hour < 10 && minute < 10 &&second >= 10)
        this->elapsedTimeLabel->display("0" + QString::number(hour) + ":" + "0" + QString::number(minute) + ":" + QString::number(second));
    else if(hour < 10 && minute >= 10 &&second < 10)
        this->elapsedTimeLabel->display("0" + QString::number(hour) + ":" + QString::number(minute) + ":" + "0" + QString::number(second));
    else if(hour < 10 && minute >= 10 &&second >= 10)
        this->elapsedTimeLabel->display("0" + QString::number(hour) + ":" + QString::number(minute) + ":" + QString::number(second));
    else if(hour >= 10 && minute < 10 &&second < 10)
        this->elapsedTimeLabel->display(QString::number(hour) + ":" + "0" + QString::number(minute) + ":" + "0" + QString::number(second));
    else if(hour >= 10 && minute < 10 &&second >= 10)
        this->elapsedTimeLabel->display(QString::number(hour) + ":" + "0" + QString::number(minute) + ":" + QString::number(second));
    else if(hour >= 10 && minute >= 10 &&second < 10)
        this->elapsedTimeLabel->display(QString::number(hour) + ":" + QString::number(minute) + ":" + "0" + QString::number(second));
    else
        this->elapsedTimeLabel->display(QString::number(hour) + ":" + QString::number(minute) + ":" + QString::number(second));
}
