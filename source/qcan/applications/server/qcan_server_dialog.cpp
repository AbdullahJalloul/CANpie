//====================================================================================================================//
// File:          qcan_server_dialog.cpp                                                                              //
// Description:   QCAN server - configuration dialog                                                                  //
//                                                                                                                    //
// Copyright (C) MicroControl GmbH & Co. KG                                                                           //
// 53844 Troisdorf - Germany                                                                                          //
// www.microcontrol.net                                                                                               //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the   //
// following conditions are met:                                                                                      //
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions, the following   //
//    disclaimer and the referenced file 'LICENSE'.                                                                   //
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       //
//    following disclaimer in the documentation and/or other materials provided with the distribution.                //
// 3. Neither the name of MicroControl nor the names of its contributors may be used to endorse or promote products   //
//    derived from this software without specific prior written permission.                                           //
//                                                                                                                    //
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance     //
// with the License. You may obtain a copy of the License at                                                          //
//                                                                                                                    //
//    http://www.apache.org/licenses/LICENSE-2.0                                                                      //
//                                                                                                                    //
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed   //
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for  //
// the specific language governing permissions and limitations under the License.                                     //
//                                                                                                                    //
//====================================================================================================================//


/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

#include "qcan_server_dialog.hpp"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QString>


/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions                                                                                                        **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------------------------------------------------*\
** Class methods                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/



//----------------------------------------------------------------------------//
// QCanServer()                                                               //
// constructor                                                                //
//----------------------------------------------------------------------------//
QCanServerDialog::QCanServerDialog(QWidget * parent)
   : QDialog(parent)
{
   uint8_t        ubNetworkIdxT;
   QCanNetwork *  pclNetworkT;
   QString        clNetNameT;


   //----------------------------------------------------------------
   // create CAN networks
   //
   setupNetworks();

   //----------------------------------------------------------------
   // setup the user interface
   //
   ui.setupUi(this);

   //----------------------------------------------------------------
   // update version number in info tab
   //
   QString clVersionT = "CANpie Server Version ";
   clVersionT += QString("%1.%2.").arg(VERSION_MAJOR).arg(VERSION_MINOR);
   clVersionT += QString("%1").arg(VERSION_BUILD);
   ui.pclLblInfoVersionM->setText(clVersionT);

   //----------------------------------------------------------------
   // add logging
   //
   pclLoggerP = new QCanServerLogger();

   //----------------------------------------------------------------
   // hide CAN server port
   //
   ui.pclLblSrvPortM->hide();
   ui.pclEdtSrvPortM->hide();


   //----------------------------------------------------------------
   // create toolbox for can interfaces
   //
   pclTbxNetworkP = new QToolBox(ui.pclTabConfigNetworkM);
   pclTbxNetworkP->setGeometry(QRect(0, 0, 101, 361));
   for(ubNetworkIdxT = 0; ubNetworkIdxT < QCAN_NETWORK_MAX; ubNetworkIdxT++)
   {
      apclCanIfWidgetP[ubNetworkIdxT]  = new QCanInterfaceWidget(ubNetworkIdxT);
      apclCanIfWidgetP[ubNetworkIdxT]->setGeometry(QRect(0, 0, 101, 145));
      pclTbxNetworkP->addItem(apclCanIfWidgetP[ubNetworkIdxT], 
                              ("CAN " + QString::number(ubNetworkIdxT+1,10)));
   }

   //----------------------------------------------------------------
   // create values for nominal bit-rate
   //
   ui.pclCbbNetNomBitrateM->clear();
   ui.pclCbbNetNomBitrateM->addItem("10 kBit/s" , (int32_t)   10000);
   ui.pclCbbNetNomBitrateM->addItem("20 kBit/s" , (int32_t)   20000);
   ui.pclCbbNetNomBitrateM->addItem("50 kBit/s" , (int32_t)   50000);
   ui.pclCbbNetNomBitrateM->addItem("100 kBit/s", (int32_t)  100000);
   ui.pclCbbNetNomBitrateM->addItem("125 kBit/s", (int32_t)  125000);
   ui.pclCbbNetNomBitrateM->addItem("250 kBit/s", (int32_t)  250000);
   ui.pclCbbNetNomBitrateM->addItem("500 kBit/s", (int32_t)  500000);
   ui.pclCbbNetNomBitrateM->addItem("800 kBit/s", (int32_t)  800000);
   ui.pclCbbNetNomBitrateM->addItem("1 MBit/s"  , (int32_t) 1000000);
   
   //----------------------------------------------------------------
   // create values for data bit-rate
   //
   ui.pclCbbNetDatBitrateM->clear();
   ui.pclCbbNetDatBitrateM->addItem("None"      , (int32_t) eCAN_BITRATE_NONE);
   ui.pclCbbNetDatBitrateM->addItem("1 MBit/s"  , (int32_t) 1000000);
   ui.pclCbbNetDatBitrateM->addItem("2 MBit/s"  , (int32_t) 2000000);
   ui.pclCbbNetDatBitrateM->addItem("4 MBit/s"  , (int32_t) 4000000);
   ui.pclCbbNetDatBitrateM->addItem("5 MBit/s"  , (int32_t) 5000000);
   
   //----------------------------------------------------------------
   // load settings
   //
   pclSettingsP = new QSettings( QSettings::NativeFormat,
                                 QSettings::UserScope,
                                 "microcontrol.net",
                                 "CANpieServer");

   //-----------------------------------------------------------
   // settings for server
   //
   pclSettingsP->beginGroup("Server");
   QHostAddress clHostAddrT = QHostAddress(pclSettingsP->value("hostAddress",
                                             "127.0.0.1").toString());
   pclCanServerP->setServerAddress(clHostAddrT);
   pclSettingsP->endGroup();

   //-----------------------------------------------------------
   // settings for network
   //
   for(ubNetworkIdxT = 0; ubNetworkIdxT < QCAN_NETWORK_MAX; ubNetworkIdxT++)
   {
      pclNetworkT = pclCanServerP->network(ubNetworkIdxT);
      pclLoggerP->addLoggingSource(pclNetworkT);
      clNetNameT  = "CAN_" + QString("%1").arg(ubNetworkIdxT+1);
      pclSettingsP->beginGroup(clNetNameT);

      pclLoggerP->setLogLevel((CAN_Channel_e)(ubNetworkIdxT + 1),
                              (LogLevel_e) pclSettingsP->value("loglevel",
                                                eLOG_LEVEL_INFO).toInt());

      pclNetworkT->setNetworkEnabled(pclSettingsP->value("enable",
                                     0).toBool());

      pclNetworkT->setErrorFrameEnabled(pclSettingsP->value("errorFrame",
                                     0).toBool());

      pclNetworkT->setFastDataEnabled(pclSettingsP->value("canFD",
                                     0).toBool());

      pclNetworkT->setListenOnlyEnabled(pclSettingsP->value("listenOnly",
                                     0).toBool());

      pclNetworkT->setBitrate(pclSettingsP->value("bitrateNom",
                              500000).toInt(),
                              pclSettingsP->value("bitrateDat",
                              eCAN_BITRATE_NONE).toInt());

      apclCanIfWidgetP[ubNetworkIdxT]->setInterface(
            pclSettingsP->value("interface" +
                                QString::number(ubNetworkIdxT),"").toString());

      pclSettingsP->endGroup();

   }


   //----------------------------------------------------------------
   // connect widgets of dialog
   //
   connect(ui.pclCkbNetEnableM, SIGNAL(clicked(bool)),
           this, SLOT(onNetworkConfEnable(bool)));

   connect(ui.pclCkbNetErrorFramesM, SIGNAL(clicked(bool)),
           this, SLOT(onNetworkConfErrorFrames(bool)));

   connect(ui.pclCkbNetCanFdM, SIGNAL(clicked(bool)),
           this, SLOT(onNetworkConfCanFd(bool)));

   connect(ui.pclCkbNetListenOnlyM, SIGNAL(clicked(bool)),
           this, SLOT(onNetworkConfListenOnly(bool)));
   
   connect(pclTbxNetworkP, SIGNAL(currentChanged(int)),
           this, SLOT(onNetworkChange(int)));

   connect(ui.pclCbbNetNomBitrateM, SIGNAL(currentIndexChanged(int)),
           this, SLOT(onNetworkConfBitrateNom(int)));

   connect(ui.pclCbbNetDatBitrateM, SIGNAL(currentIndexChanged(int)),
           this, SLOT(onNetworkConfBitrateDat(int)));

   connect(ui.pclChkServerRemoteAccessM, SIGNAL(stateChanged(int)),
           this, SLOT(onServerRemoteAccess(int)));

   connect(ui.pclChkChangeBitrateM, SIGNAL(stateChanged(int)),
           this, SLOT(onServerBitrateAccess(int)));

   connect(ui.pclPbtNetDevSpecificConfigM, SIGNAL(clicked()),
           this, SLOT(onDeviceSpecificConfig()));

   connect(ui.pclBtnStatusResetM, SIGNAL(clicked(bool)),
           this, SLOT(onNetworkReset(bool)));

   //----------------------------------------------------------------
   // connect default signals / slots for statistic
   //
   pclNetworkT = pclCanServerP->network(0);
   connect(pclNetworkT, SIGNAL(showCanFrames(CAN_Channel_e, uint32_t)),
           this, SLOT(onNetworkShowCanFrames(CAN_Channel_e, uint32_t)) );

   connect(pclNetworkT, SIGNAL(showErrFrames(CAN_Channel_e, uint32_t)),
           this, SLOT(onNetworkShowErrFrames(CAN_Channel_e, uint32_t)) );

   connect(pclNetworkT, SIGNAL(showLoad(CAN_Channel_e, uint8_t, uint32_t)),
            this, SLOT(onNetworkShowLoad(CAN_Channel_e, uint8_t, uint32_t)) );

   connect(pclNetworkT, SIGNAL(showState(CAN_Channel_e, CAN_State_e)),
            this, SLOT(onNetworkShowState(CAN_Channel_e, CAN_State_e)) );

   connect(pclNetworkT, SIGNAL(changeBitrate(CAN_Channel_e, uint32_t, int32_t)),
           this, SLOT(onNetworkChangeBitrate(CAN_Channel_e, uint32_t, int32_t)));

   //----------------------------------------------------------------
   // Intialise interface widgets for CAN interface selection
   //
   for(ubNetworkIdxT = 0; ubNetworkIdxT < QCAN_NETWORK_MAX; ubNetworkIdxT++)
   {
      connect( apclCanIfWidgetP[ubNetworkIdxT], 
               &QCanInterfaceWidget::interfaceChanged,
               this,                                       
               &QCanServerDialog::onInterfaceChange);
   }



   //----------------------------------------------------------------
   // create actions and tray icon in system tray
   //
   createActions();
   createTrayIcon();
   setIcon();
   pclIconTrayP->show();

   //----------------------------------------------------------------
   // set title of dialog window
   //
   setWindowTitle("CANpie Server");
   
   //----------------------------------------------------------------
   // show CAN channel 1 as default and update user interface
   //
   slLastNetworkIndexP = 0;

   ui.pclTabConfigM->setCurrentIndex(0);
   pclTbxNetworkP->setCurrentIndex(0);
   this->updateUI(eCAN_CHANNEL_1);

}


//----------------------------------------------------------------------------//
// QCanServer()                                                               //
// destructor                                                                 //
//----------------------------------------------------------------------------//
QCanServerDialog::~QCanServerDialog()
{
   uint8_t        ubNetworkIdxT;
   QCanNetwork *  pclNetworkT;
   QString        clNetNameT;

   //----------------------------------------------------------------
   // store settings
   //
   //----------------------------------------------------------------

   for(ubNetworkIdxT = 0; ubNetworkIdxT < QCAN_NETWORK_MAX; ubNetworkIdxT++)
   {
      //--------------------------------------------------------
      // settings for network
      //
      pclNetworkT = pclCanServerP->network(ubNetworkIdxT);
      clNetNameT  = "CAN_" + QString("%1").arg(ubNetworkIdxT+1);
      pclSettingsP->beginGroup(clNetNameT);
      pclSettingsP->setValue("bitrateNom", pclNetworkT->nominalBitrate());
      pclSettingsP->setValue("bitrateDat", pclNetworkT->dataBitrate());
      pclSettingsP->setValue("enable",     pclNetworkT->isNetworkEnabled());
      pclSettingsP->setValue("errorFrame", pclNetworkT->isErrorFrameEnabled());
      pclSettingsP->setValue("canFD",      pclNetworkT->isFastDataEnabled());
      pclSettingsP->setValue("listenOnly", pclNetworkT->isListenOnlyEnabled());
      pclSettingsP->setValue("loglevel",   pclLoggerP->logLevel((CAN_Channel_e)(ubNetworkIdxT+1)));

      pclSettingsP->setValue("interface"+QString::number(ubNetworkIdxT), 
                              apclCanIfWidgetP[ubNetworkIdxT]->name());

      pclSettingsP->endGroup();
   }
   //-----------------------------------------------------------
   // settings for server
   //
   pclSettingsP->beginGroup("Server");
   pclSettingsP->setValue("hostAddress",
                           pclCanServerP->serverAddress().toString());

   pclSettingsP->endGroup();

   delete(pclSettingsP);
   delete(pclLoggerP);
}


//----------------------------------------------------------------------------//
// createActions()                                                            //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::createActions(void)
{
   pclActionConfigP = new QAction(tr("&Configuration ..."), this);
   connect(pclActionConfigP, &QAction::triggered, this, &QWidget::showNormal);

   pclActionLoggingP = new QAction(this);
   pclActionLoggingP->setShortcut(Qt::CTRL | Qt::Key_L);
   connect(pclActionLoggingP, SIGNAL(triggered()), this, SLOT(onLoggingWindow()));
   this->addAction(pclActionLoggingP);

   pclActionQuitP = new QAction(tr("&Quit"), this);
   connect(pclActionQuitP, &QAction::triggered, qApp, &QCoreApplication::quit);
}


//----------------------------------------------------------------------------//
// createTrayIcon()                                                           //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::createTrayIcon(void)
{
   pclMenuTrayP = new QMenu(this);
   pclMenuTrayP->addAction(pclActionConfigP);

   pclMenuTrayP->addSeparator();
   pclMenuTrayP->addAction(pclActionQuitP);

   pclIconTrayP = new QSystemTrayIcon(this);
   pclIconTrayP->setContextMenu(pclMenuTrayP);
}


//----------------------------------------------------------------------------//
// onInterfaceChange()                                                        //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onInterfaceChange( uint8_t ubIdxV, 
                                          QCanInterface *pclCanIfV)
{
   QCanNetwork *  pclNetworkT;

   qDebug() << "QCanServerDialog:onInterfaceChange() ToolBox Index " << QString::number(ubIdxV);


   //----------------------------------------------------------------
   // get selected QCanNetwork
   //
   pclNetworkT = pclCanServerP->network(ubIdxV);

   //----------------------------------------------------------------
   // add / remove physical CAN interface
   //
   if (pclCanIfV == Q_NULLPTR)
   {
      pclNetworkT->removeInterface();
      apclCanIfWidgetP[ubIdxV]->setIcon(QIcon(QCAN_IF_VCAN_ICON));
      qDebug() << " set vCAN icon";
   }
   else
   {
      qDebug() << " set interface icon";
      if (pclNetworkT->addInterface(pclCanIfV) == true)
      {
         pclNetworkT->startInterface();
         apclCanIfWidgetP[ubIdxV]->setIcon(pclCanIfV->icon());
      }
      else
      {
         qWarning() << "Fail to add interface!";
      }
   }

   updateUI(CAN_Channel_e (ubIdxV + 1));
}

void QCanServerDialog::onLoggingWindow(void)
{
   pclLoggerP->show();
}


//----------------------------------------------------------------------------//
// onNetworkChange()                                                          //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkChange(int slIndexV)
{
   QCanNetwork *  pclNetworkT;

   qDebug() << "QCanServerDialog::onNetworkChange()" << slIndexV;

   //----------------------------------------------------------------
   // reconnect the signals / slots for statistic
   //
   pclNetworkT = pclCanServerP->network(slLastNetworkIndexP);
   disconnect(pclNetworkT, SIGNAL(showCanFrames(CAN_Channel_e, uint32_t)),
           this, SLOT(onNetworkShowCanFrames(CAN_Channel_e, uint32_t)) );

   disconnect(pclNetworkT, SIGNAL(showErrFrames(CAN_Channel_e, uint32_t)),
           this, SLOT(onNetworkShowErrFrames(CAN_Channel_e, uint32_t)) );

   disconnect(pclNetworkT, SIGNAL(showLoad(CAN_Channel_e, uint8_t, uint32_t)),
            this, SLOT(onNetworkShowLoad(CAN_Channel_e, uint8_t, uint32_t)) );

   disconnect(pclNetworkT, SIGNAL(showState(CAN_Channel_e, CAN_State_e)),
            this, SLOT(onNetworkShowState(CAN_Channel_e, CAN_State_e)) );

   disconnect(pclNetworkT, SIGNAL(changeBitrate(CAN_Channel_e, uint32_t, int32_t)),
           this, SLOT(onNetworkChangeBitrate(CAN_Channel_e, uint32_t, int32_t)));

   pclNetworkT = pclCanServerP->network(slIndexV);
   connect(pclNetworkT, SIGNAL(showCanFrames(CAN_Channel_e, uint32_t)),
           this, SLOT(onNetworkShowCanFrames(CAN_Channel_e, uint32_t)) );

   connect(pclNetworkT, SIGNAL(showErrFrames(CAN_Channel_e, uint32_t)),
           this, SLOT(onNetworkShowErrFrames(CAN_Channel_e, uint32_t)) );

   connect(pclNetworkT, SIGNAL(showLoad(CAN_Channel_e, uint8_t, uint32_t)),
            this, SLOT(onNetworkShowLoad(CAN_Channel_e, uint8_t, uint32_t)) );

   connect(pclNetworkT, SIGNAL(showState(CAN_Channel_e, CAN_State_e)),
            this, SLOT(onNetworkShowState(CAN_Channel_e, CAN_State_e)) );

   connect(pclNetworkT, SIGNAL(changeBitrate(CAN_Channel_e, uint32_t, int32_t)),
           this, SLOT(onNetworkChangeBitrate(CAN_Channel_e, uint32_t, int32_t)));

   //----------------------------------------------------------------
   // update user interface
   //
   this->updateUI(CAN_Channel_e (slIndexV + 1));

   //----------------------------------------------------------------
   // store index for future access
   //
   slLastNetworkIndexP = slIndexV;
}

void QCanServerDialog::onNetworkChangeBitrate(CAN_Channel_e ubChannelV, uint32_t slNomBitRateV, int32_t slDatBitRateV)
{
   qDebug() << "QCanServerDialog::onNetworkChangeBitrate(CAN channel" << ubChannelV << ")";
   updateUI(ubChannelV);
}


//----------------------------------------------------------------------------//
// onNetworkConfBitrate()                                                     //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkConfBitrateDat(int slBitrateSelV)
{
   QCanNetwork *  pclNetworkT;

   pclNetworkT = pclCanServerP->network(pclTbxNetworkP->currentIndex());
   if(pclNetworkT != Q_NULLPTR)
   {
      slBitrateSelV = ui.pclCbbNetDatBitrateM->currentData().toInt();
      qDebug() << "Change data bit-rate" << slBitrateSelV;
      pclNetworkT->setBitrate(pclNetworkT->nominalBitrate(), slBitrateSelV);
   }
}


//----------------------------------------------------------------------------//
// onNetworkConfBitrate()                                                     //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkConfBitrateNom(int slBitrateSelV)
{
   QCanNetwork * pclNetworkT;

   pclNetworkT = pclCanServerP->network(pclTbxNetworkP->currentIndex());
   if(pclNetworkT != Q_NULLPTR)
   {
      slBitrateSelV = ui.pclCbbNetNomBitrateM->currentData().toInt();
      qDebug() << "Change nominal bit-rate" << slBitrateSelV;
      pclNetworkT->setBitrate(slBitrateSelV, pclNetworkT->dataBitrate());
   }
}


//----------------------------------------------------------------------------//
// onNetworkConfCanFd()                                                       //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkConfCanFd(bool btEnableV)
{
   QCanNetwork * pclNetworkT;

   pclNetworkT = pclCanServerP->network(pclTbxNetworkP->currentIndex());
   if(pclNetworkT != Q_NULLPTR)
   {
      pclNetworkT->setFastDataEnabled(btEnableV);
   }

   //----------------------------------------------------------------
   // update user interface
   //
   if(btEnableV == true)
   {
      ui.pclLblNetDatBitrateM->setEnabled(true);
      ui.pclCbbNetDatBitrateM->setEnabled(true);
   }
   else
   {
      ui.pclLblNetDatBitrateM->setEnabled(false);
      ui.pclCbbNetDatBitrateM->setEnabled(false);
   }
}


//----------------------------------------------------------------------------//
// onNetworkConfEnable()                                                      //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkConfEnable(bool btEnableV)
{
   QCanNetwork * pclNetworkT;

   pclNetworkT = pclCanServerP->network(pclTbxNetworkP->currentIndex());
   if(pclNetworkT != Q_NULLPTR)
   {
      pclNetworkT->setNetworkEnabled(btEnableV);
   }
}


//----------------------------------------------------------------------------//
// onNetworkConfErrorFrames()                                                 //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkConfErrorFrames(bool btEnableV)
{
   QCanNetwork * pclNetworkT;

   pclNetworkT = pclCanServerP->network(pclTbxNetworkP->currentIndex());
   if(pclNetworkT != Q_NULLPTR)
   {
      pclNetworkT->setErrorFrameEnabled(btEnableV);
   }
}


//----------------------------------------------------------------------------//
// onNetworkConfListenOnly()                                                  //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkConfListenOnly(bool btEnableV)
{
   QCanNetwork * pclNetworkT;

   pclNetworkT = pclCanServerP->network(pclTbxNetworkP->currentIndex());
   if(pclNetworkT != Q_NULLPTR)
   {
      pclNetworkT->setListenOnlyEnabled(btEnableV);
   }
}


//----------------------------------------------------------------------------//
// onNetworkReset()                                                           //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkReset(bool btCheckedV)
{
   Q_UNUSED(btCheckedV);
   QCanNetwork * pclNetworkT;

   pclNetworkT = pclCanServerP->network(pclTbxNetworkP->currentIndex());
   if(pclNetworkT != Q_NULLPTR)
   {
      pclNetworkT->reset();
   }
   ui.pclBtnStatusResetM->setEnabled(false);
}


//----------------------------------------------------------------------------//
// onNetworkShowCanFrames()                                                   //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkShowCanFrames(CAN_Channel_e ubChannelV, uint32_t ulFrameCntV)
{
   Q_UNUSED(ubChannelV);

   ui.pclCntStatCanM->setText(QString("%1").arg(ulFrameCntV));
   if (ulFrameCntV > 0)
   {
      ui.pclBtnStatusResetM->setEnabled(true);
   }
}

//----------------------------------------------------------------------------//
// onNetworkShowErrFrames()                                                   //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkShowErrFrames(CAN_Channel_e ubChannelV, uint32_t ulFrameCntV)
{
   Q_UNUSED(ubChannelV);

   ui.pclCntStatErrM->setText(QString("%1").arg(ulFrameCntV));
   if (ulFrameCntV > 0)
   {
      ui.pclBtnStatusResetM->setEnabled(true);
   }
}


//----------------------------------------------------------------------------//
// onNetworkShowLoad()                                                        //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onNetworkShowLoad(CAN_Channel_e ubChannelV, uint8_t ubLoadV, uint32_t ulMsgPerSecV)
{
   Q_UNUSED(ubChannelV);

   ui.pclCntStatMsgM->setText(QString("%1").arg(ulMsgPerSecV));
   ui.pclPgbStatLoadM->setValue(ubLoadV);
}

void QCanServerDialog::onNetworkShowState(CAN_Channel_e ubChannelV, CAN_State_e teStateV)
{
   Q_UNUSED(ubChannelV);

   ui.pclTxtStatusBusM->setStyleSheet("QLabel { color : black; }");
   switch(teStateV)
   {
      case eCAN_STATE_STOPPED:
         ui.pclTxtStatusBusM->setText(tr("Stopped"));
         break;

      case eCAN_STATE_SLEEPING:
         ui.pclTxtStatusBusM->setText(tr("Sleeping"));
         break;

      case eCAN_STATE_BUS_ACTIVE:
         ui.pclTxtStatusBusM->setStyleSheet("QLabel { color : green; }");
         ui.pclTxtStatusBusM->setText(tr("OK (error active)"));
         break;

      case eCAN_STATE_BUS_WARN:
         ui.pclTxtStatusBusM->setStyleSheet("QLabel { color : orange; }");
         ui.pclTxtStatusBusM->setText(tr("Warning"));
         break;

      case eCAN_STATE_BUS_PASSIVE:
         ui.pclTxtStatusBusM->setStyleSheet("QLabel { color : orange; }");
         ui.pclTxtStatusBusM->setText(tr("Error passive"));
         break;

      case eCAN_STATE_BUS_OFF:
         ui.pclTxtStatusBusM->setStyleSheet("QLabel { color : red; }");
         ui.pclTxtStatusBusM->setText(tr("Bus-off"));
         break;
   }
}


//----------------------------------------------------------------------------//
// onDeviceSpecificConfig()                                                   //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onDeviceSpecificConfig()
{
   qDebug() << "onDeviceSpecificConfig";

   QCanNetwork *  pclNetworkT;

   pclNetworkT = pclCanServerP->network(pclTbxNetworkP->currentIndex());
   if(pclNetworkT != Q_NULLPTR)
   {
      qDebug() << "Call Configuration GUI for " << pclNetworkT->name();
      pclNetworkT->setInterfaceConfiguration();
   }
}


//----------------------------------------------------------------------------//
// onServerBitrateAccess()                                                    //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onServerBitrateAccess(int slStateV)
{

   if(slStateV == Qt::Unchecked)
   {
      pclCanServerP->enableBitrateChange(false);
   }
   else
   {
      pclCanServerP->enableBitrateChange(true);
   }
}

//----------------------------------------------------------------------------//
// onServerRemoteAccess()                                                     //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onServerRemoteAccess(int slStateV)
{
   QHostAddress clHostAddressT;

   if(slStateV == Qt::Unchecked)
   {
      clHostAddressT = QHostAddress(QHostAddress::LocalHost);
   }
   else
   {
      clHostAddressT = QHostAddress(QHostAddress::Any);
   }
   pclCanServerP->setServerAddress(clHostAddressT);
}

//----------------------------------------------------------------------------//
// onServerConfTime()                                                         //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::onServerDispatchTimeLimit(int slValueV)
{
   qDebug() << "onServerConfTime" << slValueV;
}


//----------------------------------------------------------------------------//
// setupNetworks()                                                            //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::setupNetworks(void)
{
   pclCanServerP = new QCanServer(  this,
                                    QCAN_TCP_DEFAULT_PORT,
                                    QCAN_NETWORK_MAX);

   if (pclCanServerP->isActive() == false)
   {
   }
}


//----------------------------------------------------------------------------//
// setIcon()                                                                  //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::setIcon(void)
{
    QIcon clIconT = QIcon(":images/canpie_server_512.png");

    pclIconTrayP->setIcon(clIconT);
    pclIconTrayP->setToolTip("CANpie Server");
}


//----------------------------------------------------------------------------//
// updateUI()                                                                 //
//                                                                            //
//----------------------------------------------------------------------------//
void QCanServerDialog::updateUI(const CAN_Channel_e & ubChannelR)
{
   QCanNetwork * pclNetworkT;

   qDebug() << "QCanServerDialog::updateUI(CAN channel" << ubChannelR << ")";

   //----------------------------------------------------------------
   // update network tab
   //
   pclNetworkT = pclCanServerP->network(ubChannelR - 1);
   if(pclNetworkT != Q_NULLPTR)
   {

      //--------------------------------------------------------
      // Is network enabled?
      //
      ui.pclCkbNetEnableM->setChecked(pclNetworkT->isNetworkEnabled());

      //--------------------------------------------------------
      // Does it support error frames?
      //
      if(pclNetworkT->hasErrorFrameSupport())
      {
         ui.pclCkbNetErrorFramesM->setEnabled(true);
         ui.pclCkbNetErrorFramesM->setChecked(pclNetworkT->isErrorFrameEnabled());
      }
      else
      {
         ui.pclCkbNetErrorFramesM->setChecked(false);
         ui.pclCkbNetErrorFramesM->setEnabled(false);
      }
      
      //--------------------------------------------------------
      // Does it support CAN-FD frames?
      //
      if(pclNetworkT->hasFastDataSupport())
      {
         ui.pclCkbNetCanFdM->setEnabled(true);
         ui.pclCkbNetCanFdM->setChecked(pclNetworkT->isFastDataEnabled());
      }
      else
      {
         ui.pclCkbNetCanFdM->setChecked(false);
         ui.pclCkbNetCanFdM->setEnabled(false);
      }


      //--------------------------------------------------------
      // Is CAN-FD activated?
      //
      if(ui.pclCkbNetCanFdM->isChecked())
      {
         ui.pclLblNetDatBitrateM->setEnabled(true);
         ui.pclCbbNetDatBitrateM->setEnabled(true);
      }
      else
      {
         ui.pclLblNetDatBitrateM->setEnabled(false);
         ui.pclCbbNetDatBitrateM->setEnabled(false);
      }


      //--------------------------------------------------------
      // Does it support Listen-Only mode?
      //
      if(pclNetworkT->hasListenOnlySupport())
      {
         ui.pclCkbNetListenOnlyM->setEnabled(true);
         ui.pclCkbNetListenOnlyM->setChecked(pclNetworkT->isListenOnlyEnabled());
      }
      else
      {
         ui.pclCkbNetListenOnlyM->setChecked(false);
         ui.pclCkbNetListenOnlyM->setEnabled(false);
      }

      //--------------------------------------------------------
      // Does it support Device Specific Configuration
      //
      if (pclNetworkT->hasSpecificConfigurationSupport())
      {
         ui.pclPbtNetDevSpecificConfigM->show();
         ui.pclPbtNetDevSpecificConfigM->setEnabled(true);
      } else
      {
         ui.pclPbtNetDevSpecificConfigM->setEnabled(false);
         ui.pclPbtNetDevSpecificConfigM->hide();
      }

      //--------------------------------------------------------
      // show current nominal bit-rate
      //
      int32_t slNomBitRateSelT = ui.pclCbbNetNomBitrateM->findData(pclNetworkT->nominalBitrate());
      ui.pclCbbNetNomBitrateM->setCurrentIndex(slNomBitRateSelT);
      
      //--------------------------------------------------------
      // show current data bit-rate
      //
      int32_t slDatBitRateSelT = ui.pclCbbNetDatBitrateM->findData(pclNetworkT->dataBitrate());
      ui.pclCbbNetDatBitrateM->setCurrentIndex(slDatBitRateSelT);


      //--------------------------------------------------------
      // Bus load and Messages/s are set to 0 by default
      //
      ui.pclCntStatMsgM->setText("0");
      ui.pclPgbStatLoadM->setValue(0);


      //--------------------------------------------------------
      // the reset button disabled by default, it is enabled
      // by onNetworkShowCanFrames() or onNetworkShowErrFrames()
      //
      ui.pclBtnStatusResetM->setEnabled(false);

      //--------------------------------------------------------
      // Update value for CAN messages
      //
      onNetworkShowCanFrames(ubChannelR, pclNetworkT->frameCount());

      //--------------------------------------------------------
      // Update value for Error messages
      //
      onNetworkShowErrFrames(ubChannelR, pclNetworkT->frameCountError());

      //--------------------------------------------------------
      // show current CAN bus status
      //
      onNetworkShowState(ubChannelR, pclNetworkT->state());
   }

   //----------------------------------------------------------------
   // update server tab
   //
   if(pclNetworkT != Q_NULLPTR)
   {
      QHostAddress clHostAddrT = pclNetworkT->serverAddress();
      if(clHostAddrT == QHostAddress(QHostAddress::Any))
      {
         ui.pclChkServerRemoteAccessM->setCheckState(Qt::Checked);
      }
      else
      {
         ui.pclChkServerRemoteAccessM->setCheckState(Qt::Unchecked);
      }
   }
}
