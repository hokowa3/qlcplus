/*
  Q Light Controller
  vcbuttonproperties.cpp

  Copyright (c) Heikki Junnila

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  Version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details. The license is
  in the file "COPYING".

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <QIntValidator>
#include <QKeySequence>
#include <QRadioButton>
#include <QMessageBox>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>

#include "qlcinputchannel.h"
#include "qlcinputprofile.h"
#include "qlcfixturedef.h"

#include "vcbuttonproperties.h"
#include "selectinputchannel.h"
#include "functionselection.h"
#include "virtualconsole.h"
#include "assignhotkey.h"
#include "inputpatch.h"
#include "inputmap.h"
#include "function.h"
#include "fixture.h"
#include "doc.h"

VCButtonProperties::VCButtonProperties(VCButton* button, Doc* doc)
    : QDialog(button)
    , m_doc(doc)
{
    Q_ASSERT(button != NULL);
    Q_ASSERT(doc != NULL);

    setupUi(this);

    QAction* action = new QAction(this);
    action->setShortcut(QKeySequence(QKeySequence::Close));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(reject()));
    addAction(action);

    /* Button text and function */
    m_button = button;
    m_nameEdit->setText(m_button->caption());
    slotSetFunction(button->function());

    /* Key sequence */
    m_keySequence = QKeySequence(button->keySequence());
    m_keyEdit->setText(m_keySequence.toString(QKeySequence::NativeText));

    /* External input */
    m_inputSource = m_button->inputSource();
    updateInputSource();

    /* Press action */
    if (button->action() == VCButton::Flash)
        m_flash->setChecked(true);
    else if (button->action() == VCButton::Blackout)
        m_blackout->setChecked(true);
    else if (button->action() == VCButton::StopAll)
        m_stopAll->setChecked(true);
    else
        m_toggle->setChecked(true);
    slotActionToggled();

    /* Intensity adjustment */
    m_intensityEdit->setValidator(new QIntValidator(0, 100, this));
    m_intensityGroup->setChecked(m_button->adjustIntensity());
    int intensity = int(floor(m_button->intensityAdjustment() * double(100)));
    m_intensityEdit->setText(QString::number(intensity));
    m_intensitySlider->setValue(intensity);

    /* Button connections */
    connect(m_attachFunction, SIGNAL(clicked()), this, SLOT(slotAttachFunction()));
    connect(m_detachFunction, SIGNAL(clicked()), this, SLOT(slotSetFunction()));

    connect(m_attachKey, SIGNAL(clicked()), this, SLOT(slotAttachKey()));
    connect(m_detachKey, SIGNAL(clicked()), this, SLOT(slotDetachKey()));

    connect(m_toggle, SIGNAL(toggled(bool)), this, SLOT(slotActionToggled()));
    connect(m_blackout, SIGNAL(toggled(bool)), this, SLOT(slotActionToggled()));
    connect(m_stopAll, SIGNAL(toggled(bool)), this, SLOT(slotActionToggled()));
    connect(m_flash, SIGNAL(toggled(bool)), this, SLOT(slotActionToggled()));

    connect(m_autoDetectInputButton, SIGNAL(toggled(bool)),
            this, SLOT(slotAutoDetectInputToggled(bool)));
    connect(m_chooseInputButton, SIGNAL(clicked()),
            this, SLOT(slotChooseInputClicked()));

    connect(m_intensitySlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotIntensitySliderMoved(int)));
    connect(m_intensityEdit, SIGNAL(textEdited(QString)),
            this, SLOT(slotIntensityEdited(QString)));
}

VCButtonProperties::~VCButtonProperties()
{
}

void VCButtonProperties::slotAttachFunction()
{
    FunctionSelection fs(this, m_doc);
    fs.setMultiSelection(false);
    if (fs.exec() == QDialog::Accepted)
        slotSetFunction(fs.selection().first());
}

void VCButtonProperties::slotSetFunction(quint32 fid)
{
    m_function = fid;
    Function* func = m_doc->function(m_function);
    if (func == NULL)
    {
        m_functionEdit->setText(tr("No function"));
    }
    else
    {
        m_functionEdit->setText(func->name());
        if (m_nameEdit->text().simplified().isEmpty() == true)
            m_nameEdit->setText(func->name());
    }
}

void VCButtonProperties::slotAttachKey()
{
    AssignHotKey ahk(this, m_keySequence);
    if (ahk.exec() == QDialog::Accepted)
    {
        m_keySequence = QKeySequence(ahk.keySequence());
        m_keyEdit->setText(m_keySequence.toString(QKeySequence::NativeText));
    }
}

void VCButtonProperties::slotDetachKey()
{
    m_keySequence = QKeySequence();
    m_keyEdit->setText(m_keySequence.toString(QKeySequence::NativeText));
}

void VCButtonProperties::slotAutoDetectInputToggled(bool checked)
{
    if (checked == true)
    {
        connect(m_doc->inputMap(),
                SIGNAL(inputValueChanged(quint32,quint32,uchar)),
                this, SLOT(slotInputValueChanged(quint32,quint32)));
    }
    else
    {
        disconnect(m_doc->inputMap(),
                   SIGNAL(inputValueChanged(quint32,quint32,uchar)),
                   this, SLOT(slotInputValueChanged(quint32,quint32)));
    }
}

void VCButtonProperties::slotInputValueChanged(quint32 universe, quint32 channel)
{
    m_inputSource = QLCInputSource(universe, channel);
    updateInputSource();
}

void VCButtonProperties::slotChooseInputClicked()
{
    SelectInputChannel sic(this, m_doc->inputMap());
    if (sic.exec() == QDialog::Accepted)
    {
        m_inputSource = QLCInputSource(sic.universe(), sic.channel());
        updateInputSource();
    }
}

void VCButtonProperties::updateInputSource()
{
    QString uniName;
    QString chName;

    if (m_doc->inputMap()->inputSourceNames(m_inputSource, uniName, chName) == false)
    {
        uniName = KInputNone;
        chName = KInputNone;
    }

    m_inputUniverseEdit->setText(uniName);
    m_inputChannelEdit->setText(chName);
}

void VCButtonProperties::slotActionToggled()
{
    if (m_blackout->isChecked() == true || m_stopAll->isChecked() == true)
    {
        m_generalGroup->setEnabled(false);
        m_intensityGroup->setEnabled(false);
    }
    else
    {
        m_generalGroup->setEnabled(true);
        m_intensityGroup->setEnabled(true);
    }
}

void VCButtonProperties::slotIntensitySliderMoved(int value)
{
    m_intensityEdit->setText(QString::number(value));
}

void VCButtonProperties::slotIntensityEdited(const QString& text)
{
    m_intensitySlider->setValue(text.toInt());
}

void VCButtonProperties::accept()
{
    m_button->setCaption(m_nameEdit->text());
    m_button->setFunction(m_function);
    m_button->setKeySequence(m_keySequence);
    m_button->setInputSource(m_inputSource);
    m_button->setAdjustIntensity(m_intensityGroup->isChecked());
    m_button->setIntensityAdjustment(double(m_intensitySlider->value()) / double(100));

    if (m_toggle->isChecked() == true)
        m_button->setAction(VCButton::Toggle);
    else if (m_blackout->isChecked() == true)
        m_button->setAction(VCButton::Blackout);
    else if (m_stopAll->isChecked() == true)
        m_button->setAction(VCButton::StopAll);
    else
        m_button->setAction(VCButton::Flash);

    QDialog::accept();
}

