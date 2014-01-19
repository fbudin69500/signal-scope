#include "selectsignaldialog.h"
#include "ui_selectsignaldialog.h"

#include "signaldescription.h"
#include "signalhandler.h"
#include "jointnames.h"

#include <cstdio>
#include <cassert>

class SelectSignalDialog::Internal : public Ui::SelectSignalDialog
{
public:

  QList<QListWidget*> ArrayKeyWidgets;

};

SelectSignalDialog::SelectSignalDialog(QWidget* parent) : QDialog(parent)
{
  mInternal = new Internal;
  mInternal->setupUi(this);

  QStringList channels;
  channels << "FOOT_CONTACT_ESTIMATE" << "ATLAS_STATE" << "ATLAS_STATE_EXTRA" << "ATLAS_STATUS" << "ATLAS_COMMAND" << "ATLAS_FOOT_POS_EST" << "TRUE_ROBOT_STATE" << "EST_ROBOT_STATE" << "SCALED_ROBOT_STATE" << "VICON_ATLAS" << "SE_INS_POSE_STATE" << "SE_MATLAB_DATAFUSION_REQ" << "POSE_BDI" << "STATE_ESTIMATOR_POSE";

  QStringList messageTypes = SignalHandlerFactory::instance().messageTypes();
  QStringList messageFields;

  mInternal->ChannelListBox->addItems(channels);
  mInternal->MessageTypeListBox->addItems(messageTypes);
  mInternal->MessageFieldListBox->addItems(messageFields);

  mInternal->ChannelListBox->setCurrentRow(0);
  mInternal->MessageTypeListBox->setCurrentRow(0);
  mInternal->MessageFieldListBox->setCurrentRow(0);

  this->connect(mInternal->MessageTypeListBox, SIGNAL(currentRowChanged(int)), SLOT(onMessageTypeChanged()));
  this->connect(mInternal->MessageFieldListBox, SIGNAL(currentRowChanged(int)), SLOT(onFieldNameChanged()));
  this->onMessageTypeChanged();
}

void SelectSignalDialog::onMessageTypeChanged()
{
  if (!mInternal->MessageTypeListBox->currentItem())
  {
    return;
  }

  QString messageType = mInternal->MessageTypeListBox->currentItem()->text();

  QStringList fieldNames = SignalHandlerFactory::instance().fieldNames(messageType);
  fieldNames.sort();

  mInternal->MessageFieldListBox->clear();
  mInternal->MessageFieldListBox->addItems(fieldNames);
  mInternal->MessageFieldListBox->setCurrentRow(0);
  this->onFieldNameChanged();
}


void SelectSignalDialog::onFieldNameChanged()
{
  if (!mInternal->MessageFieldListBox->currentItem())
  {
    return;
  }

  QString messageType = mInternal->MessageTypeListBox->currentItem()->text();
  QString fieldName = mInternal->MessageFieldListBox->currentItem()->text();

  foreach (QListWidget* listWidget, mInternal->ArrayKeyWidgets)
  {
    delete listWidget;
  }
  mInternal->ArrayKeyWidgets.clear();

  const QList<QList<QString> >& validArrayKeys = SignalHandlerFactory::instance().validArrayKeys(messageType, fieldName);

  bool useArrayKeys = !validArrayKeys.empty();
  mInternal->ArrayKeysLabel->setVisible(useArrayKeys);
  mInternal->ArrayKeysContainer->setVisible(useArrayKeys);

  foreach (const QList<QString> & keys, validArrayKeys)
  {
    assert(keys.size());
    QListWidget* listWidget = new QListWidget;
    listWidget->addItems(keys);
    listWidget->setCurrentRow(0);
    mInternal->ArrayKeyWidgets.append(listWidget);
    mInternal->ArrayKeysContainer->layout()->addWidget(listWidget);
  }

  if (mInternal->ArrayKeyWidgets.size())
  {
    mInternal->ArrayKeyWidgets.back()->setSelectionMode(QAbstractItemView::ExtendedSelection);
  }

}


SelectSignalDialog::~SelectSignalDialog()
{
  delete mInternal;
}

QList<SignalHandler*> SelectSignalDialog::createSignalHandlers() const
{
  QString channel = mInternal->ChannelListBox->currentItem()->text();
  QString messageType = mInternal->MessageTypeListBox->currentItem()->text();
  QString messageField = mInternal->MessageFieldListBox->currentItem()->text();


  SignalDescription desc;
  desc.mChannel = channel;
  desc.mMessageType = messageType;
  desc.mFieldName = messageField;

  QList<SignalDescription> descriptions;

  if (mInternal->ArrayKeyWidgets.length())
  {
    foreach (QListWidget* listWidget, mInternal->ArrayKeyWidgets)
    {
      if (listWidget == mInternal->ArrayKeyWidgets.back())
      {
        foreach (QListWidgetItem*	selectedItem, listWidget->selectedItems())
        {
          QString arrayKey = selectedItem->text();
          SignalDescription descCopy(desc);
          descCopy.mArrayKeys.append(arrayKey);
          descriptions.append(descCopy);
        }
      }
      else
      {
        QString arrayKey = listWidget->currentItem()->text();
        desc.mArrayKeys.append(arrayKey);
      }
    }
  }
  else
  {
    descriptions.append(desc);
  }

  QList<SignalHandler*> signalHandlers;
  foreach (const SignalDescription& description, descriptions)
  {
    SignalHandler* signalHandler = SignalHandlerFactory::instance().createHandler(&description);
    assert(signalHandler);
    signalHandlers.append(signalHandler);
  }

  return signalHandlers;
}

